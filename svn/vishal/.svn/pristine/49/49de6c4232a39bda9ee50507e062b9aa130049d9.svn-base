all: client/app_simple_client server/app_simple_server client/app_stress_client server/app_stress_server  
client/app_simple_client: client/app_simple_client.c common/seg.o client/srt_client.o 
	gcc -Wall -pedantic -std=c99 -g -pthread client/app_simple_client.c common/seg.o client/srt_client.o -o client/app_simple_client 
client/app_stress_client: client/app_stress_client.c common/seg.o client/srt_client.o 
	gcc -Wall -pedantic -std=c99 -g -pthread client/app_stress_client.c common/seg.o client/srt_client.o -o client/app_stress_client 
server/app_simple_server: server/app_simple_server.c common/seg.o server/srt_server.o 
	gcc -Wall -pedantic -std=c99 -g -pthread server/app_simple_server.c common/seg.o server/srt_server.o -o server/app_simple_server
server/app_stress_server: server/app_stress_server.c common/seg.o server/srt_server.o 
	gcc -Wall -pedantic -std=c99 -g -pthread server/app_stress_server.c common/seg.o server/srt_server.o -o server/app_stress_server
common/seg.o: common/seg.c common/seg.h
	gcc -Wall -pedantic -std=c99 -g -c common/seg.c -o common/seg.o
client/srt_client.o: client/srt_client.c client/srt_client.h 
	gcc -Wall -pedantic -std=c99 -g -c client/srt_client.c -o client/srt_client.o
server/srt_server.o: server/srt_server.c server/srt_server.h
	gcc -Wall -pedantic -std=c99 -g -c server/srt_server.c -o server/srt_server.o
clean:
	rm -rf common/*.o
	rm -rf client/*.o
	rm -rf server/*.o
	rm -rf client/app_simple_client
	rm -rf client/app_stress_client
	rm -rf server/app_simple_server
	rm -rf server/app_stress_server
	rm -rf server/receivedtext.txt



