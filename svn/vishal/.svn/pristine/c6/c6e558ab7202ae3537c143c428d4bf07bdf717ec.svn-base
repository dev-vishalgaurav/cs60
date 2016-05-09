CC = c99
CFLAGS = -Wall -O2

# LIB = -lpthread

all: file_browser

file_browser: file_browser.c
	$(CC) $(CFLAGS) -o file_browser file_browser.c $(LIB)

clean:
	rm -f *.o file_browser *~
