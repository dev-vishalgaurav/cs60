//FILE: topology/topology.c
//
//Description: this file implements some helper functions used to parse 
//the topology file 
//
//Date: May 3,2010

#define _DEFAULT_SOURCE

#include "topology.h"
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
#include "../common/constants.h"


//this function returns node ID of the given hostname
//the node ID is an integer of the last 8 digit of the node's IP address
//for example, a node with IP address 202.120.92.3 will have node ID 3
//if the node ID can't be retrieved, return -1
int topology_getNodeIDfromname(char* hostname) 
{
  struct hostent *hostInfo;
  hostInfo = gethostbyname(hostname);
  if(!hostInfo) {
  	printf("error in getting host name from string!\n");
  	return -1;
  }
  struct sockaddr_in servaddr;
  memcpy((char *) &servaddr.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
  int ipAddr = ntohl(servaddr.sin_addr.s_addr); // network order to host byte order
  //printf("topology_getNodeIDfromname ends\n");
  return ipAddr & 0x000000FF; // to get the last digit od ip address mask it with 0.0.0.255 :D 
}

void get_ip_from_host_name(char* hostname,in_addr_t *ip){
	//printf("get_ip_from_host_name starts\n");
	struct hostent *hostInfo;
	hostInfo = gethostbyname(hostname);
	if(!hostInfo) {
  		printf("error in getting host name from string!\n");
  		return;
  	}
  	printf("%s\n", (char *) hostInfo->h_addr_list[0]  );
  	memcpy((char *) ip, hostInfo->h_addr_list[0], hostInfo->h_length);
  	//printf("get_ip_from_host_name ends value = %s\n",(char *) ip);
  	return ;
}
int topology_getNodeIDfromip1(struct sockaddr_in* client){
	int nodeID;
    char ipaddress[100]; 
    inet_ntop( AF_INET, &(client->sin_addr), ipaddress, INET_ADDRSTRLEN );
    //ipaddress = inet_ntoa(client->sin_addr);
    printf("topology_getNodeIDfromip1 ip addr = %s\n", ipaddress);
    char *split = strtok(ipaddress, ".");
    printf("topology_getNodeIDfromip1 1 = %s\n", split);
    split = strtok(NULL,".");
    printf("topology_getNodeIDfromip1 2 = %s\n", split);
    split = strtok(NULL,".");
    printf("topology_getNodeIDfromip1 3 = %s\n", split);
    split = strtok(NULL,".");
    printf("topology_getNodeIDfromip1 4 = %s\n", split);
    nodeID = atoi(split);
    return nodeID;
}

//this function returns node ID from the given IP address
//if the node ID can't be retrieved, return -1
int topology_getNodeIDfromip(struct in_addr* addr)
{
	int ipAddr = ntohl(addr->s_addr);
  	return ipAddr & 0x000000FF;
}

//this function returns my node ID
//if my node ID can't be retrieved, return -1
int topology_getMyNodeID()
{
   char hostname[50];
   gethostname(hostname,sizeof(hostname)); // get the ip address of local machine
   //printf("hostname = %s \n", hostname );
  //printf("%s\n",hostname );
   return topology_getNodeIDfromname(hostname);
}

//this functions parses the topology information stored in topology.dat
//returns the number of neighbors
int topology_getNbrNum()
{
  int myNodeId = topology_getMyNodeID();
  int totalNeighbours = 0 ;
  if(myNodeId >= 0 ){
  		FILE *topologyFile = fopen(TOPOLOGY_FILE_NAME, "r");
		if(topologyFile != NULL){
			char line[120];
			while(fgets(line, sizeof(line), (FILE*)topologyFile) != NULL){
				char *firstHost = strtok(line, " ");
				char *secondHost = strtok(NULL, " ");
				//printf("firstHost = %s, secondHost = %s \n", firstHost, secondHost );
				int firstNodeId = topology_getNodeIDfromname(firstHost);
				int secondNodeId = topology_getNodeIDfromname(secondHost);
				if(firstNodeId == myNodeId || secondNodeId == myNodeId){
					totalNeighbours++;
				}
			}

		}
		fclose(topologyFile);
  }
  return totalNeighbours;
}

int is_node_exists(int nodeId , int *nodes, int size){
	for(int i = 0 ; i < size ; i++){
		if(nodes[i] == nodeId){
			return 1;
		}
	}
	return 0;
}

//this functions parses the topology information stored in topology.dat
//returns the number of total nodes in the overlay 
int topology_getNodeNum()
{ 
	int *nodesFound ;
	int size = 0;
	FILE *topologyFile = fopen(TOPOLOGY_FILE_NAME, "r");
	if(topologyFile != NULL){
		char line[120];
		while(fgets(line, sizeof(line), (FILE*)topologyFile) != NULL){
			char *firstHost = strtok(line, " ");
			char *secondHost = strtok(NULL, " ");
			int firstNodeId = topology_getNodeIDfromname(firstHost);
			int secondNodeId = topology_getNodeIDfromname(secondHost);
			//printf("firstNodeId = %d, secondNodeId = %d \n", firstNodeId, secondNodeId );
			if(size == 0){
				nodesFound = malloc(sizeof(int));
				nodesFound[size++] = firstNodeId;
			}else if(is_node_exists(firstNodeId,nodesFound,size) == 0){ // new node is found
				//printf("first node not found %d \n", firstNodeId);
				nodesFound = realloc(nodesFound,(size + 1) * sizeof(int));
				nodesFound[size] = firstNodeId;
				size++;
			}
			if(is_node_exists(secondNodeId,nodesFound,size) == 0){ // new node is found
				//printf(" second node not found %d \n", secondNodeId);
				nodesFound = realloc(nodesFound,(size + 1) * sizeof(int));
				nodesFound[size] = secondNodeId;
				size++;
			}

		}
		free(nodesFound);
		fclose(topologyFile);
  }
  return size;
}

//this functions parses the topology information stored in topology.dat
//returns a dynamically allocated array which contains all the nodes' IDs in the overlay network  
int* topology_getNodeArray()
{
	int *nodesFound ;
	int size = 0;
	FILE *topologyFile = fopen(TOPOLOGY_FILE_NAME, "r");
	if(topologyFile != NULL){
		char line[120];
		while(fgets(line, sizeof(line), (FILE*)topologyFile) != NULL){
			char *firstHost = strtok(line, " ");
			char *secondHost = strtok(NULL, " ");
			int firstNodeId = topology_getNodeIDfromname(firstHost);
			int secondNodeId = topology_getNodeIDfromname(secondHost);
			//printf("firstNodeId = %d, secondNodeId = %d \n", firstNodeId, secondNodeId );
			if(size == 0){
				nodesFound = (int *)malloc(sizeof(int));
				nodesFound[size++] = firstNodeId;
			}else if(is_node_exists(firstNodeId,nodesFound,size) == 0){ // new node is found
				//printf("first node not found %d \n", firstNodeId);
				nodesFound = realloc(nodesFound,(size + 1) * sizeof(int));
				nodesFound[size] = firstNodeId;
				size++;
			}
			if(is_node_exists(secondNodeId,nodesFound,size) == 0){ // new node is found
				//printf(" second node not found %d \n", secondNodeId);
				nodesFound = realloc(nodesFound,(size + 1) * sizeof(int));
				nodesFound[size] = secondNodeId;
				size++;
			}

		}
		fclose(topologyFile);
  }
  return nodesFound;
}

//this functions parses the topology information stored in topology.dat
//returns a dynamically allocated array which contains all the neighbors'IDs  
int* topology_getNbrArray()
{
	int *nodesFound ;
	int size = 0;
	int myNodeId = topology_getMyNodeID();
	FILE *topologyFile = fopen(TOPOLOGY_FILE_NAME, "r");
	//printf("topology_getNbrArray strted\n");
	if(topologyFile != NULL){
		//printf("topology file valid\n");
		char line[120];
		while(fgets(line, sizeof(line), (FILE*)topologyFile) != NULL){
			char *firstHost = strtok(line, " ");
			char *secondHost = strtok(NULL, " ");
			//printf("firstNode= %s, secondNode = %s \n", firstHost, secondHost);
			int firstNodeId = topology_getNodeIDfromname(firstHost);
			int secondNodeId = topology_getNodeIDfromname(secondHost);
			if(myNodeId == firstNodeId || myNodeId == secondNodeId ){
				//printf("firstNodeId = %d, secondNodeId = %d \n", firstNodeId, secondNodeId );
				int neighbourId = (firstNodeId == myNodeId) ? secondNodeId : firstNodeId;
				//printf("Neighbour found  neighbourid = %d \n", neighbourId);
				if(size == 0){
					//printf("size 0\n");
					nodesFound = (int *)malloc(sizeof(int));
					//printf("allocated\n");
					nodesFound[size++] = neighbourId;
				}else if(is_node_exists(neighbourId,nodesFound,size) == 0){ // new node is found
					nodesFound = realloc(nodesFound,(size + 1) * sizeof(int));
					nodesFound[size] = neighbourId;
					size++;
				}
		}

		}
		fclose(topologyFile);
  }
  return nodesFound;
}

//this functions parses the topology information stored in topology.dat
//returns the cost of the direct link between the two given nodes 
//if no direct link between the two given nodes, INFINITE_COST is returned
unsigned int topology_getCost(int fromNodeID, int toNodeID)
{
  int myNodeId = topology_getMyNodeID();
  int cost = (fromNodeID == toNodeID) ? 0 : INFINITE_COST ;
  if(myNodeId >= 0 ){
  		FILE *topologyFile = fopen(TOPOLOGY_FILE_NAME, "r");
		if(topologyFile != NULL){
			char line[120];
			while(fgets(line, sizeof(line), (FILE*)topologyFile) != NULL){
				char *firstHost = strtok(line, " ");
				char *secondHost = strtok(NULL, " ");
				//printf("first = %s, second = %s  \n", firstHost, secondHost );
				int firstNodeId = topology_getNodeIDfromname(firstHost);
				int secondNodeId = topology_getNodeIDfromname(secondHost);
				if((firstNodeId == fromNodeID && secondNodeId == toNodeID) || (secondNodeId == fromNodeID && firstNodeId == toNodeID)){
					char *costInString  = strtok(NULL, " ");
					cost = atoi(costInString);
				}
			}
		}
		fclose(topologyFile);
  }
  return cost;
}

int main1(){
	printf("my node id = %d\n", topology_getMyNodeID() );
	printf("my nighbours = %d\n", topology_getNbrNum() );
	int totalNodes = topology_getNodeNum();
	printf("total unique nodes = %d\n", totalNodes );
	int *nodes = topology_getNodeArray();
	for(int i = 0 ; i < totalNodes ; i++){
		printf("node = %d \n", nodes[i] );
	}
	int totalNeighbours = topology_getNbrNum();
	printf("total unique nighbours = %d\n", totalNeighbours );
	int *neighBourNodes = topology_getNbrArray();
	for(int i = 0 ; i < totalNeighbours ; i++){
		printf("node = %d \n", neighBourNodes[i] );
	}
	printf(" distance between 1 and 213  = %d\n",topology_getCost(1,213));
	printf(" distance between 1 and 97  = %d\n",topology_getCost(1,212));	
	printf(" distance between 1 and 212  = %d\n",topology_getCost(1,97));
	return 0;
}
