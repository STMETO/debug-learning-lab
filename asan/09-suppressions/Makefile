CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

suppress: suppress.c
	$(CC) $(CFLAGS) -o suppress suppress.c

.PHONY: clean
clean:
	rm -f suppress
