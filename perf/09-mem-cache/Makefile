CFLAGS = -g -Wall -Wextra -pthread

cache_demo: cache_demo.c
	$(CC) $(CFLAGS) -o cache_demo cache_demo.c

.PHONY: clean
clean:
	rm -f cache_demo perf.data*
