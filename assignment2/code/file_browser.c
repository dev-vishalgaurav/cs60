/*
 * FILE: file_browser.c 
 *
 * Description: A simple, iterative HTTP/1.0 Web server that uses the
 * GET method to serve static and dynamic content.
 *
 * Date: April 4, 2016
 */

#include <arpa/inet.h>          // inet_ntoa
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define LISTENQ  1024  // second argument to listen()
#define MAXLINE 1024   // max length of a line
#define RIO_BUFSIZE 1024
#define true 1
#define false 0
#define ERROR  -1
#define OK 1
#define MAX_CLIENTS 9999
#define MAX_POOL 11
#define RESPONSE_HEADER_SUCCESS "HTTP/1.1 200 OK\r\n"
#define FILE_BROSWER "recent_browser.txt"
#define FILE_IP "ip_address.txt"

typedef struct {
    int rio_fd;                 // descriptor for this buf
    int rio_cnt;                // unread byte in this buf
    char *rio_bufptr;           // next unread byte in this buf
    char rio_buf[RIO_BUFSIZE];  // internal buffer
} rio_t;

// simplifies calls to bind(), connect(), and accept()
typedef struct sockaddr SA;

typedef struct {
    char filename[512];
    int browser_index;
    off_t offset;              // for support Range
    size_t end;
} http_request;

typedef struct {
    const char *extension;
    const char *mime_type;
} mime_map;

mime_map meme_types [] = {
    {".css", "text/css"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".ico", "image/x-icon"},
    {".js", "application/javascript"},
    {".pdf", "application/pdf"},
    {".mp4", "video/mp4"},
    {".png", "image/png"},
    {".svg", "image/svg+xml"},
    {".xml", "text/xml"},
    {NULL, NULL},
};

char *default_mime_type = "text/plain";
char *browser_types[] = {"Chrome","Safari","Firefox","Others","Others"};

char *browser_pool[MAX_POOL];
int browserCount = 0;

char* ip_address_pool[MAX_POOL];
int ipAddressCount = 0;

// declaring variables for server socket information
int server_port_number;
struct sockaddr_in serverAddress;
int serverSocketFd;

// working directory
char *workingDirectory;

// utility function to get the format size
void format_size(char* buf, struct stat *stat){
    if(S_ISDIR(stat->st_mode)){
        sprintf(buf, "%s", "[DIR]");
    } else {
        off_t size = stat->st_size;
        if(size < 1024){
            sprintf(buf, "%lu", size);
        } else if (size < 1024 * 1024){
            sprintf(buf, "%.1fK", (double)size / 1024);
        } else if (size < 1024 * 1024 * 1024){
            sprintf(buf, "%.1fM", (double)size / 1024 / 1024);
        } else {
            sprintf(buf, "%.1fG", (double)size / 1024 / 1024 / 1024);
        }
    }
}


void appedFilesHtmlFromDir(int dirFd, char *fileName, char *buffer){
    DIR *dir ;
    struct dirent *ent;
    if ((dir = opendir (fileName)) != NULL) {
      char temp[MAXLINE] ;
      struct stat sbuf;
      int ffd = 0;
      printf("appedFilesHtmlFromDir :- %s\n",fileName);
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {
        char resultTime[30];
        char childFileName[256];
        sprintf(childFileName,"%s%s",fileName,ent->d_name); // append directory with files
        ffd = open(childFileName, O_RDONLY, 0);
        fstat(ffd, &sbuf);
        time_t time = sbuf.st_atime ;
        strftime(resultTime,30,"%Y-%m-%d %H:%M:%S", localtime(&time));
        //printf("FILE = %s\n", childFileName );
        if(S_ISREG(sbuf.st_mode)){
            bzero(temp,strlen(temp));
            char fileSizeFormat[45];
            format_size(fileSizeFormat,&sbuf);
            sprintf (temp,"<tr><td><a href=\"%s\">%s</a></td><td>%s</td><td>%s</td></tr>",ent->d_name,ent->d_name,resultTime,fileSizeFormat);
            strcat(buffer,temp);
        }else{
            printf("Not a file\n");
            fflush(stdout);
        }
        close(ffd);
      }
      closedir(dir);
    } else {
      /* could not open directory */
      printf ("Could not open directory :- %s \n", fileName);
      return ;
    }  
}

