CC := gcc
CPPFLAGS := -I.
CFLAGS := -g -Wall -Wextra
SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

.PHONY: all clean print
all: app

app: $(OBJECTS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

print:
	@echo "SOURCES=$(SOURCES)"
	@echo "OBJECTS=$(OBJECTS)"

clean:
	rm -f app $(OBJECTS)
