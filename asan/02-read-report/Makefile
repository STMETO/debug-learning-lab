CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

report: report.c
	$(CC) $(CFLAGS) -o report report.c

.PHONY: clean
clean:
	rm -f report
