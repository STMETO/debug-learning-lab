CC := gcc
CFLAGS := -g -Wall -Wextra
.PHONY: all clean
all: make_app
make_app: main.c
	$(CC) $(CFLAGS) -o $@ $<
clean:
	rm -f make_app
