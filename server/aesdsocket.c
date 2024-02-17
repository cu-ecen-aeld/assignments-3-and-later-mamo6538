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
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>

#define S_PORT "9000"
#define FILENAME "/var/tmp/aesdsocketdata"
#define BACKLOG 5 //beej.us/guide/bgnet recommends 5 as number in backlog

char host[NI_MAXHOST];

//eventually support -d argument for creating daemon

//fundtion: write to output textfile
//function: signal handler

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
	
	//open stream bound to port 9000, returns -1 upon failure to connect
	int sfd = init_socket();
	if(sfd == -1){
		closelog();	
		return -1;
	}
	syslog(LOG_DEBUG, "Socket file descriptor: %d\n", sfd);
	
	int nsfd = accept_socket(sfd);
	//if(nsfd == -1) continue;
	
	//receive data
	size_t read_len = 10; //TODO
	char read_buf[read_len];
	ssize_t num_read = read(nsfd, read_buf, read_len);
	if(num_read == -1) {
		syslog(LOG_ERR, "Failed to read from socket.");
	}
	else {
		//echo back the message!
		size_t written_b = write(nsfd, read_buf, num_read);
		syslog(LOG_DEBUG, "writing: %s, %ld bytes of %ld\n", read_buf, written_b, num_read);
	}
	
	/*
	//TODO: append received data to /var/tmp/aesdsocketdata (create if file dne)
	//make/open the file
	FILE* fp = fopen(FILENAME, "a");  //DO I NEED TO USE OPEN()?
	if(!fp) {
		syslog(LOG_ERR, "%m\n"); //%m references errno
		closelog(); //if returned
		return -1; //?should I RETURN HERE???????????
	}
	
	//close file
	fclose(fp);
	*/
	
	//TODO:buffer!!
	//return contents of above file to client after packet end ("\n")
	
	syslog(LOG_DEBUG, "Closed connection from %s\n", host);
	close(nsfd);
	
	//loops to restart accepting connections forever - what is in the while loop?
	/*while(1) {
		//TODO:graceful exit upon SIGINT or SIGTERM
		syslog(LOG_DEBUG, "??");
	}*/
	close(sfd);
	closelog();
	return 0;
}
