CC      := gcc
CFLAGS  := -g -Wall -Wextra

.PHONY: all stages clean

all: hello

hello: hello.o
	$(CC) $(CFLAGS) -o $@ $^

hello.o: hello.s
	$(CC) $(CFLAGS) -c -o $@ $<

hello.s: hello.i
	$(CC) $(CFLAGS) -S -o $@ $<

hello.i: hello.c
	$(CC) -E -o $@ $<

stages: hello.i hello.s hello.o hello

clean:
	rm -f hello hello.i hello.s hello.o
