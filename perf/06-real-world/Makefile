CFLAGS = -g -Wall -Wextra

analyzer: analyzer.c
	$(CC) $(CFLAGS) -o analyzer analyzer.c

.PHONY: clean
clean:
	rm -f analyzer perf.data perf.data.old
