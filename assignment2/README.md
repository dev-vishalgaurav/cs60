A Mini Web Server

The goal of this project was to get familiar with the basic principles of web server and gain first-hand experiences of actually building one! -- a simplified mini version though ;-) This is a simple web server implemented inc to serve static content to clients using real web browsers such as Chrome and Firefox. It combines process control, Unix I/O, the sockets interface, etc., in around 600 lines of code.

Design Overview

This mini web sever can display files in a specified directory, where the client can open/download these files. Additionally, the web server also tracks client visit history and client information (web servers in the real world do that all the time, whether you like it or not). It will display the 10 most recent visits on the webpage, listing the IP address and the browser (e.g., Safari, Chrome, IE and Firefox) for each of these 10 visits. To get a better idea on how the web page looks like, here is a demo page.

Following are few design details :- 

The input checking function

Before launching the server, it needs to check proper user input arguments such as port number and the name of the working directory. Then, the server should be able to listen to the connection requests on the specific port specified in the input. To do so, it needs to open an infinite loop to periodically accept the connection requests from the clients, handle the corresponding HTTP transaction, and close the connection.

The open_listenfd function

It is helper function to wrap the socket and connect functions. First it uses the socket function to create a socket descriptor "sockfd". The bind function tells the kernel to associate the server's socket address with the socket descriptor "sockfd". The listen function converts "sockfd" from an active socket to a listening socket that can accept connection requests from clients. The second argument for the listen function is a hint about the number of outstanding connection requests that the kernel should queue up before it starts to refuse requests. We will typically set it to a large value, such as 9999.

The process function

The process function handles one HTTP transaction. First, it reads and parses the request lines to extract the method (e.g., GET), file directory (e.g., ‘./’), and browser name (e.g., Chrome). In the lab, we only support the GET method, and we send an error message to the client if it requests other methods (e.g., POST). The process function consists of two key components: parsing the client requests from URL and response to the client with proper content.

Parse request
Since HTTP is based on text lines transmitted over Internet connections, we provide for you the RIO (Robust Reading and Writing) package to acquire information from the HTTP request. Then, by detecting the key words (e.g., “Range” stands for the HTTP range request session) from the read buffer, we can extract the information we need. Here, we need to extract the file directory and the client browser name from the HTTP request.

Serve static content
The server serves 13 different types of static content, which are defined in the “meme_types”. First, the function sends the response line and response header (e.g., “HTTP/1.1 200 OL\r\n”) to the client. Next it sends the response body by copying the contents of the requested file to the connected descriptor out_fd.

Handle directory request
The function that handles file directory request from the client. Like the serve_static function, it first sends the response headers to client. Then it needs to read the directory and client browser data from the server machine. Finally, it parses the content into HTML texts in a buffer and send the buffer to the client.
