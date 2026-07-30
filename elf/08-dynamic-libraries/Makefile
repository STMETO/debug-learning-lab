CC := gcc
CFLAGS := -g -O0 -Wall -Wextra -fPIC
BUILD := build
LIBDIR := $(BUILD)/lib
BINDIR := $(BUILD)/bin

.PHONY: all inspect run clean

all: $(BINDIR)/app

$(LIBDIR) $(BINDIR):
	mkdir -p $@

$(LIBDIR)/libgreeting.so: greeting.c greeting.h | $(LIBDIR)
	$(CC) $(CFLAGS) -shared -o $@ greeting.c

$(BINDIR)/app: main.c greeting.h $(LIBDIR)/libgreeting.so | $(BINDIR)
	$(CC) -g -o $@ main.c -L$(LIBDIR) -lgreeting -Wl,-rpath,'$$ORIGIN/../lib'

inspect: $(BINDIR)/app
	readelf -d $(BINDIR)/app | grep -E 'NEEDED|RPATH|RUNPATH'
	ldd $(BINDIR)/app

run: $(BINDIR)/app
	./$(BINDIR)/app

clean:
	rm -rf $(BUILD)
