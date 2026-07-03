# 实验 03：过滤系统调用 —— 在 50 行中只看你关心的 5 行

> 🎯 **目标**：用 `-e trace=` 精确过滤系统调用，按类别缩小追踪范围。
>
> ⏱️ 约 10 分钟
>
> 📖 **前置**：[实验 01–02](../)——你至少会用 `strace` 和读懂输出

## 信息过载问题

`strace ./hello` 会产生 30-50 行输出，大部分是程序启动时的动态链接开销。在一个真实的服务器程序上，可能是**几万行**。

你需要一个"聚焦"工具。

---

## 先看示例程序

[mixed.c](mixed.c) 做了四类操作：

| 类别 | 操作 | 涉及的系统调用 |
|------|------|-------------|
| 文件 | 打开/读写/关闭文件 | `openat`, `read`, `write`, `close` |
| 内存 | 分配/释放 | `brk`, `mmap`, `munmap` |
| 进程 | 获取 PID | `getpid`, `getppid` |
| 时间 | 短暂睡眠 | `clock_nanosleep` |

```bash
make && ./mixed
```

---

## 实验开始

### 第一步：看全量输出有多"吵"

```bash
strace -o /tmp/full.log ./mixed
wc -l /tmp/full.log
```

输出：`50 /tmp/full.log`

50 行——其中只有约 10 行是我们的程序产生的，其余是 glibc 启动开销。

### 第二步：按类别过滤

#### 只看文件相关调用

```bash
strace -e trace=file ./mixed
```

输出（过滤后只有 11 行）：

```
execve("./mixed", ["./mixed"], ...)        = 0    ← 启动自身
access("/etc/ld.so.preload", R_OK)         = -1   ← 检查预加载配置
openat(AT_FDCWD, "/etc/ld.so.cache", ...)  = 3    ← 加载 ld 缓存
newfstatat(3, "", ...)                     = 0
openat(AT_FDCWD, "/lib/.../libc.so.6", ...) = 3   ← 加载 libc
newfstatat(3, "", ...)                     = 0
newfstatat(1, "", ...)                     = 0    ← 检查 stdout
openat(AT_FDCWD, "/tmp/strace_filter.txt", ...) = 3  ← fopen(写)
newfstatat(3, "", ...)                     = 0
openat(AT_FDCWD, "/tmp/strace_filter.txt", O_RDONLY) = 3  ← open(读)
+++ exited with 0 +++
```

#### `-e trace=file` 包含哪些调用？

strace 把系统调用分成若干**类别**。常用的：

| 类别 | 包含的典型调用 | 适用场景 |
|------|-------------|---------|
| `file` | `openat`, `read`, `write`, `close`, `stat`, `lseek`, `unlink`... | "程序在读/写哪个文件？" |
| `process` | `fork`, `clone`, `execve`, `exit_group`, `wait4`... | "程序启动了哪些子进程？" |
| `network` | `socket`, `connect`, `bind`, `sendto`, `recvfrom`... | "程序在连接哪个地址？" |
| `memory` | `brk`, `mmap`, `munmap`, `mprotect`... | "内存分配在干什么？" |
| `ipc` | `pipe`, `shmget`, `semop`, `msgget`... | "进程间通信" |
| `signal` | `kill`, `sigaction`, `sigreturn`, `rt_sigaction`... | "信号处理" |

### 第三步：更精细的过滤

#### 只看一个具体调用

```bash
strace -e trace=openat ./mixed
```

只显示 `openat` 调用——所有文件的打开操作一览无余。

#### 只看多个具体调用

```bash
strace -e trace=openat,read,write,close ./mixed
```

逗号分隔——只追踪这四个调用。

#### 排除某些调用

```bash
strace -e trace=\!openat ./mixed
```

`!` = "除了"——显示所有调用**除了** `openat`。

### 第四步：实战对比 —— 不同类别看到的不同"故事"

对同一个程序，用三种过滤看到三个不同的视角：

```bash
# 视角 A：文件操作
strace -e trace=file ./mixed
# → 看到：加载 libc → 打开 /tmp/strace_filter.txt → 退出

# 视角 B：内存操作
strace -e trace=memory ./mixed
# → 看到：brk（扩展堆）→ mmap（映射 libc）→ munmap（清理）

# 视角 C：进程操作
strace -e trace=process ./mixed
# → 看到：execve（启动）→ exit_group（退出）
```

> 💡 **同一个程序，不同的过滤给出不同的"视角"**。排查文件问题时用 `file`，排查内存问题时用 `memory`，排查子进程问题时用 `process`。

---

## 📝 命令小结

| 选项 | 作用 |
|------|------|
| `-e trace=file` | 只看文件相关系统调用 |
| `-e trace=network` | 只看网络相关系统调用 |
| `-e trace=process` | 只看进程相关系统调用 |
| `-e trace=memory` | 只看内存相关系统调用 |
| `-e trace=信号名` | 只看指定信号 |
| `-e trace=函数1,函数2` | 只看指定的几个调用 |
| `-e trace=\!函数名` | 排除指定调用 |
| `-o 文件` | 输出到文件（方便分析） |

---

## 🧪 试试看

1. 用 `-e trace=file` 追踪 `ls`——ls 访问了哪些文件？
2. 用 `-e trace=network` 追踪 `curl http://example.com`——能追踪到 DNS 查询的 `connect` 吗？
3. `-e trace=file` 和 `-e trace=desc` 有什么区别？（提示：查 `man strace`）
4. 写一个同时有文件和网络操作的程序，用两种过滤分别追踪——两次看到的"故事"有什么不同？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `-e trace=all` 不是有效类别 | `all` 不是类别名 | 不加 `-e` 就是追踪全部 |
| 过滤后看不到预期的调用 | 调用不属于那个类别（比如 `mmap` 在 `memory` 不在 `file`） | 查 `man strace` 的 `%file` 列表 |
| 过滤后输出还是很多 | 类别太宽（`file` 包含几十种调用） | 精确到具体函数：`-e trace=openat,read,write` |

---

## 下一步

过滤让你"只看你想看的"。下一步 [实验 04：时间分析](../04-timing/)——给每一行加上时间戳和耗时，找出程序在**哪个调用上花了最多时间**。
