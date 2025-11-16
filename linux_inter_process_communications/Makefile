CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -O2
LDFLAGS = -lrt
VALGRINDFLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --error-limit=yes --fullpath-after=projects/helloworld/

all: ipc

ipc: main.c
	$(CC) $(CFLAGS) -o ipc main.c $(LDFLAGS)
	ls -l ipc

clean:
	rm -f ipc

run: ipc
	./ipc

plumber: ipc
	# see https://stackoverflow.com/questions/5134891/how-do-i-use-valgrind-to-find-memory-leaks
	valgrind $(VALGRINDFLAGS) --log-file=/tmp/valgrind-out.txt ./ipc
	cat /tmp/valgrind-out.txt
	rm -f /tmp/valgrind-out.txt