#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <strings.h>
#include "srt_client.h"

/*interfaces to application layer*/

//
//
//  SRT socket API for the client side application.
//  ===================================
//
//  In what follows, we provide the prototype definition for each call and limited pseudo code representation
//  of the function. This is not meant to be comprehensive - more a guideline.
//
//  You are free to design the code as you wish.
//
//  NOTE: When designing all functions you should consider all possible states of the FSM using
//  a switch statement (see the Lab3 assignment for an example). Typically, the FSM has to be
// in a certain state determined by the design of the FSM to carry out a certain action.
//
//  GOAL: Your goal for this assignment is to design and implement the 
//  prototypes below - fill the code.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


/**
* TCB array of connections currently connected to the server.
*/
client_tcb_t *clients[MAX_TRANSPORT_CONNECTIONS];
/**
* pthread variable for running 
*/
pthread_t pthread_receiver;
int total_connection = 0 ;
int mainTcpSockId;

long current_time_millis(){
	struct timeval stop, start;
	gettimeofday(&start, NULL);
	return (long)start.tv_usec;
}

void reset_clients(){
	printf("reset_clients\n");
	for(int i = 0 ; i < MAX_TRANSPORT_CONNECTIONS ; i++){
		if(clients[i] != NULL){
			free(clients[i]);
		}
		clients[i] = NULL;
	}
	fflush(stdout);
}

client_tcb_t * get_client_from_port(int port_num){
	printf("get_client_from_port\n");
	for(int i = 0 ; i < MAX_TRANSPORT_CONNECTIONS ; i++){
		if(clients[i] != NULL && clients[i]->client_portNum == port_num){
			return clients[i];
		}
	}
	fflush(stdout);
	return NULL;
}

int freeClient(int sock_fd){
	printf("freeClient\n");
	int result = -1;
	if(sock_fd < MAX_TRANSPORT_CONNECTIONS){
		free(clients[sock_fd]);
		result = 1 ;
	}
	fflush(stdout);
	return result;
}

int get_new_client_socket(int client_port){
	printf("get_new_client\n");
	int clientCount = -1;
	if(total_connection < MAX_TRANSPORT_CONNECTIONS){
		/* checking for the first null position in array */
		for(clientCount = 0 ; clientCount <  MAX_TRANSPORT_CONNECTIONS && clients[clientCount] != NULL; clientCount++);
		// above for loop finished with an index which is null. It assumes that total_connection is working correctly
		printf("new socket id allocated to - %d\n", clientCount);	
		clients[clientCount] = (client_tcb_t *)malloc(sizeof(client_tcb_t));
		clients[clientCount]->client_portNum = client_port;
		clients[clientCount]->state = CLOSED;
		//TODO set to default value 	
	}else{
		printf("MAX CLIENTS REACHED :- %d \n", total_connection );
	}
	fflush(stdout);
	return clientCount;
}

// srt client initialization
//
// This function initializes the TCB table marking all entries NULL. It also initializes
// a global variable for the overlay TCP socket descriptor ‘‘conn’’ used as input parameter
// for snp_sendseg and snp_recvseg. Finally, the function starts the seghandler thread to
// handle the incoming segments. There is only one seghandler for the client side which
// handles call connections for the client.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

void srt_client_init(int conn) {
	printf("srt_client_init\n");
	mainTcpSockId = conn ;
	reset_clients();
	fflush(stdout);
	pthread_create(&pthread_receiver,NULL,seghandler,(void*)NULL);
  	return;
}

// Create a client tcb, return the sock
//
// This function looks up the client TCB table to find the first NULL entry, and creates
// a new TCB entry using malloc() for that entry. All fields in the TCB are initialized
// e.g., TCB state is set to CLOSED and the client port set to the function call parameter
// client port.  The TCB table entry index should be returned as the new socket ID to the client
// and be used to identify the connection on the client side. If no entry in the TC table
// is available the function returns -1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int srt_client_sock(unsigned int client_port) {
	printf("srt_client_sock\n");
	fflush(stdout);
  	return get_new_client_socket(client_port);
}

