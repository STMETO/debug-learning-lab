CC := gcc
CFLAGS := -g -Wall -Wextra
all: process_demo
process_demo: worker.c
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: clean
clean:
	rm -f process_demo
