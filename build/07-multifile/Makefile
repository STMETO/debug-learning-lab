CC := gcc
CPPFLAGS := -I.
CFLAGS := -g -Wall -Wextra

.PHONY: all run clean
all: app

app: main.o calc.o
	$(CC) -o $@ $^

main.o: main.c calc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

calc.o: calc.c calc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

run: app
	./app

clean:
	rm -f app *.o
