CFLAGS=-Wall -g
all:
	make clean 
	gcc client/client.c const.c -o assignment1_client
	gcc const.c server/server.c -o assignment1_server
	gcc const.c server/server_data.c -o assignment1_server_data
clean:
	rm -f assignment1_client assignment1_server assignment1_server_data

