# 实验 01：Hello ltrace —— 站在 C 代码和内核之间

> 🎯 **目标**：第一次运行 `ltrace`，看到程序中每一个库函数调用的参数和返回值。
>
> ⏱️ 约 10 分钟

## 先看示例程序

[hello.c](hello.c) 调用了一些最常见的 C 库函数：

```c
printf("=== Hello from ltrace! ===\n");
int *data = (int *)malloc(10 * sizeof(int));
size_t len = strlen("debugging with ltrace");
FILE *fp = fopen("/tmp/ltrace_hello.txt", "w");
fprintf(fp, "Written via ltrace demo.\n");
memcpy(dst, src, strlen(src) + 1);
```

`printf`、`malloc`、`strlen`、`fopen`、`memcpy`——这些都不是 C 语言的关键字，它们是 libc 提供的**库函数**。ltrace 让你亲眼看到它们的调用。

```bash
make && ./hello
```

---

## 实验开始

### 第一步：运行 ltrace

```bash
ltrace ./hello
```

输出（stderr 部分）：

```
puts("=== Hello from ltrace! ===")               = 27
malloc(40)                                       = 0x582f2be522b0
printf("malloc 返回: %p\n", 0x582f2be522b0)      = 30
strlen("debugging with ltrace")                  = 21
printf("字符串 \"%s\" 的长度: %zu\n", "debugging with ltrace", 21) = 48
free(0x582f2be522b0)                             = <void>
fopen("/tmp/ltrace_hello.txt", "w")              = 0x582f2be522e0
fclose(0x582f2be522e0)                           = 0
strlen("memory copy test")                       = 16
memcpy(0x7ffe..., "memory copy test\0", 17)      = 0x7ffe...
```

每一行 = 一个库函数被调用。和 strace 的格式完全一样：`函数(参数) = 返回值`。

### 第二步：逐行对照 C 代码

把你的 C 代码和 ltrace 看到的放在一起：

| C 代码 | ltrace 看到的 | 返回值 |
|--------|-------------|--------|
| `printf("=== Hello...\n")` | `puts("=== Hello...")` | `= 27`（输出了 27 个字符） |
| `malloc(10 * sizeof(int))` | `malloc(40)` | `= 0x582f...`（堆地址） |
| `strlen("debugging with ltrace")` | `strlen("debugging with ltrace")` | `= 21`（字符串长度） |
| `free(data)` | `free(0x582f...)` | `= <void>`（无返回值） |
| `fopen("/tmp/ltrace_hello.txt", "w")` | `fopen("/tmp/...txt", "w")` | `= 0x582f...`（FILE*） |
| `fclose(fp)` | `fclose(0x582f...)` | `= 0`（成功） |
| `memcpy(dst, src, 17)` | `memcpy(0x7ffe..., "memory...", 17)` | `= 0x7ffe...`（目标地址） |

### 第三步：一个有趣的发现 —— 编译器会"偷换"函数

注意第一行！你的代码写的是：

```c
printf("=== Hello from ltrace! ===\n");
```

但 ltrace 看到的是：

```
puts("=== Hello from ltrace! ===") = 27
```

**编译器（gcc）在背后做了优化**：`printf("固定字符串\n")` 末尾有 `\n` 且没有格式化参数时，编译器自动把它替换为 `puts()`——因为 `puts` 更简单更快。

同理，`fprintf(fp, "固定字符串\n")` 被替换成了 `fwrite()`。

> 这是一个重要的教训：**你写的代码 ≠ 编译器实际生成的调用**。ltrace 让你看到编译器和库之间的真实交互。

### 第四步：和 strace 对比 —— 同一个程序的两个视角

用实验 01 的 strace 经验对比：

```bash
# strace 看到的是：
write(1, "=== Hello from ltrace! ===\n", 27) = 27
brk(0x...)                                    = 0x...
openat(AT_FDCWD, "/tmp/ltrace_hello.txt", ...) = 3
write(3, "Written via ltrace demo.\n", 25)    = 25
close(3)                                      = 0
```

