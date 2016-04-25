//
// FILE: srt_client.c
//
// Description: this file contains client states' definition, some important data structures
// and the client SRT socket interface definitions. You need to implement all these interfaces
//
// Date: April 18, 2008
//       April 21, 2008 **Added more detailed description of prototypes fixed ambiguities** ATC
//       April 26, 2008 ** Added GBN and send buffer function descriptions **
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include "srt_client.h"

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
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
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

void wait_for_some_time(int nano_sec){
	// sleep for 1000 nano seconds before next iteration 
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = nano_sec;
	nanosleep(&tim,NULL);
}
void reset_clients(){
	printf("reset_clients\n");
	for(int i = 0 ; i < MAX_TRANSPORT_CONNECTIONS ; i++){
		if(clients[i] != NULL){
			free(clients[i]);
		}
		clients[i] = NULL;
	}
	printf("reset_clients ends\n");
	fflush(stdout);
}

client_tcb_t * get_client_from_port(int port_num){
	printf("get_client_from_port port num = %d \n", port_num);
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
		printf("found\n");
		pthread_mutex_destroy(clients[sock_fd]->bufMutex);
        free(clients[sock_fd]->bufMutex);
		free(clients[sock_fd]);
		result = 1 ;
	}
	printf("freeClient ends \n");
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
		clients[clientCount] = (client_tcb_t *)malloc(sizeof(client_tcb_t));
		clients[clientCount]->client_portNum = client_port;
		clients[clientCount]->state = CLOSED;
		clients[clientCount]->next_seqNum = 0;
		clients[clientCount]->sendBufHead = NULL ;
		clients[clientCount]->sendBufunSent = NULL;
		clients[clientCount]->sendBufTail = NULL;
		clients[clientCount]->unAck_segNum = 0;
		clients[clientCount]->bufMutex =  (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
		assert(clients[clientCount]->bufMutex!=NULL);
		pthread_mutex_init(clients[clientCount]->bufMutex,NULL);
		printf("new socket id allocated to - %d for port num %d\n", clientCount, clients[clientCount]->client_portNum);	
		//TODO set to default value 	
	}else{
		printf("MAX CLIENTS REACHED :- %d \n", total_connection );
	}
	fflush(stdout);
	return clientCount;
}


int is_sock_fd_valid(int sockfd){
	printf("is_sock_fd_valid\n");
	if(sockfd < 0 || sockfd > MAX_TRANSPORT_CONNECTIONS || clients[sockfd] == NULL)
	{
		printf("Error in validating socket sockfd = %d max clients supported = %d\n", sockfd, MAX_TRANSPORT_CONNECTIONS);
		return 0;
	}
	return 1;
}
// This function initializes the TCB table marking all entries NULL. It also initializes 
// a global variable for the overlay TCP socket descriptor ``conn'' used as input parameter
// for snp_sendseg and snp_recvseg. Finally, the function starts the seghandler thread to 
// handle the incoming segments. There is only one seghandler for the client side which
// handles call connections for the client.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
void srt_client_init(int conn)
{
    printf("srt_client_init\n");
	mainTcpSockId = conn ;
	reset_clients();
	fflush(stdout);
	pthread_create(&pthread_receiver,NULL,seghandler,(void*)NULL);
  	return;
}


// This function looks up the client TCB table to find the first NULL entry, and creates
// a new TCB entry using malloc() for that entry. All fields in the TCB are initialized 
// e.g., TCB state is set to CLOSED and the client port set to the function call parameter 
// client port.  The TCB table entry index should be returned as the new socket ID to the client 
// and be used to identify the connection on the client side. If no entry in the TC table  
// is available the function returns -1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int srt_client_sock(unsigned int client_port)
{
  	printf("srt_client_sock\n");
	fflush(stdout);
  	return get_new_client_socket(client_port);
}


