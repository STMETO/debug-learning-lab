CFLAGS = -g -Wall -Wextra -fno-omit-frame-pointer

workload: workload.c
	$(CC) $(CFLAGS) -o workload workload.c

.PHONY: clean
clean:
	rm -f workload perf.data*
