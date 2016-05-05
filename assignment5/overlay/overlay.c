//FILE: overlay/overlay.c
//
//Description: this file implements a ON process 
//A ON process first connects to all the neighbors and then starts listen_to_neighbor threads each of which keeps receiving the incoming packets from a neighbor and forwarding the received packets to the SNP process. Then ON process waits for the connection from SNP process. After a SNP process is connected, the ON process keeps receiving sendpkt_arg_t structures from the SNP process and sending the received packets out to the overlay network. 
//
//Date: April 28,2008


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/utsname.h>
#include <assert.h>

#include "../common/constants.h"
#include "../common/pkt.h"
#include "overlay.h"
#include "../topology/topology.h"
#include "neighbortable.h"

//you should start the ON processes on all the overlay hosts within this period of time
#define OVERLAY_START_DELAY 60

/**************************************************************/
//declare global variables
/**************************************************************/

//declare the neighbor table as global variable 
nbr_entry_t* nt; 
//declare the TCP connection to SNP process as global variable
int network_conn; 

int topology_my_node_id ;
int topology_total_neighbours ;


/**************************************************************/
//implementation overlay functions
/**************************************************************/


int getListeningSocketFD(int port){
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
	// BIND socket
	if(bind(conn_listen_fd, (struct sockaddr *)&node_addr, sizeof(node_addr)) < 0) {
        printf("Error in binding to socket %d \n",conn_listen_fd);
        return -1;
    }
    
    if (listen(conn_listen_fd, 1) < 0) { // max process to connect is 1
        printf("Error in binding to socket %d \n",conn_listen_fd);
        return -1;
    }
    fflush(stdout);
    return conn_listen_fd;
}

// This thread opens a TCP port on CONNECTION_PORT and waits for the incoming connection from all the neighbors that have a larger node ID than my nodeID,
// After all the incoming connections are established, this thread terminates 
void* waitNbrs(void* arg) {
	printf("waitNbrs started \n");
	if(nt){
		int conn_listen_fd = getListeningSocketFD(CONNECTION_PORT);
		if(conn_listen_fd < 0 ){
			printf("unable to create listening socket \n Exiting program !! \n");
			exit(0) ;
		}
		struct sockaddr_in node_client_addr; // client node coonecting to this node
		socklen_t client_addr_length ;
		for(int index = 0 ; index < topology_total_neighbours ; index++){
			bzero(&node_client_addr,sizeof(node_client_addr));
			// this node will connect with all the nodes with lesser than it self
			if(nt[index].nodeID > topology_my_node_id){
				printf("Found Waiting for connection from nodeID %d \n", nt[index].nodeID);
				int conn = -1;
    			conn = accept(conn_listen_fd,(struct sockaddr*)&node_client_addr,&client_addr_length);
				if(conn < 0){
					printf("waitNbrs error in accepting connectin from nodeID %d \n Exiting program !!\n",nt[index].nodeID);
					exit(0);
				}
				printf("Connection Established conn id = %d\n", conn);
				nt_addconn(nt, topology_getNodeIDfromip(&node_client_addr.sin_addr), conn);
			}
		}
		printf("waitNbrs ended with success.. \n");
		return (void *)NULL;
	}
	printf("waitNbrs ended with error.. \n");
	fflush(stdout);
	return (void *)NULL;
}

int connectToNode(int nt_index){
	int conn = -1;
	conn = socket (AF_INET, SOCK_STREAM, 0);
	if(conn < 0)
	{
		printf("Error creating new scoket in connectToNode\n");
		return -1;
	}
	struct sockaddr_in node_addr;
	bzero(&node_addr,sizeof(node_addr));
	node_addr.sin_addr.s_addr = nt[nt_index].nodeIP;
	node_addr.sin_family = AF_INET;
    node_addr.sin_port = htons(CONNECTION_PORT);
    if (connect(conn, (struct sockaddr *) &node_addr, sizeof(node_addr)) < 0) {
    	printf("Error in connecting to %d in connectToNode \n",topology_getNodeIDfromip((struct in_addr *)&nt[nt_index].nodeIP));
        return -1;
    }
    return conn;
}

// This function connects to all the neighbors that have a smaller node ID than my nodeID
// After all the outgoing connections are established, return 1, otherwise return -1
int connectNbrs() {
	printf("connect neighbours started \n");
	if(nt){
		int result = 1;
		for(int index = 0 ; index < topology_total_neighbours ; index++){
			// this node will connect with all the nodes with lesser than it self
			if(nt[index].nodeID < topology_my_node_id){
				printf("Node id found connecting to :- %d \n", nt[index].nodeID);
				int conn = -1;
				conn = connectToNode(index);
				if(conn < 0){
					printf("Error in creating new connection\n");
					return -1;
				}
				printf("Connection Established conn id = %d\n", conn);
				nt_addconn(nt, nt[index].nodeID, conn);
			}
		}
		printf("connectNbrs ENDEND with SUCCESS \n");
		return result;
	}
	printf("connect neighbours ended with fail \n");
	return -1;
}

