#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <strings.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include "srt_server.h"
#include "../common/constants.h"
#include "srt_server.h"


/*interfaces to application layer*/

//
//
//  SRT socket API for the server side application.
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
//  GOAL: Your job is to design and implement the prototypes below - fill in the code.
//

/**
* TCB array of connections currently connected to the server.
*/
svr_tcb_t *servers[MAX_TRANSPORT_CONNECTIONS];
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

int freeServer(int sock_fd){
	printf("freeServer\n");
	int result = -1;
	if(sock_fd < MAX_TRANSPORT_CONNECTIONS){
		free(servers[sock_fd]);
		result = 1 ;
	}
	printf("freeServer\n");
	fflush(stdout);
	return result;
}
void reset_servers(){
	printf("reset_servers\n");
	for(int i = 0 ; i < MAX_TRANSPORT_CONNECTIONS ; i++){
		if(servers[i] != NULL){
			free(servers[i]);
		}
		servers[i] = NULL;
	}
	fflush(stdout);
}
int get_new_server_socket(int server_port){
	printf("get_new_server_socket\n");
	int serverCount = -1;
	if(total_connection < MAX_TRANSPORT_CONNECTIONS){
		/* checking for the first null position in array */
		for(serverCount = 0 ; serverCount <  MAX_TRANSPORT_CONNECTIONS && servers[serverCount] != NULL; serverCount++);
		// above for loop finished with an index which is null. It assumes that total_connection is working correctly
		printf("new socket id allocated to - %d\n", serverCount);	
		servers[serverCount] = (svr_tcb_t *)malloc(sizeof(svr_tcb_t));
		servers[serverCount]->svr_portNum = server_port;
		servers[serverCount]->state = CLOSED;
		//TODO set to default value 	
	}else{
		printf("MAX CLIENTS REACHED :- %d \n", total_connection );
	}
	printf("get_new_server_socket ends\n");
	fflush(stdout);
	return serverCount;
}
svr_tcb_t * get_server_fron_port(int port_num){
	printf("get_server_fron_port\n");
	for(int i = 0 ; i < MAX_TRANSPORT_CONNECTIONS ; i++){
		if(servers[i] != NULL && servers[i]->svr_portNum == port_num){
			fflush(stdout);
			return servers[i];
		}
	}
	fflush(stdout);
	return NULL;
}

// srt server initialization
//
// This function initializes the TCB table marking all entries NULL. It also initializes
// a global variable for the overlay TCP socket descriptor ‘‘conn’’ used as input parameter
// for snp_sendseg and snp_recvseg. Finally, the function starts the seghandler thread to
// handle the incoming segments. There is only one seghandler for the server side which
// handles call connections for the client.
//

void srt_server_init(int conn) {
	printf("srt_server_init\n");
	mainTcpSockId = conn ;
	reset_servers();
	fflush(stdout);
	pthread_create(&pthread_receiver,NULL,seghandler,(void*)NULL);
	printf("srt_server_init ends\n");
  return;
}

// Create a server sock
//
// This function looks up the client TCB table to find the first NULL entry, and creates
// a new TCB entry using malloc() for that entry. All fields in the TCB are initialized
// e.g., TCB state is set to CLOSED and the server port set to the function call parameter
// server port.  The TCB table entry index should be returned as the new socket ID to the server
// and be used to identify the connection on the server side. If no entry in the TCB table
// is available the function returns -1.

int srt_server_sock(unsigned int port) {
	printf("srt_server_sock\n");
	fflush(stdout);
  return get_new_server_socket(port);
}

// Accept connection from srt client
//
// This function gets the TCB pointer using the sockfd and changes the state of the connetion to
// LISTENING. It then starts a timer to ‘‘busy wait’’ until the TCB’s state changes to CONNECTED
// (seghandler does this when a SYN is received). It waits in an infinite loop for the state
// transition before proceeding and to return 1 when the state change happens, dropping out of
// the busy wait loop. You can implement this blocking wait in different ways, if you wish.
//

