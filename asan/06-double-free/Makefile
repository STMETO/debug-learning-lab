CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

dfree: dfree.c
	$(CC) $(CFLAGS) -o dfree dfree.c

.PHONY: clean
clean:
	rm -f dfree
