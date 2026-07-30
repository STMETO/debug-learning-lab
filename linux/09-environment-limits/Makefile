CC := gcc
all: env_demo
env_demo: env_demo.c
	$(CC) -g -Wall -Wextra -o $@ $<
.PHONY: clean
clean:
	rm -f env_demo
