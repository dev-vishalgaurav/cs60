//FILE: network/network.c
//
//Description: this file implements network layer process  
//
//Date: April 29,2008



#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>

#include "../common/constants.h"
#include "../common/pkt.h"
#include "../common/seg.h"
#include "../topology/topology.h"
#include "network.h"
#include "nbrcosttable.h"
#include "dvtable.h"
#include "routingtable.h"

//network layer waits this time for establishing the routing paths 
#define NETWORK_WAITTIME 60

/**************************************************************/
//delare global variables
/**************************************************************/
int overlay_conn; 			//connection to the overlay
int transport_conn;			//connection to the transport
nbr_cost_entry_t* nct;			//neighbor cost table
dv_t* dv;				//distance vector table
pthread_mutex_t* dv_mutex;		//dvtable mutex
routingtable_t* routingtable;		//routing table
pthread_mutex_t* routingtable_mutex;	//routingtable mutex


/**************************************************************/
//implementation network layer functions
/**************************************************************/
int getListeningSocketFD(int port, int maxConn){
	int conn_listen_fd = -1;
	// create socket 
	conn_listen_fd = socket (AF_INET, SOCK_STREAM, 0);
	if(conn_listen_fd < 0)
	{
		printf("Error creating new scoket in acceptConnectionFromNode\n");
		return -1;
	}
	struct sockaddr_in node_addr; // server
	bzero(&node_addr,sizeof(node_addr));
	node_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	node_addr.sin_family = AF_INET;
    node_addr.sin_port = htons(port);
    int yes = 1;
	if ( setsockopt(conn_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 )
	{
    	close(conn_listen_fd);
    	printf("setsockopt error in binding ");
    	return -1;
	}
	// BIND socket
	if(bind(conn_listen_fd, (struct sockaddr *)&node_addr, sizeof(node_addr)) < 0) {
		close(conn_listen_fd);
        printf("Error in binding to socket %d \n",conn_listen_fd);
        return -1;
    }
    
    if (listen(conn_listen_fd, maxConn) < 0) { // max process to connect is 1
    	close(conn_listen_fd);
        printf("Error in binding to socket %d \n",conn_listen_fd);
        return -1;
    }
    fflush(stdout);
    return conn_listen_fd;
}
//This function is used to for the SNP process to connect to the local ON process on port OVERLAY_PORT.
//TCP descriptor is returned if success, otherwise return -1.
int connectToOverlay() { 
	int out_conn;
	struct sockaddr_in servaddr;
	struct hostent *hostInfo;
	char *hostname = "localhost"; // connecting with the localhost as it is a local process 
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
//The route update packet contains this node's distance vector. 
//Broadcasting is done by set the dest_nodeID in packet header as BROADCAST_NODEID
//and use overlay_sendpkt() to send the packet out using BROADCAST_NODEID address.
void* routeupdate_daemon(void* arg) {
	//put your code here
	printf("routeupdate_daemon started \n");
	pkt_routeupdate_t *updated_route_packet = (pkt_routeupdate_t *) malloc(sizeof(pkt_routeupdate_t));
	snp_pkt_t *route_packet = (snp_pkt_t *) malloc(sizeof(snp_pkt_t));
	while(overlay_conn > 0){
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
		fflush(stdout);
	}
	free(updated_route_packet);
	printf("routeupdate_daemon going to end \n");
	pthread_exit(NULL);
	return 0;
}

//This thread handles incoming packets from the ON process.
//It receives packets from the ON process by calling overlay_recvpkt().
//If the packet is a SNP packet and the destination node is this node, forward the packet to the SRT process.
//If the packet is a SNP packet and the destination node is not this node, forward the packet to the next hop according to the routing table.
//If this packet is an Route Update packet, update the distance vector table and the routing table. 
void* pkthandler(void* arg) {
	snp_pkt_t pkt;
	bzero(&pkt,sizeof(pkt));
	while(overlay_recvpkt(&pkt,overlay_conn)>0) {
		printf("Routing: received a packet in node %d from neighbor %d\n", topology_getMyNodeID(), pkt.header.src_nodeID);
	}
	close(overlay_conn);
	printf("Routing: Packet handler thread STOPPED\n");
	overlay_conn = -1;
	pthread_exit(NULL);
}

//This function stops the SNP process. 
//It closes all the connections and frees all the dynamically allocated memory.
//It is called when the SNP process receives a signal SIGINT.
void network_stop() {
	//add your code here
	close(overlay_conn);
	printf(" network_stop called :- SNP process stopped\n Exiting .. !! \n");
	exit(0);
}
//After the local SRT process is connected, this function keeps receiving sendseg_arg_ts which contains the segments and their destination node addresses from the SRT process. The received segments are then encapsulated into packets (one segment in one packet), and sent to the next hop using overlay_sendpkt. The next hop is retrieved from routing table.
//When a local SRT process is disconnected, this function waits for the next SRT process to connect.
void keepReceivingPacketsFromSRT(){
	printf("keepReceivingPacketsFromSRT starts \n");
	snp_pkt_t *received_packet = (snp_pkt_t *)malloc(sizeof(snp_pkt_t));
    int *srcNodeID = (int *)malloc(sizeof(int));
    // recieve the segment 
    seg_t *segPtr = (seg_t *)malloc(sizeof(seg_t));
    printf("SRT LAYER STARTED\n");
    while (getsegToSend(transport_conn, srcNodeID, segPtr) > 0) {
    	printf("packet received from SRT node id = %d\n", *srcNodeID);
    	fflush(stdout);
    }
	printf("SRT packet receiving ended for SRT socket %d \n",transport_conn);
    free(received_packet);
    free(srcNodeID);
    close(transport_conn);
    transport_conn = -1;
    printf("keepReceivingPacketsFromSRT ends \n");
    printf("SRT LAYER ENDED\n");
    fflush(stdout);
}
//This function opens a port on NETWORK_PORT and waits for the TCP connection from local SRT process.
//After the local SRT process is connected, this function keeps receiving sendseg_arg_ts which contains the segments and their destination node addresses from the SRT process. The received segments are then encapsulated into packets (one segment in one packet), and sent to the next hop using overlay_sendpkt. The next hop is retrieved from routing table.
//When a local SRT process is disconnected, this function waits for the next SRT process to connect.
void waitTransport() {
	//put your code here
	// transport_conn = accept;
	int snp_server_fd = -1;
	snp_server_fd = getListeningSocketFD(NETWORK_PORT,1);
	if(snp_server_fd < 0)
	{
		printf("ERROR in waitNetwork unable to get socket.. sleeping ... \n !! Exiting program !! \n");
		exit(0);
	}
	struct sockaddr_in node_client_addr; 
	// client node coonecting to this node
	socklen_t client_addr_length  = sizeof(node_client_addr);
	while(1){
		bzero(&node_client_addr,sizeof(node_client_addr));
		printf("Waiting for SNP process to connect\n");
	    transport_conn = accept(snp_server_fd,(struct sockaddr*)&node_client_addr,&client_addr_length);	
		if(transport_conn < 0){
			printf("error in accepting SNP connection return %d\n Exiting program !! \n",transport_conn );
		}else{
			char ipaddress[100]; 
    		inet_ntop( AF_INET, &(node_client_addr.sin_addr), ipaddress, INET_ADDRSTRLEN );
			printf("SNP process connected with conn fd %d and ip address %s \n", transport_conn, ipaddress );
			keepReceivingPacketsFromSRT();
		}
		sleep(1); // wait for 1 sec to get new SNP connection
	}
	printf("waitTransport ended \n");
	return;
}

int main(int argc, char *argv[]) {
	printf("network layer is starting, pls wait...\n");

	//initialize global variables
	nct = nbrcosttable_create();
	dv = dvtable_create();
	dv_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(dv_mutex,NULL);
	routingtable = routingtable_create();
	routingtable_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(routingtable_mutex,NULL);
	overlay_conn = -1;
	transport_conn = -1;

	nbrcosttable_print(nct);
	dvtable_print(dv);
	routingtable_print(routingtable);

	//register a signal handler which is used to terminate the process
	signal(SIGINT, network_stop);

	//connect to local ON process 
	overlay_conn = connectToOverlay();
	if(overlay_conn<0) {
		printf("can't connect to overlay process\n");
		exit(1);		
	}
	
	//start a thread that handles incoming packets from ON process 
	pthread_t pkt_handler_thread; 
	pthread_create(&pkt_handler_thread,NULL,pkthandler,(void*)0);

	//start a route update thread 
	pthread_t routeupdate_thread;
	pthread_create(&routeupdate_thread,NULL,routeupdate_daemon,(void*)0);	

	printf("network layer is started...\n");
	printf("waiting for routes to be established\n");
	sleep(NETWORK_WAITTIME);
	routingtable_print(routingtable);

	//wait connection from SRT process
	printf("waiting for connection from SRT process\n");
	waitTransport(); 

}


