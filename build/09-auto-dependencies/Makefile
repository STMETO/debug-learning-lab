CC := gcc
CFLAGS := -g -Wall -Wextra -MMD -MP
SOURCES := main.c calc.c
OBJECTS := $(SOURCES:.c=.o)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean
all: app
app: $(OBJECTS)
	$(CC) -o $@ $^
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
clean:
	rm -f app $(OBJECTS) $(DEPS)

-include $(DEPS)
