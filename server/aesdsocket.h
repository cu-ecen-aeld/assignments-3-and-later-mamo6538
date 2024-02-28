//-------------------------INCLUDES-------------------------
//Assignment 6 includes:
#include <pthread.h>
#include "queue.h"
//Assignment 5 includes:
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

//-------------------------DEFINES-------------------------
#define S_PORT "9000"
#define FILENAME "/var/tmp/aesdsocketdata"
#define BACKLOG 5 //beej.us/guide/bgnet recommends 5 as number in backlog
#define MAX_BUF_SIZE 20 //just to buffer

//-------------------------GLOBALS-------------------------
int fd;

//-------------------------STRUCTS-------------------------
/**
 * This structure should be dynamically allocated and passed as
 * an argument to your thread using pthread_create.
 * It should be returned by your thread so it can be freed by
 * the joiner thread.
 */
struct thread_data{
	pthread_mutex_t* m;
	int nsfd; //file descriptor for the socket
	int complete_flag; //1 if success, -1 if failure, 0 if not complete
};

//Linked list of threads structure
typedef struct slist_thread_s slist_thread_t; //for ease of use
struct slist_thread_s {
	pthread_t thread;
	struct thread_data* td;
	SLIST_ENTRY(slist_thread_s) entries;
};

//-------------------------FUNCTIONS-------------------------
/* THREADFUNC 
 * Description: function called upon accept or thread creation
 *  This function will 
 *  - accept packets until the connection is closed.
 *  - write the packets to the specified file.  
 *  - echo back the file upon a packet reception.
 * Input:
 *  thread_param = pointer to thread_data struct
 * Output:
 *  thread_param = pointer to the input or NULL upon failure
 * Safety:
 *   This function (and all called inside) is thread safe.
 */
void* threadfunc(void* thread_param);
