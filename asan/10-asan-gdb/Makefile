CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

asan_gdb: asan_gdb.c
	$(CC) $(CFLAGS) -o asan_gdb asan_gdb.c

.PHONY: clean
clean:
	rm -f asan_gdb core*
