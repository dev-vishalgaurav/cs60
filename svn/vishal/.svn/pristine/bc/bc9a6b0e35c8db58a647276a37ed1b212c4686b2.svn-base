/*
    client.c
    Cory Cornelius (2009)
    COSC 60: Networks
    Lab 2
    
    YOU ARE BOUND BY THE HONOR CODE TO NOT REDISTRIBUTE THIS FILE OR ANY
    ACCOMPANYING FILES.
    
    This is a simple example of how to connect to the Three Mile Island
    sensor network. It gets the job done easily enough.

    Compile with: gcc -g -o client client.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

int prompt();
void printSensorReading(char *sensorName);

int connectToHost(char *host, int port);
int recvline(int fd, char **line);
int recvlinef(int fd, char *format, ...);
int sendline(int fd, char *format, ...);

int main(int argc, char **argv)
{
  printf("WELCOME TO THE THREE MILE ISLAND SENSOR NETWORK\n\n\n");

  // Connect to server until ctl-c and return sensor reading or error message
  while (1)
  {
    int sensor = prompt();
    
    switch (sensor)
    {
      case 1:
      printSensorReading("WATER TEMPERATURE");
      break;
      
      case 2:
      printSensorReading("REACTOR TEMPERATURE");
      break;
      
      case 3:
      printSensorReading("POWER LEVEL");
      break;
      
      default:
      printf("\n*** Invalid selection.\n\n");
      break;
    }
  }
}
/* End main */

// prompt()
// Description: Print command prompt to user and obtain user input.
// Input: None
// Output: int corresponding to which sensor to display.
int prompt()
{
  printf("Which sensor would you like to read:\n\n");
  printf("\t(1) Water temperature\n");
  printf("\t(2) Reactor temperature\n");
  printf("\t(3) Power level\n\n");
  printf("Selection: ");
  
  int sensor;
  
  if (scanf("%d", &sensor) == 1)
  {
    return sensor;
  }
  else
  {
    return -1;
  }
}
/* End prompt */

// printSensorReading(char *sensorName)
// Description: Connect to server adhering to Three Mile Island protocol and
//              print sensor readings server sends.
// Input: String of which sensor to get reading.
// Output: None
void printSensorReading(char *sensorName)
{
  /*
   Connect to redirect server to determine which sensor to connect to.
  */
  //int sockfd = connectToHost("threemileisland.info", 0xbaad);
  int sockfd = connectToHost("threemileisland.cs.dartmouth.edu", 0xbaad);
  
  if (sockfd < 0)
  {
    return;
  }
  
  sendline(sockfd, "AUTH secretpassword");
  
  char hostname[64];
  int port;
  char auth[32];
  
  if (recvlinef(sockfd, "CONNECT %64s %d %32s", hostname, &port, auth) != 3)
  {
    fprintf(stderr, "Failed to read connect data.\n");
    shutdown(sockfd, SHUT_RDWR);
    return;
  }
  
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);
  
  /*
    Connect to the specified sensor we were told to to retrieve reading.
  */
  int sensorfd = connectToHost(hostname, port);
  
  if (sensorfd < 0)
  {
    return;
  }
 
  //sendline(sensorfd, "AUTH %s", auth);
  sendline(sensorfd, "AUTH networks");
 
  char value[64];
  
  if (recvlinef(sensorfd, "%64s", value) != 1)
  {
    fprintf(stderr, "Failed to read auth data.\n");
    shutdown(sensorfd, SHUT_RDWR);
    return;
  }
 
  if (strcmp(value, "SUCCESS") != 0)
  {
    fprintf(stderr, "Authentication failed.");
    printf("Authstr: AUTH %s\n",value);
    shutdown(sensorfd, SHUT_RDWR);
    return;
  }
  
  sendline(sensorfd, sensorName);
  
  time_t timestamp;
  int reading;
  char units[16];
  
  if (recvlinef(sensorfd, "%d %d %16s", &timestamp, &reading, units) != 3)
  {
    fprintf(stderr, "Failed to read sensor data.\n");
    shutdown(sensorfd, SHUT_RDWR);
    return;
  }
  
  char formatted_time[64];
  strftime(formatted_time, 64, "%c", localtime(&timestamp));
  
  printf("\n\tThe last %s was taken %s and was %d %s\n\n\n", sensorName, formatted_time, reading, units);
  
  sendline(sensorfd, "CLOSE");
  
  shutdown(sensorfd, SHUT_RDWR);
  close(sensorfd);
}
/* End printSensorReading */

