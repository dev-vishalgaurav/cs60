//FILE: common/constants.h

//Description: this file contains constants used by srt protocol

//Date: April 18,200

#ifndef CONSTANTS_H
#define CONSTANTS_H

//overlay port opened by the server. the client will connect to this port. You should choose a random port to avoid conflicts with your classmates. Because you may log onto the same computer.
#define OVERLAY_PORT 9009
//this is the MAX connections can be supported by SRT. You TCB table should contain MAX_TRANSPORT_CONNECTIONS entries
#define MAX_TRANSPORT_CONNECTIONS 10
//Maximum segment length
//MAX_SEG_LEN = 1500 - sizeof(seg header) - sizeof(ip header)
#define MAX_SEG_LEN  1464
//The packet loss rate is 10%
#define PKT_LOSS_RATE 0.1
//SYN_TIMEOUT value in nano seconds
#define SYN_TIMEOUT 100000000
#define SYN_TIMEOUT_MS SYN_TIMEOUT/1000000
//SYN_TIMEOUT value in nano seconds
#define FIN_TIMEOUT 100000000
//SYN_TIMEOUT value in mili seconds
#define FIN_TIMEOUT_MS FIN_TIMEOUT/1000000
#define LOOP_WAITING_TIME 1000
//max number of SYN retransmissions in srt_client_connect()
#define SYN_MAX_RETRY 5
//max number of FIN retransmission in srt_client_disconnect()
#define FIN_MAX_RETRY 5
//server close wait timeout value in seconds
#define CLOSEWAIT_TIME 1
#endif
