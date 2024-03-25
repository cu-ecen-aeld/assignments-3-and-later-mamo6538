/* Assignment 5-9 Socket code
 * Author: Madeleine Monfort
 * Description:
 *  Creates a socket bound to port 9000 that echoes back everything it receives.
 *  It also saves the received data into a file, and buffers before echoing back.
 *  This program has the ability to run as a daemon with the '-d' flag.
 *  
 *  Assignment 6 addition:
 *    This program will also spawn new threads upon each accept and print timestamps.
 *
 *  Assignment 8 addition:
 *    This program will also use an aesd char driver instead of a file
 *    if the USE_AESD_CHAR_DEVICE flag is defined. It will not print timestamps. 
 *
 *  Assignment 9 addition:
 *    This program will utilize the aesd-char-driver's llseek and ioctl
 *    and therefore swaps out pread for read.  
 *
 * Exit:
 *  This application will exit upon reciept of a signal or failure to connect.  
 *  It will specifically handle SIGINT and SIGTERM gracefully.
 *
 * Return value:
 *  0 upon successful termination.  -1 upon socket connection failure.
 */

#include "aesdsocket.h"

//function: signal handler
// to handle the SIGINT and SIGTERM signals
// force exit from main while loop
// and shutdown the socket
static void signal_handler( int sn ) {
	if(sn == SIGTERM || sn == SIGINT) {
		caught_sig = 1;
		shutdown(sfd, SHUT_RDWR);
	}
}

static void timer_handler( int sn ) {
	if(sn == SIGALRM) {
		caught_timer = 1;
	}
}

/* DO_IOCTL
 * Description: handles running the IOCTL driver command
 *   If the data buffer is in fact an ioctl command.
 * Inputs:
 *   fd = file descriptor for the device driver
 *   data = data buffer holding the ioctl command
 *   len = size of data buffer
 * Outputs:
 *   result = 0 if successful ioctl command run,
 *	     -1 upon failure or an invalid command for ioctl
 */
int do_ioctl(int fd, char* data, ssize_t len) {
	int result = 0;
	
	//CHECK that it is a valid IOCTL command
	if(!data) {
		syslog(LOG_ERR, "ERROR:do_ioctl received Null pointer");
		return -1;
	}
	if(len < IOCTL_CMD_L) {
		syslog(LOG_DEBUG, "Not IOCTL.");	
		return -1;
	}
	result = strncmp(data, IOCTL_CMD, IOCTL_CMD_L);
	if(result != 0) { //no match
		syslog(LOG_DEBUG, "Not IOCTL.");
		return -1;
	}
	
	//setup cmd and offset
	const char delimiters[] = ":,";
	char* token = strtok(data, delimiters);
	if(!token) {
		syslog(LOG_ERR, "ERROR: IOCTL not formatted correctly.");
		return -1;
	}
	char* cmd_c = strtok(NULL, delimiters);
	if(!cmd_c) {
		syslog(LOG_ERR, "ERROR: IOCTL not formatted correctly.");
		return -1;
	}
	char* offset_c = strtok(NULL, delimiters);
	if(!offset_c) {
		syslog(LOG_ERR, "ERROR: IOCTL not formatted correctly.");
		return -1;
	}
	
	//convert cmd and offset to 32-bit integers
	int cmd = atoi(cmd_c);
	int offset = atoi(offset_c);
	
	//setup seekto
	struct aesd_seekto seekto;
	seekto.write_cmd = cmd;
	seekto.write_cmd_offset = offset;
	
	//call ioctl
	result = ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto);
	
	return result;
}

/* FILE_WRITE 
 * Description: writes packet to end of file
 *   or performs ioctl command
 *   specifically handles errors and locking
 * Input:
 *  fd = file descriptor
 *  data = address of data to write
 *  len = length of the data to write
 *  m = mutext to control file access
 * Output: -1 if error, 0 if success
 */