int appendBrowser(char *buffer){
    printf("appendBrowser\n");
    char fileName[MAXLINE] ;
    int result = -1;
    sprintf(fileName,"%s%s",workingDirectory,"recent_browser.txt");
    FILE *file = fopen (fileName,"r");
    printf("Reading browsers from  %s \n", fileName);
    char temp[MAXLINE] ;
    if(file != NULL){
        sprintf(temp,"%s","<table><tr><td>The last visited browsers:</td>"); 
        result = 0;
        size_t maxLine = 256;
        char line[128];
        int read = -1;
        while (fgets(line, 255, (FILE*)file) > 0)
        {   
            sprintf(temp,"%s<td>%s</td>",temp,line); 
            fflush(stdout);
        }
        sprintf(temp,"%s%s",temp,"</tr>"); 
        fclose(file);
        strcat(buffer,temp);
    }else{
        printf("Error in update ip address log file\n");
    }
    printf("finish appendBrowser\n");
    return result;
}
int appendIp(char *buffer){
    printf("appendIp\n");
    char fileName[MAXLINE] ;
    int result = -1;
    sprintf(fileName,"%s%s",workingDirectory,"ip_address.txt");
    FILE *file = fopen (fileName,"r");
    printf("Reading ip addresses from  %s \n", fileName);
    char temp[MAXLINE] ;
    if(file != NULL){
        sprintf(temp,"%s","<tr><td>The corresponding IP addresses:</td>"); 
        result = 0;
        size_t maxLine = 256;
        char line[128];
        int read = -1;
        while (fgets(line, 255, (FILE*)file) > 0)
        {   
            sprintf(temp,"%s<td>%s</td>",temp,line); 
            fflush(stdout);
        }
        sprintf(temp,"%s%s",temp,"</tr>"); 
        fclose(file);
        strcat(buffer,temp);
    }else{
        printf("Error in update ip address log file\n");
    }
    printf("finish appendIp\n");
    return result;
}

