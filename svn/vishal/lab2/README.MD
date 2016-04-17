Root folders has following directory structure
	<root>
		Makefile
		README.MD
		code
			welcome.txt
			favicon.ico
			recent_browser.txt
			file_browser.c
			test.html
			ip_address.txt
1. Open terminal and navigate to root folder. cd <path-to-root>
2. type make
3. above command will generate a object file called browser
4. type ./browser ./code/ 56783 in terminal (1st argument is working directory and 2nd argument is an optional port number)
5. open browser and open this url http://<ipaddress-of-your-machine>:<portno>/ for ex :- http://localhost:56783/
