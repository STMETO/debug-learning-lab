CC := gcc
CFLAGS := -g -O2 -Wall -Wextra

config_demo: config.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f config_demo
