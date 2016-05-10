
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
    int *allNeighboursNodes = topology_getNbrArray();
    int mallocSize = topology_getNbrNum() * sizeof(nbr_cost_entry_t);
    nbr_cost_entry_t *costTable = NULL;
    if(mallocSize > 0 && allNeighboursNodes){
       costTable = (nbr_cost_entry_t *)malloc(mallocSize);
      if(costTable){
        //set the values in neighbor cost table
        for (int neighourIndex = 0; neighourIndex < topology_getNbrNum(); neighourIndex++) {
            costTable[neighourIndex].nodeID = allNeighboursNodes[neighourIndex];
            // get cost between this node and neighbour
            costTable[neighourIndex].cost = topology_getCost(topology_getMyNodeID(), costTable[neighourIndex].nodeID); 
        }
        free(allNeighboursNodes);
    }
  }
    return costTable;
}

//This function destroys a neighbor cost table. 
//It frees all the dynamically allocated memory for the neighbor cost table.
void nbrcosttable_destroy(nbr_cost_entry_t* nct)
{
  free(nct);
}

//This function is used to get the direct link cost from neighbor.
//The direct link cost is returned if the neighbor is found in the table.
//INFINITE_COST is returned if the node is not found in the table.
unsigned int nbrcosttable_getcost(nbr_cost_entry_t* nct, int nodeID)
{
  int resultCost = INFINITE_COST;
  for (int neighourIndex = 0; neighourIndex < topology_getNbrNum(); neighourIndex++) {
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
  printf("neighbor cost table for %d \n", topology_getMyNodeID());
  printf("NodeID \t Cost \n");
  for (int neighourIndex = 0; neighourIndex < topology_getNbrNum(); neighourIndex++) {
  		printf("%d \t %d \n", nct[neighourIndex].nodeID, nct[neighourIndex].cost );
  }	
}
