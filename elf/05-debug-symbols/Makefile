CC := gcc
WARN := -Wall -Wextra

debug: demo.c
	$(CC) $(WARN) -g -O0 -o $@ $<

release: demo.c
	$(CC) $(WARN) -O2 -o $@ $<

stripped: debug
	cp debug stripped
	strip --strip-debug stripped

.PHONY: clean
clean:
	rm -f debug release stripped
