# -fsanitize=address 是开启 ASan 的关键参数
# -g 生成调试符号，让 ASan 报告能显示行号
# -fno-omit-frame-pointer 让调用栈更完整
CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer

hello: hello.c
	$(CC) $(CFLAGS) -o hello hello.c

.PHONY: clean
clean:
	rm -f hello
