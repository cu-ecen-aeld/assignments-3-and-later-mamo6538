CC = $(CROSS_COMPILER)gcc
CFLAGS += -g -Wall -Werror

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.c
    
clean: 
	rm -rf *.o *stackdump aesdsocket
