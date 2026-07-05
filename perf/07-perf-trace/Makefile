CFLAGS = -g -Wall -Wextra

syscalls: syscalls.c
	$(CC) $(CFLAGS) -o syscalls syscalls.c

.PHONY: clean
clean:
	rm -f syscalls perf.data*
