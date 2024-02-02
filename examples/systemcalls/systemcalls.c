#include "systemcalls.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * Done  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int result = system(cmd);
    if(result != 0) { //if failure
    	perror("a3_system");
    	return false;
    }
    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];
    
    va_end(args);
    
    //flush for clarity
    fflush(stdout);
    pid_t cpid = fork();
    if(cpid == 0) { //this is child process
    	execv(command[0], command);
    	//this is in event of an error!
    	perror("a3_execv");
    	return false;
    	
    }
    else if(cpid == -1){ //this is failure condition of fork
    	perror("a3_fork");
    	return false;
    }
    else { //this is the parent process
    	int status;
    	pid_t expid = wait(&status);
    	if(expid == -1) {
    	    perror("a3_wait");
            return false;
    	}
    	//need to check the return value of the child
    	if(WIFEXITED(status)) {
    	    if(status != 0) return false;
    	}
    	else {
    	    perror("a3_wifexit");
    	    return false;
    	}
    	
    }

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];
    va_end(args);

/*
 * Done
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    //setup redirect as in link above
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if(fd < 0){ 
        perror("a3_open");
        return false;
    }
    
    //flush for clarity
    fflush(stdout);
    pid_t cpid = fork();
    if(cpid == 0) { //this is child process
    	//perform redirect as in link above (similar to echo _ 2>&1)
    	int nf = dup2(fd, 1);
    	if(nf == -1){ 
    	    perror("a3_dup2");
    	    return false;
    	}
    	close(fd);
    	//perform execv
    	execv(command[0], command);
    	//will not return unless failure....
    	perror("a3_execv");
    	return false;
    }
    else if(cpid == -1){ //this is failure condition of fork
    	close(fd);
    	perror("a3_fork");
    	return false;
    }
    else { //this is the parent process
    	close(fd);
    	int status;
    	pid_t exited_pid = wait(&status);
    	if(exited_pid == -1) {
    	    perror("a3_wait");
            return false;
    	}
    	
    	//need to check the return value of the child
    	if(WIFEXITED(status)) {
    	    if(status != 0) return false;
    	}
    	else {
    	    printf("child didn't exit normally.");
    	    return false;
    	}
    }

    return true;
}
