CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

leak: leak.c
	$(CC) $(CFLAGS) -o leak leak.c

.PHONY: clean
clean:
	rm -f leak
