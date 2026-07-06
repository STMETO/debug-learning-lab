CFLAGS = -g -Wall -Wextra -fno-omit-frame-pointer

ui_demo: ui_demo.c
	$(CC) $(CFLAGS) -o ui_demo ui_demo.c

.PHONY: clean
clean:
	rm -f ui_demo *.perfetto-trace
