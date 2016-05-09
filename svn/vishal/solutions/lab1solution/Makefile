CC = gcc
CFLAGS = -Wall -O3 --std=gnu99 -pendantic
CFLAGS2 = -g -Wall --std=gnu99 -pendantic -DDEBUG
OBJS = client.o server.o

all: client server

client.o: client.c
	$(CC) -c $(CFLAGS) -o client.o client.c
server.o: server.c
	$(CC) -c $(CFLAGS) -o server.o server.c
client: client.o
	$(CC) -o client client.o 

server: server.o
	$(CC) -o server server.o 

debug:
	$(CC) $(CFLAGS2) -o client client.c
	$(CC) $(CFLAGS2) -o server server.c

clean: 
	rm -rf client server $(OBJS) *.o *.dSYM
