CFLAGS = -g -Wall -Wextra -pthread
LDFLAGS = -pthread

threads: threads.c
	$(CC) $(CFLAGS) -o threads threads.c $(LDFLAGS)

.PHONY: clean
clean:
	rm -f threads
