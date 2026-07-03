# 实验 05：追踪子进程 —— 不止看父进程，还要看整棵树

> 🎯 **目标**：用 `-f` 追踪 `fork` 出的子进程，用 `-ff` 把每个进程的输出写到单独文件。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 03](../03-filtering/)（过滤）、[实验 04](../04-timing/)（时间分析）

## 默认行为 —— 只追踪父进程

```bash
strace ./multi        # 父进程的系统调用 → ✅ 能看到
                      # 子进程的系统调用 → ❌ 看不到
```

strace 默认只 attach 到**你指定的那个进程**。如果它 `fork` 了子进程，子进程的行为对你是**隐形**的。

---

## 先看示例程序

[multi.c](multi.c) 是一个多进程程序：

- **父进程**：fork 3 个子进程，然后写一个文件，最后 `wait` 等待
- **每个子进程**：写自己的文件，然后退出

```bash
make && ./multi
```

---

## 实验开始

### 第一步：不带 `-f` —— 子进程隐形

```bash
strace -e trace=openat,write,close -o /tmp/nof.log ./multi
cat /tmp/nof.log
```

输出（简化）：

```
openat(AT_FDCWD, "/etc/ld.so.cache", ...) = 3    ← 父进程启动
close(3)                                = 0
openat(AT_FDCWD, "/lib/.../libc.so.6", ...) = 3  ← 父进程启动
close(3)                                = 0
openat(AT_FDCWD, "/tmp/strace_parent.txt", ...) = 3  ← 父进程写文件
write(3, "Parent process wrote this.\n", 27) = 27
close(3)                                = 0
--- SIGCHLD ... ---                                 ← 子进程退出信号
--- SIGCHLD ... ---
--- SIGCHLD ... ---
+++ exited with 0 +++
```

> 你只看到了父进程的行为——打开 libc、写 `strace_parent.txt`。**三个子进程的文件操作完全不可见。** 唯一的线索是三条 `SIGCHLD` 信号（子进程退出时通知父进程）。

### 第二步：带 `-f` —— 子进程浮现

```bash
strace -f -e trace=openat,write,close -o /tmp/f.log ./multi
cat /tmp/f.log
```

输出（注意每行开头的 PID）：

```
257122 openat(AT_FDCWD, "/etc/ld.so.cache", ...) = 3    ← 父进程
257122 close(3)                         = 0
257122 openat(AT_FDCWD, "/lib/.../libc.so.6", ...) = 3  ← 父进程
257122 close(3)                         = 0
257126 openat(AT_FDCWD, "/tmp/strace_child1.txt", ...) = 3  ← 子进程 1！
257127 openat(AT_FDCWD, "/tmp/strace_child2.txt", ...) = 3  ← 子进程 2！
257126 write(3, "Data from child process 1...", 39) = 39
257127 write(3, "Data from child process 2...", 39) = 39
257122 openat(AT_FDCWD, "/tmp/strace_parent.txt", ...) = 3  ← 父进程
257128 openat(AT_FDCWD, "/tmp/strace_child3.txt", ...) = 3  ← 子进程 3！
257126 close(3)                         = 0
257127 close(3)                         = 0
257122 write(3, "Parent process wrote this.\n", 27) = 27
257128 write(3, "Data from child process 3...", 39) = 39
257122 close(3)                         = 0
257128 close(3)                         = 0
257126 write(1, ...)                    = 142
257127 +++ exited with 0 +++
257126 +++ exited with 0 +++
257128 +++ exited with 0 +++
257122 +++ exited with 0 +++
```

> **每一行都带 PID**：`257122` = 父进程，`257126`/`257127`/`257128` = 三个子进程。四个进程的系统调用明确标注。

### 第三步：解读并发 —— `<unfinished>` 和 `<resumed>`

多进程并发时，系统调用会交错。strace 用 `<unfinished ...>` 和 `<... resumed>` 表示一个调用跨越了多个事件：

```
257126 openat(..., "/tmp/strace_child1.txt", ... <unfinished ...>
257127 openat(..., "/tmp/strace_child2.txt", ... <unfinished ...>
257126 <... openat resumed>)            = 3
257127 <... openat resumed>)            = 3
```

解读：
1. 子进程 1 开始 `openat`，但还没完成
2. 子进程 2 也开始 `openat`
3. 子进程 1 的 `openat` 完成（返回 3）
4. 子进程 2 的 `openat` 完成（返回 3）

> 这种显示模式揭示了一个重要事实：**这两个 `openat` 是在不同 CPU 核上同时执行的**（或在一个核上快速切换）。

### 第四步：`-ff` —— 每个进程写单独文件

当进程很多时，所有 PID 混在一起很难读。`-ff` 为每个进程创建单独的日志文件：

```bash
strace -ff -o /tmp/trace ./multi
```

这会产生：

```
/tmp/trace.257122    ← 父进程的 strace 日志
/tmp/trace.257126    ← 子进程 1 的 strace 日志
/tmp/trace.257127    ← 子进程 2 的 strace 日志
/tmp/trace.257128    ← 子进程 3 的 strace 日志
```

每个文件是该进程**自己的视角**——干净、不交错、易读。相当于你同时对四个进程各开了一个独立的 `strace`。

---

## 📝 命令小结

| 选项 | 作用 |
|------|------|
| `-f` | 追踪 `fork`/`vfork`/`clone` 出的所有子进程 |
| `-ff` | 每个进程的追踪输出写入单独文件（`文件名.PID`） |
| `-o 文件名` | 指定输出文件（与 `-ff` 配合时自动追加 `.PID`） |

### 输出中的新元素

| 元素 | 含义 |
|------|------|
| `PID ` | `-f` 时每行开头显示发起调用的进程 PID |
| `<unfinished ...>` | 进程 A 开始调用但还没结束，进程 B 的调用被报告了 |
| `<... 函数名 resumed>` | 进程 A 的调用现在完成了 |
| `+++ exited with N +++` | 子进程退出 |

---

## 🧪 试试看

1. 用 `-f` 追踪 `strace -f bash -c 'ls /tmp'`——你能看到 bash、ls 的进程树吗？
2. 用 `-ff -o /tmp/tree ./multi` 追踪，然后对比父进程和某个子进程的日志——它们各自打开的文件有什么不同？
3. `wait4` 系统调用在 strace 输出里长什么样？你能从 `wait4` 的参数看出它在等哪个子进程吗？
4. 如果子进程又 fork 了孙进程，`-f` 能追踪到吗？（提示：`-f` 递归追踪整个进程树）

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 子进程的日志里也有 libc 加载 | 每个子进程是独立地址空间，需要各自加载动态库 | 正常——子进程不会继承已映射的库 |
| `-ff` 生成的文件太多 | 程序频繁 fork 短命子进程 | 考虑用 `-e` 过滤减少输出量 |
| 文件名 `文件名.PID` 中 PID 和输出里的 PID 不一致 | `-ff` 为每个进程创建文件，`-f` 在每行标注 PID | 正常——两者一致 |

---

## 下一步

🟡 阶段二全部完成！你现在掌握了 strace 的三大核心技能：

- [实验 03](../03-filtering/)：**过滤**——在海量调用中聚焦
- [实验 04](../04-timing/)：**计时**——定位性能瓶颈
- [实验 05](../05-follow-forks/)：**追踪进程树**——完整的父子视角

下一步进入 🟠 阶段三：[实验 06：统计模式](../06-statistics/)——用 `-c` 一键生成系统调用的统计摘要，不用逐行读日志。
