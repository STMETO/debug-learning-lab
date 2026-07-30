CC := gcc
CFLAGS := -g -Wall -Wextra

hello: hello.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f hello