void showErrorAndExit(char *msg){
    perror(msg);
    exit(0);
}
// set up an empty read buffer and associates an open file descriptor with that buffer
void rio_readinitb(rio_t *rp, int fd){
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

// utility function for writing user buffer into a file descriptor
ssize_t written(int fd, void *usrbuf, size_t n){
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0){
        if ((nwritten = write(fd, bufp, nleft)) <= 0){
            if (errno == EINTR)  // interrupted by sig handler return
                nwritten = 0;    // and call write() again
            else
                return -1;       // errorno set by write()
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}


/*
 *    This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n){
    int cnt;
    while (rp->rio_cnt <= 0){  // refill if buf is empty
        
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
                           sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0){
            if (errno != EINTR) // interrupted by sig handler return
                return -1;
        }
        else if (rp->rio_cnt == 0)  // EOF
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf; // reset buffer ptr
    }
    
    // copy min(n, rp->rio_cnt) bytes from internal buf to user buf
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

// robustly read a text line (buffered)
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen){
    int n, rc;
    char c, *bufp = usrbuf;
    
    for (n = 1; n < maxlen; n++){
        if ((rc = rio_read(rp, &c, 1)) == 1){
            *bufp++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0){
            if (n == 1)
                return 0; // EOF, no data read
            else
                break;    // EOF, some data was read
        } else
            return -1;    // error
    }
    *bufp = 0;
    return n;
}



// pre-process files in the "home" directory and send the list to the client
void handle_directory_request(int out_fd, int dir_fd, char *filename){
    printf("Handle directory request\n");
    char buffer[MAXLINE] ;
    // send response headers to client e.g., "HTTP/1.1 200 OK\r\n"
    bzero(buffer,MAXLINE);
    strcat(buffer,"HTTP/1.0 200 OK\n\n");
    int n = write(out_fd,buffer,strlen(buffer));
    bzero(buffer,MAXLINE);
    strcat(buffer,"<html><head><style>body{font-family: monospace; font-size: 13px;}td {padding: 1.5px 6px;}</style></head><body><table>");
    appedFilesHtmlFromDir(dir_fd,filename,buffer);
    strcat(buffer,"</html>\n");
    appendBrowser(buffer);
    appendIp(buffer);
    //printf("%s\n",buffer);
    // get file directory
    
    // read directory
    
    // send the file buffers to the client
    
    // send recent browser data to the client
    n = written(out_fd,buffer,strlen(buffer));
    if(n > 0){
        printf("Bytes written successfully %d\n", n);
        fflush(stdout);
    }else{
        printf("Error in writting to buffer\n");
        fflush(stdout);
    }
}

// utility function to get the MIME (Multipurpose Internet Mail Extensions) type
static const char* get_mime_type(char *filename){
    char *dot = strrchr(filename, '.');
    if(dot){ // strrchar Locate last occurrence of character in string
        mime_map *map = meme_types;
        while(map->extension){
            if(strcmp(map->extension, dot) == 0){
                return map->mime_type;
            }
            map++;
        }
    }
    return default_mime_type;
}

// open a listening socket descriptor using the specified port number.
int open_listenfd(int port){
    server_port_number = port;
    serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *) &serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(server_port_number);
    if (bind(serverSocketFd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0){
        char buffer[256];
        sprintf(buffer,"Error in binding to port %d \n",port); 
        showErrorAndExit(buffer);
    }
    listen(serverSocketFd,MAX_CLIENTS);
    printf("Started listening at port %d for http requests \n",server_port_number);

    // create a socket descriptor

    // eliminate "Address already in use" error from bind.

    // 6 is TCP's protocol number
    // enable this, much faster : 4000 req/s -> 17000 req/s

    // Listenfd will be an endpoint for all requests to port
    // on any IP address for this host

    // make it a listening socket ready to accept connection requests
}

// decode url
void url_decode(char* src, char* dest, int max) {

}

// parse request to get url
int parse_request(int clientSocketId, http_request *req){
    rio_t rio ;
    rio_readinitb(&rio,clientSocketId); 
    char buffer[MAXLINE] ;
    int totalRead = rio_readlineb(&rio,buffer,MAXLINE);
    char *token;
    int result = -1;
    if(totalRead > 0 && strstr(buffer,"GET") != NULL){
        printf("buffer is :- %s\n",buffer);
        /*GET / HTTP/1.1 */
        token = strtok(buffer," ");
        // GET 
        token = strtok(NULL," ");
        bzero(req->filename,MAXLINE);
        strcat(req->filename,workingDirectory);
        strcat(req->filename,token);
        bzero(buffer,MAXLINE);
        while((totalRead = rio_readlineb(&rio,buffer,MAXLINE)) > 0 && strcmp(buffer,"\r\n") != 0 && strcmp(buffer,"\n\n") != 0 ){
            if(strstr(buffer,"User-Agent") != NULL){
                fflush(stdout);
                if(strstr(buffer,"Chrome") != NULL){
                    printf("browser detected chrome\n");
                    req->browser_index = 0;
                }else if (strstr(buffer,"Safari") != NULL){
                    printf("browser detected safari\n");
                    req->browser_index = 1;
                }else if(strstr(buffer,"Firefox") != NULL){
                    printf("browser detected firefox\n");
                    req->browser_index = 2;
                }else{
                    printf("browser detected others\n");
                    req->browser_index = 3;
                }
                bzero(buffer,MAXLINE);
            }
        }
        printf("File name is %s\n",req->filename );
        //totalRead = rio_readlineb(&rio,buffer,MAXLINE);
        // GET request success
        result = 0;
    }
    printf("reading done :- \n");
    // Rio (Robust I/O) Buffered Input Functions
    
    // read all
    
    // update recent browser data
    
    // decode url
    return result;
    
}
int writeIpAddresses(){
    char fileName[MAXLINE] ;
    int result = -1;
    sprintf(fileName,"%s%s",workingDirectory,"ip_address.txt");
    FILE *file = fopen ( fileName, "w" );
    printf("Write ip addresses to %s \n", fileName);
    if(file!=NULL){
        result = 0;
        char line[256];
        for(int i = 0 ; i < MAX_POOL ; i++){
            if(ip_address_pool[i]!=NULL){
                fprintf (file, "%s",ip_address_pool[i]);
                //printf("ip addresses written %s",ip_address_pool[i]);
                free(ip_address_pool[i]);
            }
        }
        fclose (file);
    }else{
        printf("Error in update ip address log file\n");
    }
    printf("finish write ip address\n");
    return result;
}
int readIpAddresses(){
    printf("readIpAddresses\n");
    char fileName[MAXLINE] ;
    int result = -1;
    sprintf(fileName,"%s%s",workingDirectory,"ip_address.txt");
    FILE *file = fopen (fileName,"r");
    for(int i = 0 ; i < MAX_POOL ; i++){
        free(ip_address_pool[i]);
    }
    printf("Reading ip addresses from  %s \n", fileName);
    if(file != NULL){
        result = 0;
        size_t maxLine = 256;
        char line[128];
        ipAddressCount = 0;
        int read = -1;
        while (fgets(line, 255, (FILE*)file) > 0 &&  ipAddressCount < MAX_POOL)
        {   
            //printf("ip addresses found %s",line);
            ip_address_pool[ipAddressCount] = (char *)malloc(sizeof(line));
            sprintf(ip_address_pool[ipAddressCount],"%s",line); 
            //printf("ip addresses at %d = %s",ipAddressCount,ip_address_pool[ipAddressCount]);
            ipAddressCount++;
            fflush(stdout);
        }
        fclose(file);
    }else{
        printf("Error in update ip address log file\n");
    }
    printf("finish read ip address\n");
    return result;
}
int readBrowserLog(){
    printf("readBrowserLog\n");
    fflush(stdout);
    char fileName[MAXLINE] ;
    int result = -1;
    sprintf(fileName,"%s%s",workingDirectory,"recent_browser.txt");
    FILE *file = fopen (fileName,"r");
    for(int i = 0 ; i < MAX_POOL ; i++){
        free(browser_pool[i]);
    }
    printf("Reading browsers from  %s \n", fileName);
    if(file != NULL){
        result = 0;
        size_t maxLine = 256;
        char line[128];
        browserCount = 0;
        int read = -1;
        while (fgets(line, 255, (FILE*)file) > 0 &&  browserCount < MAX_POOL)
        {   
            printf("browser found %s",line);
            fflush(stdout);
            browser_pool[browserCount] = (char *)malloc(sizeof(line));
            sprintf(browser_pool[browserCount],"%s",line); 
            printf("browserat %d = %s",browserCount,browser_pool[browserCount]);
            fflush(stdout);
            browserCount++;
        }
        fclose(file);
    }else{
        printf("Error in update browser log file\n");
    }
    printf("finish readBrowserLog\n");
    fflush(stdout);
    return result;
}
int writeBrowserLog(){
    char fileName[MAXLINE] ;
    int result = -1;
    sprintf(fileName,"%s%s",workingDirectory,"recent_browser.txt");
    FILE *file = fopen ( fileName, "w" );
    printf("Write browser addresses to %s \n", fileName);
    fflush(stdout);
    if(file!=NULL){
        result = 0;
        char line[256];
        for(int i = 0 ; i < MAX_POOL ; i++){
            if(browser_pool[i]!=NULL){
                fprintf (file, "%s",browser_pool[i]);
                printf("browser written %s",browser_pool[i]);
                fflush(stdout);
                free(browser_pool[i]);
            }else{
                printf("browser not written ");
            }
        }
        fclose (file);
    }else{
        printf("Error in update ip address log file\n");
    }
    printf("finish write ip address\n");
    return result;
}
void logBrowserData(http_request *req){
    printf("logBrowserData\n");
    if(readBrowserLog() == 0){
        printf("total count = %d\n", browserCount);
        if(browserCount >= MAX_POOL){
            browserCount = 0;
            free(browser_pool[browserCount]); // free the last item in list
            for(int i = 0 ; i < MAX_POOL - 1 ; i++){
                browser_pool[i] = browser_pool[i+1];
            }
            browserCount = MAX_POOL - 1 ;
        }else{
            browserCount++;
        }
        printf("will update browser count  = %d and value swapped to %d \n", browserCount,req->browser_index);
        fflush(stdout);
        browser_pool[browserCount] =  (char *)malloc(sizeof(browser_types[req->browser_index]));
        sprintf(browser_pool[browserCount],"%s\n",browser_types[req->browser_index]); 
        printf("updated count = %d and value = %s\n", browserCount,browser_pool[browserCount]);
        fflush(stdout);
        writeBrowserLog();
    }
    printf("logBrowserData finished\n");
    fflush(stdout);
}
void logIpAddress(struct sockaddr_in *c_addr){
    printf("logIpAddress\n");
    char *clientIpAddress = inet_ntoa(c_addr->sin_addr);
    if(readIpAddresses() == 0){
        printf("total count = %d\n", ipAddressCount);
        if(ipAddressCount >= MAX_POOL){
            ipAddressCount = 0;
            free(ip_address_pool[ipAddressCount]); // free the last item in list
            for(int i = 0 ; i < MAX_POOL - 1 ; i++){
                ip_address_pool[i] = ip_address_pool[i+1];
            }
            ipAddressCount = MAX_POOL - 1 ;
        }else{
            ipAddressCount++;
        }
        printf("will update count  = %d and value swapped to %s \n", ipAddressCount,clientIpAddress);
        ip_address_pool[ipAddressCount] =  (char *)malloc(sizeof(clientIpAddress));
        sprintf(ip_address_pool[ipAddressCount],"%s\n",clientIpAddress); 
        printf("updated count = %d and value = %s\n", ipAddressCount,ip_address_pool[ipAddressCount]);
        writeIpAddresses();
    }
    printf("logIpAddress finished\n");
    fflush(stdout);
}