int srt_server_accept(int sockfd) {
	printf("srt_server_accept\n");
	if(sockfd < 0 || sockfd > MAX_TRANSPORT_CONNECTIONS || servers[sockfd] == NULL){
		printf("Error in srt_server_accept sockfd = %d max clients supported = %d\n", sockfd, MAX_TRANSPORT_CONNECTIONS);
		return -1;
	}
	svr_tcb_t *server = servers[sockfd];
	if(server->state == CLOSED){
		printf("changing state to LISTENING\n");
		server->state = LISTENING;
		while(server->state != CONNECTED){
			// sleep for 1000 nano seconds before next iteration 
			struct timespec tim;
			tim.tv_sec = 0;
			tim.tv_nsec = 1000;
			nanosleep(&tim,NULL);
		}
		printf("state changed to CONNECTED \n");
	}else{
		printf("server state id not CLOSED hence exiting\n");
		return -1;
	}
	printf("srt_server_accept ends\n");
	return 1; // state should be equal to CONNECTED 
}

// Receive data from a srt client
//
// Receive data to a srt client. Recall this is a unidirectional transport
// where DATA flows from the client to the server. Signaling/control messages
// such as SYN, SYNACK, etc.flow in both directions. You do not need to implement
// this for Lab3. We will use this in Lab4 when we implement a Go-Back-N sliding window.
//
int srt_server_recv(int sockfd, void* buf, unsigned int length) {
	return 1;
}

// Close the srt server
//
// This function calls free() to free the TCB entry. It marks that entry in TCB as NULL
// and returns 1 if succeeded (i.e., was in the right state to complete a close) and -1
// if fails (i.e., in the wrong state).
//

int srt_server_close(int sockfd) {
	printf("srt_server_close\n");
	return freeServer(sockfd);
}
void *closewait_timeout(void *server_args){
	svr_tcb_t *server = (svr_tcb_t *) server_args;
	if(server){
		sleep(CLOSEWAIT_TIME);
		server->state = CLOSED;
	}
}
void handle_syn_recieve(svr_tcb_t *server, seg_t *msg){
	printf("handle_syn_recieve \n");
	if(server->state == LISTENING){
		server->state = CONNECTED;
	}
	server->client_portNum = msg->header.src_port; // update client port
	seg_t segment;
	bzero(&segment,sizeof(segment));
	segment.header.src_port = msg->header.dest_port;
	segment.header.dest_port = msg->header.src_port;
	segment.header.type = SYNACK;
	segment.header.length = 0;
	//  first trial
	if(sendseg(mainTcpSockId,&segment) < 0){ // error check for sendseg when there is TCP socket error
		printf("Error in sending message sockfd = %d \n", mainTcpSockId);
	}
	printf("handle_syn_recieve ends\n");
}
void handle_fin_recieve(svr_tcb_t *server, seg_t *msg){
	printf("handle_fin_recieve \n");
	if(server->state == CONNECTED){
		server->state = CLOSEWAIT;
		pthread_t closewait ;
		pthread_create(&closewait,NULL,closewait_timeout,(void*)server);
		printf("handle_fin_recieve ends\n");
	}
	seg_t segment;
	bzero(&segment,sizeof(segment));
	segment.header.src_port = msg->header.dest_port;
	segment.header.dest_port = msg->header.src_port;
	segment.header.type = FINACK;
	segment.header.length = 0;
	//  first trial
	if(sendseg(mainTcpSockId,&segment) < 0){ // error check for sendseg when there is TCP socket error
		printf("Error in sending message sockfd = %d \n", mainTcpSockId);
	}
	printf("handle_fin_recieve ends \n");
}
// Thread handles incoming segments
//
// This is a thread  started by srt_server_init(). It handles all the incoming
// segments from the client. The design of seghanlder is an infinite loop that calls snp_recvseg(). If
// snp_recvseg() fails then the overlay connection is closed and the thread is terminated. Depending
// on the state of the connection when a segment is received  (based on the incoming segment) various
// actions are taken. See the client FSM for more details.
//

void *seghandler(void* arg) {
  printf("seghandler receive\n");
  svr_tcb_t *server ;
  seg_t msg ;
  while(1){
  	if(recvseg(mainTcpSockId,&msg) < 0){
  		printf("client recvseg return negative hence exiting\n");
  		printf("exiting thread and closing main tcp connection\n");
  		fflush(stdout);
  		close(mainTcpSockId);
		pthread_exit(NULL);
  	}
  	printf("seghandler message receive\n");
  	server = get_server_fron_port(msg.header.dest_port);
  	if(server){
  		switch (msg.header.type){
  			case SYN:{
  				printf("SYN received\n");
  				handle_syn_recieve(server,&msg);
  			}
  			break;
  			case FIN:{
  				handle_syn_recieve(server,&msg);
  				printf("FIN received\n");
  			}
  			break;
  		}
  	}else{
  		printf("server TCB not found for %d \n", msg.header.dest_port);
  	}
  }
  return 0;
}
