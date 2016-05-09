//FILE: common/constants.h

//Description: this file contains constants used by srt protocol

//Date: April 29,2008

#ifndef CONSTANTS_H
#define CONSTANTS_H

// this is the MAX connections can be supported by SRT. You TCB table 
// should contain MAX_TRANSPORT_CONNECTIONS entries
#define MAX_TRANSPORT_CONNECTIONS 10
#define PORT 39008
//Maximum segment length
//MAX_SEG_LEN = 1500 - sizeof(seg header) - sizeof(ip header)
//#define MAX_SEG_LEN  1464
#define MAX_SEG_LEN  1500
//size of receive buffer
#define RECEIVE_BUF_SIZE 1000000
//The packet loss rate
#define LOSS_RATE 0.05
//SYN_TIMEOUT value in nano seconds
#define SYNSEG_TIMEOUT_NS 500000000
//SYN_TIMEOUT value in nano seconds
#define FINSEG_TIMEOUT_NS 500000000
//max number of SYN retransmissions in srt_client_connect()
#define SYN_MAX_RETRY 5
//max number of FIN retransmission in srt_client_disconnect()
#define FIN_MAX_RETRY 5
//server close wait timeout value in seconds
#define CLOSEWAIT_TIME 1
#endif