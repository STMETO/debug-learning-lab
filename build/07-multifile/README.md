# 实验 07：多文件 Makefile —— 分别编译，最后链接

> 🎯 **目标**：把 `.c → .o` 与 `.o → app` 分成清晰规则。

手工命令是：

```bash
gcc -I. -g -Wall -Wextra -c main.c -o main.o
gcc -I. -g -Wall -Wextra -c calc.c -o calc.o
gcc main.o calc.o -o app
```

Makefile 原样表达这三步。编译规则使用 `-c`，只生成目标文件；链接规则不使用 `-c`，把所有对象组合为程序。

`main.o` 依赖 `main.c calc.h`，`calc.o` 依赖 `calc.c calc.h`。修改头文件后，两者都要重编译。

```bash
make
touch calc.c
make
touch calc.h
make
```

观察每次执行哪些规则。

## 选项为什么分开？

- `CPPFLAGS`：`-I`、`-D`；
- `CFLAGS`：警告、调试、优化；
- 链接命令以后会使用 `LDFLAGS` 和 `LDLIBS`。

## 常见错误

| 错误 | 常见原因 |
|------|----------|
| `fatal error: calc.h` | 缺少 `-I` 或路径不对 |
| `undefined reference to add` | `calc.o` 没参与链接 |
| 修改头文件但没重编译 | 规则漏写头文件依赖 |

## 🧪 试试看

1. 只修改 `calc.c`，确认 `main.o` 没有重编译。
2. 修改 `calc.h`，确认两个对象都重编译。
3. 从链接规则中删掉 `calc.o`，观察编译成功、链接失败的区别。

下一步：[模式规则](../08-pattern-rules/)。
