//FILE: client/app_client.c
//
//Description: this is the client application code. The client first starts the overlay by creating a direct TCP link between the client and the server. Then it initializes the SRT client by calling srt_client_init(). It creates 2 sockets and connects to the server  by calling srt_client_sock() and srt_client_connect() twice. After some time, the client disconnects from the server by calling srt_client_disconnect(). Finally the client closes the socket by calling srt_client_close(). Overlay is stoped by calling overlay_end().

//Date: April 18,2008

//Input: None

//Output: SRT client states

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../common/constants.h"
#include "srt_client.h"

//two connection are created, one uses client port 87 and server port 88. the other usese client port 89 and server port 90
#define CLIENTPORT1 87
#define SVRPORT1 88
#define CLIENTPORT2 89
#define SVRPORT2 90
//after the conenctions are created, wait 5 seconds, and then close the connections
#define WAITTIME 5

// create a tcp connection to the server
// this function starts the overlay by creating a direct TCP connection between the client and the server. 
// The TCP socket descriptor is returned. If the TCP connection fails, return -1. The TCP socket desciptor 
// returned will be used by SRT to send segments.
int overlay_start() {
	int out_conn;
	struct sockaddr_in servaddr;
	struct hostent *hostInfo;
	
	char hostname_buf[50];
	printf("Enter server name to connect:");
	scanf("%s",hostname_buf);

	hostInfo = gethostbyname(hostname_buf);
	if(!hostInfo) {
		printf("host name error!\n");
		return -1;
	}
		
	servaddr.sin_family =hostInfo->h_addrtype;	
	memcpy((char *) &servaddr.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
	servaddr.sin_port = htons(PORT);

	out_conn = socket(AF_INET,SOCK_STREAM,0);  
	if(out_conn<0) {
		return -1;
	}
	if(connect(out_conn, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
		return -1; 
	return out_conn;
}

//close tcp connection from the server
void overlay_stop(int overlay_conn) {
	close(overlay_conn);
}

int main() {
	//random seed for loss rate
	srand(time(NULL));
	//start overlay	
	int overlay_conn = overlay_start();
	if(overlay_conn<0) {
		printf("fail to start overlay\n");
		exit(1);
	}

	//initialize srt client	
	srt_client_init(overlay_conn);

	//create a srt client sock on port CLIENTPORT1 and connect to srt server at port SVRPORT1
	int sockfd = srt_client_sock(CLIENTPORT1);
	if(sockfd<0) {
		printf("fail to create srt client sock");
		exit(1);
	}
	if(srt_client_connect(sockfd,SVRPORT1)<0) {
		printf("fail to connect to srt server\n");
		exit(1);
	}
	printf("client connect to server, client port:%d, server port %d\n",CLIENTPORT1,SVRPORT1);
	
	//create a srt client sock on port CLIENTPORT2 and connect to srt server at port SVRPORT2
	int sockfd2 = srt_client_sock(CLIENTPORT2);
	if(sockfd2<0) {
		printf("fail to create srt client sock");
		exit(1);
	}
	if(srt_client_connect(sockfd2,90)<0) {
		printf("fail to connect to srt server\n");
		exit(1);
	}
	printf("client connect to server, client port:89, server port 90\n");

	//wait for a while and close the connection to srt server
	sleep(5);
	if(srt_client_disconnect(sockfd)<0) {
		printf("fail to disconnect from srt server\n");
		exit(1);
	}
	if(srt_client_close(sockfd)<0) {
		printf("failt to close srt client\n");
		exit(1);
	}
	
	if(srt_client_disconnect(sockfd2)<0) {
		printf("fail to disconnect from srt server\n");
		exit(1);
	}
	if(srt_client_close(sockfd2)<0) {
		printf("failt to close srt client\n");
		exit(1);
	}
	//close overlay tcp connection
	overlay_stop(overlay_conn);
}
