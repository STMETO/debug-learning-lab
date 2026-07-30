CC       := gcc
CPPFLAGS := -Iinclude
CFLAGS   := -g -Wall -Wextra -MMD -MP
BUILD    := build
TARGET   := $(BUILD)/dependency_demo
SOURCES  := $(wildcard src/*.c)
OBJECTS  := $(SOURCES:src/%.c=$(BUILD)/%.o)
DEPS     := $(OBJECTS:.o=.d)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BUILD)
	$(CC) -o $@ $^

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD)

-include $(DEPS)
