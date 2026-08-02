CC := gcc
CFLAGS := -g -Wall -Wextra

app: main.o helper.o
	@echo "target=$@ prerequisites=$^ newer=$?"
	$(CC) -o $@ $^

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

helper.o: helper.c
	$(CC) $(CFLAGS) -c -o $@ $<
