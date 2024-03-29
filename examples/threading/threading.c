#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    if(!thread_param) {
    	return NULL;
    }
    
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_data_p = (struct thread_data *) thread_param;
    
    bool success = true;
    
    //wait for thread_data_p->wait_obtain_ms
    int rc = usleep(thread_data_p->wait_obtain_ms); //usleep is the milisecond version of sleep
    if(rc != 0) success = false;
    
    //obtain thread_data_p->m
    int result = pthread_mutex_lock(thread_data_p->m);
    if(result != 0) success = false;
    
    //wait for thread_data_p->wait_release_ms
    rc = usleep(thread_data_p->wait_release_ms);
    if(rc != 0) success = false;
    
    //release thread_data_p->m
    result = pthread_mutex_unlock(thread_data_p->m);
    if(result != 0) success = false;
    
    thread_data_p->thread_complete_success = success;
    
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
     //allocate memory for thread_data
     struct thread_data* td = (struct thread_data*)malloc(sizeof(struct thread_data));
     if(!td) {
     	return false;
     }
     //setup mutex and wait arguments
     td->m = mutex;
     td->wait_obtain_ms = wait_to_obtain_ms;
     td->wait_release_ms = wait_to_release_ms;
     
     int rc = pthread_create(thread, NULL, &threadfunc, td);
     int result = true;
     if(rc != 0) {
     	result = false;
     	free(td);
     }
     
     
    return result;
}

