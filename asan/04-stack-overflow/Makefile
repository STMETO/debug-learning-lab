CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

stack: stack.c
	$(CC) $(CFLAGS) -o stack stack.c

.PHONY: clean
clean:
	rm -f stack
