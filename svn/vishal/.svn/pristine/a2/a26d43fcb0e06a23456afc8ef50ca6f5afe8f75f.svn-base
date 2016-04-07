#include "client.h"

#define CAPTION_LEN 128
int input = 0;
char readBuffer[256];
char writeBuffer[256];

char resultTime[256];
char resultValue[256];
char resultUnit[256];

int totalWrite;
int totalRead;
char* tokenPointer;


char* startMessage = "AUTH secretpassword\n";
char dataServerUrl[256] ;
char dataServerAuth[256] ;
char dataServerPortString[256] ;
char* caption;

int mainServerSocketD ;
struct sockaddr_in mainServerAdd;
struct hostent *mainServer;
int mainServerPort;

int dataServerSocketD;
struct sockaddr_in dataServerAdd;
struct hostent *dataServer;
int dataServerPort;
int isConnected = false;

// convert port no in string to integer

int main(int argc, char *argv[]){
    allocMemory();
    displayMenu();
    while(1){
        bzero(caption,CAPTION_LEN);
        getUserInput();
        // printf("User input is %s\n", caption);
        char* token ;
        token = strtok_r(caption, " ",&tokenPointer);
        while(token!=NULL){
            // printf("Token =  %s\n", token);
            input = atoi(token);
            if( input <=4){
                //printf("Input = %d \n" , input );
                if(input == 4){
                    printf("Exit menu selected\n");
                    exit(0);
                }
                if(doAuthRequest() != ERROR){
                //printf("Input = %d \n" , input );
                    doDataRequest();
                }else{
                    printf("Auth Error from server\n");
                }             
            }else{
                printf("Invalid Input ***\n");
            }

            token = strtok_r(NULL, " ",&tokenPointer);
        }
        //clearScreen(1);
        callEnterToContinue();   
        displayMenu();
    }
    deAllocMemory();
    return 0;    
}
/**
* allocate memory
*/
void allocMemory(){
    caption = (char *)malloc(CAPTION_LEN);
}
/**
* free memory
*/
void deAllocMemory(){
    free(caption);
}

