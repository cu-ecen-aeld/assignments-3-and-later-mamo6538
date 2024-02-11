CFLAGS=-g -Wall -Werror

all: writer

writer: writer.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o writer writer.o

writer.o: writer.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c writer.c

clean:
	rm -rf *.o *stackdump writer
