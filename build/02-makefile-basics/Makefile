CC      := gcc
CFLAGS  := -g -Wall -Wextra
TARGET  := hello_make
SOURCE  := main.c

.PHONY: all run clean print-vars

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

print-vars:
	@echo "CC=$(CC)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "TARGET=$(TARGET)"

clean:
	rm -f $(TARGET)
