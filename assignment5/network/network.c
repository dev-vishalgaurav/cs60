//FILE: network/network.c
//
//Description: this file implements SNP process  
//
//Date: April 29,2008



#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>
#include <strings.h>

#include "../common/constants.h"
#include "../common/pkt.h"
#include "../topology/topology.h"
#include "network.h"

/**************************************************************/
//declare global variables
/**************************************************************/
int overlay_conn; 		//connection to the overlay


/**************************************************************/
//implementation network layer functions
/**************************************************************/


//this function is used to for the SNP process to connect to the local ON process on port OVERLAY_PORT
//connection descriptor is returned if success, otherwise return -1
int connectToOverlay() { 
	int out_conn;
	struct sockaddr_in servaddr;
	struct hostent *hostInfo;
	char *hostname = "localhost";
	hostInfo = gethostbyname(hostname);
	if(!hostInfo) {
		printf("connectToOverlay ERROR - host name error!\n");
		return -1;
	}
	servaddr.sin_family =hostInfo->h_addrtype;	
	memcpy((char *) &servaddr.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
	servaddr.sin_port = htons(OVERLAY_PORT);
	out_conn = socket(AF_INET,SOCK_STREAM,0);  
	if(out_conn<0) {
		printf("connectToOverlay ERROR - unable to create socket to %s!\n",hostname);
		return -1;
	}
	if(connect(out_conn, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
	{
		printf("connectToOverlay ERROR - connect failed to %s!\n", hostname);
		return -1;
	} 
	return out_conn; 
}

//This thread sends out route update packets every ROUTEUPDATE_INTERVAL time
//In this lab this thread only broadcasts empty route update packets to all the neighbors, broadcasting is done by set the dest_nodeID in packet header as BROADCAST_NODEID
void* routeupdate_daemon(void* arg) {
	//put your code here
	printf("routeupdate_daemon started \n");
	pkt_routeupdate_t *updated_route_packet = (pkt_routeupdate_t *) malloc(sizeof(pkt_routeupdate_t));
	snp_pkt_t *route_packet = (snp_pkt_t *) malloc(sizeof(snp_pkt_t));
	while(1){
		sleep(ROUTEUPDATE_INTERVAL);
		printf("preparing route updated\n");
		// reset data
		bzero(route_packet,sizeof(snp_pkt_t));
		route_packet->header.dest_nodeID =  BROADCAST_NODEID;
		route_packet->header.src_nodeID = topology_getMyNodeID();
		route_packet->header.length = 0 ;
		route_packet->header.type = ROUTE_UPDATE;
		overlay_sendpkt(BROADCAST_NODEID,route_packet,overlay_conn);
		printf("route update sent \n");
	}
	free(updated_route_packet);
	printf("routeupdate_daemon going to end \n");
	pthread_exit(NULL);
	return 0;
}

//this thread handles incoming packets from the ON process
//It receives packets from the ON process by calling overlay_recvpkt()
//In this lab, after receiving a packet, this thread just outputs the packet received information without handling the packet 
void* pkthandler(void* arg) {
	snp_pkt_t pkt;
	bzero(&pkt,sizeof(pkt));
	while(overlay_recvpkt(&pkt,overlay_conn)>0) {
		printf("Routing: received a packet from neighbor %d\n",pkt.header.src_nodeID);
	}
	close(overlay_conn);
	printf("Routing: Packet handler thread STOPPED\n");
	overlay_conn = -1;
	pthread_exit(NULL);
}

//this function stops the SNP process 
//it closes all the connections and frees all the dynamically allocated memory
//it is called when the SNP process receives a signal SIGINT
void network_stop() {
	//add your code here
	close(overlay_conn);
	printf(" network_stop called :- SNP process stopped\n");
}


int main(int argc, char *argv[]) {
	printf("network layer is starting, pls wait...\n");

	//initialize global variables
	overlay_conn = -1;

	//register a signal handler which will kill the process
	signal(SIGINT, network_stop);

	//connect to overlay
	overlay_conn = connectToOverlay();
	if(overlay_conn<0) {
		printf("can't connect to ON process\n");
		exit(1);		
	}
	
	//start a thread that handles incoming packets from overlay
	pthread_t pkt_handler_thread; 
	pthread_create(&pkt_handler_thread,NULL,pkthandler,(void*)0);

	//start a route update thread 
	pthread_t routeupdate_thread;
	pthread_create(&routeupdate_thread,NULL,routeupdate_daemon,(void*)0);	

	printf("network layer is started...\n");

	//sleep forever
	while(1) {
		sleep(60);
	}
}


