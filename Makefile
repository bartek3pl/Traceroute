CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Wpedantic 

all: traceroute

loggers.o: loggers.h loggers.c

traceroute.o: traceroute.c

main.o: main.c

traceroute: main.o traceroute.o loggers.o

distclean:
	rm -f *.o traceroute
clean:
	rm -f *.o