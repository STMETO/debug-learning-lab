CFLAGS = -g -Wall -Wextra -fno-omit-frame-pointer

hotspot: hotspot.c
	$(CC) $(CFLAGS) -o hotspot hotspot.c

.PHONY: clean
clean:
	rm -f hotspot perf.data*
