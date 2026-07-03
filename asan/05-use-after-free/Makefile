CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

uaf: uaf.c
	$(CC) $(CFLAGS) -o uaf uaf.c

.PHONY: clean
clean:
	rm -f uaf
