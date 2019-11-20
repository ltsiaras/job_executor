CC=gcc

CFLAGS= -g3

all: jobCommander jobExecutorServer

jobCommander: jobCommander.o
	$(CC) $(CFLAGS) -o jobCommander jobCommander.o

jobExecutorServer: jobExecutorServer.o list.o list_type.o
	$(CC) $(CFLAGS) -o jobExecutorServer jobExecutorServer.o list.o list_type.o

jobCommander.o:	jobCommander.c
	$(CC) $(CFLAGS) -c jobCommander.c

jobExecutorServer.o: jobExecutorServer.c
	$(CC) $(CFLAGS) -c jobExecutorServer.c

list.o:	list.c
		$(CC) $(CFLAGS) -c list.c

list_type.o: list_type.c
		$(CC) $(CFLAGS) -c list_type.c

clean:
	rm -rf jobCommander jobExecutorServer *.o
