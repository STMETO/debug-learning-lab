CC := gcc
WARN := -Wall -Wextra

debug: calc.c
	$(CC) $(WARN) -g -O0 -o $@ $<

release: calc.c
	$(CC) $(WARN) -g -O2 -o $@ $<

.PHONY: clean
clean:
	rm -f debug release
