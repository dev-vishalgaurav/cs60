#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define true 1
#define false 0
#define ERROR  -1
#define OK 1
#define BUFFER_LENGTH 256
#define AUTH  "AUTH"
#define PASSWORD  "secretpassword"
#define PASSWORD_NETWORKS  "networks"
#define UNAUTHORIZED  "Unauthorized\n"
#define SOCKET_READ_ERROR  "Socket Read Error\n"
#define NEW_LINE  "\n"
#define SPACE  " "
#define SUCCESS  "SUCCESS\n"
#define CONNECT  "CONNECT"
#define CLOSE  "CLOSE"
#define BYE  "BYE"
#define WATER  "WATER TEMPERATURE\n"
#define REACTOR  "REACTOR TEMPERATURE\n"
#define POWER  "POWER LEVEL\n"
#define SERVER_PORT "55123"
#define DATA_SERVER_PORT "55512"
#define UNIT_MW  "MW"
#define UNIT_F  "F"
#define MAX_CLIENTS 5
#define DATA_SERVER_ADDRESS "localhost"

void showErrorAndExit(char *msg);
int input_string(char *message, char *buffer, int len);
