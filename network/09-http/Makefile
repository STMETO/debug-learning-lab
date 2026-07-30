CC := gcc
all: http_demo
http_demo: http.c
	$(CC) -g -Wall -Wextra -o $@ $<
.PHONY: clean
clean:
	rm -f http_demo
