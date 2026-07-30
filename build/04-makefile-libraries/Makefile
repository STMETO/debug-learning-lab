CC       := gcc
CPPFLAGS := -Iinclude
CFLAGS   := -g -Wall -Wextra -fPIC
BUILD    := build
OBJDIR   := $(BUILD)/obj
LIBDIR   := $(BUILD)/lib
BINDIR   := $(BUILD)/bin
LIBOBJ   := $(OBJDIR)/mathlib.o
MAINOBJ  := $(OBJDIR)/main.o
STATIC   := $(LIBDIR)/libmathlab.a
SHARED   := $(LIBDIR)/libmathlab.so

.PHONY: all static shared run-static run-shared inspect clean

all: static shared

static: $(BINDIR)/app_static
shared: $(BINDIR)/app_shared

$(OBJDIR) $(LIBDIR) $(BINDIR):
	mkdir -p $@

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(STATIC): $(LIBOBJ) | $(LIBDIR)
	ar rcs $@ $^

$(SHARED): $(LIBOBJ) | $(LIBDIR)
	$(CC) -shared -o $@ $^

$(BINDIR)/app_static: $(MAINOBJ) $(STATIC) | $(BINDIR)
	$(CC) -o $@ $(MAINOBJ) $(STATIC)

$(BINDIR)/app_shared: $(MAINOBJ) $(SHARED) | $(BINDIR)
	$(CC) -o $@ $(MAINOBJ) -L$(LIBDIR) -lmathlab -Wl,-rpath,'$$ORIGIN/../lib'

run-static: $(BINDIR)/app_static
	./$(BINDIR)/app_static

run-shared: $(BINDIR)/app_shared
	./$(BINDIR)/app_shared

inspect: all
	@echo "== static archive =="; ar t $(STATIC)
	@echo "== shared dependencies =="; ldd $(BINDIR)/app_shared

clean:
	rm -rf $(BUILD)
