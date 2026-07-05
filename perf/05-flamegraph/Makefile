CFLAGS = -g -Wall -Wextra -fno-omit-frame-pointer

calltree: calltree.c
	$(CC) $(CFLAGS) -o calltree calltree.c

.PHONY: clean
clean:
	rm -f calltree perf.data perf.data.old flamegraph.svg out.perf-script out.folded