// This function is used to connect to the server. It takes the socket ID and the 
// server's port number as input parameters. The socket ID is used to find the TCB entry.  
// This function sets up the TCB's server port number and a SYN segment to send to
// the server using snp_sendseg(). After the SYN segment is sent, a timer is started. 
// If no SYNACK is received after SYNSEG_TIMEOUT timeout, then the SYN is 
// retransmitted. If SYNACK is received, return 1. Otherwise, if the number of SYNs 
// sent > SYN_MAX_RETRY,  transition to CLOSED state and return -1.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int srt_client_connect(int sockfd, unsigned int server_port)
{
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
	if(snp_sendseg(mainTcpSockId,&segment) < 0){ // error check for sendseg when there is TCP socket error
		printf("Error in sending message in TCP layer = %d \n", mainTcpSockId);
		return -1;
	}
	long start_time = current_time_millis(); // timer started
	trialNum++; // increment trial num
	client->state = SYNSENT ;
	while(client->state == SYNSENT){
		wait_for_some_time(LOOP_WAITING_TIME);
		long now = current_time_millis();
		long diff = now - start_time ;
		if((diff ) > SYN_TIMEOUT_MS){
			printf("TIMEOUT in connect %d for trial num = %d \n", sockfd, trialNum);
			if(trialNum < SYN_MAX_RETRY){
				printf("resending segment \n");
				if(snp_sendseg(mainTcpSockId,&segment) < 0){ // error check for sendseg when there is TCP socket error
					printf("ERROR in sending message sockfd = %d \n", sockfd);
					return -1;
				}
				start_time = (long)time(NULL); // reset timer
				trialNum++;
			}else{
				printf("MAX TRIAL reached in connect segment of sock fd %d\n" ,sockfd);
				client->state = CLOSED;
				return -1;
			}
		}	
	}
 	printf("srt_client_connect ends \n");
 	fflush(stdout);
  return 1; // returning success it can be rechecked with state = CONNECTED
}
/**
* this method will add a new segment to tcb. It will take care of head and tail to point to proper location. 
* It will also add proper sequence number to each segments and will update the next seq number in client tcb
* sequence number of segment passed would be the current seq number of client tcb
* nextSeq number would be next_seqNum + length of data segment i.e newSeg->seg.header.length
*/
void append_segment_to_client(client_tcb_t *client, segBuf_t *newSeg){
	printf("append_segment_to_client starts \n");
	pthread_mutex_lock(client->bufMutex);
	newSeg->seg.header.seq_num = client->next_seqNum;
	client->next_seqNum += newSeg->seg.header.length;
	if(client->sendBufHead != NULL) { // append to list
		printf("append to list \n");
		client->sendBufTail->next = newSeg;
		client->sendBufTail = client->sendBufTail->next;
	}else{ // first element of the linked list
		printf("append to head \n");
		client->sendBufHead = client->sendBufTail = client->sendBufunSent = newSeg;
	}
	pthread_mutex_unlock(client->bufMutex);
	printf("append_segment_to_client ends \n" );
}

// Send data to a srt server. This function should use the socket ID to find the TCP entry. 
// Then It should create segBufs using the given data and append them to send buffer linked list. 
// If the send buffer was empty before insertion, a thread called sendbuf_timer 
// should be started to poll the send buffer every SENDBUF_POLLING_INTERVAL time
// to check if a timeout event should occur. If the function completes successfully, 
// it returns 1. Otherwise, it returns -1.
// 
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int srt_client_send(int sockfd, void* data, unsigned int length)
{
	printf("srt_client_send starts for sockfd = %d \n",sockfd);
	if(is_sock_fd_valid(sockfd))
	{
		printf("sock fd is valid\n");
		client_tcb_t *client = clients[sockfd];
		if(client){
			printf("client is OK \n");
		}
		// initialize the buffer timeout thread;
		printf("initializing the POLLING thread\n");
		int totalSegment = length / MAX_SEG_LEN ;
		totalSegment = (length % MAX_SEG_LEN) ? totalSegment + 1: totalSegment ;
		printf("lenght = %d totalSegments = %d \n",length,totalSegment);
		for(int i = 0 ; i < totalSegment ; i++){
			printf("%d\n",i);
			segBuf_t *newSeg = (segBuf_t *) malloc(sizeof(segBuf_t));
			bzero(newSeg,sizeof(segBuf_t));
			newSeg->seg.header.src_port = client->client_portNum;
			newSeg->seg.header.dest_port = client->svr_portNum;
			newSeg->seg.header.type = DATA;
			if(i == totalSegment - 1 && length % MAX_SEG_LEN){
				printf("last segment number %d is less than max length\n",i);
				newSeg->seg.header.length = length % MAX_SEG_LEN;
			}else{
				newSeg->seg.header.length = MAX_SEG_LEN;
			}
			printf("segment length to be chunked is %d \n",newSeg->seg.header.length);
			// now casting data to char* since segment structure has a char array
			char *data_in_char = (char *)data;
			// newSeg->seg.data is char of size MAX_SEG_LEN
			// data_in_char is a an array of chars
			memcpy(newSeg->seg.data,&data_in_char[i*MAX_SEG_LEN],newSeg->seg.header.length);
			printf("chunked \"%s\" from full data %s\n", newSeg->seg.data , data_in_char);
		}
		return 1;
	}
	printf("srt_client_send ends\n");
    fflush(stdout);
	return -1;
}


