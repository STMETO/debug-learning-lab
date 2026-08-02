CC := gcc
CFLAGS := -g -Wall -Wextra

.DEFAULT_GOAL := all
.PHONY: all run clean help

all: app

app: main.c
	$(CC) $(CFLAGS) -o $@ $<

run: app
	./app

clean:
	rm -f app

help:
	@echo "make        build app"
	@echo "make run    build and run"
	@echo "make clean  remove generated files"
