/*
 * file_browser.c - A simple, iterative HTTP/1.0 Web server that uses the
 * GET method to serve static and dynamic content.
 */

#include <arpa/inet.h>          // inet_ntoa
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
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


#define LISTENQ  1024  // second argument to listen()
#define MAXLINE 1024   // max length of a line
#define RIO_BUFSIZE 1024
#define MAX_RECENT_FILES 10
#define TIME_OUT_SEC 10

void time_out_exit(int s){
    exit(3);
}
typedef struct {
    int rio_fd;                 // descriptor for this buf
    int rio_cnt;                // unread byte in this buf
    char *rio_bufptr;           // next unread byte in this buf
    char rio_buf[RIO_BUFSIZE];  // internal buffer
} rio_t;

// Simplifies calls to bind(), connect(), and accept()
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

//  sets up an empty read buffer and associates an open file descriptor with that buffer
void rio_readinitb(rio_t *rp, int fd){
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

// utility function for writing user buffer into file descriptor
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

    // Copy min(n, rp->rio_cnt) bytes from internal buf to user buf
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

//pre-process files in the "home" directory and send the list to the client
void handle_directory_request(int out_fd, int dir_fd){
    char buf[MAXLINE], m_time[32], size[16], buf_tmp1[MAXLINE];
    struct stat statbuf;
    
    //Send response headers to client e.g., "HTTP/1.1 200 OK\r\n"
    sprintf(buf, "HTTP/1.1 200 OK\r\n%s%s%s%s%s",
            "Content-Type: text/html\r\n\r\n",
            "<html><head><style>",
            "body{font-family: monospace; font-size: 13px;}",
            "td {padding: 1.5px 6px;}",
            "</style></head><body><table>\n");
    written(out_fd, buf, strlen(buf));
    
    //get file directory
    DIR *d = fdopendir(dir_fd);
    struct dirent *dp;
    int ffd;
    
    //read directory
    while ((dp = readdir(d)) != NULL){
        if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")){
            continue;
        }
        if ((ffd = openat(dir_fd, dp->d_name, O_RDONLY)) == -1){
            perror(dp->d_name);
            continue;
        }
        fstat(ffd, &statbuf);
        strftime(m_time, sizeof(m_time),
                 "%Y-%m-%d %H:%M", localtime(&statbuf.st_mtime));
        format_size(size, &statbuf);
        if(S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode)){
            char *d = S_ISDIR(statbuf.st_mode) ? "/" : "";
            sprintf(buf, "<tr><td><a href=\"%s%s\">%s%s</a></td><td>%s</td><td>%s</td></tr>\n",
                    dp->d_name, d, dp->d_name, d, m_time, size);
            written(out_fd, buf, strlen(buf));
        }
        close(ffd);
    }
    
    // send the file buffers to the client
    sprintf(buf, "</table>");
    written(out_fd, buf, strlen(buf));
    
    //send recent browser data to the client
    char buf_tmp[MAXLINE];
    char recent_browser[50];
    int read_lines = 0, browsers[MAX_RECENT_FILES];
    FILE *fp;
    sprintf(recent_browser,"./recent_browser.txt");
    fp = fopen(recent_browser, "ab+");
    for (int i = 0; i < MAX_RECENT_FILES; i++){
        browsers[i] = 0;
    }
    
    while(!feof(fp))
    {
        char ch = fgetc(fp);
        if(ch == '\n')
        {
            read_lines++;
        }else{
            browsers[read_lines] = ch - '0';
        }
    }
    fclose(fp);
    
    sprintf(buf_tmp, "<table><tr><td>The last 10 visited browsers:</td>");
    for (int i = read_lines-1; i >= 0; i--){
        if (browsers[i] == 1){
            sprintf(buf_tmp1, "<td>Chrome</td>");
            strcat(buf_tmp, buf_tmp1);
        }else if(browsers[i] == 2){
            sprintf(buf_tmp1, "<td>Safari</td>");
            strcat(buf_tmp, buf_tmp1);
        }else if(browsers[i] == 3){
            sprintf(buf_tmp1, "<td>Firefox</td>");
            strcat(buf_tmp, buf_tmp1);
        }else if(browsers[i] == 4){
            sprintf(buf_tmp1, "<td>IE</td>");
            strcat(buf_tmp, buf_tmp1);
        }
    }
    strcat(buf_tmp, "</tr>\n");
    sprintf(buf, "%s", buf_tmp);
    written(out_fd, buf, strlen(buf));
    
    //send recent ip address to the client
    char ip_address[50], ip_addr_stack[MAX_RECENT_FILES][MAXLINE];
    read_lines = 0;
    sprintf(ip_address,"./ip_address.txt");
    fp = fopen(ip_address, "ab+");
    
    int counter = 0;
    while(!feof(fp))
    {
        char ch = fgetc(fp);
        if(ch == '\n')
        {
            read_lines++;
            counter = 0;
        }else{
            ip_addr_stack[read_lines][counter++] = ch;
        }
    }
    fclose(fp);
    
    sprintf(buf_tmp, "<tr><td>The corresponding IP address:</td>");
    for (int i = read_lines-1; i >= 0; i--){
        sprintf(buf_tmp1, "<td>%s</td>", ip_addr_stack[i]);
        strcat(buf_tmp, buf_tmp1);
    }
    strcat(buf_tmp, "</table>\n");
    sprintf(buf, "%s", buf_tmp);
    written(out_fd, buf, strlen(buf));
    
    closedir(d);
}

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
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    // Create a socket descriptor
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    // Eliminates "Address already in use" error from bind.
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    // 6 is TCP's protocol number
    // enable this, much faster : 4000 req/s -> 17000 req/s
    if (setsockopt(listenfd, 6, TCP_CORK,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    // Listenfd will be an endpoint for all requests to port
    // on any IP address for this host
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    // Make it a listening socket ready to accept connection requests
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
}

void url_decode(char* src, char* dest, int max) {
    char *p = src;
    char code[3] = { 0 };
    while(*p && --max) {
        if(*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char)strtoul(code, NULL, 16);
            p += 2;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

void write_to_local_txt(int browser_index, char ip_addr[MAXLINE]){
    int read_lines;
    FILE *fp;
    // write recent browser to the txt
    char recent_browser[50];
    int browsers[MAX_RECENT_FILES];
    sprintf(recent_browser,"recent_browser.txt");
    fp = fopen(recent_browser, "ab+");
    
    if (fp == NULL)
    {
        printf("Error recent_browser.txt!\n");
        exit(1);
    }
    
    read_lines = 0;
    while(!feof(fp))
    {
        char ch = fgetc(fp);
        if(ch == '\n')
        {
            read_lines++;
        }else{
            if (read_lines < MAX_RECENT_FILES){
                browsers[read_lines] = ch - '0';
            }
        }
    }
    fclose(fp);
    
    if (read_lines < MAX_RECENT_FILES){
        browsers[read_lines] = browser_index;
    }else{
        for (int i = 1; i < MAX_RECENT_FILES; i++){
            browsers[i-1] = browsers[i];
        }
        browsers[MAX_RECENT_FILES - 1] = browser_index;
    }
    
    FILE *f = fopen("recent_browser.txt", "w");
    if (f == NULL)
    {
        printf("Error recent_browser.txt!\n");
        exit(1);
    }
    if (read_lines < MAX_RECENT_FILES){
        read_lines++;
    }
    for (int i = 0; i < read_lines; i++){
        fprintf(f, "%d\n", browsers[i]);
        
    }
    fclose(f);
    
    // write recent ip address to the txt
    char ip_address[50];
    char ip_addr_stack[MAX_RECENT_FILES][MAXLINE];
    sprintf(ip_address,"ip_address.txt");
    fp = fopen(ip_address, "ab+");
    
    if (fp == NULL)
    {
        printf("Error ip_address.txt!\n");
        exit(1);
    }
    
    read_lines = 0;
    int counter = 0;
    while(!feof(fp))
    {
        char ch = fgetc(fp);
        if(ch == '\n')
        {
            read_lines++;
            counter = 0;
        }else{
            if (read_lines < MAX_RECENT_FILES){
                ip_addr_stack[read_lines][counter++] = ch;
            }
            
        }
    }
    fclose(fp);
    
    if (read_lines < MAX_RECENT_FILES){
        sprintf(ip_addr_stack[read_lines], "%s", ip_addr);
    }else{
        for (int i = 1; i < MAX_RECENT_FILES; i++){
            sprintf(ip_addr_stack[i-1], "%s", ip_addr_stack[i]);
        }
        sprintf(ip_addr_stack[MAX_RECENT_FILES - 1], "%s", ip_addr);
    }
    
    f = fopen("ip_address.txt", "w");
    if (f == NULL)
    {
        printf("Error ip_address.txt!\n");
        exit(1);
    }
    if (read_lines < MAX_RECENT_FILES){
        read_lines++;
    }
    for (int i = 0; i < read_lines; i++){
        fprintf(f, "%s\n", ip_addr_stack[i]);
    }
    fclose(f);
}

void parse_request(int fd, http_request *req, struct sockaddr_in *clientaddr){
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE];
    req->offset = 0;
    req->end = 0;              /* default */
    
    // get ip address
    char ip_addr[MAXLINE];
    sprintf(ip_addr, "%s", inet_ntoa(clientaddr->sin_addr));

    // Rio (Robust I/O) Buffered Input Functions
    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s", method, uri);
    
    
    // read all
    while(buf[0] != '\n' && buf[1] != '\n') { /* \n || \r\n */
        rio_readlineb(&rio, buf, MAXLINE);
        if(buf[0] == 'R' && buf[1] == 'a' && buf[2] == 'n'){
            sscanf(buf, "Range: bytes=%lu-%lu", &req->offset, &req->end);
            // Range: [start, end]
            if( req->end != 0) req->end ++;
        }else if (buf[0] == 'U'){ //extract User-Agent
            req->browser_index = 0;
            if (strstr(buf, "Chrome")){
                req->browser_index = 1;
            }else if (strstr(buf, "Safari")){
                req->browser_index = 2;
            }else if (strstr(buf, "Firefox")){
                req->browser_index = 3;
            }else if (strstr(buf, "MSIE")){
                req->browser_index = 4;
            }
        }
    }
    
    //update recent browser data
    if (req->browser_index > 0){
        write_to_local_txt(req->browser_index, ip_addr);
    }
    
    char* filename = uri;
    if(uri[0] == '/'){
        filename = uri + 1;
        int length = strlen(filename);
        if (length == 0){
            filename = ".";
        } else {
            for (int i = 0; i < length; ++ i) {
                if (filename[i] == '?') {
                    filename[i] = '\0';
                    break;
                }
            }
        }
    }
    url_decode(filename, req->filename, MAXLINE);
}


void log_access(int status, struct sockaddr_in *c_addr, http_request *req){
    printf("%s:%d %d - %s\n", inet_ntoa(c_addr->sin_addr),
           ntohs(c_addr->sin_port), status, req->filename);
}

void client_error(int fd, int status, char *msg, char *longmsg){
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, msg);
    sprintf(buf + strlen(buf),
            "Content-length: %lu\r\n\r\n", strlen(longmsg));
    sprintf(buf + strlen(buf), "%s", longmsg);
    written(fd, buf, strlen(buf));
}

void serve_static(int out_fd, int in_fd, http_request *req,
                  size_t total_size){
    char buf[256];
    
    //Send response headers to client e.g., "HTTP/1.1 200 OK\r\n"
    if (req->offset > 0){
        sprintf(buf, "HTTP/1.1 206 Partial\r\n");
        sprintf(buf + strlen(buf), "Content-Range: bytes %lu-%lu/%lu\r\n",
                req->offset, req->end, total_size);
    } else {
        sprintf(buf, "HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\n");
    }
    sprintf(buf + strlen(buf), "Cache-Control: no-cache\r\n");

    sprintf(buf + strlen(buf), "Content-length: %lu\r\n",
            req->end - req->offset);
    sprintf(buf + strlen(buf), "Content-type: %s\r\n\r\n",
            get_mime_type(req->filename));
    written(out_fd, buf, strlen(buf));
    
    // Send response body to client
    off_t offset = req->offset; /* copy */
    while(offset < req->end){
        if(sendfile(out_fd, in_fd, &offset, req->end - req->offset) <= 0) {
            break;
        }
        printf("offset: %d \n\n", (int)offset);
        close(out_fd);
        break;
    }
}

// handle one HTTP request/response transaction
void process(int fd, struct sockaddr_in *clientaddr){
    printf("accept request, fd is %d, pid is %d\n", fd, getpid());
    http_request req;
    parse_request(fd, &req, clientaddr);

    struct stat sbuf;
    int status = 200; //server status init as 200
    int ffd = open(req.filename, O_RDONLY, 0);
    if(ffd <= 0){
        // detect 404 error and print error log
        status = 404;
        char *msg = "File not found";
        client_error(fd, status, "Not found", msg);
    } else {
        // get descriptor status
        fstat(ffd, &sbuf);
        if(S_ISREG(sbuf.st_mode)){
            //server serves static content
            if (req.end == 0){
                req.end = sbuf.st_size;
            }
            if (req.offset > 0){
                status = 206;
            }
            serve_static(fd, ffd, &req, sbuf.st_size);
        } else if(S_ISDIR(sbuf.st_mode)){
            //server handle directory request
            status = 200;
            handle_directory_request(fd, ffd);
        } else {
            // detect 400 error and print error log
            status = 400;
            char *msg = "Unknow Error";
            client_error(fd, status, "Error", msg);
        }
        close(ffd);
    }
    
    //print log/status on the terimal
    log_access(status, clientaddr, &req);
}

// main function:
// get the user input for the file directory and port number
int main(int argc, char** argv){
    struct sockaddr_in clientaddr;
	struct itimerval tout_val;
    int default_port = 9999,
        listenfd,
        connfd;
    
    // get the name of the current working directory
    socklen_t clientlen = sizeof clientaddr;
    
    //user input checking
    if(argc == 2) {
        if(argv[1][0] >= '0' && argv[1][0] <= '9') {
            default_port = atoi(argv[1]);
        } else {
            if(chdir(argv[1]) != 0) {
                perror(argv[1]);
                exit(1);
            }
        }
    }
    else if (argc == 3) {
        default_port = atoi(argv[2]);
        if(chdir(argv[1]) != 0) {
            perror(argv[1]);
            exit(1);
        }
    }else{
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = open_listenfd(default_port);
    if (listenfd > 0) {
        printf("listen on port %d, fd is %d\n", default_port, listenfd);
    } else {
        perror("ERROR");
        exit(listenfd);
    }
    
    // Ignore SIGPIPE signal, so if browser cancels the request, it
    // won't kill the whole process.
    signal(SIGPIPE, SIG_IGN);
    
    // Reap zombie children processes
    signal(SIGCHLD, SIG_IGN);
    
    while (1){
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        if (fork () == 0){
            
            close(listenfd);
            tout_val.it_interval.tv_sec = 0;
            tout_val.it_interval.tv_usec = 0;
            tout_val.it_value.tv_sec = TIME_OUT_SEC;
            tout_val.it_value.tv_usec = 0;
            setitimer(ITIMER_REAL, &tout_val,0);
            
            signal(SIGALRM, time_out_exit); /* set the Alarm signal capture */
            process(connfd, &clientaddr);
            close(connfd);
            exit(0);
        }
        close(connfd);
    }

    return 0;
}
