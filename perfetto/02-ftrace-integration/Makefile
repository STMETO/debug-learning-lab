CFLAGS = -g -Wall -Wextra

ftrace_demo: ftrace_demo.c
	$(CC) $(CFLAGS) -o ftrace_demo ftrace_demo.c

.PHONY: clean
clean:
	rm -f ftrace_demo *.perfetto-trace
