#include "server_data.h"

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
     serverPortNumber = atoi(DATA_SERVER_PORT);
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
   strcat(expectedBuffer,PASSWORD_NETWORKS);
   strcat(expectedBuffer,NEW_LINE);
   n = read(clientSocketId,buffer,255);
   if (n < 0){
   		n = write(clientSocketId,SOCKET_READ_ERROR,strlen(SOCKET_READ_ERROR));
   		return;
   	}
   	printf("%s",expectedBuffer);
   	printf("%s",buffer);
   	if(strncmp(expectedBuffer,buffer,strlen(expectedBuffer) - 1) == 0){
   		n = write(clientSocketId,SUCCESS,strlen(SUCCESS));
   		if(n > 0)
   		{
   			bzero(buffer,BUFFER_LENGTH);
   			n = read(clientSocketId,buffer,255);
   			if(n > 0)
   			{
   				time_t currentTime = time(NULL);
   				char date[10];
   				sprintf(date,"%d",(int)currentTime);
   				char* unit;
   				char* value = "123";
	   			if(strncmp(buffer,WATER, strlen(WATER) -1) == 0)
	   			{
	   				unit = UNIT_F;
	   			}
	   			else if(strncmp(buffer,REACTOR, strlen(REACTOR) -1) == 0)
	   			{
	   				unit = UNIT_F;
	   			}
	   			else if(strncmp(buffer,POWER, strlen(POWER) -1) == 0)
	   			{
	   				unit = UNIT_MW;
	   			}else
	   			{
	   				n = write(clientSocketId,UNAUTHORIZED,strlen(UNAUTHORIZED));
   					return;
	   			}
	   		   bzero(expectedBuffer,BUFFER_LENGTH);
			   strcat(expectedBuffer,date);
			   strcat(expectedBuffer,SPACE);
			   strcat(expectedBuffer,value);
			   strcat(expectedBuffer,SPACE);
			   strcat(expectedBuffer,unit);
			   strcat(expectedBuffer,NEW_LINE);
			   n = write(clientSocketId,expectedBuffer,strlen(expectedBuffer));
			   if(n > 0 ){
			   		bzero(expectedBuffer,strlen(expectedBuffer));
			   		bzero(buffer,strlen(buffer));
			   		n = read(clientSocketId,buffer,255);
			   		strcat(expectedBuffer,CLOSE);
			   		strcat(expectedBuffer,NEW_LINE);
			   		if(strncmp(expectedBuffer,buffer, strlen(expectedBuffer) -1) == 0)
			   		{
			   			bzero(buffer,strlen(buffer));
			   			strcat(buffer,BYE);
			   			strcat(buffer,NEW_LINE);
			   			n = write(clientSocketId,buffer,strlen(buffer));
			   			return;
			   		}
			   		else
			   		{
			   			n = write(clientSocketId,SOCKET_READ_ERROR,strlen(SOCKET_READ_ERROR));
   						return;
			   		}
			   }
   			}
   			else
   			{
   				n = write(clientSocketId,SOCKET_READ_ERROR,strlen(SOCKET_READ_ERROR));
   				return;
   			}

   		}
   }
   else
   {
   		n = write(clientSocketId,UNAUTHORIZED,strlen(UNAUTHORIZED));
   		return;
   }
}