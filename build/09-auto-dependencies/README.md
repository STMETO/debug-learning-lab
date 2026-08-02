# 实验 09：自动头文件依赖 —— 让编译器告诉 make

> 🎯 **目标**：理解 `-MMD -MP` 生成的 `.d` 文件和 `-include`。

手写每个头文件依赖容易遗漏。GCC 在编译时可以生成：

```bash
gcc -MMD -MP -c main.c -o main.o
```

同时得到 `main.d`：

```makefile
main.o: main.c calc.h
```

Makefile 最后读取它：

```makefile
-include $(DEPS)
```

前面的 `-` 表示 `.d` 首次尚不存在时不要失败。

| 选项 | 作用 |
|------|------|
| `-MMD` | 生成用户头文件依赖，不列系统头文件 |
| `-MP` | 为头文件生成空规则，删除头文件时错误更友好 |

```bash
make
cat main.d calc.d
touch calc.h
make
```

观察两个对象都被重编译。

## 🧪 试试看

1. 运行 `make clean && make`，打开生成的两个 `.d` 文件。
2. 从 `main.c` 中删除 `#include "calc.h"`，重新构建并比较 `main.d`。
3. 临时去掉 `-MP`，删除一个仍记录在 `.d` 中的头文件，比较报错信息。

下一步：[目录与并行](../10-directories-parallel/)。
