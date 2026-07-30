CC := gcc
all: counter
counter: counter.c
	$(CC) -g -Wall -Wextra -o $@ $<
.PHONY: clean
clean:
	rm -f counter status.txt
