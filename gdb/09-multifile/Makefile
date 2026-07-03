CC = gcc
CFLAGS = -g -Wall -Wextra

# 多文件编译：每个 .c 编译为 .o，再链接
app: main.o math_utils.o string_utils.o
	$(CC) $(CFLAGS) -o app main.o math_utils.o string_utils.o

main.o: main.c math_utils.h string_utils.h
	$(CC) $(CFLAGS) -c main.c

math_utils.o: math_utils.c math_utils.h
	$(CC) $(CFLAGS) -c math_utils.c

string_utils.o: string_utils.c string_utils.h
	$(CC) $(CFLAGS) -c string_utils.c

.PHONY: clean
clean:
	rm -f app *.o
