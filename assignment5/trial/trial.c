
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

int topology_getNodeIDfromname(char* hostname) 
{
  struct hostent *hostInfo;
  hostInfo = gethostbyname2(hostname,AF_INET);
  if(!hostInfo) {
  	printf("error in getting host name from string!\n");
  	return -1;
  }
  struct sockaddr_in servaddr;
  memcpy((char *) &servaddr.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
  int ipAddr = ntohl(servaddr.sin_addr.s_addr); // network order to host byte order
  return ipAddr & 0x000000FF; // to get the last digit od ip address mask it with 0.0.0.255 :D 
}

//129.170.212.20
int main() {
	char* url = "wildcat.cs.dartmouth.edu";
	printf("last digit = %d\n", topology_getNodeIDfromname(url));
	return 0 ;
}