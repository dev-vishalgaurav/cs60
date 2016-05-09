
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "nbrcosttable.h"
#include "../common/constants.h"
#include "../topology/topology.h"

//This function creates a neighbor cost table dynamically 
//and initialize the table with all its neighbors' node IDs and direct link costs.
//The neighbors' node IDs and direct link costs are retrieved from topology.dat file. 
nbr_cost_entry_t* nbrcosttable_create()
{
  	int layerNodeId = topology_getMyNodeID();
  	int totalNeighbors = topology_getNbrNum();
    int *allNeighboursNodes = topology_getNbrArray();
    int mallocSize = totalNeighbors * sizeof(nbr_cost_entry_t);
    nbr_cost_entry_t *costTable = (nbr_cost_entry_t *)malloc(mallocSize);
    
    //set the values in neighbor cost table
    for (int neighourIndex = 0; neighourIndex < totalNeighbors; neighourIndex++) {
        costTable[neighourIndex].nodeID = allNeighboursNodes[neighourIndex];
        costTable[neighourIndex].cost = topology_getCost(layerNodeId, allNeighboursNodes[neighourIndex]); // cost between this node and neighbour
    }
    
    free(allNeighboursNodes);
    return costTable;
}

//This function destroys a neighbor cost table. 
//It frees all the dynamically allocated memory for the neighbor cost table.
void nbrcosttable_destroy(nbr_cost_entry_t* nct)
{
  free(nct);	
  return;
}

//This function is used to get the direct link cost from neighbor.
//The direct link cost is returned if the neighbor is found in the table.
//INFINITE_COST is returned if the node is not found in the table.
unsigned int nbrcosttable_getcost(nbr_cost_entry_t* nct, int nodeID)
{
  int resultCost = INFINITE_COST;
  int totalNeighbors = topology_getNbrNum();
  for (int neighourIndex = 0; neighourIndex < totalNeighbors; neighourIndex++) {
  	if(nct[neighourIndex].nodeID == nodeID){
  		resultCost = nct[neighourIndex].cost;
  		break;
  	}
  }
  return resultCost;
}

//This function prints out the contents of a neighbor cost table.
void nbrcosttable_print(nbr_cost_entry_t* nct)
{
  int totalNeighbors = topology_getNbrNum();
  printf("neighbor cost table for %d \n", topology_getMyNodeID());
  printf("NodeID \t Cost \n");
  for (int neighourIndex = 0; neighourIndex < totalNeighbors; neighourIndex++) {
  		printf("%d \t %d \n", nct[neighourIndex].nodeID, nct[neighourIndex].cost );
  }	
  return;
}
