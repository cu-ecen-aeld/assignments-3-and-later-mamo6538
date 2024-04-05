#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../aesd-char-driver/aesd_ioctl.h"
#include "aesdsocket.h"
/* DO_IOCTL
 *
 */
int do_ioctl(int fd, char* data, ssize_t len) {
	int result = 0;
	
	//error checking
	if(!data) {
		printf("ERROR:do_ioctl received Null pointer\n");
		return -1;
	}
	if(len < (IOCTL_CMD_L + 4) ) {
		printf("Not IOCTL: len = %ld\n", len);	
		return -1;
	}
	result = strncmp(data, IOCTL_CMD, IOCTL_CMD_L);
	if(result != 0) { //no match
		printf("Not IOCTL.\n");
		return -1;
	}
	
	//setup cmd and offset
	const char delimiters[] = ":,";
	char* token = strtok(data, delimiters);
	if(!token) {
		printf("ERROR: IOCTL not formatted correctly.\n");
		return -1;
	}
	char* cmd_c = strtok(NULL, delimiters);
	if(!cmd_c) {
		printf("ERROR: IOCTL not formatted correctly.\n");
		return -1;
	}
	char* offset_c = strtok(NULL, delimiters);
	if(!offset_c) {
		printf("ERROR: IOCTL not formatted correctly.\n");
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

int main() {
	int result = 0;
	
	char test_fail[] = "AESDCHAR_IOCSEEKTO:2,3";
	int fd = open(FILENAME, O_RDWR);
	if(fd == -1) {
		printf("ERROR opening file:%m\n");
		return -1;
	}
	
	result = do_ioctl(fd, test_fail, strlen(test_fail));
	printf("Returned:%d\n", result);
	
	//try to read now
	char buf[10];
	memset(buf, 0, 10);
	int rc = read(fd, buf, 10);
	
	printf("Read %d bytes:%s\n", rc, buf);
	
	close(fd);
	return 0;
}
