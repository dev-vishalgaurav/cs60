///
// srt_server.c
//
// 4/26/2010
//
// This file contains pseudocode for the srt functions.
// See srt_server.h for descriptions of the functions.
//

#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "srt_server.h"
#include "../common/constants.h"

//declare tcbtable as global variable
svr_tcb_table_t tcbtable;
//declare the connection to the network layer as global variable
int nl_conn;

///////////////////
// Helper functions
///////////////////

/*tcb table operations*/
//get the tcb indexed by sockfd
svr_tcb_t* tcbtable_gettcb(int sockfd) {
	if(tcbtable.isUsed[sockfd])
		return &tcbtable.tcb[sockfd];
	else
		return 0;
}

//get the tcb  with given server port 
svr_tcb_t* tcbtable_gettcbFromPort(unsigned int serverPort)
{
	int i;
	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		if(tcbtable.tcb[i].svr_portNum==serverPort&&tcbtable.isUsed[i]) {
			return &tcbtable.tcb[i];
		}
	}
	return 0;
}


//get a new tcb from tcbtable and return the sockfd
int tcbtable_newtcb(unsigned int port) {
	int i;
	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		if(tcbtable.isUsed[i]==1&&tcbtable.tcb[i].svr_portNum==port) {
			return -1;
		}
	}

	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++) {
		if(tcbtable.isUsed[i]==0) {
			tcbtable.isUsed[i]=1;
			tcbtable.tcb[i].svr_portNum = port;
			return i;
		}
	}
	return -1;
}

//////////////////////////////////
// Interfaces to application layer
//////////////////////////////////

// srt_server_init(int conn)
// Initialize srt server.
// Input: conn
// Return :
//
// Pseudocode
//
// 1) Set all tcbs in tcbtable to NOT_USED
// 2) Spawn thread for seghandler
//
void srt_server_init(int conn) {
	//initialize global variables
	int i;
	for(i=0;i<MAX_TRANSPORT_CONNECTIONS;i++)
		tcbtable.isUsed[i] = 0;
	nl_conn = conn;

	//create seghandler thread 
	pthread_t seghandler_thread;
	pthread_create(&seghandler_thread,NULL,seghandler, (void*)0);
}

// srt_server_sock(unsigned int port)
// Create a server sock structure
// Input: port
// Return : sockfd if successful, -1 on failure
//
// Pseudocode
//
// 1) Find an unused tcb structure for this connection
// 2) Set the tcb state to CLOSED
//
int srt_server_sock(unsigned int port) {
	//get a tcb from tcb table
	int sockfd = tcbtable_newtcb(port);
	if(sockfd<0)
		return -1;
	
	// initialize  server tcb
	tcbtable.tcb[sockfd].state = SVR_CLOSED;
	return sockfd;
}

// srt_server_accept(int sockfd)
// Accept connection from srt client
// Input: sockfd
// Return : 1 when tcb state in CONNECTED state, -1 on failure
//
// Pseudocode
//
// 1) Get pointer to tcb corresponding to  sockfd
// 2) Set server tcb state to LISTENING
// 3) while(1)
//      if(tcb state == CONNECTED)
//        break;
//      else
//        nanosleep(BUSY_WAIT_TIME)
//
int srt_server_accept(int sockfd) {
	//get tcb indexed by sockfd
	svr_tcb_t* my_servertcb;
	my_servertcb = tcbtable_gettcb(sockfd);
	if(!my_servertcb)
		return -1;

	//state transition
	my_servertcb->state = SVR_LISTENING;

	//busy wait until state transitions to CONNECTED
	while(1) {
		if (my_servertcb->state == SVR_CONNECTED)
			break;
		else {
			struct timespec req;
			req.tv_sec = 0;
			req.tv_nsec = 10000000;
			struct timespec rm;
			nanosleep(&req,&rm);
		}
	}
	return 1;
}

// srt_server_recv(int sockfd, void* buf, unsigned int length)
// Receive data from a srt client
int srt_server_recv(int sockfd, void* buf, unsigned int length) {
	return 1;
}

// srt_server_close(int sockfd)
// Close the srt server
// Input: sockfd
// Return : 1 when tcb in state CLOSED, -1 on failure
//
// Pseudocode
//
// 1) Get pointer to tcb structure corresponding to sockfd
// 2) Set tcb state to CLOSED
// 3) Mark tcb as unused
//
int srt_server_close(int sockfd) {
	//get tcb indexed by sockfd
	svr_tcb_t* servertcb;
	servertcb = tcbtable_gettcb(sockfd);
	if(!servertcb)
		return -1;
	
	if(servertcb->state!=SVR_CLOSED)
		return -1;

	tcbtable.isUsed[sockfd]= 0;	
	return 1;
}

