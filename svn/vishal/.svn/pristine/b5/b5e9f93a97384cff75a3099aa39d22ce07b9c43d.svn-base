
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../common/constants.h"
#include "../topology/topology.h"
#include "dvtable.h"

//This function creates a dvtable(distance vector table) dynamically.
//A distance vector table contains the n+1 entries, where n is the number of the neighbors of this node, and the rest one is for this node itself. 
//Each entry in distance vector table is a dv_t structure which contains a source node ID and an array of N dv_entry_t structures where N is the number of all the nodes in the overlay.
//Each dv_entry_t contains a destination node address the the cost from the source node to this destination node.
//The dvtable is initialized in this function.
//The link costs from this node to its neighbors are initialized using direct link cost retrived from topology.dat. 
//Other link costs are initialized to INFINITE_COST.
//The dynamically created dvtable is returned.
/**
//dv_entry_t structure definition
typedef struct distancevectorentry {
	int nodeID;		//destnation nodeID	
	unsigned int cost;	//cost to the destination
} dv_entry_t;


//A distance vector table contains the n+1 dv_t entries, where n is the number of the neighbors of this node, and the rest one is for this node itself. 
typedef struct distancevector {
	int nodeID;		//source nodeID
	dv_entry_t* dvEntry;	//an array of N dv_entry_ts, each of which contains the destination node ID and the cost to the destination from the source node. N is the total number of nodes in the overlay.
} dv_t;

*/
dv_t* dvtable_create()
{
  //printf("dvtable_create called\n");
  dv_t *dv_table = NULL;
  int total_neighbours = topology_getNbrNum();
  //printf("1\n");
  int total_overlay_nodes = topology_getNodeNum();
  //printf("dvtable 2\n");
  int *allNodes = topology_getNodeArray();
  //printf("all nodes done\n");
  int *allNeighbours = topology_getNbrArray();
  int malloc_size = sizeof(dv_t) * (total_neighbours + 1);
  dv_table = (dv_t *)malloc(malloc_size);
  //printf("dvtable\n");
  if(dv_table && allNeighbours && allNodes){
  	/* last one is the current node */
  	dv_table[total_neighbours].nodeID = topology_getMyNodeID();
  	dv_table[total_neighbours].dvEntry = dvtable_create_dventries(dv_table[total_neighbours].nodeID,allNodes,total_overlay_nodes);
  	for(int index = 0 ; index < total_neighbours ; index++){
  		dv_table[index].nodeID = allNeighbours[index];
  		dv_table[index].dvEntry = dvtable_create_dventries(dv_table[index].nodeID,allNodes,total_overlay_nodes);
  	}
  	free(allNodes);
  	free(allNeighbours);
  } 
  return dv_table;
}
dv_entry_t* dvtable_create_dventries(int nodeID, int* allNodes, int total_overlay_nodes){
	int malloc_size_nodes = total_overlay_nodes * sizeof(dv_entry_t) ;
	dv_entry_t *dv_entries = (dv_entry_t *) malloc(malloc_size_nodes);
	if(dv_entries){
  		for(int index = 0 ; index < total_overlay_nodes ; index++){
  			dv_entries[index].nodeID = allNodes[index];
  			if(nodeID == dv_entries[index].nodeID){
  				dv_entries[index].cost = 0;
  			}else{
  				dv_entries[index].cost = topology_getCost(nodeID,dv_entries[index].nodeID); // infinite or actual cost
  			}
  		}
  	}
  	return dv_entries;	
}

//This function destroys a dvtable. 
//It frees all the dynamically allocated memory for the dvtable.
void dvtable_destroy(dv_t* dvtable)
{
  free(dvtable);
}

//This function sets the link cost between two nodes in dvtable.
//If those two nodes are found in the table and the link cost is set, return 1.
//Otherwise, return -1.
int dvtable_setcost(dv_t* dvtable,int fromNodeID,int toNodeID, unsigned int cost)
{
  int result = -1;
  if(dvtable && fromNodeID >= 0 && toNodeID >= 0){ // error checking
	// first search for all the nodes plus my node in the db table
	for(int index = 0 ; index <= topology_getNbrNum() ; index++){
  		if(dvtable[index].nodeID == fromNodeID){
  			// from node id is found, now search from its dventries
  			for(int nodeIndex = 0 ; nodeIndex < topology_getNodeNum() ; nodeIndex++){
  				if(dvtable[index].dvEntry[nodeIndex].nodeID == toNodeID){
  					// found
  					dvtable[index].dvEntry[nodeIndex].cost = cost;
  					result = 1;
  					break;
  				}
  			}
  		}
	}
  }
  return result;
}

//This function returns the link cost between two nodes in dvtable
//If those two nodes are found in dvtable, return the link cost. 
//otherwise, return INFINITE_COST.
unsigned int dvtable_getcost(dv_t* dvtable, int fromNodeID, int toNodeID)
{
  int cost = INFINITE_COST;
  if(dvtable && fromNodeID >= 0 && toNodeID >= 0){ // error checking
	// first search for all the nodes plus my node in the db table
	for(int index = 0 ; index <= topology_getNbrNum() ; index++){
  		if(dvtable[index].nodeID == fromNodeID){
  			// from node id is found, now search from its dventries
  			for(int nodeIndex = 0 ; nodeIndex < topology_getNodeNum() ; nodeIndex++){
  				if(dvtable[index].dvEntry[nodeIndex].nodeID == toNodeID){
  					// found 
  				  	cost = dvtable[index].dvEntry[nodeIndex].cost;
  					break;
  				}
  			}
  		}
	}
  }
  return cost;
}

//This function prints out the contents of a dvtable.
void dvtable_print(dv_t* dvtable)
{
 	printf("Printing DV Table\n");
 	if(dvtable){ // error checking
		// first search for all the nodes plus my node in the db table
		int *allNodes = topology_getNodeArray();
		printf("%10s","NodeID");
		for(int columns = 0 ; columns < topology_getNodeNum() ; columns++){
			printf("%10d", allNodes[columns]);
		}
		printf("\n");
		for(int index = 0 ; index <= topology_getNbrNum() ; index++){
  			printf("%10d",dvtable[index].nodeID);
  			for(int nodeIndex = 0 ; nodeIndex < topology_getNodeNum() ; nodeIndex++){
  				  	printf("%10d",dvtable[index].dvEntry[nodeIndex].cost); 					
  			}
  			printf("\n");
		}
	}
}
