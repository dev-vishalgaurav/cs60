//
// FILE: seg.h

// Description: This file contains segment definitions and interfaces to send and receive segments. 
// The prototypes support snp_sendseg() and snp_rcvseg() for sending to the network layer.
//
// Date: April 18, 2008
//       April 21, 2008 **Added more detailed description of prototypes fixed ambiguities** ATC

//

#ifndef SEG_H
#define SEG_H

#include "constants.h"

//Segment type definition. Used by SRT.
#define	SYN 0
#define	SYNACK 1
#define	FIN 2
#define	FINACK 3
#define	DATA 4
#define	DATAACK 5

//segment header definition. 

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

//
//
//  SNP API for the client and server sides 
//  =======================================
//
//  In what follows, we provide the prototype definition for each call and limited pseudo code representation
//  of the function. This is not meant to be comprehensive - more a guideline. 
// 
//  You are free to design the code as you wish.
//
//  NOTE: snp_sendseg() and snp_recvseg() are services provided by the networking layer
//  i.e., simple network protocol to the transport layer. 
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int sendseg(int connection, seg_t* segPtr);

// Send a SRT segment over the overlay network (this is simply a single TCP connection in the
// case of Lab3). TCP sends data as a byte stream. In order to send segments over the overlay TCP connection, 
// delimiters for the start and end of the packet must be added to the transmission. 
// That is, first send the characters ``!&'' to indicate the start of a  segment; then 
// send the segment seg_t; and finally, send end of packet markers ``!#'' to indicate the end of a segment. 
// Return 1 in case of success, and -1 in case of failure. snp_sendseg() uses
// send() to first send two chars, then send() again but for the seg_t, and, then
// send() two chars for the end of packet. 
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int recvseg(int connection, seg_t* segPtr);

// Receive a segment over overlay network (this is a single TCP connection in the case of
// Lab3). We recommend that you recieve one byte at a time usin recv(). Here you are looking for 
// ``!&'' characters then seg_t and then ``!#''. This is a FSM of sorts and you
// should code it that way. Make sure that you cover cases such as ``#&bbb!b!bn#bbb!#''
// The assumption here (fairly limiting but simplistic) is that !& and !# will not 
// be seen in the data in the segment. You should read in one byte as a char at 
// a time and copy the data part into a buffer to be returned to the caller.
//
// IMPORTANT: once you have parsed a segment you should call seglost(). Here is the code
// for seglost():
// 
// if segment is lost (we emulate that the packet is lost on the network) the return 1; 
// otherwise we assume that the packet is received OK and return 0
//
// int seglost() {
//	int random = rand()%100;
//	if(random<LOSS_RATE*100)
//		return 1;
//	else
//		return 0;
//}
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int seglost();
#endif
