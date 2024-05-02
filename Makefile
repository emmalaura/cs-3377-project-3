CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pthread
LIBS=

all: dbclient dbserver

dbclient: dbclient.o
	$(CC) $(CFLAGS) -o dbclient dbclient.o $(LIBS)

dbserver: dbserver.o
	$(CC) $(CFLAGS) -o dbserver dbserver.o $(LIBS)

dbclient.o: dbclient.c msg.h
	$(CC) $(CFLAGS) -c dbclient.c

dbserver.o: dbserver.c msg.h
	$(CC) $(CFLAGS) -c dbserver.c

clean:
	rm -f dbclient dbserver *.o
