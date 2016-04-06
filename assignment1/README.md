Assignment 1: 
Reverse-Engineering Sockets: a Puzzle
In this lab, we will build on your skills of sniffing packets using Wireshark to reverse-engineer a protocol that uses sockets. After this lab, you will gain additional skills of socket programming, and you will get some insights into reverse-engineering a protocol - this is all cool! We will learn the important skill of distributed programming using socket programming in C and the socket API. A socket is similar to the pipe abstraction, not between two processes on the same machine, but between process on different machines on the Internet.

You have been tasked to rewrite an outdated sensor network system deployed at the Three Mile Island nuclear reactor. This particular sensor network is so outdated that the source code has since gone missing. But there is hope! Because this sensor network is still operational, we can give you binary executables that the staff uses to monitor sensors. Your task is to reverse-engineer the protocol these two components speak, and rewrite the client in C.

Luckily, we have executables for two operating systems. You can download them below using "save as a file” option. Once you are done, make sure you have the right file permissions to execute the file; try“ chmod +x client” under Unix. Here are the binaries for Mac and Linux machines:

Intel Mac OS X Executable
Linux Executable
If you have any problems, contact the TAs - when you run the program, you will see a prompt as follows:


          WELCOME TO THE THREE MILE ISLAND SENSOR NETWORK


          Which sensor would you like to read:

            (1) Water temperature
            (2) Reactor temperature
            (3) Power level

          Selection: 
          
Your task is to run Wireshark at the same time and watch the packets going from your computer to the server(s) the executable is connecting and talking to. Sometimes it helps if you stop all other Internet-related activity, because it can be difficult to tell what the program is doing amongst a whole slew of other traffic. You will want to capture all the packets associated with each option since you are trying to faithfully reproduce the source code of the given executable. Note that if you are unable to run the executable, contact the TAs immediately and we will see what we can do. If worse comes to worse, the TAs can provide you with a packet trace of the executable that you can use to reverse-engineer the protocol. It is important that you reverse-engineer the protocol before coding your client. If you have trouble understanding or need clarifications, please ask the TAs.

Once you think you fully understand the protocol (it might be useful to draw a diagram to outline the messages that need to be sent), start coding your C client. Your C client should be able to connect to the appropriate server(s), retrieve the sensor reading, and output this to the user.

Extra hacker credit: After you have reverse-engineered the client and have it successfully working with the existing servers, then write the server side too. You will have to use a different port. Submit the client and server.

Coding style: Please put comments in your code to make it more readable.

The final software should include the following components:

  1 A Wireshark dump of the packets you analyzed to reverse-engineer the protocol;
  2 The reverse-engineered client;
  3 Some example outputs of your program.

Again, you should also provide a Makefile to build your program (one for the client and one for the server) and a concise README file describing briefly your design choices and how to run the program.
