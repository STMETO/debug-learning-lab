# 实验 09：多文件调试 —— 在源文件之间自由穿梭

> 🎯 **目标**：学会在由多个 `.c` 文件组成的项目中设断点、单步执行，在文件之间自由跳转。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 02](../02-breakpoints/)、[实验 03](../03-stepping/)

## 为什么会需要这个？

实验 01–08 的程序都只有一个 `.c` 文件。但真实项目是这样的：

```
my_project/
├── main.c           ← 程序入口
├── math_utils.c     ← 数学工具
├── math_utils.h
├── string_utils.c   ← 字符串工具
├── string_utils.h
└── Makefile         ← 把多个 .o 链接成一个可执行文件
```

在 GDB 里调试时，你需要：
- 在 `math_utils.c` 的某个函数里设断点
- 在 `main.c` 调用它时单步**进入**，然后穿梭到 `math_utils.c`
- 看完后跳回 `main.c`

---

## 先看示例程序

我们这个小项目有三个源文件：

| 文件 | 作用 |
|------|------|
| [main.c](main.c) | 入口，调用 math 和 string 工具 |
| [math_utils.c](math_utils.c) | `gcd()`, `factorial()`, `power()` |
| [string_utils.c](string_utils.c) | `str_length()`, `word_count()`, `reverse_print()` |

```bash
make && ./app
```

注意 Makefile 的不同——多个 `.c` 先编译成 `.o`，再链接：

```bash
# Makefile 做的事：
gcc -g -c main.c          # main.c → main.o
gcc -g -c math_utils.c    # math_utils.c → math_utils.o
gcc -g -c string_utils.c  # string_utils.c → string_utils.o
gcc -o app main.o math_utils.o string_utils.o  # 链接
```

---

## 实验开始

```bash
gdb ./app
```

### 第一步：跨文件设断点

指定 `文件名:行号` 在任意源文件中设断点：

```bash
(gdb) break math_utils.c:4       # gcd 函数的第一条语句
(gdb) break string_utils.c:5     # str_length 函数的第一条语句
(gdb) break main.c:11            # 第一个 printf
```

```bash
(gdb) info breakpoints
```

输出：

```
Num     Type           Disp Enb Address            What
1       breakpoint     keep y   ... in gcd at math_utils.c:4
2       breakpoint     keep y   ... in str_length at string_utils.c:5
3       breakpoint     keep y   ... in main at main.c:11
```

> 每个断点精确标注了它在**哪个文件**里——即使函数名相同也不怕混淆。

### 第二步：观察跨文件调用栈

```bash
(gdb) run
```

第一次停在 `main.c:11`（`printf` 调用 gcd 之前）：

```bash
(gdb) continue
```

进入 `gcd` 函数——注意源码窗口会自动切换到 `math_utils.c`：

```
Breakpoint 1, gcd (a=15, b=6) at math_utils.c:4
4       while (b != 0) {
```

现在是关键——看看谁调用了你：

```bash
(gdb) backtrace
#0  gcd (a=15, b=6) at math_utils.c:4
#1  0x... in main () at main.c:11
```

`#0` 在 `math_utils.c`，`#1` 在 `main.c`——调用栈跨越了两个文件。

```bash
(gdb) print a
$1 = 15
(gdb) print b
$2 = 6
```

继续到 string_utils 的断点：

```bash
(gdb) continue
```

```
Breakpoint 2, str_length (s=0x... "Hello World") at string_utils.c:5
5       int len = 0;
```

GDB 自动切换到了 `string_utils.c`。你看源码的来源文件在断点之间无缝切换。

### 第三步：在文件之间 step

先删掉所有断点，从头开始：

```bash
(gdb) delete
(gdb) break main.c:12           # gcd 调用那行
(gdb) run
```

```
Breakpoint ..., main () at main.c:12
12      printf("gcd(%d, %d) = %d\n", a, b, gcd(a, b));
```

现在这行调用了 `gcd` 函数。按 `step` 进入它：

```bash
(gdb) step
```

GDB 进入 `math_utils.c` 的 `gcd` 函数内部：

```
gcd (a=15, b=6) at math_utils.c:4
4       while (b != 0) {
```

单步几次，然后在 `gcd` 返回前用 `finish` 跳出来：

```bash
(gdb) finish
```

回到 `main.c`。

> 这种"跳进去 → 看看 → 跳出来"的能力，在多文件项目中是核心工作流。

### 第四步：查看所有源文件

```bash
(gdb) info sources
```

列出所有编译进可执行文件的源文件。在大型项目中，你还可以搜索：

```bash
(gdb) info functions gcd       # 搜索包含 "gcd" 的函数定义
(gdb) info variables counter   # 搜索包含 "counter" 的变量
```

---

## 📝 命令小结

| 命令 | 作用 |
|------|------|
| `break 文件.c:行号` | 在指定文件的某行设断点 |
| `break 文件.c:函数名` | 在指定文件的函数入口设断点 |
| `step` / `s` | 进入函数（可能跳到另一个文件） |
| `finish` / `fin` | 跳出当前函数，回到调用方的文件 |
| `backtrace` / `bt` | 跨文件的完整调用栈 |
| `info sources` | 列出所有编译进来的源文件 |
| `info functions 关键词` | 搜索函数定义 |

## 🧪 试试看

1. 在 `factorial` 函数里设断点，用 `backtrace` 能看到多少层递归？每层的源文件都一样吗？
2. 在 `main.c` 中 `step` 进入 `reverse_print`，你能一步步从 `main.c` → `string_utils.c` → 再回到 `main.c` 吗？
3. `info sources` 列出了哪些文件？有没有你没想到的文件？（提示：`printf` 的实现来自 glibc，但源文件不在你的项目里）
4. 尝试在两个不同的断点之间用 `continue` 跳跃，观察 GDB 如何自动告诉你当前在哪个文件

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `break math_utils.c:4` 报 `No source file named` | GDB 找不到源文件（可能是相对路径问题） | 用 `info sources` 看 GDB 知道的文件名，或用 `break 函数名` |
| `step` 后不知道自己在哪个文件 | 正常——GDB 的提示行会显示文件名 | 读 `(gdb)` 前面的提示，如 `math_utils.c:4` |
| 跨文件 `backtrace` 只看到地址 | 某个 `.o` 可能没加 `-g` 编译 | 检查 Makefile，确保每个 `gcc -c` 都有 `-g` |

---

## 下一步

🟠 阶段三全部完成！你现在掌握了：

- [实验 07](../07-conditional-break/)——**精准打击**（条件断点）
- [实验 08](../08-core-dump/)——**事后破案**（core dump 分析）
- [实验 09](../09-multifile/)——**全局视野**（多文件调试）

这三个是实际工作中最高频的实战技能。下一步进入 🔴 阶段四：[实验 10：多线程调试](../10-threads/)，处理更复杂的并发场景。
