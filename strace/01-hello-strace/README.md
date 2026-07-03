# 实验 01：Hello strace —— 看见程序的"隐形对话"

> 🎯 **目标**：第一次运行 `strace`，看到程序运行时对内核发出的每一个系统调用。
>
> ⏱️ 约 10 分钟

## 先看示例程序

[hello.c](hello.c) 是一个普通的小程序——打印信息、打开文件、写入数据、关闭文件、分配内存。

```c
printf("=== Hello from strace lab! ===\n");
FILE *fp = fopen("/tmp/strace_demo.txt", "w");
fprintf(fp, "This was written via strace demo.\n");
fclose(fp);
int *heap_data = (int *)malloc(10 * sizeof(int));
```

这些操作背后都变成了什么？`printf` 怎么把字符串输出到屏幕？`fopen` 怎么和文件系统交互？`malloc` 怎么向内核要内存？

用 strace 来亲眼看看。

```bash
make && ./hello
```

---

## 实验开始

### 第一步：第一次运行 strace

```bash
strace ./hello
```

你会看到大量输出。不用担心——我们一步步解读。先看最开头和最后面：

**开头几行**（程序启动，动态链接器加载 libc）：

```
execve("./hello", ["./hello"], ...)       = 0
brk(NULL)                                  = 0x...
mmap(NULL, 8192, PROT_READ|PROT_WRITE, ...) = ...
openat(AT_FDCWD, "/etc/ld.so.cache", ...)  = 3
openat(AT_FDCWD, "/lib/.../libc.so.6", ...) = 3
read(3, "\177ELF\2\1\1\3...", 832)         = 832
```

> 这些是操作系统在**加载你的程序**——打开 libc 库文件、读取 ELF 头、映射到内存。不是你写的代码，但 strace 也看见了。**程序在 `main()` 之前就已经在执行大量系统调用了。**

**中间的"我们的代码"部分**：

```
openat(AT_FDCWD, "/tmp/strace_demo.txt", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 3
write(3, "This was written via strace demo"..., 77) = 77
close(3)                                = 0
write(1, "=== Hello from strace lab! ===\n"..., 218) = 218
```

**结尾**：

```
exit_group(0)                           = ?
+++ exited with 0 +++
```

### 第二步：理解一行 strace 输出

strace 输出的每一行有固定的结构：

```
函数名(参数列表) = 返回值
```

以打开文件的那行为例：

```
openat(AT_FDCWD, "/tmp/strace_demo.txt", O_WRONLY|O_CREAT|O_TRUNC, 0666) = 3
 │      │        │                        │                              │   │
 │      │        │                        │                              │   └─ 返回值：fd=3
 │      │        │                        │                              └── 权限：0666
 │      │        │                        └── 打开方式：只写|创建|截断
 │      │        └── 文件路径
 │      └── 目录：当前工作目录 (AT_FDCWD = "从当前目录开始找")
 └── 系统调用名
```

> 只消看一眼 `= 3`，你就知道文件打开成功了（返回了一个有效的文件描述符 3）。如果失败，会看到 `= -1 ENOENT (No such file or directory)`。

### 第三步：追踪我们的程序——逐行对照

把 C 代码和它产生的系统调用并排看：

| C 代码 | 产生的系统调用 | 解读 |
|--------|--------------|------|
| `fopen("/tmp/strace_demo.txt", "w")` | `openat(...) = 3` | 打开文件，内核返回 fd=3 |
| `fprintf(fp, "...")` | `write(3, "...", 77) = 77` | 向 fd=3 写入 77 字节 |
| `fclose(fp)` | `close(3) = 0` | 关闭 fd=3 |
| `printf("=== Hello...")` | `write(1, "...", 218) = 218` | 向 fd=1（stdout）写入 218 字节 |
| `malloc(40)` | `brk(0x...) = 0x...` | 扩展堆空间（向内核申请更多内存） |
| `return 0` | `exit_group(0) = ?` | 退出程序，返回码 0 |

