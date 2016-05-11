

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../common/constants.h"
#include "../topology/topology.h"
#include "routingtable.h"

//This is the hash function used the by the routing table
//It takes the hash key - destination node ID as input, 
//and returns the hash value - slot number for this destination node ID.
//
//You can copy makehash() implementation below directly to routingtable.c:
//int makehash(int node) {
//	return node%MAX_ROUTINGTABLE_ENTRIES;
//}
//
int makehash(int destNodeID)
{
  return destNodeID % MAX_ROUTINGTABLE_SLOTS;
}
/*
//routingtable_entry_t is the routing entry contained in the routing table.
typedef struct routingtable_entry {
	int destNodeID;		//destination node ID
	int nextNodeID;		//next node ID to which the packet should be forwarded
	struct routingtable_entry* next;	//pointer to the next routingtable_entry_t in the same routing table slot
} routingtable_entry_t;

//A routing table is a hash table containing MAX_ROUTINGTABLE_SLOTS slots. Each slot is a linked list of routing entries.
typedef struct routingtable {
	routingtable_entry_t* hash[MAX_ROUTINGTABLE_SLOTS];
} routingtable_t;
*/

void reset_routing_table(routingtable_t* routing_table){
	for (int count = 0; count < MAX_ROUTINGTABLE_SLOTS; ++count)
	{
		routing_table->hash[count] = NULL;
	}
}

//This function creates a routing table dynamically.
//All the entries in the table are initialized to NULL pointers.
//Then for all the neighbors with a direct link, create a routing entry using the neighbor itself as the next hop node, and insert this routing entry into the routing table. 
//The dynamically created routing table structure is returned.
routingtable_t* routingtable_create()
{
  routingtable_t *routing_table = (routingtable_t *) malloc( sizeof(routingtable_t));
  // after malloc reset the routing table
  reset_routing_table(routing_table);
  int* allNeighbours = topology_getNbrArray();
  int total_neighbour_count = topology_getNbrNum();
  for (int index = 0; index < total_neighbour_count; index++)
  {
  	// reusing the set next node method defined below.
  	routingtable_setnextnode(routing_table,allNeighbours[index],allNeighbours[index]);
  }
  free(allNeighbours);
  return routing_table;
}


//This funtion destroys a routing table. 
//All dynamically allocated data structures for this routing table are freed.
void routingtable_destroy(routingtable_t* routingtable)
{	
	if(routingtable){
		for (int index = 0; index < MAX_ROUTINGTABLE_SLOTS; index++)
		{
			if(routingtable->hash[index] != NULL){
				routingtable_entry_t *head = routingtable->hash[index];
				while(head != NULL){
					routingtable_entry_t *next = head->next; // store the next entry
					free(head); // free current entry
					head = next; // assign the next pointer
				}
			}
		}
	}
	free(routingtable);
}

routingtable_entry_t* get_new_route_entry(int destNodeID, int nextNodeID){
	routingtable_entry_t *newNode = (routingtable_entry_t *) malloc(sizeof(routingtable_entry_t));
	newNode->destNodeID = destNodeID;
	newNode->nextNodeID = nextNodeID;
	newNode->next = NULL;
	return newNode;
}

//This function updates the routing table using the given destination node ID and next hop's node ID.
//If the routing entry for the given destination already exists, update the existing routing entry.
//If the routing entry of the given destination is not there, add one with the given next node ID.
//Each slot in routing table contains a linked list of routing entries due to conflicting hash keys (differnt hash keys (destination node ID here) may have same hash values (slot entry number here)).
//To add an routing entry to the hash table:
//First use the hash function makehash() to get the slot number in which this routing entry should be stored. 
//Then append the routing entry to the linked list in that slot.
void routingtable_setnextnode(routingtable_t* routingtable, int destNodeID, int nextNodeID)
{
	int slotIndex = makehash(destNodeID);
	if(slotIndex >= 0){
		if(routingtable->hash[slotIndex] == NULL){
			routingtable->hash[slotIndex] = get_new_route_entry(destNodeID,nextNodeID);
		}else{
			routingtable_entry_t *head = routingtable->hash[slotIndex];
			while(head->next != NULL){
				if(head->destNodeID == destNodeID){
					head->nextNodeID = nextNodeID;
					return;
				}
				head = head->next;
			}
			head->next = get_new_route_entry(destNodeID,nextNodeID);
		}
	}
}

//This function looks up the destNodeID in the routing table.
//Since routing table is a hash table, this opeartion has O(1) time complexity.
//To find a routing entry for a destination node, you should first use the hash function makehash() to get the slot number and then go through the linked list in that slot to search the routing entry.
//If the destNodeID is found, return the nextNodeID for this destination node.
//If the destNodeID is not found, return -1.
int routingtable_getnextnode(routingtable_t* routingtable, int destNodeID)
{	
	int result = -1;
	int slotIndex = makehash(destNodeID);
	if(slotIndex >= 0){
		routingtable_entry_t *head = routingtable->hash[slotIndex];
		while(head != NULL){
			if(head->destNodeID == destNodeID){
				result = head->nextNodeID ;
				break;
			}
			head = head->next;
		}
	}
	return result;
}

//This function prints out the contents of the routing table
void routingtable_print(routingtable_t* routingtable)
{
  printf("Printing Routing table \n");
  if(routingtable){
  	printf("%10s\n","Hash No" );
  	for(int index = 0 ; index < MAX_ROUTINGTABLE_SLOTS ; index++){
  		if(routingtable->hash[index] != NULL){
  			printf("%10d %5s", index,"");
  			routingtable_entry_t *head = routingtable->hash[index];
			while(head != NULL){
				printf("{'dest':%d, 'next':%d}",head->destNodeID,head->nextNodeID );
				if(head->next!=NULL){
					printf(" -> \n");
				}
				head = head->next;
			}
  		}else{
  			printf("%10d %10s", index," {NULL}");
  		}
  		printf("\n");
  	}	
  }
}
