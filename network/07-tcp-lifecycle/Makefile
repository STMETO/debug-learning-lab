CC := gcc
all: tcp_demo
tcp_demo: server.c
	$(CC) -g -Wall -Wextra -o $@ $<
.PHONY: clean
clean:
	rm -f tcp_demo