// log files
void log_access(int status, struct sockaddr_in *c_addr, http_request *req){
    printf("log_access\n");
    logIpAddress(c_addr);
    logBrowserData(req);
    printf("FInish log access\n");
}

// echo client error e.g. 404
void client_error(int clientSocketFd, int status, char *msg, char *longmsg){
    printf("Printing client error html %d %s %s\n",status,msg,longmsg );
    char buffer[MAXLINE], body[MAXLINE];
    bzero(buffer,MAXLINE);
    bzero(body,MAXLINE);
    /* Build the HTTP error response body */
    sprintf(body, "<html><head><style>body{font-family: monospace; font-size: 13px;}td {padding: 1.5px 6px;}</style><title>Server Error</title></head>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s %d: %s\r\n", body, status, msg);
    sprintf(body, "%s<p>%s: %s\r\n", body, msg, longmsg);
    sprintf(body, "%s<hr><em>Error page for Mini Web server</em>\r\n", body);
    sprintf(body, "%s</body></html>\r\n", body);

    /* Print the HTTP response */
    sprintf(buffer, "HTTP/1.0 %d ERROR\n\n", status);
    written(clientSocketFd, buffer, strlen(buffer));
    written(clientSocketFd, body, strlen(body));
    printf("%s\n",buffer );
    printf("%s\n",body );
}