```bash
# ltrace 看到的是：
puts("=== Hello from ltrace! ===")            = 27
malloc(40)                                    = 0x...
fopen("/tmp/ltrace_hello.txt", "w")           = 0x...
fwrite("Written via ltrace demo.\n", ...)    = 25
fclose(0x...)                                 = 0
```

并排对比同一操作的三个层次：

| 操作 | ltrace（库调用） | strace（系统调用） |
|------|-----------------|-------------------|
| 打印到屏幕 | `puts("hello")` | `write(1, "hello\n", 6)` |
| 分配内存 | `malloc(40)` | `brk(0x...)` |
| 打开文件 | `fopen("f.txt", "w")` | `openat(..., "f.txt", O_WRONLY\|O_CREAT, 0666)` |
| 写文件 | `fwrite("data", 1, 4, fp)` | `write(3, "data", 4)` |
| 关闭文件 | `fclose(fp)` | `close(3)` |

> 💡 **层次模型**：
> - 你的 C 代码调用 `printf` → `printf` 内部调用 `write` 系统调用→内核完成实际 I/O
> - ltrace 看到 `printf`（库函数层）
> - strace 看到 `write`（系统调用层）

### 第五步：`-e` 过滤 —— 只看你关心的函数

strace 用 `-e trace=`，ltrace 用 `-e`（更简单）：

```bash
ltrace -e malloc+free+strlen+fopen+fclose ./hello
```

`+` 连接多个函数名。输出中每个函数标注了来源库：

```
hello->malloc(40)                    = 0x...
hello->strlen("debugging...")       = 21
hello->free(0x...)                  = <void>
hello->fopen("/tmp/...", "w")       = 0x...
hello->fclose(0x...)                = 0
```

> `hello->malloc` 表示这个 `malloc` 调用来自 `hello` 可执行文件（你的代码），而之前全量输出中 `libc.so.6->memcpy` 表示 `memcpy` 来自 libc 内部（比如 `printf` 内部调用了 `memcpy`）。

---

## 📝 命令小结

| 命令 | 作用 |
|------|------|
| `ltrace ./program` | 追踪程序的所有库函数调用 |
| `ltrace -e 函数1+函数2 ./program` | 只追踪指定的函数 |
| `ltrace -o 文件 ./program` | 将 ltrace 报告写入文件 |

### ltrace 和 strace 的差异速查

| | ltrace | strace |
|------|--------|--------|
| 第一印象 | `puts("hello")` — 和 C 代码更像 | `write(1, "hello\n", 6)` — 更底层 |
| 看到 `malloc(40)` | ✅ | ❌（看到的是 `brk` 或 `mmap`） |
| 看到 `strlen("hello")` | ✅ | ❌（`strlen` 纯用户态，没有系统调用） |
| 看到 `fopen` | ✅ | ❌（看到的是 `openat`） |
| 程序启动时的调用 | 少（用户态初始化） | 多（动态链接+内存映射） |

---

## 🧪 试试看

1. 用 `ltrace ls /tmp` —— `ls` 调用了哪些库函数来列出文件？
2. 把 `printf("字符串...")` 换成 `printf("%s", "字符串...")`——ltrace 看到的还是 `puts` 吗？
3. 对比 `ltrace ./hello` 和 `strace ./hello` 的输出行数——哪个更多？为什么？
4. 在 `hello.c` 里加一个 `sqrt(2.0)`（需要 `#include <math.h>` 和 `-lm`），ltrace 能看到这个调用吗？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| ltrace 输出中出现了不认识的函数 | 编译器优化替换了你的函数 | 正常——`printf→puts`、`fprintf→fwrite` 是常见优化 |
| 某个库函数调用没出现在 ltrace 中 | 函数可能被内联了（inline）或静态链接 | 编译时加 `-fno-inline` 或 `-O0` |
| ltrace 输出比 strace 少但是程序正常工作了 | 很多操作（如数学计算）不需要系统调用 | 正常——ltrace 和 strace 追踪不同层级 |

---

## 下一步

你能看到库函数调用了。下一步 [实验 02：读懂输出](../02-read-output/)——深入理解 ltrace 输出中的 `<unfinished ...>`、返回值格式、以及库来源标记。
