CC := gcc
CPPFLAGS := -Iinclude
CFLAGS := -g -Wall -Wextra -MMD -MP
BUILD := build
SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:src/%.c=$(BUILD)/%.o)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean
all: $(BUILD)/app
$(BUILD)/app: $(OBJECTS) | $(BUILD)
	$(CC) -o $@ $^
$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
$(BUILD):
	mkdir -p $@
clean:
	rm -rf $(BUILD)
-include $(DEPS)
