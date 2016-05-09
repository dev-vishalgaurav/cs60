//
// srt_client.c
//
// 4/26/2010
//
// This file contains pseudocode for each of the srt functions.
// See srt_client.h for more in depth descriptions.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "srt_client.h"

// declare tcbtable as global variable
client_tcb_table_t tcbtable;
// declare the connection to the network layer as global variable, 
// it's an overlay tcp connection for lab4 and lab5
int nl_conn;

///////////////////////////////
/* tcbtable helper functions */
///////////////////////////////

//get the sock tcb indexed by sockfd
client_tcb_t* tcbtable_gettcb(int sockfd) {
	if(tcbtable.isUsed[sockfd])
		return &tcbtable.tcb[sockfd];
	else
		return 0;
}

//get the sock tcb from the given client port number 
client_tcb_t* tcbtable_gettcbFromPort(unsigned int clientPort)
{
	int i;
	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		if(tcbtable.tcb[i].client_portNum==clientPort && tcbtable.isUsed[i]) {
			return &tcbtable.tcb[i];
		}
	}
	return 0;
}

//get an new tcb from tcbtable, assign the client port
int tcbtable_newtcb(unsigned int port) {
	int i;

	for(i=0; i<MAX_TRANSPORT_CONNECTIONS; i++) {
		if( tcbtable.isUsed[i] == 1 && tcbtable.tcb[i].client_portNum == port ) {
			return -1;
		}
	}

	for(i=0; i<MAX_TRANSPORT_CONNECTIONS; i++) {
		if(tcbtable.isUsed[i] == 0) {
			tcbtable.isUsed[i] = 1;
			tcbtable.tcb[i].client_portNum = port;
			return i;
		}
	}
	return -1;
}

/////////////////////////////////////
/* Interfaces to application layer */
/////////////////////////////////////

// srt_client_init(int conn)
// Initialize data structures for srt functions.
// 
// Input: Connection socket
// Return:
//
// Pseudocode
//
// 1) Go through tcb table and initialize tcb entries to 0
// 2) Store connection for use later
// 3) Spawn new thread for segment handler
//
void srt_client_init(int conn) {
	//initialize global variables
	int i;
	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		tcbtable.isUsed[i] = 0;
	}
	nl_conn = conn;

	//create the seghandler 
	pthread_t seghandler_thread;
	pthread_create(&seghandler_thread,NULL,seghandler, (void*)0);
}

// srt_client_sock(unsigned int client_port)
// Create a client tcb and return the sockfd.
// Input : Client port
// Return : sockfd on success, -1 on failure
//
// Pseudocode
//
// 1) Find an unused tcb for this connection
// 2) Obtain pointer to that tcb
// 3) Initialize the tcb for a new connection
//
int srt_client_sock(unsigned int client_port) {
	// get a new tcb
	int sockfd = tcbtable_newtcb(client_port);
	if(sockfd<0)
		return -1;
	client_tcb_t* my_clienttcb = tcbtable_gettcb(sockfd);

	//initialize tcb
	my_clienttcb->next_seqNum = 0;
	my_clienttcb->state = CLIENT_CLOSED;	
	my_clienttcb->sendBufHead = 0;
	my_clienttcb->sendBufunSent = 0;
	my_clienttcb->sendBufTail = 0;
	my_clienttcb->unAck_segNum = 0;

	return sockfd;

}

// srt_client_connect(int sockfd, unsigned int server_port)
// Connect to a srt server.
// Input : sockfd , server_port
// Return : 1 when client in CONNECTED state, -1 on failure
//
// Psueudocode
//
// 1) Get pointer to tcb for sockfd
// 2) Set server port num of tcb to server_port input
// 3) Fill in header of a seg_t
// 4) Send seg_t to server and update state of tcb to SYNSENT
// 5) while(RETRY > 0) // Try again if didn't work
//      nanosleep(SYNSEG_TIMEOUT_NS)
//      if(tcp->state == CONNECTED)
//        return 1
//      else
//        send seg_t again
//        RETRY--
//
int srt_client_connect(int sockfd, unsigned int server_port) {
	//get tcb indexed by sockfd
	client_tcb_t* clienttcb;
	clienttcb = tcbtable_gettcb(sockfd);
	if(!clienttcb)
		return -1;

	//assigned the given server port
	clienttcb->svr_portNum = server_port;

	//send SYN to server
	seg_t syn;
	syn.header.type = SYN;
	syn.header.src_port = clienttcb->client_portNum;
	syn.header.dest_port = clienttcb->svr_portNum;
	syn.header.seq_num = 0;
	syn.header.length = 0;
	snp_sendseg(nl_conn, &syn);	
	printf("CLIENT: SYN SENT\n");
	
	//state transition
	clienttcb->state = CLIENT_SYNSENT;
	
	//retry in case  timeout
	int retry = SYN_MAX_RETRY;
	while(retry>0) {
		struct timespec req;
		req.tv_sec = 0;
		req.tv_nsec = SYNSEG_TIMEOUT_NS;
		struct timespec rm;
		nanosleep(&req,&rm);
		if(clienttcb->state == CLIENT_CONNECTED) {
			return 1;
		}
		else { 
			snp_sendseg(nl_conn, &syn);	
			retry--;
		}
	}
	return -1;
}

