#include "server.h"

int main(int argc, char *argv[])
{
     int dataServerSocketFd, clientSocketFd, serverPortNumber, clilen, pid;
     struct sockaddr_in serverAddress, clientAddress;
     dataServerSocketFd = socket(AF_INET, SOCK_STREAM, 0);
     if (dataServerSocketFd < 0)
     {
     		showErrorAndExit("ERROR on initiating server Socket");
     }
     bzero((char *) &serverAddress, sizeof(serverAddress));
     serverPortNumber = atoi(SERVER_PORT);
     serverAddress.sin_family = AF_INET;
     serverAddress.sin_addr.s_addr = INADDR_ANY;
     serverAddress.sin_port = htons(serverPortNumber);
     if (bind(dataServerSocketFd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
     {
        showErrorAndExit("ERROR on binding");
     }
     listen(dataServerSocketFd,MAX_CLIENTS);
     clilen = sizeof(clientAddress);
     printf("Started listening\n");
     while (1) {
         clientSocketFd = accept(dataServerSocketFd, (struct sockaddr *) &clientAddress, &clilen);
         if (clientSocketFd < 0)
         {
     		showErrorAndExit("ERROR on accepting client socket ");
     	 }
         pid = fork();
         if (pid < 0)
             showErrorAndExit("ERROR on fork");
         if (pid == 0)  {
             close(dataServerSocketFd);
             talkToClient(clientSocketFd);
             exit(0);
         }
         else close(clientSocketFd);
     } 
     return 0;
}

void talkToClient(int clientSocketId){
   int n;
   char buffer[BUFFER_LENGTH];
   char expectedBuffer[BUFFER_LENGTH];
   bzero(buffer,BUFFER_LENGTH);
   // DO AUTH
   		// GET AUTH READING
   		// CHECK and Proceed by sending SUCCESS
   // READ PARAMS
   // 

   // 
   bzero(expectedBuffer,BUFFER_LENGTH);
   strcat(expectedBuffer,AUTH);
   strcat(expectedBuffer,SPACE);
   strcat(expectedBuffer,PASSWORD);
   strcat(expectedBuffer,NEW_LINE);
   n = read(clientSocketId,buffer,255);
   if (n < 0){
   		n = write(clientSocketId,SOCKET_READ_ERROR,strlen(SOCKET_READ_ERROR));
   		return;
   	}
   	printf("%s",expectedBuffer);
   	printf("%s",buffer);
   	if(strncmp(expectedBuffer,buffer,strlen(expectedBuffer) - 1) == 0){
      bzero(expectedBuffer,BUFFER_LENGTH);
      strcat(expectedBuffer,CONNECT);
      strcat(expectedBuffer,SPACE);
      strcat(expectedBuffer,DATA_SERVER_ADDRESS);
      strcat(expectedBuffer,SPACE);
      strcat(expectedBuffer,DATA_SERVER_PORT);
      strcat(expectedBuffer,SPACE);
      strcat(expectedBuffer,PASSWORD_NETWORKS);
      strcat(expectedBuffer,NEW_LINE);
   		n = write(clientSocketId,expectedBuffer,strlen(expectedBuffer));
   		if(n < 0)
   		{
   		   printf("Server write error after successful auth\n");
      }
      return;
   }
   else
   {
   		n = write(clientSocketId,UNAUTHORIZED,strlen(UNAUTHORIZED));
   		return;
   }
}