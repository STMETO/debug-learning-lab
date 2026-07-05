CFLAGS = -g -Wall -Wextra

blockio: blockio.c
	$(CC) $(CFLAGS) -o blockio blockio.c

.PHONY: clean
clean:
	rm -f blockio perf.data* /tmp/perf_blockio_test.dat
