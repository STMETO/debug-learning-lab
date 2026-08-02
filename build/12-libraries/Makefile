CC := gcc
CFLAGS := -g -Wall -Wextra -fPIC
BUILD := build

.PHONY: all run clean
all: $(BUILD)/app_static $(BUILD)/app_shared
$(BUILD):
	mkdir -p $@
$(BUILD)/mathlib.o: mathlib.c mathlib.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/main.o: main.c mathlib.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/libmathlab.a: $(BUILD)/mathlib.o
	ar rcs $@ $^
$(BUILD)/libmathlab.so: $(BUILD)/mathlib.o
	$(CC) -shared -o $@ $^
$(BUILD)/app_static: $(BUILD)/main.o $(BUILD)/libmathlab.a
	$(CC) -o $@ $(BUILD)/main.o $(BUILD)/libmathlab.a
$(BUILD)/app_shared: $(BUILD)/main.o $(BUILD)/libmathlab.so
	$(CC) -o $@ $(BUILD)/main.o -L$(BUILD) -lmathlab -Wl,-rpath,'$$ORIGIN'
run: all
	./$(BUILD)/app_static
	./$(BUILD)/app_shared
clean:
	rm -rf $(BUILD)