// Connect to a srt server
//
// This function is used to connect to the server. It takes the socket ID and the
// server’s port number as input parameters. The socket ID is used to find the TCB entry.
// This function sets up the TCB’s server port number and a SYN segment to send to
// the server using snp_sendseg(). After the SYN segment is sent, a timer is started.
// If no SYNACK is received after SYNSEG_TIMEOUT timeout, then the SYN is
// retransmitted. If SYNACK is received, return 1. Otherwise, if the number of SYNs
// sent > SYN_MAX_RETRY,  transition to CLOSED state and return -1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int srt_client_connect(int sockfd, unsigned int server_port) {
	printf("srt_client_connect\n");
	if(sockfd < 0 || sockfd > MAX_TRANSPORT_CONNECTIONS || clients[sockfd] == NULL)
	{
		printf("Error in srt_client_connect sockfd = %d max clients supported = %d\n", sockfd, MAX_TRANSPORT_CONNECTIONS);
		return -1;
	}
	client_tcb_t *client = clients[sockfd];
	seg_t segment;
	bzero(&segment,sizeof(segment));
	segment.header.src_port = client->client_portNum;
	segment.header.dest_port = client->svr_portNum = server_port;
	segment.header.type = SYN;
	segment.header.length = 0;
	int trialNum = 0 ;
	//  first trial
	if(sendseg(mainTcpSockId,&segment) < 0){ // error check for sendseg when there is TCP socket error
		printf("Error in sending message in TCP layer = %d \n", mainTcpSockId);
		return -1;
	}
	long start_time = current_time_millis(); // timer started
	trialNum++; // increment trial num
	client->state = SYNSENT ;
	while(client->state == SYNSENT){
		long now = current_time_millis();
		long diff = now - start_time ;
		if((diff ) > SYNSEG_TIMEOUT_MS){
			printf("time out in connect %d for trial num = %d \n", sockfd, trialNum);
			if(trialNum < SYN_MAX_RETRY){
				printf("resending segment \n");
				if(sendseg(mainTcpSockId,&segment) < 0){ // error check for sendseg when there is TCP socket error
					printf("Error in sending message sockfd = %d \n", sockfd);
					return -1;
				}
				start_time = (long)time(NULL); // reset timer
				trialNum++;
			}else{
				printf("max trial reached in connect segment of sock fd %d\n" ,sockfd);
				client->state = CLOSED;
				return -1;
			}
		}	
	}
  printf("srt_client_connect ends \n");
  fflush(stdout);
  return 1; // returning success it can be rechecked with state = CONNECTED
}

// Send data to a srt server
//
// Send data to a srt server. You do not need to implement this for Lab3.
// We will use this in Lab4 when we implement a Go-Back-N sliding window.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int srt_client_send(int sockfd, void* data, unsigned int length) {
	printf("srt_client_send\n");
    fflush(stdout);
	return 1;
}

// Disconnect from a srt server
//
// This function is used to disconnect from the server. It takes the socket ID as
// an input parameter. The socket ID is used to find the TCB entry in the TCB table.
// This function sends a FIN segment to the server. After the FIN segment is sent
// the state should transition to FINWAIT and a timer started. If the
// state == CLOSED after the timeout the FINACK was successfully received. Else,
// if after a number of retries FIN_MAX_RETRY the state is still FINWAIT then
// the state transitions to CLOSED and -1 is returned.

