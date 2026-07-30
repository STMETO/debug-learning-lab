CC       := gcc
CPPFLAGS := -Iinclude
WARNINGS := -Wall -Wextra -Wpedantic
BUILD    := build
MODE     ?= debug

ifeq ($(MODE),debug)
  OPTFLAGS := -g -O0
else ifeq ($(MODE),release)
  OPTFLAGS := -O2 -DNDEBUG
else ifeq ($(MODE),asan)
  OPTFLAGS := -g -O1 -fsanitize=address -fno-omit-frame-pointer
else
  $(error unknown MODE '$(MODE)'; use debug, release, or asan)
endif

CFLAGS      := $(WARNINGS) $(OPTFLAGS) -MMD -MP
OBJDIR      := $(BUILD)/$(MODE)/obj
BINDIR      := $(BUILD)/$(MODE)/bin
APP         := $(BINDIR)/parser_app
TEST        := $(BINDIR)/test_parser
APP_OBJECTS := $(OBJDIR)/main.o $(OBJDIR)/parser.o
TEST_OBJECTS:= $(OBJDIR)/test_parser.o $(OBJDIR)/parser.o
DEPS        := $(APP_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)

.PHONY: all run test debug release asan clean help

all: $(APP)

debug:
	$(MAKE) MODE=debug all

release:
	$(MAKE) MODE=release all

asan:
	$(MAKE) MODE=asan all

$(OBJDIR) $(BINDIR):
	mkdir -p $@

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/test_parser.o: tests/test_parser.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(APP): $(APP_OBJECTS) | $(BINDIR)
	$(CC) $(OPTFLAGS) -o $@ $^

$(TEST): $(TEST_OBJECTS) | $(BINDIR)
	$(CC) $(OPTFLAGS) -o $@ $^

run: $(APP)
	./$(APP) 42

test: $(TEST)
	./$(TEST)

clean:
	rm -rf $(BUILD)

help:
	@echo "make [MODE=debug|release|asan] [all|run|test|clean]"

-include $(DEPS)
