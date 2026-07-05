CFLAGS = -g -Wall -Wextra -fno-omit-frame-pointer

cpu_demo: cpu_demo.c
	$(CC) $(CFLAGS) -o cpu_demo cpu_demo.c

.PHONY: clean
clean:
	rm -f cpu_demo *.perfetto-trace
