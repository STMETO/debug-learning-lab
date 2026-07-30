CC := gcc
CFLAGS := -g -O0 -Wall -Wextra
all: resource_demo
resource_demo: busy.c
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: clean
clean:
	rm -f resource_demo
