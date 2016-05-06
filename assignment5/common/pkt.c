// File pkt.c
// May 03, 2010

#include "pkt.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/utsname.h>
#include <assert.h>

// overlay_sendpkt() is called by the SNP process to request the ON 
// process to send a packet out to the overlay network. The 
// ON process and SNP process are connected with a local TCP connection. 
// In overlay_sendpkt(), the packet and its next hop's nodeID are encapsulated 
// in a sendpkt_arg_t data structure and sent over this TCP connection to the ON process. 
// The parameter overlay_conn is the TCP connection's socket descriptior 
// between the SNP process and the ON process.
// When sending the sendpkt_arg_t data structure over the TCP connection between the SNP 
// process and the ON process, use '!&'  and '!#' as delimiters. 
// Send !& sendpkt_arg_t structure !# over the TCP connection.
// Return 1 if sendpkt_arg_t data structure is sent successfully, otherwise return -1.
int overlay_sendpkt(int nextNodeID, snp_pkt_t* pkt, int overlay_conn)
{
  int result = -1;
  if(overlay_conn >= 0 ){
  	char startChars[2] ;
  	startChars[0] = '!';
  	startChars[1] = '&';
  	char endChars[2] ;
  	endChars[0] = '!';
  	endChars[1] = '#';
  	sendpkt_arg_t *pktData = (sendpkt_arg_t *)malloc(sizeof(sendpkt_arg_t));
  	pktData->nextNodeID = nextNodeID; // assign node ID in paket data to be sent
  	pktData->pkt = *pkt; // assign packet in data 
  	// calculating segment length = size of nextNodeId + size of snp header (mandatory) + length of data mentioned in header.
  	int segmentLength =  sizeof(int) + sizeof(snp_hdr_t) + pkt->header.length ;
  	// all send should be success first send is start of message, middle is data and last sent is end of message
  	if(send(overlay_conn,startChars,2 ,0) >= 0 && send(overlay_conn, pktData,segmentLength ,0) >= 0 && send(overlay_conn, endChars,2 ,0) >=  0){ 
  		printf("PACKET sendpkt_arg_t sent to conn id %d\n",overlay_conn);
		result = 1;  // packet sending success		
  	}else{
  		printf("error in sending overlay_sendpkt nextNodeID = %d, overlay_conn = %d \n" , nextNodeID, overlay_conn);
  	}
  	free(pktData); // free pktData in all cases 
  }
  return result;
}


// overlay_recvpkt() function is called by the SNP process to receive a packet 
// from the ON process. The parameter overlay_conn is the TCP connection's socket 
// descriptior between the SNP process and the ON process. The packet is sent over 
// the TCP connection between the SNP process and the ON process, and delimiters 
// !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data 
// Return 1 if a packet is received successfully, otherwise return -1.
int overlay_recvpkt(snp_pkt_t* pkt, int overlay_conn)
{
	char buf[sizeof(snp_pkt_t)+2]; 
	char c;
	int idx = 0;
	// state can be 0,1,2,3; 
	// 0 starting point 
	// 1 '!' received
	// 2 '&' received, start receiving segment
	// 3 '!' received,
	// 4 '#' received, finish receiving segment 
	int state = 0; 
	while(recv(overlay_conn,&c,1,0)>0) {
		if (state == 0) {
		        if(c=='!')
				state = 1;
		}
		else if(state == 1) {
			if(c=='&') 
				state = 2;
			else
				state = 0;
		}
		else if(state == 2) {
			if(c=='!') {
				buf[idx]=c;
				idx++;
				state = 3;
			}
			else {
				buf[idx]=c;
				idx++;
			}
		}
		else if(state == 3) {
			if(c=='#') {
				buf[idx]=c;
				idx++;
				memcpy(pkt,buf,sizeof(snp_pkt_t));
				printf("PACKET in overlay_recvpkt received \n");
				state = 0;
				idx = 0 ;
				return 1;
			}
			else if(c=='!') {
				buf[idx]=c;
				idx++;
			}
			else {
				buf[idx]=c;
				idx++;
				state = 2;
			}
		}
	}
	printf("PACKET ERROR in receiving overlay_recvpkt\n");
	return -1;
}


// This function is called by the ON process to receive a sendpkt_arg_t data structure.
// A packet and the next hop's nodeID is encapsulated  in the sendpkt_arg_t structure.
// The parameter network_conn is the TCP connection's socket descriptior between the
// SNP process and the ON process. The sendpkt_arg_t structure is sent over the TCP 
// connection between the SNP process and the ON process, and delimiters !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data
// Return 1 if a sendpkt_arg_t structure is received successfully, otherwise return -1.
int getpktToSend(snp_pkt_t* pkt, int* nextNode, int network_conn)
{	
  // sendpkt_arg_t required inorder to get nextNodeId 
  sendpkt_arg_t *recvPackerArg = (sendpkt_arg_t *)malloc(sizeof(sendpkt_arg_t));
  char buf[sizeof(sendpkt_arg_t)+2]; 
  char c;
  int idx = 0;
  // state can be 0,1,2,3; 
  // 0 starting point 
  // 1 '!' received
  // 2 '&' received, start receiving segment
  // 3 '!' received,
  // 4 '#' received, finish receiving segment 
  int state = 0; 
  while(recv(network_conn,&c,1,0)>0) {
  	if (state == 0) {
  		if(c=='!')
  			state = 1;
  		}
		else if(state == 1) {
			if(c=='&') 
				state = 2;
			else
				state = 0;
		}
		else if(state == 2) {
			if(c=='!') {
				buf[idx]=c;
				idx++;
				state = 3;
			}
			else {
				buf[idx]=c;
				idx++;
			}
		}
		else if(state == 3) {
			if(c=='#') {
				buf[idx]=c;
				idx++;
				memcpy(recvPackerArg,buf,sizeof(sendpkt_arg_t));
				*pkt = recvPackerArg->pkt;
				*nextNode = recvPackerArg->nextNodeID;
				state = 0;
				idx = 0;
				printf("PACKET recieved in getpktToSend received \n");
				free(recvPackerArg);
				return 1;
			}
			else if(c=='!') {
				buf[idx]=c;
				idx++;
			}
			else {
				buf[idx]=c;
				idx++;
				state = 2;
			}
		}
	}
	
	printf("PACKET ERROR in receiving getpktToSend \n");
	free(recvPackerArg);
	return -1;
}




