CFLAGS = -g -Wall -Wextra

workload: workload.c
	$(CC) $(CFLAGS) -o workload workload.c

.PHONY: clean
clean:
	rm -f workload perf.data*