int file_write(int fd, char* data, ssize_t len, pthread_mutex_t* m) {
	int result;
	
	if(USE_AESD_CHAR_DEVICE) { //check ioctl
		int rc = do_ioctl(fd, data, len);
		if(rc == 0) return 0;
	}
	
	//try to lock
	result = pthread_mutex_lock(m);
	if(result != 0) { //failure
		syslog(LOG_ERR, "ERROR mutex lock:%d\n", result);
		return -1;
	}
	
	//write data to file
	ssize_t rc = write(fd, data, len);
	
	//unlock
	result = pthread_mutex_unlock(m);
	if(result != 0) { //failure
		syslog(LOG_ERR, "ERROR mutex unlock:%d\n", result);
	}
	
	if(rc == -1) { //failure to write!
		syslog(LOG_ERR, "Failed to file write:%m\n");
		result = -1;
	}
	else if(rc != len) {
		syslog(LOG_ERR, "failed to write full message\n");
		result = -1;
	}
	else result = 0;
	
	return result;
}

/*SEND_LINE
 * Description: sends a portion of the file at a time (defined by MAX_BUF_SIZE)
 * Input: 
 *  socket = the socket to echo the file to
 *  fd = file descriptor
 * Output:
 *  -1 if error, 0 if successful
 */
int send_line(int socket, int fd) {
	char read_buf[MAX_BUF_SIZE];
	off_t cur_off = 0;
	
	int result;
	char last_byte = 0;
	
	while(1) {
		//read from socket the max allowed at a time
		ssize_t num_read = 0;
		if(USE_AESD_CHAR_DEVICE)
			num_read = read(fd, read_buf, MAX_BUF_SIZE);
		else
			num_read = pread(fd, read_buf, MAX_BUF_SIZE, cur_off);
		if(num_read == -1) {
			syslog(LOG_ERR, "Buffered file read:%m\n");
			result = -1;
			break;
		}
		if(num_read == 0) { //end of file reached
			result = 0;
			if(last_byte != '\n') {
				int rc = send(socket, "\n", 1, 0);
				if(rc == -1) syslog(LOG_ERR, "failed to send:%m\n");
			}
			break;
		}
		
		//send *some* data via socket
		ssize_t rc = send(socket, read_buf, num_read, 0);
		if(rc == -1) {
			syslog(LOG_ERR, "Failed to send:%m\n");
			result = -1;
			break;
		}
		last_byte = read_buf[num_read-1];
		
		//increase offset to read
		cur_off += num_read;
	
	}//end while
	
	return result;
}

/*READ_PACKET 
 * Description: buffered reads the packet of data
 *  assumes the end of a packet is from null terminator
 *  writes the data out to specified file
 * Inputs: 
 *  socket = socket file descriptor to read data from
 *  fd = file descriptor of specified file
 *  m = mutex to control file access
 * Output:
 *  result = -1 upon failure, 0 if connection closed, 1 if successful
 */
int read_packet(int socket, int fd, pthread_mutex_t* m) {
	int result;
	char read_buf[MAX_BUF_SIZE];
	
	char* buffer = malloc(1);
	if(!buffer) {
		syslog(LOG_ERR, "Failed to malloc: %m\n");
		result = -1;
	}
	*buffer = '\0'; //initialize to null pointer terminated string
	ssize_t bufs = 0;
	
	while(1) {
		memset(read_buf, 0, MAX_BUF_SIZE); //default to 0s
		//read from socket the max allowed at a time
		ssize_t num_read = recv(socket, read_buf, MAX_BUF_SIZE-1, 0);
		if(num_read == -1) {
			syslog(LOG_ERR, "Failed to recv: %m\n");
			result = -1;
			break;
		}
		else  if(num_read == 0) { //connection closed
			result = 0;
			break;
		}
		else {
			//reallocate buffer with string lengths of char arrays + \0
			size_t new_len = strlen(read_buf) + strlen(buffer) + 1;
			char* tmp = realloc(buffer, new_len);
			if(!tmp) {
				syslog(LOG_ERR, "Failed to realloc write buffer: %m\n");
				result = -1;
				break;
			}
			//mem handling
			buffer = tmp;
			
			//increase total size of packet
			bufs += num_read;
			
			//concatenate contents into buffer
			strcat(buffer, read_buf);
			
			//break out if read the end of packet
			int eop = num_read -1;
			if(read_buf[eop] == '\n') {
				result = 1;
				break;
			}
		} //end if-else
	
	}//end while
	
	//only write packet upon successful read
	if(result == 1 || result == 0) {
		//write buffer to file
		int num_w = file_write(fd, buffer, bufs, m);
		if(num_w != 0) {
			syslog(LOG_ERR, "Failed to write to the file\n");
			result = -1;
		}
	}
	free(buffer);
	return result;
}