// forwardpktToSNP() function is called after the ON process receives a packet from 
// a neighbor in the overlay network. The ON process calls this function 
// to forward the packet to SNP process. 
// The parameter network_conn is the TCP connection's socket descriptior between the SNP 
// process and ON process. The packet is sent over the TCP connection between the SNP process 
// and ON process, and delimiters !& and !# are used. 
// Send !& packet data !# over the TCP connection.
// Return 1 if the packet is sent successfully, otherwise return -1.
int forwardpktToSNP(snp_pkt_t* pkt, int network_conn)
{
  int result = -1;
  if(network_conn >= 0 ){
  	char startChars[2] ;
  	startChars[0] = '!';
  	startChars[1] = '&';
  	char endChars[2] ;
  	endChars[0] = '!';
  	endChars[1] = '#';
  	// calculating segment length = size of snp header (mandatory) + length of data mentioned in header.
  	int segmentLength =  sizeof(snp_hdr_t) + pkt->header.length ;
  	// all send should be success first send is start of message, middle is data and last sent is end of message
  	if(send(network_conn,startChars,2 ,0) >= 0 && send(network_conn, pkt,segmentLength ,0) >= 0 && send(network_conn, endChars,2 ,0) >=  0){ 
  		printf("PACKET forwardpktToSNP sent to conn id %d\n",network_conn);
		result = 1;  // packet sending success		
  	}else{
  		printf("error forwardpktToSNP conn id = %d \n",network_conn);
  	}
  }
  return result;
}



// sendpkt() function is called by the ON process to send a packet 
// received from the SNP process to the next hop.
// Parameter conn is the TCP connection's socket descritpor to the next hop node.
// The packet is sent over the TCP connection between the ON process and a neighboring node,
// and delimiters !& and !# are used. 
// Send !& packet data !# over the TCP connection
// Return 1 if the packet is sent successfully, otherwise return -1.
int sendpkt(snp_pkt_t* pkt, int conn)
{
  int result = -1;
  if(conn >= 0 ){
  	char startChars[2] ;
  	startChars[0] = '!';
  	startChars[1] = '&';
  	char endChars[2] ;
  	endChars[0] = '!';
  	endChars[1] = '#';
  	// calculating segment length = size of snp header (mandatory) + length of data mentioned in header.
  	int segmentLength =  sizeof(snp_hdr_t) + pkt->header.length ;
  	// all send should be success first send is start of message, middle is data and last sent is end of message
  	if(send(conn,startChars,2 ,0) >= 0 && send(conn, pkt,segmentLength ,0) >= 0 && send(conn, endChars,2 ,0) >=  0){ 
  		printf("PACKET sendpkt sent to conn id %d\n",conn);
		result = 1;  // packet sending success		
  	}else{
  		printf("error sendpkt conn id = %d \n",conn);
  	}
  	fflush(stdout);
  }
  return result;
}



// recvpkt() function is called by the ON process to receive 
// a packet from a neighbor in the overlay network.
// Parameter conn is the TCP connection's socket descritpor to a neighbor.
// The packet is sent over the TCP connection  between the ON process and the neighbor,
// and delimiters !& and !# are used. 
// To receive the packet, this function uses a simple finite state machine(FSM)
// PKTSTART1 -- starting point 
// PKTSTART2 -- '!' received, expecting '&' to receive data 
// PKTRECV -- '&' received, start receiving data
// PKTSTOP1 -- '!' received, expecting '#' to finish receiving data 
// Return 1 if the packet is received successfully, otherwise return -1.
int recvpkt(snp_pkt_t* pkt, int conn)
{
	char buf[sizeof(snp_pkt_t)+2]; 
	char c;
	int idx = 0;
	// state can be 0,1,2,3; 
	// 0 starting point 
	// 1 '!' received
	// 2 '&' received, start receiving segment
	// 3 '!' received,
	// 4 '#' received, finish receiving segment 
	int state = 0; 
	while(recv(conn,&c,1,0)>0) {
		if (state == 0) {
		        if(c=='!')
				state = 1;
		}
		else if(state == 1) {
			if(c=='&') 
				state = 2;
			else
				state = 0;
		}
		else if(state == 2) {
			if(c=='!') {
				buf[idx]=c;
				idx++;
				state = 3;
			}
			else {
				buf[idx]=c;
				idx++;
			}
		}
		else if(state == 3) {
			if(c=='#') {
				buf[idx]=c;
				idx++;
				memcpy(pkt,buf,sizeof(snp_pkt_t));
				printf("PACKET in recvpkt received \n");
				state = 0;
				idx = 0 ;
				return 1;
			}
			else if(c=='!') {
				buf[idx]=c;
				idx++;
			}
			else {
				buf[idx]=c;
				idx++;
				state = 2;
			}
		}
	}
	printf("PACKET ERROR in receiving recvpkt for connection %d \n", conn);
	return -1;}

int main2(){
	return 0 ;
}