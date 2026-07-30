CC := gcc
CFLAGS := -g -O0 -Wall -Wextra
BUILD := build
LIB := $(BUILD)/libhelper.a

.PHONY: all inspect wrong-order clean

all: $(BUILD)/app

$(BUILD):
	mkdir -p $@

$(BUILD)/main.o: main.c helper.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/helper.o: helper.c helper.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(BUILD)/helper.o
	ar rcs $@ $^

$(BUILD)/app: $(BUILD)/main.o $(LIB)
	$(CC) -o $@ $(BUILD)/main.o $(LIB)

inspect: $(LIB)
	ar t $(LIB)
	nm -s $(LIB)

wrong-order: $(BUILD)/main.o $(LIB)
	$(CC) -o $(BUILD)/wrong_order $(LIB) $(BUILD)/main.o

clean:
	rm -rf $(BUILD)
