/* Assignment 5 Socket code
 * Author: Madeleine Monfort
 * Description:
 *  Creates a socket bound to port 9000 that echoes back everything it receives.
 *  It also saves the received data into a file, and buffers before echoing back.
 *  This program has the ability to run as a daemon with the '-d' flag.
 *
 * Exit:
 *  This application will exit upon reciept of a signal or failure to connect.  
 *  It will specifically handle SIGINT and SIGTERM gracefully.
 *
 * Return value:
 *  0 upon successful termination.  -1 upon socket connection failure.
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#define S_PORT "9000"
#define FILENAME "/var/tmp/aesdsocketdata"
#define BACKLOG 5 //beej.us/guide/bgnet recommends 5 as number in backlog
#define MAX_BUF_SIZE 20 //just to buffer

char host[NI_MAXHOST];
int caught_sig = 1;

//TODO:upport -d argument for creating daemon


//function: signal handler
static void signal_handler( int sn ) {
	caught_sig = 0;
}

/* FILE_WRITE 
 * Description: appends to already opened file
 *   specifically handles errors - may not be necessary as separate function
 */
int file_write(int fd, char* data, size_t len) {
	int result;
	
	//write data to file
	int rc = write(fd, data, len);
	if(rc == -1) { //failure to write!
		syslog(LOG_ERR, "%m\n");
		result = -1;
	}
	else if(rc != len) {
		syslog(LOG_ERR, "failed to write full message\n");
		result = -1;
	}
	else result = 0;
	
	return result;
}

//SEND_FILE

int send_file(int fd, int socket) {
	char data[MAX_BUF_SIZE];
	
	//read from start of file
	ssize_t num_read = pread(fd, data, 10, 0);
	printf("read: %ld", num_read);
	if(num_read == -1) {
		syslog(LOG_ERR, "Trying to read file failed: %m\n");
		return -1;
	}
	//send full file via socket
	int rc = send(socket, data, num_read, 0);
	if(rc == -1) {
		syslog(LOG_ERR, "Failed to send:%m\n");
		return -1;
	}
	return 0;
}

/*READ_PACKET 
 * Description: buffered reads the packet of data
 *  assumes the end of a packet is from null terminator
 * Inputs: 
 *  socket = socket file descriptor to read data from
 *  buffer = malloc'd char* to hold the entire packet
 * Output:
 *  result = -1 upon failure, 0 if success, 1 if connection closed
 */
int read_packet(int socket, char* buffer) {
	int result;
	char read_buf[MAX_BUF_SIZE];
	memset(&read_buf, 0, sizeof(read_buf)); //default to 0s
	
	while(1) {
		//read from socket the max allowed at a time
		ssize_t num_read = recv(socket, read_buf, MAX_BUF_SIZE, 0);
		if(num_read == -1) {
			syslog(LOG_ERR, "%m\n");
			result = -1;
			break;
		}
		else  if(num_read == 0) { //connection closed
			syslog(LOG_DEBUG, "Receiving stopped.\n");
			result = 1;
			break;
		}
		else {
			//reallocate buffer with strlen(read_buf) + strlen(buffer) + \0
			size_t new_len = strlen(read_buf) + strlen(buffer) + 1;
			buffer = realloc(buffer, new_len);
			if(!buffer) {
				syslog(LOG_ERR, "%m\n");
				result = -1;
				break;
			}
			
			//concatenate contents into buffer
			strcat(buffer, read_buf);
			
			//break out if read the end of packet
			if(read_buf[num_read] == '\0') {
				result = 0;
				break;
			}
		} //end if-else
	
	}//end while
	return result;
}

/* ACCEPT_SOCKET
 * Description: tries to accept connections from client
 * Input: sfd = original socket file descriptor
 * Output: new_sfd = new socket file descriptor for receiving, -1 on error
 */
int accept_socket(int sfd) {
	//accept connection
	struct sockaddr_storage client_addr;
	socklen_t client_addr_size = sizeof client_addr;
	int new_sfd = accept(sfd, (struct sockaddr*)&client_addr, &client_addr_size);
	if(new_sfd == -1){
		syslog(LOG_ERR, "%m\n");
		return -1;
	}
	//pull client_ip from client_addr
	getnameinfo((struct sockaddr*)&client_addr, client_addr_size, host, sizeof(host), NULL, 0, NI_NUMERICHOST);
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
		syslog(LOG_ERR, "%m\n");
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
		syslog(LOG_ERR, "%s\n", gai_strerror(rc));
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
		syslog(LOG_ERR, "%m\n"); //errno is set on bind
		return -1;
	}
	
	//listen to socket
	int result = listen(sfd, BACKLOG); 
	if(result == -1) {
		syslog(LOG_ERR, "%m\n");
		close(sfd);	
		return -1;
	}
	return sfd;
}

int main(int argc, char* argv[]) {
	//setup syslog
	openlog("assignment_5_1_", 0, LOG_USER);
	
	//setup signal handling
	struct sigaction new_act;
	memset(&new_act, 0, sizeof(struct sigaction)); //default the sigaction struct
	new_act.sa_handler = signal_handler; //setup the signal handling function
	int rc = sigaction(SIGTERM, &new_act, NULL); //register for SIGTERM
	if(rc != 0) {
		syslog(LOG_ERR, "Error %d registering for SIGTERM", errno);
		closelog();
		return -1;
	}
	rc = sigaction(SIGINT, &new_act, NULL); //register for SIGINT
	if(rc != 0) {
		syslog(LOG_ERR, "Error %d registering for SIGINT", errno);
		closelog();
		return -1;
	}
	
	//make/open the file for appending
	int fd = open(FILENAME, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(fd == -1) {
		syslog(LOG_ERR, "%m\n");
		closelog();
		return -1;
	}
	
	//open stream bound to port 9000, returns -1 upon failure to connect
	int sfd = init_socket();
	if(sfd == -1){
		close(fd);
		closelog();	
		return -1;
	}
	
	//continually accept!
	int nsfd;
	char* buffer;
	while(caught_sig) {
		nsfd = accept_socket(sfd);
		if(nsfd == -1) continue;
		
		//continuously read on a socket
		while(1) {
			buffer = malloc(1);
			if(!buffer) {
				syslog(LOG_ERR, "Failed to malloc: %m");
				break;
			}
			*buffer = '\0'; //initialize to null pointer terminated string?
		
			//read full packet
			int rc = read_packet(nsfd, buffer);
			if(rc == -1) { //reading/echoing failed in some way
				syslog(LOG_DEBUG, "Not reading correctly.\n");
				break;
			}
			if(rc == 1) { //connection ended
				break;
			}
			
			//write buffer to file
			size_t num_to_write = strlen(buffer);
			int num_w = file_write(fd, buffer, num_to_write);
			if(num_w != num_to_write) {
				syslog(LOG_ERR, "Failed to write to the buffer\n");
			}
			
			//echo the file back
			num_w = file_write(nsfd, buffer, num_to_write);
			printf("tried to echo back.\n");
			
			free(buffer);
		} //end of reading packets
		
		if(buffer) free(buffer);
		syslog(LOG_DEBUG, "Closed connection from %s\n", host);
		close(nsfd); //close accepted socket
	}
	
	//only upon a signal will this occur
	if(buffer) free(buffer); //only free if it still exists: will need to do further efforts.  
	close(fd); //close writing file
	close(nsfd); //close accepted socket
	close(sfd); //close socket
	
	//delete writing file
	closelog();
	return 0;
}
