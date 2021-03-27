
CFLAGS += -Wall -g
LDLIBS += -lm

all: process scheduler

scheduler: scheduler.o file_format.o
	$(CC) $(CFLAGS) $(LDLIBS) $^ -o $@

process: process.o
	$(CC) $(CFLAGS) $(LDLIBS) $^ -o $@

scheduler.o: scheduler.c scheduler.h file_format.h
file_format.o: file_format.c file_format.h
process.o: process.c

clean:
	rm -f *.o scheduler process

