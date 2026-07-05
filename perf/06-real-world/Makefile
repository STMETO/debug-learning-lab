CFLAGS = -g -Wall -Wextra -fno-omit-frame-pointer

analyzer: analyzer.c
	$(CC) $(CFLAGS) -o analyzer analyzer.c

.PHONY: clean
clean:
	rm -f analyzer perf.data perf.data.old