// srt_client_send(int sockfd, void* data, unsigned int length)
// Send data to a srt server
int srt_client_send(int sockfd, void* data, unsigned int length) {
	return 1;
}

// srt_client_disconnect(int sockfd)
// Disconnect from a srt server
// Input: sockfd
// Return : 1 when tcb state becomes CLOSED, -1 on failure
//
// Pseudocode
//
// 1) Get pointer to tcb for sockfd
// 2) If tcb state not CONNECTED return -1
// 3) Fill out a seg_t header for FIN message
// 4) while(RETRY > 0)
//      nanosleep(FINSEG_TIMEOUT_NS)
//      if(tcb state == CLOSED)
//        zero out tcb
//      else
//        resend FIN message
//        RETRY--
//
int srt_client_disconnect(int sockfd) {
	//get the tcb indexed by sockfd
	client_tcb_t* clienttcb;
	clienttcb = tcbtable_gettcb(sockfd);
	if(!clienttcb)
		return -1;

	if(clienttcb->state != CLIENT_CONNECTED)	
		return -1;	
	//send FIN 
	seg_t fin;
	fin.header.type = FIN;
	fin.header.src_port = clienttcb->client_portNum;
	fin.header.dest_port = clienttcb->svr_portNum;
	fin.header.length = 0;
	snp_sendseg(nl_conn, &fin);
	printf("CLIENT: FIN SENT\n");
	//state transition
	clienttcb->state = CLIENT_FINWAIT;
	printf("CLIENT: FINWAIT\n");
	
	//resend in case of timeout
	int retry = FIN_MAX_RETRY;
	while(retry>0) {
		struct timespec req;
		req.tv_sec = 0;
		req.tv_nsec = FINSEG_TIMEOUT_NS;
		struct timespec rm;
		nanosleep(&req,&rm);
		if(clienttcb->state == CLIENT_CLOSED) {
			clienttcb->svr_nodeID = 0;
			clienttcb->svr_portNum = 0;
			clienttcb->next_seqNum = 0;
			return 1;
		}
		else {
			printf("CLIENT: FIN RESENT\n");
			snp_sendseg(nl_conn, &fin);
			retry--;
		}	
	}
	return -1;
}




// srt_client_close(int sockfd)
// Close srt client.
// Input: sockfd
// Return : 1 on success, -1 on failure
//
// Pseudocode
//
// 1) Get pointer to tcb corresponding to sockfd
// 2) Set tcb state to CLOSED
// 3) Set sockfd entry of tcbtable to NOT_USED
//
int srt_client_close(int sockfd) {
	//get tcb indexed by sockfd
	client_tcb_t* clienttcb;
	clienttcb = tcbtable_gettcb(sockfd);
	if(!clienttcb)
		return -1;

	if(clienttcb->state!=CLIENT_CLOSED)
		return -1;

	tcbtable.isUsed[sockfd] = 0;
	return 1;
}

// seghandler(void* arg)
// Thread that handles incoming segments.
// Input:
// Return:
//
// Pseudocode
//
// while(1)
//  1) Get segment using snp function snp_recvseg, kill thread if something
//     went wrong
//  2) Find tcb to handle this segment by looking up tcbs using the dest
//     port in the segment header
//  3) switch(tcb state)
//      case CLOSED: break;
//      case CONNECTED: break;
//      case SYNSENT:
//        if(seg.header.type==SYNACK && tcb->svrport==seg.header.srcport)
//          set tcb state to CONNECTED
//        break;
//      case FINWAIT:
//        if(seg.header.type==FINACK && tcb->svrport==seg.header.srcport)
//          set tcb state to CLOSED
//          break;
//
void* seghandler(void* arg) {
	seg_t segBuf;
	while(1) {
		//receive a segment
		if(snp_recvseg(nl_conn,&segBuf)<0) {
			close(nl_conn);
			pthread_exit(NULL);
		}

		//find the tcb to handle the segment
		client_tcb_t* my_clienttcb = tcbtable_gettcbFromPort(segBuf.header.dest_port);
		if(!my_clienttcb)
			continue;

		//segment handling
		switch(my_clienttcb->state) {
			case CLIENT_CLOSED:
			break;
			case CLIENT_SYNSENT:
			if(segBuf.header.type==SYNACK&&my_clienttcb->svr_portNum==segBuf.header.src_port) {
				printf("CLIENT: SYNACK RECEIVED\n");
				my_clienttcb->state = CLIENT_CONNECTED;
				printf("CLIENT: CONNECTED\n");
			}
			break;
			case CLIENT_CONNECTED:	
			break;
			case CLIENT_FINWAIT:
			if(segBuf.header.type==FINACK&&my_clienttcb->svr_portNum==segBuf.header.src_port) {
				printf("CLIENT: FINACK RECEIVED\n");
				my_clienttcb->state = CLIENT_CLOSED;
				printf("CLIENT: CLOSED\n");
			}
			break;
		}
	}
}
