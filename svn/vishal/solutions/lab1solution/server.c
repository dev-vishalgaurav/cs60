/*
	 example_server.c
	 created by Qingyuan Kong at April 7,2011
	 COSC 78: Networks
	 Lab 2

	 ===
	 This solution should not be distributed to any other student in or
	 outside of this class under any circumstances.
	 ===

	 This is a simple example of how to write the server.

	 Instead of threemileisland, this sever should be run on tahoe.cs.dartmouth.edu

	 Compile with: gcc -o example_server example_server.c
	 */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>


#define SERV_PORT 47789
#define LISTENQ 30
#define MAXLINE 4096
#define TIME_OUT_SEC 10

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void time_out_exit(int s){
	exit(3);
}


int receiveline(int sockid, char* buf, int bufsize)
{
	int n;
	//clear buffer
	memset(buf,0,bufsize);
	if((n=recv(sockid,buf,bufsize,0))>0)
	{
		if(n==bufsize)
		{
			perror("read overflow!\n");
			return 1;
		}
		buf[bufsize-1]=0; //make sure the buf end with 0;
		return 0;
	}
	else
	{
		perror("read error!\n");
		return -1;
	}
}

int sendline(int sockid, char* buf) // the null character at the end of string is not sent;
{
	return send(sockid,buf,strlen(buf),0);
}

int main()
{
	int listenfd, connfd, nextport,value, nr;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	char buf[MAXLINE];
	char unit[3];
	unsigned int curtime;
	struct itimerval tout_val;
	struct sigaction sa;


    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

	//Create a socket for the soclet
	//If sockfd<0 there was an error in the creation of the socket/
	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0)
	{
		perror("Problem in creating the socket");
		exit(2);
	}

	//preparation of the socket address
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen (listenfd, LISTENQ);

	printf("%s\n","Server running...waiting for connections.");

	while(1)
	{
		clilen = sizeof(cliaddr);
		//accept a connection
		connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
		if (connfd == -1){
			perror("accept");
			continue;
		}
		
		printf("%s\n","Received request...");

		if ( (childpid = fork ()) == 0 ) //child process
		{
			close(listenfd); //close the listen socket in the child process


			tout_val.it_interval.tv_sec = 0;
  			tout_val.it_interval.tv_usec = 0;
  			tout_val.it_value.tv_sec = TIME_OUT_SEC; 
  			tout_val.it_value.tv_usec = 0;
  			setitimer(ITIMER_REAL, &tout_val,0);

  			signal(SIGALRM, time_out_exit); /* set the Alarm signal capture */

			//step 1: receive passwd
			nr = receiveline(connfd, buf, MAXLINE);
			if( nr == -1 || strcmp(buf,"AUTH secretpassword\n") !=0)
			{
				sendline(connfd,"Authorization failed! Connection close!\n");
				close(connfd);
				exit(1);
			}

			//step 2: send the client the host name and next post
			curtime=time(NULL);
			srand(curtime);
			nextport=SERV_PORT+rand()%15000;
			memset(buf,0,MAXLINE);
			sprintf(buf,"CONNECT threemileisland.cs.dartmouth.edu %d networks\n", nextport);
			if (sendline(connfd,buf) < 0){
				close(connfd);
				exit(1);
			}
			close(connfd);

			//step 3: set up next socket and wait for connection
			if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0)
			{
				perror("Problem in creating the socket");
				exit(2);
			}

			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
			servaddr.sin_port = htons(nextport);
			bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
			listen (listenfd, LISTENQ);
			
			//step 4: receive next passwd
			connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
			close(listenfd);
			nr = receiveline(connfd, buf, MAXLINE);
			if( nr == -1 || strcmp(buf,"AUTH networks\n") !=0)
			{
				sendline(connfd,"Authorization failed\n");
				close(connfd);
				exit(1);
			}
			if (sendline(connfd,"SUCCESS\n")<0){
				close(connfd);
				exit(1);
			}

			//step 5: receive request
			nr = receiveline(connfd, buf, MAXLINE);
			if( nr == -1)
			{
				close(connfd);
				exit(1);
			}
			
			switch(buf[0])
			{
				case 'W':
					value = 100 + (rand() % 150);
					memset(unit,0,3);
					sprintf(unit,"F");
					break;
				case 'R':
					value = 1000 + (rand() % 300);
					memset(unit,0,3);
					sprintf(unit,"F");
					break;
				case 'P':
					value = 400 + (rand() % 100);
					memset(unit,0,3);
					sprintf(unit,"MW");
					break;
				default:
					break;
			}
			memset(buf,0,MAXLINE);
			sprintf(buf, "%d %d %s\n", curtime, value, unit);
			if (sendline(connfd,buf) < 0){
				close(connfd);
				exit(1);	  	  
			}
			//step 6: receive close request and close
			receiveline(connfd, buf, MAXLINE);
			if(strcmp(buf,"CLOSE\n")==0)
				sendline(connfd,"BYE\n");
			close(connfd);
			exit(0);	  	  
		}
		close(connfd);
	}
}
