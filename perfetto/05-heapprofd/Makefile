CFLAGS = -g -Wall -Wextra -fno-omit-frame-pointer

heap_demo: heap_demo.c
	$(CC) $(CFLAGS) -o heap_demo heap_demo.c

.PHONY: clean
clean:
	rm -f heap_demo *.perfetto-trace
