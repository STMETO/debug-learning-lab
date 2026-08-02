# 实验 04：变量 —— 赋值、引用与命令行覆盖

> 🎯 **目标**：掌握 `$(NAME)` 和 `= := ?= +=`。

```makefile
CC      := gcc
CFLAGS := -g -Wall
TARGET := hello
```

引用变量：

```makefile
	$(CC) $(CFLAGS) hello.c -o $(TARGET)
```

| 写法 | 含义 |
|------|------|
| `A = value` | 使用时再展开右侧 |
| `A := value` | 定义时立即展开 |
| `A ?= value` | 尚未定义时才赋值 |
| `A += value` | 追加内容 |

初学项目优先用 `:=`，行为更直观。`?=` 适合可覆盖的默认值。

```bash
make print
make TARGET=app
make CFLAGS='-g -O2 -Wall -Wextra'
```

常见编译变量：

| 变量 | 通常内容 |
|------|----------|
| `CC/CXX` | C/C++ 编译器 |
| `CPPFLAGS` | `-I`、`-D` |
| `CFLAGS/CXXFLAGS` | 警告、调试、优化 |
| `LDFLAGS` | `-L`、链接器选项 |
| `LDLIBS` | `-lm`、`-lpthread` |

## 🧪 试试看

1. 执行 `make TARGET=app && ./app`。
2. 用命令行覆盖 `CFLAGS`。
3. 比较 `=` 和 `:=` 在引用后定义变量时的结果。

## 下一步

[实验 05：自动变量](../05-automatic-variables/) 消除规则中的文件名重复。

