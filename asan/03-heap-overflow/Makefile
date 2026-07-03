CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

heap: heap.c
	$(CC) $(CFLAGS) -o heap heap.c

.PHONY: clean
clean:
	rm -f heap
