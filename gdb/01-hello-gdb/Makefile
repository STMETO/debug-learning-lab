# 注意 -g 参数：它让编译器生成"调试符号"，GDB 靠它才能看到源码和变量名
# 没有 -g，GDB 里只能看到汇编指令和十六进制地址
CFLAGS = -g -Wall -Wextra

hello: hello.c
	$(CC) $(CFLAGS) -o hello hello.c

.PHONY: clean
clean:
	rm -f hello
