CC := gcc
CFLAGS := -g -Wall -Wextra
all: proc_demo
proc_demo: inspect.c
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: clean
clean:
	rm -f proc_demo proc_demo.log
