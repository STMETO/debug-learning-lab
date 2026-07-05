CFLAGS = -g -Wall -Wextra

hello: hello.c
	$(CC) $(CFLAGS) -o hello hello.c

.PHONY: clean
clean:
	rm -f hello *.perfetto-trace
