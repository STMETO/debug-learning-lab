CC := gcc
all: lsof_demo
lsof_demo: holder.c
	$(CC) -g -Wall -Wextra -o $@ $<
.PHONY: clean
clean:
	rm -f lsof_demo held.log
