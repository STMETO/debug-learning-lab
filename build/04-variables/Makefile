CC       := gcc
CFLAGS  := -g -Wall
CFLAGS  += -Wextra
TARGET  ?= hello
SOURCE   = hello.c

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

print:
	@echo "CC=$(CC)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "TARGET=$(TARGET)"
