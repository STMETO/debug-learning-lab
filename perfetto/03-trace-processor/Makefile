CFLAGS = -g -Wall -Wextra

sql_demo: sql_demo.c
	$(CC) $(CFLAGS) -o sql_demo sql_demo.c

.PHONY: clean
clean:
	rm -f sql_demo *.perfetto-trace
