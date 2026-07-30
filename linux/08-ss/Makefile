CC := gcc
all: ss_demo
ss_demo: server.c
	$(CC) -g -Wall -Wextra -o $@ $<
.PHONY: clean
clean:
	rm -f ss_demo
