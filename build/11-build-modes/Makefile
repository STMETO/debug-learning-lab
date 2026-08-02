CC := gcc
WARNINGS := -Wall -Wextra -Wpedantic
MODE ?= debug
BUILD := build/$(MODE)

ifeq ($(MODE),debug)
  MODE_FLAGS := -g -O0
else ifeq ($(MODE),release)
  MODE_FLAGS := -O2 -DNDEBUG
else ifeq ($(MODE),asan)
  MODE_FLAGS := -g -O1 -fsanitize=address -fno-omit-frame-pointer
else
  $(error MODE must be debug, release, or asan)
endif

.PHONY: all run clean
all: $(BUILD)/buggy
$(BUILD):
	mkdir -p $@
$(BUILD)/buggy: buggy.c | $(BUILD)
	$(CC) $(WARNINGS) $(MODE_FLAGS) -o $@ $<
run: $(BUILD)/buggy
	./$(BUILD)/buggy
clean:
	rm -rf build
