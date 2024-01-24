all: writer

writer: writer.o
	$(CROSS_COMPILE)gcc -g -Wall -o writer writer.o

writer.o: writer.c
	$(CROSS_COMPILE)gcc -g -Wall -c writer.c

clean:
	rm -rf *o *stackdump writer
