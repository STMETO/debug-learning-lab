CFLAGS = -g -Wall -Wextra
LDLIBS = -lm

decode: decode.c
	$(CC) $(CFLAGS) -o decode decode.c $(LDLIBS)

.PHONY: clean
clean:
	rm -f decode