int srt_client_disconnect(int sockfd) {
	printf("srt_client_disconnect\n");
	if(sockfd < 0 || sockfd > MAX_TRANSPORT_CONNECTIONS || clients[sockfd] == NULL)
	{
		printf("Error in srt_client_disconnect sockfd = %d max clients supported = %d\n", sockfd, MAX_TRANSPORT_CONNECTIONS);
		return -1;
	}
	client_tcb_t *client = clients[sockfd];
	seg_t segment;
	bzero(&segment,sizeof(segment));
	segment.header.src_port = client->client_portNum;
	segment.header.dest_port = client->svr_portNum;
	segment.header.type = FIN;
	segment.header.length = 0;
	int trialNum = 0 ;
	//  first trial
	if(sendseg(mainTcpSockId,&segment) < 0){ // error check for sendseg when there is TCP socket error
		printf("Error in sending message in TCP layer = %d \n", mainTcpSockId);
		return -1;
	}
	long start_time = current_time_millis(); // timer started
	trialNum++; // increment trial num
	client->state = FINWAIT ;
	while(client->state == FINWAIT){
		long now = current_time_millis();
		long diff = now - start_time ;
		if((diff ) > FINSEG_TIMEOUT_MS){
			printf("time out in connect %d for trial num = %d \n", sockfd, trialNum);
			if(trialNum < FIN_MAX_RETRY){
				printf("resending segment \n");
				if(sendseg(mainTcpSockId,&segment) < 0){ // error check for sendseg when there is TCP socket error
					printf("Error in sending message sockfd = %d \n", sockfd);
					return -1;
				}
				start_time = (long)time(NULL); // reset timer
				trialNum++;
			}else{
				printf("max trial reached in connect segment of sock fd %d\n" ,sockfd);
				client->state = CLOSED;
				return -1;
			}
		}	
	}
	printf("srt_client_disconnect ends\n");
  	fflush(stdout);
  	return 1;
}


// Close srt client

// This function calls free() to free the TCB entry. It marks that entry in TCB as NULL
// and returns 1 if succeeded (i.e., was in the right state to complete a close) and -1
// if fails (i.e., in the wrong state).
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int srt_client_close(int sockfd) {
	printf("srt_client_close\n");
  	fflush(stdout);
	return freeClient(sockfd);
}
/**
typedef struct srt_hdr {
	unsigned int src_port;        //source port number
	unsigned int dest_port;       //destination port number
	unsigned int seq_num;         //sequence number, used for data transmission. you will not use them in lab3
	unsigned int ack_num;         //ack number, used for data transmission. you will not use them in lab3
	unsigned short int length;    //segment data length
	unsigned short int  type;     //segment type
	unsigned short int  rcv_win;  //currently not used
	unsigned short int checksum;  //currently not used
} srt_hdr_t;

//segment definition

typedef struct segment {
	srt_hdr_t header;
	char data[MAX_SEG_LEN];
} seg_t;
**/
// The thread handles incoming segments
//
// This is a thread  started by srt_client_init(). It handles all the incoming
// segments from the server. The design of seghanlder is an infinite loop that calls snp_recvseg(). If
// snp_recvseg() fails then the overlay connection is closed and the thread is terminated. Depending
// on the state of the connection when a segment is received  (based on the incoming segment) various
// actions are taken. See the client FSM for more details.
//

void *seghandler(void* arg) {
  printf("seghandler receive\n");
  // int recvseg(int connection, seg_t* segPtr);
  seg_t msg ;
  client_tcb_t *client ;
  while(1){
  	if(recvseg(mainTcpSockId,&msg) < 0){
  		printf("client recvseg return negative hence exiting\n");
  		printf("exiting thread and closing main tcp connection\n");
  		fflush(stdout);
  		close(mainTcpSockId);
		pthread_exit(NULL);
  	}
  	printf("seghandler message received \n");
  	client = get_client_from_port(msg.header.dest_port);
	  	if(client){
	  	switch(msg.header.type){
	  		case SYNACK :{
	  			printf("SYNACK RECIEVED client port = %d and server port = %d\n", client->svr_portNum, msg.header.src_port   );
	  			if(client->state == SYNSENT){
	  				printf("CONNECTED");
	  				client->state = CONNECTED;
	  			}else{
	  				printf("ALREADY CONNECTED\n");
	  			}
	  		}
	  		break;
	  		case FINACK :{
	  			printf("FINACK RECIEVED client port = %d and server port = %d \n", client->svr_portNum, msg.header.src_port   );
	  			if(client->state == FINWAIT){
	  				printf("CLOSED");
	  				client->state = CLOSED;
	  			}else{
	  				printf("NOT CLOSED\n");
	  			}

	  		}
	  		break;
	  	}
    }else{
  		printf("client TCB not found for %d \n",msg.header.src_port);
  	}
  }
  fflush(stdout);
  return 0;
}