// connectToHost(char *hostname, int port)
// Description: Socket implementation of connecting to a host at a specific
//              port
// Input: String of host name, int of which port
// Output: File description of socket
int connectToHost(char *hostname, int port)
{
  /*
    Lookup the ip address of the SENSOR_HOST
  */
  struct hostent *host = gethostbyname(hostname);
  
  if (host == NULL)
  {
    perror("gethostbyname");
    return -1;
  }
  
  /*
    Create a socket, fill in sockaddr_in structure with appropriate 
    values from gethostbyname.
  */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
  if (sockfd < 0)
  {
    perror("sock");
    return -1;
  }
  
  struct sockaddr_in addr;
  
  addr.sin_family       = AF_INET;
  addr.sin_port         = htons(port);
  memcpy(&(addr.sin_addr), host->h_addr_list[0], host->h_length);
  
  if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("connect");
    return -1;
  }
  
  return sockfd;
}
/* End connectToHost */

// recvline(int fd, char **line)
// Description: Retrieve string from socket fd (could be any file descriptor
//              though)
// Input: int of file descriptor, char** that the line will be written into
// Output: int of number of bytes read
int recvline(int fd, char **line)
{
  int retVal;
  int lineIndex = 0;
  int lineSize  = 128;
  
  *line = (char *)malloc(sizeof(char) * lineSize);
  
  if (*line == NULL)
  {
    perror("malloc");
    return -1;
  }
  
  while ((retVal = read(fd, *line + lineIndex, 1)) == 1)
  {
    if ('\n' == (*line)[lineIndex])
    {
      (*line)[lineIndex] = 0;
      break;
    }
    
    lineIndex += 1;
    
    /*
      Reallocate line buffer if we get too many characters.
    */
    if (lineIndex > lineSize)
    {
      lineSize *= 2;
      char *newLine = realloc(*line, sizeof(char) * lineSize);
      
      if (newLine == NULL)
      {
        retVal    = -1; /* realloc failed */
        break;
      }
      
      *line = newLine;
    }
  }
  
  if (retVal < 0)
  {
    free(*line);
    return -1;
  }
  #ifdef NDEBUG
  else
  {
    fprintf(stdout, "%03d> %s\n", fd, *line);
  }
#endif

  return lineIndex;
}
/* End recvline */

// recvlinef(int fd, char *format, ...)
// Description: Receive data from a file descriptor (socket in this case).
//              Here you can specify a format string and points to variables 
//              to store the results in (like scanf)
// Input: int of file descriptor, char* the format string of the line, and any
//        pointers to variables to store results in
// Output: int number of bytes read
int recvlinef(int fd, char *format, ...)
{
  va_list argv;
  va_start(argv, format);
  
  int retVal = -1;
  char *line;
  int lineSize = recvline(fd, &line);
  
  if (lineSize > 0)
  {
    retVal = vsscanf(line, format, argv);
    free(line);
  }
  
  va_end(argv);
  
  return retVal;
}
/* End recvlinef */

// sendline(int fd, char *format, ..)
// Description: Writes formatted output to file descriptor
// Input: int of file descriptor and string to write, any variables the value
//        of which goes into the string
// Output: int of number of bytes written
int sendline(int fd, char *format, ...)
{
  va_list argv;
  va_start(argv, format);
  
  int lineSize = vsnprintf(NULL, 0, format, argv) + 1;
  char line[lineSize+1];
  
  vsnprintf(line, lineSize, format, argv);
  
  line[lineSize - 1]  = '\n';
  line[lineSize]  = 0;
  
  int retVal = write(fd, line, lineSize);
  
#ifdef NDEBUG
  fprintf(stdout, "%03d< %s", fd, line);
#endif

  va_end(argv);
  
  return retVal;
}
