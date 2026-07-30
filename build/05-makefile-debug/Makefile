CC       := gcc
WARNINGS := -Wall -Wextra -Wpedantic
BUILD    := build

.PHONY: all debug release asan run-debug run-release run-asan clean

all: debug

debug: $(BUILD)/debug/buggy
release: $(BUILD)/release/buggy
asan: $(BUILD)/asan/buggy

$(BUILD)/debug $(BUILD)/release $(BUILD)/asan:
	mkdir -p $@

$(BUILD)/debug/buggy: buggy.c | $(BUILD)/debug
	$(CC) $(WARNINGS) -g -O0 -o $@ $<

$(BUILD)/release/buggy: buggy.c | $(BUILD)/release
	$(CC) $(WARNINGS) -O2 -DNDEBUG -o $@ $<

$(BUILD)/asan/buggy: buggy.c | $(BUILD)/asan
	$(CC) $(WARNINGS) -g -O1 -fsanitize=address -fno-omit-frame-pointer -o $@ $<

run-debug: debug
	./$(BUILD)/debug/buggy

run-release: release
	./$(BUILD)/release/buggy

run-asan: asan
	./$(BUILD)/asan/buggy

clean:
	rm -rf $(BUILD)
