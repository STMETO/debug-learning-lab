CC := gcc
CFLAGS := -g -O0 -Wall -Wextra

symbols: main.o symbols.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f symbols *.o
