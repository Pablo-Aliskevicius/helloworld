CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -O2
LDFLAGS = -lrt

all: ipc

ipc: main.c
	$(CC) $(CFLAGS) -o ipc main.c $(LDFLAGS)

clean:
	rm -f ipc
