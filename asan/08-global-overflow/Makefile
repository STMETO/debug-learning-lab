CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

global: global.c
	$(CC) $(CFLAGS) -o global global.c

.PHONY: clean
clean:
	rm -f global
