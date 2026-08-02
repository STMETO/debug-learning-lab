CC := gcc
CPPFLAGS := -Iinclude
CFLAGS := -g -Wall -Wextra -MMD -MP
BUILD := build
APP := $(BUILD)/app
TEST := $(BUILD)/test_calc
COMMON := $(BUILD)/calc.o
DEPS := $(BUILD)/main.d $(BUILD)/calc.d $(BUILD)/test_calc.d

.DEFAULT_GOAL := all
.PHONY: all run test clean help
all: $(APP)
$(BUILD):
	mkdir -p $@
$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
$(BUILD)/test_calc.o: tests/test_calc.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
$(APP): $(BUILD)/main.o $(COMMON)
	$(CC) -o $@ $^
$(TEST): $(BUILD)/test_calc.o $(COMMON)
	$(CC) -o $@ $^
run: $(APP)
	./$(APP)
test: $(TEST)
	./$(TEST)
clean:
	rm -rf $(BUILD)
help:
	@echo "make | make run | make test | make clean"
-include $(DEPS)
