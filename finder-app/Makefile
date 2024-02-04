CC := $(CROSS_COMPILE)gcc
CFLAGS := -g -Wall -Werror

all: writer

writer: writer.o
	$(CC) $(CFLAGS) -o writer writer.o

writer.o: writer.c
	$(CC) $(CFLAGS) -c writer.c

clean:
	rm -rf *.o *stackdump writer