/* ACCEPT_SOCKET
 * Description: tries to accept connections from client
 * Input: sfd = original socket file descriptor
 * Output: new_sfd = new socket file descriptor for receiving, -1 on error
 */
int accept_socket(int sfd, char* host) {
	//accept connection
	struct sockaddr_storage client_addr;
	socklen_t client_addr_size = sizeof client_addr;
	int new_sfd = accept(sfd, (struct sockaddr*)&client_addr, &client_addr_size);
	if(new_sfd == -1){
		syslog(LOG_ERR, "socket accept fail: %m\n");
		return -1;
	}
	//pull client_ip from client_addr
	int rc = getnameinfo((struct sockaddr*)&client_addr, client_addr_size, host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
	if(rc != 0) {
		syslog(LOG_ERR, "Failed to get new hostname:%m\n");
	}
	syslog(LOG_DEBUG, "Accepted connection from %s\n", host);
	return new_sfd;
}

/* INIT_SOCKET
 * Description: setups a server socket
 * Input: N/A
 * Output: 
 *   sfd = socket file descriptor or -1 upon error
 */ 
int init_socket() {
	int sfd = socket(AF_INET, SOCK_STREAM, 0); //create an IPv4 stream(TCP) socket w/ auto protocol
	if(sfd < 0) {
		syslog(LOG_ERR, "failed to create socket:%m\n");
		return -1; //return to main with failure to connect
	}
	int yes = 1; 
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes); //tip for possible bind failure
	
	//need to get address in addrinfo struct
	struct addrinfo hint; //need to make a hint for getaddrinfo function
	memset(&hint, 0, sizeof(hint)); //default to 0s
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE; //to make the address suitable for bind/accept
	
	struct addrinfo* addr_sp;
	int rc = getaddrinfo(NULL, S_PORT, &hint, &addr_sp);
	if(rc != 0) {
		close(sfd);
		syslog(LOG_ERR, "getaddr fail:%s\n", gai_strerror(rc));
		return -1;
	}
	
	//try to bind the socket
	for(struct addrinfo* rp = addr_sp; rp != NULL; rp = rp->ai_next) { //result from getaddrinfo is linked list
		rc = bind(sfd, rp->ai_addr, rp->ai_addrlen);
		if(rc == 0) { //success
			break;
		}
	}
	freeaddrinfo(addr_sp); //FREE!
	if(rc != 0) {
		close(sfd);
		syslog(LOG_ERR, "Failed to bind.%m\n"); //errno is set on bind
		return -1;
	}
	
	//listen to socket
	int result = listen(sfd, BACKLOG); 
	if(result == -1) {
		syslog(LOG_ERR, "Failed to listen.%m\n");
		close(sfd);	
		return -1;
	}
	return sfd;
}

void* threadfunc(void* thread_param)
{
	//setup threading info
	if(!thread_param) {
		syslog(LOG_ERR, "Null pointer exception in thread func.\n");
		return NULL;
	}
	struct thread_data* tdp = (struct thread_data *) thread_param;
	int success = 1;
    
	//continuously read on a socket
	while(1) {
		//read full packet
		int rc = read_packet(tdp->nsfd, tdp->fd, tdp->m);
		if(rc == -1) { //reading/echoing failed in some way
			syslog(LOG_ERR, "Not reading correctly.\n");
			success = -1;
			break;
		}
		if(rc == 0) { //connection ended
			break;
		}
		
		syslog(LOG_DEBUG,"Read packet.\n");
		//attempt to echo the file back
		send_line(tdp->nsfd, tdp->fd);
		syslog(LOG_DEBUG,"sent back file.\n");
		
	} //end of reading packets
    
	tdp->complete_flag = success;
    
	return thread_param;
}

