CC       := gcc
CPPFLAGS := -Iinclude
CFLAGS   := -g -Wall -Wextra -Wpedantic -MMD -MP
BUILD    := build/make
OBJECTS  := $(BUILD)/main.o $(BUILD)/stats.o
DEPS     := $(OBJECTS:.o=.d)
TARGET   := $(BUILD)/compare_demo

.PHONY: all run clean

all: $(TARGET)

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

-include $(DEPS)