// serve static content
void serve_static(int out_fd, int in_fd, http_request *req, size_t total_size){
    // send response headers to client e.g., "HTTP/1.1 200 OK\r\n"
    printf("serve_static request size = %d \n", (int)total_size);
    char *srcp ;
    char *success = "HTTP/1.0 200 OK\n\n" ;
    int n = written(out_fd,success,strlen(success));
    char buffer[MAXLINE] ;
    int totalRead ;
    rio_t rio ;
    rio_readinitb(&rio,in_fd);
    while( (totalRead = rio_read(&rio,buffer,MAXLINE)) > 0){
        written(out_fd,buffer,totalRead);
        bzero(buffer,totalRead);
    }
}
// handle one HTTP request/response transaction
void process(int clientSocketId, struct sockaddr_in *clientaddr){
    http_request req;
    char *clientIpAddress = inet_ntoa(clientaddr->sin_addr);
    printf("accept request, clientSocketId is %d, pid is %d and ip address is %s \n", clientSocketId, getpid(),clientIpAddress);
    int result = parse_request(clientSocketId, &req);
    int status = 200;
    if(result >= 0){
        struct stat sbuf;
        //server status init as 200
        int ffd = open(req.filename, O_RDONLY, 0);
        if(ffd <= 0){
            status = 404;
            client_error(clientSocketId,status,"Page not found","The webpage you requested is not found in the server");
            printf("detect %d error and print error log\n",status);
        } else {
            // get descriptor status
            fstat(ffd, &sbuf);
            if(S_ISREG(sbuf.st_mode)){
                printf("is FILE = %s \n", req.filename );
                serve_static(clientSocketId,ffd,&req,sbuf.st_size);
                // server serves static content
                
            } else if(S_ISDIR(sbuf.st_mode)){
                printf("is DIR = %s \n", req.filename );
                // server handle directory request
                log_access(status, clientaddr, &req);
                handle_directory_request(clientSocketId,ffd, req.filename);
                fflush(stdout);
                printf("handle directory request finshed\n");
               // 

            } else {
                status = 404;
                client_error(clientSocketId,status,"Page not found","The webpage you requested is not found in the server");
                printf("detect %d error and print error log\n",status);
            }
            close(ffd);
        }
    }else{
        status = 405;
        client_error(clientSocketId,status,"Method not supported"," This mini server only supports GET method for http communication");
        // detect 404 error and print error log
        printf("detect %d error and print error log\n",status);
    }
    // print log/status on the terminal
    printf("finish process request\n");
}