**关键发现**：

1. **`printf` → `write(1, ...)`**：`printf` 是 C 库函数，不是系统调用。C 库最终调用 `write` 系统调用来完成实际输出。`1` 是 stdout 的文件描述符。

2. **`fopen` → `openat(...) = 3`**：返回的 `3` 是文件描述符——内核用数字标记"你打开的那个文件"。之后所有对这个文件的操作（读、写、关闭）都用这个数字。

3. **`malloc` → `brk(...)`**：`malloc` 管理内存池。当池子不够用时，通过 `brk` 向内核申请扩展堆空间。

4. **所有输出被合并到一个 `write`**：strace 的 stderr 先输出，程序的 stdout 后输出——两者在终端里**混合**在一起。注意到 `write(1, "...", 218)` 那行的末尾直接接了程序的输出文本。

### 第四步：过滤——只看感兴趣的调用

完整输出太长了。用 `-e trace=` 只看几种调用：

```bash
strace -e trace=openat,write,close,brk,exit_group ./hello
```

输出简洁多了：

```
brk(NULL)                               = 0x...
openat(AT_FDCWD, "/etc/ld.so.cache", ...) = 3    ← 加载 libc（启动）
close(3)                                = 0
openat(AT_FDCWD, "/lib/.../libc.so.6", ...) = 3  ← 加载 libc
close(3)                                = 0
brk(NULL)                               = 0x...
brk(0x...)                              = 0x...   ← malloc 触发
openat(AT_FDCWD, "/tmp/strace_demo.txt", ...) = 3 ← fopen 触发
write(3, "This was written...", 77)     = 77       ← fprintf 触发
close(3)                                = 0        ← fclose 触发
write(1, "=== Hello from strace lab!...", 218) = 218  ← printf 触发
exit_group(0)                           = ?        ← return 0
```

> 这样你就能清晰看到：启动时的 4 个调用 + 你自己的代码产生的 5 个调用。

---

## 📝 核心认知

### stdout 和 stderr 的竞赛

注意 `write(1, ...)` 那行末尾混合了程序输出——这是因为 strace 默认把它的报告输出到 stderr（fd=2），而你的程序输出到 stdout（fd=1）。两者在终端同时显示时会混乱。

用 `-o` 把 strace 报告写到文件：

```bash
strace -o trace.log ./hello
cat trace.log
```

### 文件描述符速查

| fd | 名称 | 说明 |
|----|------|------|
| 0 | stdin | 标准输入 |
| 1 | stdout | 标准输出 |
| 2 | stderr | 标准错误 |
| 3+ | 你打开的 | `openat` 返回的第一个可用 fd = 3 |

---

## 🧪 试试看

1. 把 `fopen("/tmp/strace_demo.txt", ...)` 改为一个不存在的路径（比如 `/root/no_access.txt`），strace 看到的 `openat` 返回值是什么？
2. `strace -c ./hello`——这个命令输出什么？（预告：实验 06 会讲）
3. 试试 `strace ls` 和 `strace pwd`——两个最简单的 Linux 命令各产生了多少个系统调用？
4. 在 `hello.c` 里加一个 `fopen` 打开第二个文件但不关闭——strace 能看到什么？`close` 被调用了还是漏掉了？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `strace: command not found` | strace 未安装 | `sudo apt install strace` |
| strace 输出太长，终端滚过去了 | 默认所有调用都打印 | `-e trace=函数1,函数2` 或 `-o 文件` |
| 看不到 `printf` 而是看到 `write` | `printf` 是库函数，`write` 才是系统调用 | 正常——strace 只显示系统调用 |
| `openat` 而不是 `open` | 现代 Linux 中 `open` 被 `openat` 替代了 | 功能相同——`openat` 是更通用的版本 |

---

## 下一步

你现在能用 `strace` 看到程序的"内核对话"了。下一步学习 [实验 02：读懂输出](../02-read-output/)——深入理解 strace 每一行的完整格式：参数编码、错误码、信号、十六进制转储。
