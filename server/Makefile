CC ?= $(CROSS_COMPILER)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -pthread -lrt
TARGET ?= aesdsocket

all: aesdsocket

aesdsocket: aesdsocket.c aesdsocket.h queue.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) aesdsocket.c
	
test: ioctl_test.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o ioctlTest ioctl_test.c
    
clean: 
	rm -rf *.o *stackdump aesdsocket ioctlTest
