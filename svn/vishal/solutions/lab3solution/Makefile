CFLAGS = -Wall --std=gnu99 -pedantic
LIBS = -lpthread

all: cs78

cs78: server/app_server.o client/app_client.o common/seg.o server/srt_server.o client/srt_client.o 
	gcc -g server/app_server.o common/seg.o server/srt_server.o -o app_server $(LIBS)
	gcc -g client/app_client.o common/seg.o client/srt_client.o -o app_client $(LIBS)

client/app_client.o: client/app_client.c 
	gcc $(CFLAGS) -g -c client/app_client.c -o client/app_client.o 
server/app_server.o: server/app_server.c 
	gcc $(CFLAGS) -g -c server/app_server.c -o server/app_server.o

common/seg.o: common/seg.c common/seg.h
	gcc $(CFLAGS) -g -c common/seg.c -o common/seg.o
client/srt_client.o: client/srt_client.c client/srt_client.h 
	gcc $(CFLAGS) -g -c client/srt_client.c -o client/srt_client.o
server/srt_server.o: server/srt_server.c server/srt_server.h
	gcc $(CFLAGS) -g -c server/srt_server.c -o server/srt_server.o

clean:
	rm -rf common/*.o
	rm -rf client/*.o
	rm -rf server/*.o
	rm -rf app_client
	rm -rf app_server