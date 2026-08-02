# 实验 08：模式规则 —— `%` 如何消除重复规则

> 🎯 **目标**：理解 `%`、`wildcard`、`patsubst`，不要把它们当魔法。

实验 07 的两个编译规则结构相同：

```makefile
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
```

`%` 表示同一段文件名。匹配 `main.o` 时，`% = main`，依赖就是 `main.c`；匹配 `calc.o` 时，`% = calc`。

```makefile
SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))
```

- `wildcard` 查找匹配文件；
- `patsubst` 把每个 `.c` 名称转换成 `.o`。

先执行 `make print`，看清变量实际值，再执行 `make`。

> 自动收集源码很方便，但大型项目常显式列出源文件，避免新文件被意外加入目标。这里学习的是语法，不是规定唯一风格。

## 🧪 试试看

1. 新增一个 `.c` 文件，观察 `SOURCES`。
2. 在模式规则中打印 `$*`。
3. 把 `patsubst` 改成替换引用：`$(SOURCES:.c=.o)`。

下一步：[自动头文件依赖](../09-auto-dependencies/)。

