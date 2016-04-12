#include <arpa/inet.h>          // inet_ntoa
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define LISTENQ  1024  // second argument to listen()
#define MAXLINE 1024   // max length of a line
#define RIO_BUFSIZE 1024
#define true 1
#define false 0
#define ERROR  -1
#define OK 1
#define MAX_CLIENTS 9999
 #define MAX_POOL 10
#define RESPONSE_HEADER_SUCCESS "HTTP/1.1 200 OK\r\n"

int main(int argc, char** argv){
	FILE *file = fopen ("ip_address.txt","r");
	char buff[255];
	if(file != NULL){
		printf("File read success\n");
		while(fgets(buff, 255, (FILE*)file) > 0){
			printf("I read %s \n", buff);
		}
	}else{
		printf("File open error\n");
	}
	fclose(file);

}