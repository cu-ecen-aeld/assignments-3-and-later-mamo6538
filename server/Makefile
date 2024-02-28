CC = $(CROSS_COMPILER)gcc
CFLAGS += -g -Wall -Werror
LDFLAGS += -pthread -lpthread

all: aesdsocket

aesdsocket: aesdsocket.c aesdsocket.h queue.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o aesdsocket aesdsocket.c
    
clean: 
	rm -rf *.o *stackdump aesdsocket
