#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include "seg.h"

int snp_sendseg(int connection, seg_t* segPtr) {
	char bufstart[2];
	char bufend[2];
	bufstart[0] = '!';
	bufstart[1] = '&';
	bufend[0] = '!';
	bufend[1] = '#';
	if (send(connection, bufstart, 2, 0) < 0) {
		return -1;
	}
	if(send(connection,segPtr,sizeof(seg_t),0)<0) {
		return -1;
	}
	if(send(connection,bufend,2,0)<0) {
		return -1;
	}
	return 1;
}
// Send a SRT segment over the overlay network (this is simply a single TCP connection in the
// case of Lab4). TCP sends data as a byte stream. In order to send segments over the overlay TCP connection, 
// delimiters for the start and end of the packet must be added to the transmission. 
// That is, first send the characters ``!&'' to indicate the start of a  segment; then 
// send the segment seg_t; and finally, send end of packet markers ``!#'' to indicate the end of a segment. 
// Return 1 in case of success, and -1 in case of failure. snp_sendseg() uses
// send() to first send two chars, then send() again but for the seg_t, and, then
// send() two chars for the end of packet. 
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

int snp_recvseg(int connection, seg_t* segPtr) {
	char buf[sizeof(seg_t)+2]; 
	char c;
	int idx = 0;
	// state can be 0,1,2,3; 
	// 0 starting point 
	// 1 '!' received
	// 2 '&' received, start receiving segment
	// 3 '!' received,
	// 4 '#' received, finish receiving segment 
	int state = 0; 
	while(recv(connection,&c,1,0)>0) {
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
				state = 0;
				idx = 0;
				if(seglost()>0) {
                                	printf("seg lost!!!\n");
                                	continue;
                        	}
				memcpy(segPtr,buf,sizeof(seg_t));
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
	return -1;
}
// Receive a segment over overlay network (this is a single TCP connection in the case of
// Lab4). We recommend that you recieve one byte at a time usin recv(). Here you are looking for 
// ``!&'' characters then seg_t and then ``!#''. This is a FSM of sorts and you
// should code it that way. Make sure that you cover cases such as ``#&bbb!b!bn#bbb!#''
// The assumption here (fairly limiting but simplistic) is that !& and !# will not 
// be seen in the data in the segment. You should read in one byte as a char at 
// a time and copy the data part into a buffer to be returned to the caller.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

//if lost, return 1; otherwise return 0
int seglost() {
	int random = rand()%100;
	if(random<LOSS_RATE*100)
		return 1;
	else
		return 0;
}

