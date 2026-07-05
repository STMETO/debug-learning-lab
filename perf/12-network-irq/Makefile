CFLAGS = -g -Wall -Wextra -pthread

netdemo: netdemo.c
	$(CC) $(CFLAGS) -o netdemo netdemo.c

.PHONY: clean
clean:
	rm -f netdemo perf.data*