/**
* this method will display the menu to be shown to user for his input
*/
void displayMenu(){
    printf("\n\tWELCOME TO THE THREE MILE ISLAND SENSOR NETWORK\n\n\n\tWhich sensor data would you like to read:\n\n\t\t(1) Water temperature \n\t\t(2) Reactor temperature \n\t\t(3) Power level \n\t\t(4) Exit ! \n\n\n\tSelection: ");
}
void getUserInput(){
    input_string("",caption,CAPTION_LEN);
}
int getIntInput(){
    int newInput;
    scanf("%d", &newInput);
    getchar();
    return newInput;
}
void resetDataServerParams(){
     bzero(dataServerUrl,256);
     bzero(dataServerAuth,256);
     bzero(dataServerPortString,256);
}
int doAuthRequest(){
    int result = -1;
            //1. Create Socket
        // create socket for the main server address and store descriptor
        mainServerPort = atoi(MAIN_SERVER_PORT);
        mainServerSocketD = socket(AF_INET, SOCK_STREAM, 0);
        if (mainServerSocketD < 0) {
            showErrorAndExit("ERROR opening socket");
        }
        mainServer = gethostbyname(MAIN_SERVER_IP);
        if (mainServer == NULL) {
           showErrorAndExit("Server address is not valid \n exiting...");
        }
        
        // make all chars to bit 0
        bzero((char *) &mainServerAdd, sizeof(mainServerAdd));
        mainServerAdd.sin_family = AF_INET;
        // copy addresses from DNS lookup to socket address
        bcopy((char *)mainServer->h_addr, (char *)&mainServerAdd.sin_addr.s_addr, mainServer->h_length);
        mainServerAdd.sin_port = htons(mainServerPort); // changin the byte order to network order
        //2. Connect to predfined server traced from WireShark       
        if(connect(mainServerSocketD,(struct sockaddr *)&mainServerAdd,sizeof(mainServerAdd)) < 0){
            printf("connection failed to %s",MAIN_SERVER_IP); 
            showErrorAndExit("ERROR connecting to server");
         }
         //printf("server connected %s\n" , MAIN_SERVER_IP);
         bzero(writeBuffer,256);
         strcat(writeBuffer,AUTH);
         strcat(writeBuffer,SPACE);
         strcat(writeBuffer,PASSWORD);
         strcat(writeBuffer,NEW_LINE);
         totalWrite = write(mainServerSocketD,writeBuffer,strlen(writeBuffer));
         bzero(readBuffer,256);
         totalRead = read(mainServerSocketD,readBuffer,255);
         resetDataServerParams();
         if(totalRead > 0){
            //printf("%s",readBuffer);
            char *token;
            /* get the first token */
            token = strtok(readBuffer," ");
            /* first check if success then walk through other tokens */
            if(strcmp(token,CONNECT) == 0){ // request was successful according to wireshark
                //printf( "Authorized \n");
                result = OK;
                token = strtok(NULL, " ");
                strcpy(dataServerUrl,token);
                token = strtok(NULL, " ");
                strcpy(dataServerPortString,token);
                token = strtok(NULL, " ");
                strcpy(dataServerAuth,token);
                //printf("%s\n",dataServerUrl);
                //printf("%s\n",dataServerPortString);                
                //printf("%s\n",dataServerAuth);
            }else{
                printf("UnAuthorized Try Again ! \n");
            }
         }else{
            printf("No data recieve from server\n");
         }
         if (close(mainServerSocketD) < 0){ // finally call close()
              printf("Socket not closed");
         }
 
    return result;
}
void displayResult(char* queryParam){
    char* token;
    token = strtok(readBuffer," "); 
    strcpy(resultTime,token);   
    time_t time = (time_t)atoi(resultTime);
    struct tm *info;
    info = localtime( &time );
    strftime(resultTime,255,"%c", info);
    token = strtok(NULL, " ");
    strcpy(resultValue,token);
    token = strtok(NULL, " ");
    strcpy(resultUnit,token);
    printf("The last %s was taken %s and was %s %s\n",queryParam,resultTime,resultValue,resultUnit);
}
void requestData(char* queryParam){

    //1. Create Socket
        // create socket for the main server address and store descriptor
        dataServerPort = atoi(dataServerPortString);
        dataServerSocketD = socket(AF_INET, SOCK_STREAM, 0);
        if (dataServerSocketD < 0) {
            showErrorAndExit("ERROR opening socket for Water temperature");
        }
        dataServer = gethostbyname(dataServerUrl);
        if (dataServer == NULL) {
           showErrorAndExit("Server address for water temperature is not valid \n exiting...\n");
        }
        
        // make all chars to bit 0
        bzero((char *) &dataServerAdd, sizeof(dataServerAdd));
        dataServerAdd.sin_family = AF_INET;
        // copy addresses from DNS lookup to socket address
        bcopy((char *)dataServer->h_addr, (char *)&dataServerAdd.sin_addr.s_addr, dataServer->h_length);
        dataServerAdd.sin_port = htons(dataServerPort); // changin the byte order to network order
        //2. Connect to predfined server traced from WireShark       
        if(connect(dataServerSocketD,(struct sockaddr *)&dataServerAdd,sizeof(dataServerAdd)) < 0){
            printf("Water temp connection failed to %s\n",dataServerUrl); 
            showErrorAndExit("ERROR connecting to server");
            return;
         }
         //printf("water temp server connected %s\n" , dataServerUrl);
         bzero(writeBuffer,256);
         strcat(writeBuffer,AUTH);
         strcat(writeBuffer,SPACE);
         strcat(writeBuffer,dataServerAuth);
         //strcat(writeBuffer,NEW_LINE);
         //printf("Writing to Server:- %s\n",writeBuffer);
         totalWrite = write(dataServerSocketD,writeBuffer,strlen(writeBuffer));
         bzero(readBuffer,256);
         totalRead = read(dataServerSocketD,readBuffer,255);
         if(totalRead > 0){
           // printf("%s",readBuffer);
            char *token;
            /* get the first token */
            token = strtok(readBuffer," ");
            //printf("Token = %s",token);
            /* first check if success then walk through other tokens */
            if(strcmp(token,SUCCESS) == 0){ // request was successful according to wireshark
                //printf( "Water temp Authorized \n");
                totalWrite = write(dataServerSocketD,queryParam,strlen(writeBuffer));
                bzero(readBuffer,256);
                totalRead = read(dataServerSocketD,readBuffer,255);
                //printf("%s\n",readBuffer);
                displayResult(queryParam);
            }else{
                printf("UnAuthorized Try Again ! \n");
            }
         bzero(writeBuffer,256);
         strcat(writeBuffer,CLOSE);
         strcat(writeBuffer,NEW_LINE);
         totalWrite = write(dataServerSocketD,writeBuffer,strlen(writeBuffer));
         bzero(readBuffer,256);
         totalRead = read(dataServerSocketD,readBuffer,255);
         }else{
            printf("No data recieve from server\n");
         }
         // if (close(dataServerSocketD) < 0){ // finally call close()
         //      printf("Socket not closed");
         // }
    
}

int doDataRequest(){
    if(input == REQUEST_WATER_TEMP ){
        //printf("Request Water");
        requestData(WATER);
    }else if(input == REQUEST_REACTOR_TEMP ){
            requestData(REACTOR);
        //printf("Request REACTOR");
    }else if(input == REQUEST_POWER_LEVEL ){
        requestData(POWER);
    }
    sleep(1);
    printf("\n");
}
/*
* this methid will call "clear command" through exec
*/
int clearScreen(int shouldWaitToFinish){
    int pid, rc, status ;
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    /* child process for clearing screen starts */
    rc  = fork();
    if (0 == rc) {
        pid = getppid();
        char *args[] = {NULL};
        rc = execvp("clear",args);
        exit(11);
    }
    pid = rc ;
    rc = waitpid(pid, &status, 0);
    return rc ;
    /* child process for clear ends */
}
/**
*helper method to ask user to press enter and continue
*/
void callEnterToContinue(void){
    printf("Please press enter to continue : \n");
        while (true){
            int c = getchar();
      if (c == '\n' || c == EOF)
                break;
            }
}

