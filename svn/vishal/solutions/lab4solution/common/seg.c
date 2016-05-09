//FILE: common/seg.c
//
//Description: this file contains segment interfaces implementation 
//
//Date: April 18,2008

#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include "seg.h"

//states used by snp_recvseg()
// START1 starting point 
// START2 '!' received
// RECV '&' received, start receiving segment
// STOP1 '!' received,
#define START1 0
#define START2 1
#define RECV 2
#define STOP1 3

// Send a segment through overlay TCP
// in form of !&segment!#  
// 
// Pseudocode
// 1) send '!&' first
// 2) then send the segment
// 3) finally send '!#'
//
int snp_sendseg(int connection, seg_t* segPtr) {
	segPtr->header.checksum = checksum(segPtr);
	char bufstart[2] = "!&";
	char bufend[2] = "!#";
	if (send(connection, bufstart, 2, 0) < 0) {
		return -1;
	}
	int segsize = sizeof(srt_hdr_t) + segPtr->header.length;
	if(send(connection,segPtr,segsize,0)<0) {
		return -1;
	}
	if(send(connection,bufend,2,0)<0) {
		return -1;
	}
	return 1;
}

// receive a segment from overlay TCP connection
// this function uses a simple FSM
// START1 -- starting point 
// START2 -- '!' received, expecting '&' to receive segment
// RECV -- '&' received, start receiving segment
// STOP1 -- '!' received, expecting '#' to finish receiving segment
// when a segment is received, use seglost to determine if the segment should bediscarded 
//
// Pseudocode
// 1) While recv(connection,&c,1,)
//      Based on value of c jump between states described above
//      When we get a segment use checkchecksum to verify integrity
//
int snp_recvseg(int connection, seg_t* segPtr) {
	char buf[sizeof(seg_t)+2]; 
	char c;
	int idx = 0;

	int state = START1; 
	while(recv(connection,&c,1,0)>0) {
		switch(state) {
			case START1:
 				if(c=='!')
				state = START2;
				break;
			case START2:
				if(c=='&') 
					state = RECV;
				else
					state = START1;
				break;
			case RECV:
				if(c=='!') {
					buf[idx]=c;
					idx++;
					state = STOP1;
				}
				else {
					buf[idx]=c;
					idx++;
				}
				break;
			case STOP1:
				if(c=='#') {
					buf[idx]=c;
					memcpy(segPtr,buf,idx-1);

					state = START1;
					idx = 0;
				

					//add segment error	
					if(seglost(segPtr)>0) {
						continue;	
				         }

					if(checkchecksum(segPtr)<0) {
						printf("checksum error,drop!\n");
						continue;
					}
					return 1;
				}
				else if(c=='!') {
					buf[idx]=c;
					idx++;
				}
				else {
					buf[idx]=c;
					idx++;	
					state = RECV;
				}
				break;
			default:
				break;
	
		}
	}
	return -1;
}

//lost rate is PKT_LOSS_RATE defined in constant.h
//if a segment has is lost, return 1; otherwise return 0 
//PKT_LOSS_RATE/2 probability of segment loss
//PKT_LOSS_RATE/2 probability of invalid checksum
//
// Pseudocode
// 1) Randomly decide whether to mess with this segment
// 2) If so, flip a coin, if heads seglost, if tails
//    flip a bit and return the result
//
int seglost(seg_t* segPtr) {
	int random = rand()%100;
	if(random<PKT_LOSS_RATE*100) {
		//50% probability of losing a segment
		if(rand()%2==0) {
			printf("seg lost!!!\n");
                        return 1;
		}
		//50% chance of invalid checksum
		else {
			//get data length
			int len = sizeof(srt_hdr_t)+segPtr->header.length;
			//get a random bit that will be fliped
			int errorbit = rand()%(len*8);
			//flip the bit
			char* temp = (char*)segPtr;
			temp = temp + errorbit/8;
			*temp = *temp^(1<<(errorbit%8));
			return 0;
		}
	}
	return 0;

}

//check checksum
//Denote the data the checksum is calculated as D
//D = segment header + segment data
//if size of D (in bytes) is odd number
//append an byte with all bits set as 0 to D
//Divide D into 16-bits-long values
//add all these 16-bits-long values using 1s complement
//the sum should be FFFF if it's valid checksum
//so flip all the bits of the sum 
//return 1 if the result is 0 
//return -1 if the result is not 0 
//
// Pseudocode
// 1) sum = 0
// 2) Get number of 16-bit blocks in segment (len)
//      increment by 1 if need to
// 3) temp = segment (temp is a pointer)
// 4) while(len > 0)
//      sum += *temp
//      temp++
//      if(sum & 0x10000)
//        sum = (sum & 0xFFFF)+1 // Check for and take care of overflow
//      len--
// 5) result = ~sum
// 6) if(resul == 0) return 1
//    else return -1
//
int checkchecksum(seg_t *segment){
        long sum = 0;
        //len is the number of 16-bit data to calculate the checksum
        int len = sizeof(srt_hdr_t)+segment->header.length;
        if(len%2==1)
        	len++;
        len = len/2;
        unsigned short* temp = (unsigned short*)segment;

        while(len > 0){
        	sum += *temp;
       		temp++;
        	//if overflow, round the most significant 1
        	if(sum & 0x10000)
        		sum = (sum & 0xFFFF) + 1;
        	len --;
       }
        
       unsigned short result =~sum;
       if(result == 0)
		return 1;
	else
		return -1;
}


//calculate checksum over the segment
//clear the checksum field in segment to be 0
//Denote the data from which the checksum is calculated as D
//D = segment header + segment data
//if size of D (in bytes) is odd number
//append a byte with all bits set as 0 to D
//Divide D into 16-bits-long values
//add all these 16-bits-long values using 1s complement
//flip the all the bits of the sum to get the checksum
unsigned short checksum(seg_t *segment){
	segment->header.checksum = 0;
	if(segment->header.length%2==1)
		segment->data[segment->header.length] = 0;
	
	long sum = 0;  
	//len is the number of 16-bit data to calculate the checksum
	int len = sizeof(srt_hdr_t)+segment->header.length;
	if(len%2==1)
		len++;
	len = len/2;
	unsigned short* temp = (unsigned short*)segment;

        while(len > 0){
        	sum += *temp;
		temp++;
		//if overflow, round the most significant 1
             	if(sum & 0x10000)   
               		sum = (sum & 0xFFFF) + 1;
             	len --;
        }
           	
	return ~sum;
}