// This function is used to disconnect from the server. It takes the socket ID as 
// an input parameter. The socket ID is used to find the TCB entry in the TCB table.  
// This function sends a FIN segment to the server. After the FIN segment is sent
// the state should transition to FINWAIT and a timer started. If the 
// state == CLOSED after the timeout the FINACK was successfully received. Else,
// if after a number of retries FIN_MAX_RETRY the state is still FINWAIT then
// the state transitions to CLOSED and -1 is returned.


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int srt_client_disconnect(int sockfd)
{
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
	if(snp_sendseg(mainTcpSockId,&segment) < 0){ // error check for sendseg when there is TCP socket error
		printf("Error in sending message in TCP layer = %d \n", mainTcpSockId);
		return -1;
	}
	long start_time = current_time_millis(); // timer started
	trialNum++; // increment trial num
	client->state = FINWAIT ;
	while(client->state == FINWAIT){
		wait_for_some_time(LOOP_WAITING_TIME);
		long now = current_time_millis();
		long diff = now - start_time ;
		if((diff ) > FIN_TIMEOUT_MS){
			printf("TIMEOUT in connect %d for trial num = %d \n", sockfd, trialNum);
			if(trialNum < FIN_MAX_RETRY){
				printf("resending segment \n");
				if(snp_sendseg(mainTcpSockId,&segment) < 0){ // error check for sendseg when there is TCP socket error
					printf("Error in sending message sockfd = %d \n", sockfd);
					return -1;
				}
				start_time = (long)time(NULL); // reset timer
				trialNum++;
			}else{
				printf("MAX TRIAL reached in connect segment of sock fd %d\n" ,sockfd);
				client->state = CLOSED;
				return -1;
			}
		}	
	}
	printf("srt_client_disconnect ends\n");
  	fflush(stdout);
  	return 1;
  }


// This function calls free() to free the TCB entry. It marks that entry in TCB as NULL
// and returns 1 if succeeded (i.e., was in the right state to complete a close) and -1 
// if fails (i.e., in the wrong state).
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
int srt_client_close(int sockfd)
{
	printf("srt_client_close\n");
  	fflush(stdout);
	return freeClient(sockfd);
}


// This is a thread  started by srt_client_init(). It handles all the incoming 
// segments from the server. The design of seghanlder is an infinite loop that calls snp_recvseg(). If
// snp_recvseg() fails then the overlay connection is closed and the thread is terminated. Depending
// on the state of the connection when a segment is received  (based on the incoming segment) various
// actions are taken. See the client FSM for more details.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void *seghandler(void* arg)
{
  printf("seghandler receive\n");
  seg_t msg ;
  client_tcb_t *client ;
  while(1){
  	if(snp_recvseg(mainTcpSockId,&msg) < 0){
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
	  				printf("CONNECTED\n");
	  				client->state = CONNECTED;
	  			}else{
	  				printf("ALREADY CONNECTED\n");
	  			}
	  		}
	  		break;
	  		case FINACK :{
	  			printf("FINACK RECIEVED client port = %d and server port = %d \n", client->svr_portNum, msg.header.src_port   );
	  			if(client->state == FINWAIT){
	  				printf("CLOSED\n");
	  				client->state = CLOSED;
	  			}else{
	  				printf("NOT CLOSED\n");
	  			}

	  		}
	  		break;
	  		case DATAACK: {

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



// This thread continuously polls send buffer to trigger timeout events
// It should always be running when the send buffer is not empty
// If the current time -  first sent-but-unAcked segment's sent time > DATA_TIMEOUT, a timeout event occurs
// When timeout, resend all sent-but-unAcked segments
// When the send buffer is empty, this thread terminates
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void* sendBuf_timer(void* clienttcb)
{	
  client_tcb_t *client = (client_tcb_t *) clienttcb ;	
  while(client->sendBufHead){
  		wait_for_some_time(SENDBUF_POLLING_INTERVAL);
  		printf("POLLING sendBuf_timer\n");
  		long currentTime = current_time_millis();
  		long diff = currentTime - client->sendBufHead->sentTime;
  		if(diff >= DATA_TIMEOUT){
  			printf("TIMEOUT for sent-but-unAcked\n");
  		}
  		// resend all 	
  }
  return 0;
}