// seghandler(void* arg)
// Thread that handles incoming segments
// Input:
// Return :
//
// Pseudocode
//
// while(1)
//  Get segment using snp function recvseg
//  Get tcb by looking up tcb table by dest port in segment header
//  switch(tcb state)
//    case CLOSED: break;
//    case LISTENING:
//      if(seg.header.type==SYN)
//        tcb.portnum = seg.header.srcport
//        acknowledge SYN with syn_received function
//        tcb.state = CONNECTED
//      break;
//    case CONNECTED:
//      if(seg.header.type==SYN && tcb.portnum==seg.header.srcport)
//        acknowledge SYN with syn_received function
//      else if(seg.header.type==DATA)
//      else if(seg.header.type==FIN && tcb.portnum==seg.header.srcport)
//        tcb state = CLOSEWAIT
//        spawn thread to handle CLOSEWAIT state transition (see closewait below)
//        acknowledge FIN with fin_received
//      break;
//    case CLOSEWAIT
//      acknowledge FIN with fin_received
//      break;
//
void* seghandler(void* arg) {
	seg_t segBuf;
	svr_tcb_t* my_servertcb;

	while(1) {
		//receive a segment
		if(snp_recvseg(nl_conn, &segBuf)<0) {
			close(nl_conn);
			pthread_exit(NULL);
		}
		//find the tcb to handle the segment
		my_servertcb = tcbtable_gettcbFromPort(segBuf.header.dest_port);
		if(!my_servertcb)
			continue;
		
		//segment handling
		switch(my_servertcb->state) {
			case SVR_CLOSED:
				break;
			case SVR_LISTENING:
				//waiting for SYN segment from client
				if(segBuf.header.type==SYN) {
					// SYN received
					printf("SERVER: SYN RECEIVED\n");
					//update servertcb and send SYNACK back
					my_servertcb->client_portNum = segBuf.header.src_port;
					syn_received(my_servertcb,&segBuf);
					//state srtsition
					my_servertcb->state=SVR_CONNECTED;
					printf("SERVER: CONNECTED\n");
				}
				break;
			case SVR_CONNECTED:	
				if(segBuf.header.type==SYN&&my_servertcb->client_portNum==segBuf.header.src_port) {
					// SYN received
					printf("SERVER: SYN RECEIVED\n");
					//update servertcb
					syn_received(my_servertcb,&segBuf);
				}
				else if(segBuf.header.type==DATA) {
					//data_received(my_servertcb,&segBuf);
				}
				else if(segBuf.header.type==FIN&&my_servertcb->client_portNum==segBuf.header.src_port) {
					//state transition
					printf("SERVER: FIN RECEIVED\n");
		 			my_servertcb->state = SVR_CLOSEWAIT;	
					printf("SERVER: CLOSEWAIT\n");
					//start a closewait timer
					pthread_t cwtimer;
					pthread_create(&cwtimer,NULL,closewait, (void*)my_servertcb);
					//send FINACK back
					fin_received(my_servertcb,&segBuf);
				}		
				break;
			case SVR_CLOSEWAIT:
				if(segBuf.header.type==FIN&&my_servertcb->client_portNum==segBuf.header.src_port) {
					printf("SERVER: FIN RECEIVED\n");
					//send FINACK back
					fin_received(my_servertcb,&segBuf);
				}
				break;
		}
	}
}

///////////////////////
// Other help functions
///////////////////////

// closewait(void* servertcb)
// Thread that implements closewait timer to transition from
// CLOSEWAIT to CLOSED
// Input: servertcb to close
// Return :
//
// Pseudocode
//
// 1) Sleep for CLOSEWAIT_TIME
// 2) Set tcb state to CLOSED
//
void* closewait(void* servertcb) {
	svr_tcb_t* my_servertcb = (svr_tcb_t*)servertcb;
	sleep(CLOSEWAIT_TIME);
	//timerout, state transitions to SVR_CLOSED
	my_servertcb->state = SVR_CLOSED;
	printf("SERVER: CLOSED\n");
	return NULL;
}

// syn_received(svr_tcb_t* svrtcb, seg_t* syn)
// Send SYNACK to client
// Input: server tcb struct, segment
// Return :
//
// Pseudocode
//
// 1) Set tcb.expected_seqnum to seg.header.seq_num
// 2) Fill out seg_t structure for SYNACK
// 3) Send SYNACK using snp function sendseg
//
void syn_received(svr_tcb_t* svrtcb, seg_t* syn) {
	//update expected sequence
	svrtcb->expect_seqNum = syn->header.seq_num;
	//send SYNACK back
	seg_t synack;
	synack.header.type=SYNACK;
	synack.header.src_port = svrtcb->svr_portNum;
	synack.header.dest_port = svrtcb->client_portNum;
	synack.header.length = 0;
	snp_sendseg(nl_conn,&synack);
	printf("SERVER: SYNACK SENT\n");
}

// data_received(svr_tcb_t* svrtcb, seg_t* data)
// Handling DATA segment
void data_received(svr_tcb_t* svrtcb, seg_t* data) {
}

// fin_received(svr_tcb_t* svrtcb, seg_t* fin)
// Handle FIN segment
// Input: server tcb, fin segment to send
// Return :
//
// Pseudocode
//
// 1) Fill seg_t header with FINACH and correct port numbers
// 2) Use snp sendseg() function to send segment to client
//
void fin_received(svr_tcb_t* svrtcb, seg_t* fin) {
	seg_t finack;
	finack.header.type=FINACK;
	finack.header.src_port = svrtcb->svr_portNum;
	finack.header.dest_port = svrtcb->client_portNum;
	finack.header.length = 0;
	snp_sendseg(nl_conn,&finack);
	printf("SERVER: FINACK SENT\n");
}