int checkAndUpdateUserInput(int argc, char** argv){
    int result = ERROR;
    int default_port = 9999;
    if(argc == 1){
        showErrorAndExit("Working directory is required and port number is optional\n");
        return result;
    }        
    if(argc == 2){
        printf("Setting default port number to %d\n",default_port);
        workingDirectory = argv[1];
        server_port_number = default_port;
        result = OK;
        return result;
    }
    if(argc == 3){
        workingDirectory = argv[1];
        server_port_number = atoi(argv[2]);
        result = OK;
        return result;
    }
}
// main function:
// get the user input for the file directory and port number
int main(int argc, char** argv){
    struct sockaddr_in clientAddress;
    int client = sizeof(clientAddress);
    int clientSocketFd;
    int clientPID;
    int listenfd, connfd;
    char buf[256];
    checkAndUpdateUserInput(argc,argv);
    printf("Server working directory is %s\n", workingDirectory );
    printf("Server listening port is %d\n", server_port_number );
    // get the name of the current working directory
    // user input checking

    
    // ignore SIGPIPE signal, so if browser cancels the request, it
    // won't kill the whole process.
    signal(SIGPIPE, SIG_IGN);
    open_listenfd(server_port_number);
    while(1){
        // permit an incoming connection attempt on a socket.
        clientSocketFd = accept(serverSocketFd, (struct sockaddr *) &clientAddress, &client);
        if(clientSocketFd >= 0){
            // fork children to handle parallel clients
            // handle one HTTP request/response transaction
            clientPID = fork();
            if (clientPID < 0){
                char buffer[256];
                sprintf(buffer,"Error in binding to port %d \n",clientSocketFd); 
                showErrorAndExit(buffer);
            }
            if (clientPID == 0)  {
                // child code
                close(serverSocketFd);
                process(clientSocketFd,&clientAddress);
                printf("process finished\n");
                exit(0);
            }
            else {
               printf("closing client %d \n",clientSocketFd);
               close(clientSocketFd);
            }
        }else{
            printf("Error in accepting client \n");
        }
  
    }

    return 0;
}