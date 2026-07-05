CFLAGS = -g -Wall -Wextra

trace_app: trace_app.c
	$(CC) $(CFLAGS) -o trace_app trace_app.c

.PHONY: clean
clean:
	rm -f trace_app *.perfetto-trace
