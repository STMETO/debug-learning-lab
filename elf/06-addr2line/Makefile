CC := gcc
CFLAGS := -g -O0 -Wall -Wextra -no-pie

address_demo: crash.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f address_demo
