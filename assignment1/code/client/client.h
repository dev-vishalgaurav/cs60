#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include<string.h>
#include<unistd.h>
#include<time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "../const.h"
#define MAIN_SERVER_IP "129.170.213.101"
#define MAIN_SERVER_PORT "47789"


const char REQUEST_WATER_TEMP = 1;
const char REQUEST_REACTOR_TEMP = 2;
const char REQUEST_POWER_LEVEL = 3;

void showErrorAndExit(char *msg);
void displayMenu();
int getIntInput();
void resetDataServerParams();
int doAuthRequest();
void displayResult(char* queryParam);
void requestData(char* queryParam);
int doDataRequest();
int clearScreen(int shouldWaitToFinish);
void callEnterToContinue(void);