//Each listen_to_neighbor thread keeps receiving packets from a neighbor. It handles the received packets by forwarding the packets to the SNP process.
//all listen_to_neighbor threads are started after all the TCP connections to the neighbors are established 
void* listen_to_neighbor(void* arg) {
	//put your code here
	printf("listen_to_neighbor started \n");
	int neighbourIndex = *((int *) arg);
	if(neighbourIndex >= 0){
		snp_pkt_t *received_packet = (snp_pkt_t *)malloc(sizeof(snp_pkt_t));
		while(recvpkt(received_packet, nt[neighbourIndex].conn) > 0){
			printf("listen neighbour packet received \n");
			forwardpktToSNP(received_packet, network_conn);	
		}
		printf(" freeing and closing connection for index %d \n", neighbourIndex );
		close(nt[neighbourIndex].conn);
    	nt[neighbourIndex].conn = -1;
    	free(received_packet);
	}else{
		printf("listen_to_neighbor index error\n");
	}
	printf("listen_to_neighbor ended \n");
	fflush(stdout);
	return NULL;
}
//this function keeps getting sendpkt_arg_ts from SNP process, and sends the packets to the next hop in the overlay network. If the next hop's nodeID is BROADCAST_NODEID, 
//the packet should be sent to all the neighboring nodes.
void keepReceivingPacketsFromSNP(){
	printf("keepReceivingPacketsFromSNP starts \n");
	snp_pkt_t *received_packet = (snp_pkt_t *)malloc(sizeof(snp_pkt_t));
    int *fromNode = (int *)malloc(sizeof(int));
    int totalNeighbours = topology_total_neighbours;
    while (getpktToSend(received_packet, fromNode, network_conn) > 0) {
    	printf("packet received from SNP node id = %d\n", *fromNode);
    	fflush(stdout);
    	for(int index = 0 ; index < totalNeighbours ; index++){
    		if(*fromNode == BROADCAST_NODEID){
    			printf("BROADCAST packet received forwarding to %d \n", nt[index].nodeID );
    			sendpkt(received_packet, nt[index].conn);
    		}else if(*fromNode == nt[index].nodeID){
    			printf("NEXT HOP packet received sending to %d \n", nt[index].nodeID);
    			sendpkt(received_packet, nt[index].conn);
    			break; // packet sent for the node id hence break;
    		}
    	}
    }
	printf("SNP packet receiving ended \n");
    free(received_packet);
    free(fromNode);
    close(network_conn);
    printf("keepReceivingPacketsFromSNP ends \n");
    fflush(stdout);
}

//This function opens a TCP port on OVERLAY_PORT, and waits for the incoming connection from local SNP process. After the local SNP process is connected, 
//this function keeps getting sendpkt_arg_ts from SNP process, and sends the packets to the next hop in the overlay network. If the next hop's nodeID is BROADCAST_NODEID, 
//the packet should be sent to all the neighboring nodes.
void waitNetwork() {
	//put your code here
	printf("wait network started \n");
	int snp_server_fd = getListeningSocketFD(OVERLAY_PORT);
	if(snp_server_fd < 0)
	{
		printf("ERROR in waitNetwork unable to get socket \n Exiting program !! \n");
		exit(0);
	}
	struct sockaddr_in node_client_addr; 
	// client node coonecting to this node
	socklen_t client_addr_length ;
	bzero(&node_client_addr,sizeof(node_client_addr));
	printf("Waiting for SNP process to connect\n");

    network_conn = accept(snp_server_fd,(struct sockaddr*)&node_client_addr,&client_addr_length);	
	if(network_conn < 0){
		printf("error in accepting SNP connection return %d\n Exiting program !! \n",network_conn );
		exit(0);
	}
	keepReceivingPacketsFromSNP();
	printf("SNP process connected with conn fd %d \n", network_conn );

	printf("wait network ended \n");

}

//this function stops the overlay
//it closes all the connections and frees all the dynamically allocated memory
//it is called when receiving a signal SIGINT
void overlay_stop() {
	//put your code here
	printf("overlay_stop called\n");
	close(network_conn);
	nt_destroy(nt);
	exit(1);
	printf("overlay_stop ends\n");
}

int main() {
	//start overlay initialization
	topology_my_node_id = topology_getMyNodeID();
	printf("Overlay: Node %d initializing...\n",topology_my_node_id);	
	topology_total_neighbours = topology_getNbrNum();
	printf("Total Neighbours = %d \n", topology_total_neighbours);
	//create a neighbor table
	nt = nt_create();
	//initialize network_conn to -1, means no SNP process is connected yet
	network_conn = -1; // this will be set in waitForNbrs
	
	//register a signal handler which is sued to terminate the process
	signal(SIGINT, overlay_stop);

	//print out all the neighbors
	int nbrNum = topology_total_neighbours;
	int i;
	for(i=0;i<nbrNum;i++) {
		printf("Overlay: neighbor %d:%d\n",i+1,nt[i].nodeID);
	}

	//start the waitNbrs thread to wait for incoming connections from neighbors with larger node IDs
	pthread_t waitNbrs_thread;
	pthread_create(&waitNbrs_thread,NULL,waitNbrs,(void*)0);

	//wait for other nodes to start
	sleep(OVERLAY_START_DELAY);
	
	//connect to neighbors with smaller node IDs
	connectNbrs();

	//wait for waitNbrs thread to return
	pthread_join(waitNbrs_thread,NULL);	

	//at this point, all connections to the neighbors are created
	
	//create threads listening to all the neighbors
	for(i=0;i<nbrNum;i++) {
		int* idx = (int*)malloc(sizeof(int));
		*idx = i;
		pthread_t nbr_listen_thread;
		pthread_create(&nbr_listen_thread,NULL,listen_to_neighbor,(void*)idx);
	}
	printf("Overlay: node initialized...\n");
	printf("Overlay: waiting for connection from SNP process...\n");

	//waiting for connection from  SNP process
	waitNetwork();
}