int main(int argc, char* argv[]) {
	int result = 0;
	int fd;
	
	//setup syslog
	openlog("assignment_8", 0, LOG_USER);
	
	//open stream bound to port 9000, returns -1 upon failure to connect
	sfd = init_socket();
	if(sfd == -1){	
		result = -1;
	}
	
	//make/open the file for appending and read/write
	if(!USE_AESD_CHAR_DEVICE) {
		fd = open(FILENAME, O_CREAT | O_RDWR | O_APPEND, 00666);
		if(fd == -1) {
			syslog(LOG_ERR, "ERROR opening file:%m\n");
			result = -1;
		}
	}
	
	//setup signal handling
	struct sigaction new_act;
	memset(&new_act, 0, sizeof(struct sigaction)); //default the sigaction struct
	new_act.sa_handler = signal_handler; //setup the signal handling function
	int rc = sigaction(SIGTERM, &new_act, NULL); //register for SIGTERM
	if(rc != 0) {
		syslog(LOG_ERR, "Error %d registering for SIGTERM\n", errno);
		result = -1;
	}
	rc = sigaction(SIGINT, &new_act, NULL); //register for SIGINT
	if(rc != 0) {
		syslog(LOG_ERR, "Error %d registering for SIGINT\n", errno);
		result = -1;
	}
	
	if(!USE_AESD_CHAR_DEVICE) {
		new_act.sa_handler = timer_handler; //setup the signal handling function
		rc = sigaction(SIGALRM, &new_act, NULL); //register for SIGALRM
		if(rc != 0) {
			syslog(LOG_ERR, "Error %d registering for SIGALRM\n", errno);
			result = -1;
		}
	}
	
	
	//support -d argument for creating daemon
	if(argc == 2) {
		//compare argv[1] with "-d"
		int res = strcmp(argv[1], "-d");
		if(res != 0) {
			syslog(LOG_ERR, "ERROR: incorrect arguments.\n");
			syslog(LOG_ERR, "Usage: ./aesdsocket or ./aesdsocket -d\n");
			result = -1;
		}
		
		//fork to create daemon here-- (socket bound, signal actions will carry over)
		pid_t cpid = fork();
		if(cpid == -1){ //this is failure condition of fork
			syslog(LOG_ERR,"a5_fork:%m\n");
			exit(-1);
		}
		else if(cpid != 0) { //this is parent process
			//exit in parent
			exit(0); //success
		}
		
		//setsid and change directory
		setsid();
		chdir("/");
		//close file descriptors - NOPE I need them.
		//redirect stdin/out/err to /dev/null
		freopen("/dev/null", "r", stdin);
		freopen("/dev/null", "w", stdout);
		freopen("/dev/null", "w", stderr);
	}
	
	//continually accept!

	//create linked list
	SLIST_HEAD(slisthead, slist_thread_s) head;
	SLIST_INIT(&head);
	
	//create single mutex for all threads to share
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	
	//setup 10 second timer
	struct itimerval delay;
	char data[MAX_TIME_SIZE];
	time_t rawNow;
	struct tm* now = (struct tm*)malloc(sizeof(struct tm));
	if(!USE_AESD_CHAR_DEVICE) {
		delay.it_value.tv_sec = 10;
		delay.it_value.tv_usec = 0;
		delay.it_interval.tv_sec = 10;
		delay.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &delay, NULL);
		memset(&data, 0, MAX_TIME_SIZE);
	}
	
	while(!caught_sig && !result) {
		/*------CREATE SOCKET RX THREAD------*/
		char host[NI_MAXHOST];
		int nsfd = accept_socket(sfd, host);
		if(nsfd != -1) { //success
			//----create a new thread----
			pthread_t thread;
			
			if(USE_AESD_CHAR_DEVICE) {
				fd = open(FILENAME, O_RDWR);
				if(fd == -1) {
					syslog(LOG_ERR, "ERROR opening file:%m\n");
					result = -1;
				}
			}
	    		
	    		//allocate memory for thread_data
			struct thread_data* td = (struct thread_data*)malloc(sizeof(struct thread_data));
			if(!td) {
				syslog(LOG_ERR, "Failed to allocate thread_data.\n");
				result = -1;
				continue;
			}
			//setup arguments
			td->m = &mutex;
			td->nsfd = nsfd;
			td->fd = fd;
			td->complete_flag = 0;
			memcpy(td->host, host, NI_MAXHOST);
			
			//setup linked list element
			slist_thread_t* threadp = malloc(sizeof(slist_thread_t));
			if(!threadp) { //NO MORE MEMORY
				syslog(LOG_ERR, "Failed to allocate ll element.\n");
				free(td);
				result = -1;
				continue;
			}

			int rc = pthread_create(&thread, NULL, &threadfunc, td);
			if(rc != 0) {
				syslog(LOG_ERR, "Failed to create thread.\n");
				free(td);
				free(threadp);
				result = -1;
				continue;
			}
	    		
			//----add to linked list----
			threadp->thread = thread;
			threadp->td = td;
			SLIST_INSERT_HEAD(&head, threadp, entries);
		}
		
		/*------CHECK TIMER------*/
		if(caught_timer) {
			caught_timer = 0; //clear it
			//get now
			time(&rawNow);
			now = localtime_r(&rawNow, now);
			
			//format timestamp
			memset(&data, 0, MAX_TIME_SIZE);
			strftime(data, MAX_TIME_SIZE, RFC2822_FORMAT, now);

			rc = pthread_mutex_lock(&mutex);
			if(rc != 0) {
				syslog(LOG_ERR, "Failed to lock timestamp\n");
				result = -1;
				continue;
			}

			//write timestamp to file
			write(fd, data, strlen(data));

			rc = pthread_mutex_unlock(&mutex);
		}
		
		/*------MANAGE RUNNING THREADS------*/
		slist_thread_t* tp = NULL;
		slist_thread_t* next = NULL;
		SLIST_FOREACH_SAFE(tp, &head, entries, next) {
			//check if thread is done
			if(tp->td->complete_flag) {
				//remove from linked list
				SLIST_REMOVE(&head, tp, slist_thread_s, entries);
				
				//join thread
				void* thread_rtn = NULL;
				int rc = pthread_join(tp->thread, &thread_rtn);
				if(rc != 0) {
					syslog(LOG_ERR, "Failed to end thread:%ld\n", tp->thread);
					result = -1;
				}
				
				//check thread success
				if(!thread_rtn) //failure
					syslog(LOG_ERR, "threadfunc failed.\n");
				struct thread_data* tdp = (struct thread_data *) thread_rtn;
				
				//close the socket(s)
				syslog(LOG_DEBUG, "Closed connection from %s\n", tdp->host);
				close(tdp->nsfd); //close accepted socket	
				if(USE_AESD_CHAR_DEVICE)
					close(tdp->fd); //close the driver	
			
				//free the thread
				free(tdp);
				free(tp);
			}
			
		}//end list loop
	}//end while
	syslog(LOG_DEBUG, "Caught signal, exiting\n");
	
	if(result == -1) {
		close(sfd);
	}
	
	//free linked list
	void* thread_rtn;
	while(!SLIST_EMPTY(&head)) {
		slist_thread_t* threadp = SLIST_FIRST(&head);
		SLIST_REMOVE_HEAD(&head, entries);
		pthread_join(threadp->thread, &thread_rtn);
		
		//close the socket(s)
		struct thread_data* tdp = (struct thread_data *) thread_rtn;
		syslog(LOG_DEBUG, "Closed connection from %s\n", tdp->host);
		close(tdp->nsfd); //close accepted socket	
		if(USE_AESD_CHAR_DEVICE)
			close(tdp->fd); //close the driver
		
		free(thread_rtn);
		free(threadp);
		threadp = NULL;
	}
	
	syslog(LOG_DEBUG, "Made it through the threads.\n");
	
	free(now);
	pthread_mutex_destroy(&mutex);
	 
	if(!USE_AESD_CHAR_DEVICE) close(fd); //close writing file
	close(sfd); //close socket
	
	if(!USE_AESD_CHAR_DEVICE) unlink(FILENAME); //remove file
	closelog();
	return result;
}
