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
void talkToClient(int clientSocketId);

