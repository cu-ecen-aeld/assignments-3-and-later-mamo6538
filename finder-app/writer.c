/* Assignment 2 C code
 * Author: Madeleine Monfort
 */

//library includes - File IO
#include <stdio.h>
#include <syslog.h>
#include <errno.h>

int writer(const char* writefile, char* writestr) {
	//perform checks
	if(!writefile) {
		//print that you need a file
		syslog(LOG_ERR, "Missing a file to write to.\n");
		return 1;
	}
	if(!writestr) {
		//print that you need a string to write
		syslog(LOG_ERR, "Missing a string to write to the file.\n");
		return 1;
	}
	//make/open the file (don't need to make directories)
	FILE* wfp = fopen(writefile, "w");
	//check errno! use %m to reference in syslog
	if(!wfp) {
		syslog(LOG_ERR, "%m\n");
		return 1;
	}
	
	//write the string
	int result = fputs(writestr, wfp);
	//error checking
	if(result == EOF) {
		syslog(LOG_ERR, "%m\n");
		return 1;
	}
	
	syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
	
	//close file
	fclose(wfp);
	
	return 0;
}

int main(int argc, char *argv[]) {
	//setup syslog
	openlog("assignment2_", 0, LOG_USER);
	
	//ERROR checking
	if (argc < 2) {
		syslog(LOG_ERR, "Not enough arguments:%d Need a writefile and writestr argument.\n", argc);
		return 1;
	}
	int result = writer(argv[1], argv[2]);
	
	closelog();
	return result;
}
