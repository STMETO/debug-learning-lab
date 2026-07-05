# 实验 07：事件定制与 perf trace —— 比 strace 更轻量的追踪

> 🎯 **目标**：用 `-e` 精确选择追踪事件；用 `perf trace` 低开销追踪系统调用，理解它和 strace 的区别。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 01](../01-hello-perf/)（perf stat 基础）、[strace 模块](../../strace/)（系统调用概念）

## perf trace 是什么？

`perf trace` 是 perf 家族中最像 strace 的工具——它追踪程序的系统调用。但关键区别在于**实现原理**：

```
strace:
  程序 → ptrace() 拦截 → 内核暂停程序 → 记录 syscall → 恢复程序
  每次系统调用都要"停车检查" → 开销大（~10-50% 减速）

perf trace:
  程序 → 内核 tracepoint → perf 异步读取 → 不影响程序执行
  像"监控摄像头"而不是"路障" → 开销低（~2-5% 减速）
```

> 对于生产环境中的高吞吐服务，`perf trace` 的低开销是决定性优势。

---

## 先看示例程序

[syscalls.c](syscalls.c) 执行四类系统调用——文件、内存、网络、进程：

```bash
make && ./syscalls
```

---

## 实验开始

### 第一步：perf trace 初体验

```bash
perf trace ./syscalls
```

输出（程序输出和 trace 混合在一起）：

```
=== perf trace Demo ===
Step 1: file operations...
  [file] read back: Hello from perf trace demo!
Step 2: memory operations...
  [memory] 100 alloc/free cycles done
Step 3: network operations...
  [network] socketpair ping-pong done
Step 4: process info...
  [process] pid=49750 uid=0
=== Done! ===
         ? (         ): syscalls/49750  ... [continued]: execve())                      = 0
     0.128 ( 0.012 ms): syscalls/49750 brk()                                            = 0x6151d65fd000
     0.241 ( 0.021 ms): syscalls/49750 mmap(len: 8192, prot: READ|WRITE, ...)           = 0x71187a1d2000
     0.300 ( 0.015 ms): syscalls/49750 openat(dfd: CWD, filename: 0x7a20221b, ...)      = 3
     0.358 ( 0.007 ms): syscalls/49750 close(fd: 3)                                     = 0
     0.399 ( 0.009 ms): syscalls/49750 read(fd: 3, buf: ..., count: 832)                = 832
```

每一行的格式和 strace 类似：`耗时(执行时间): 进程/线程  系统调用(参数...) = 返回值`

### 第二步：perf trace vs strace —— 对比

对同一个程序跑两个工具：

```bash
# strace 版本
strace -c ./syscalls    # -c 统计模式

# perf trace 版本
perf trace -s ./syscalls  # -s 统计模式
```

| 对比维度 | strace | perf trace |
|---------|--------|-----------|
| 原理 | ptrace（暂停-检查-恢复） | 内核 tracepoint（异步读取） |
| 性能开销 | ~10-50% 减速 | ~2-5% 减速 |
| 输出格式 | 参数/返回值完整 | 参数/返回值完整 |
| 过滤能力 | `-e trace=file,network` | `-e` 表达式 |
| 查看参数内容 | 字符串、buffer 内容 | 字符串、buffer 内容 |
| 适合场景 | 调试、排查单个请求 | 生产环境、长时间运行、高性能要求 |

> **经验法则**：调试开发环境用 strace（信息更全、容错更好），生产环境或性能敏感场景用 perf trace。

### 第三步：按系统调用类别过滤

```bash
# 只看文件相关的系统调用
perf trace -e 'open*,close,read,write' ./syscalls

# 只看网络相关的系统调用
perf trace -e 'socket*,connect*,send*,recv*,bind,listen' ./syscalls
```

perf trace 的 `-e` 支持通配符（`open*` 匹配 `open`、`openat`、`openat2`），比 strace 的 `-e trace=file` 更灵活。

### 第四步：用 tracepoint 做精确事件采集

前面实验用的 `perf stat -e cycles,instructions` 是 CPU 内置事件。perf 还可以采集内核中预埋的**静态追踪点（tracepoint）**：

```bash
# 列出所有 tracepoint
perf list tracepoint | head -20

# 只看和系统调用相关的
perf list 'syscalls:sys_enter_*' | head -15
```

追踪 `openat` 系统调用——每次进入时记录：

```bash
perf record -e 'syscalls:sys_enter_openat' -o perf.data ./syscalls
perf script -i perf.data | head -10
```

> 这里 `syscalls:sys_enter_openat` 和 `perf trace -e openat` 追踪的是同一件事，但 tracepoint 给了你更底层的控制——你可以只记录"进入"（enter）而不记录"退出"（exit），或者反过来。

### 第五步：自定义多事件追踪

一次采集多种事件：

```bash
# 同时追踪文件打开、内存分配、进程创建
perf record \
    -e 'syscalls:sys_enter_openat' \
    -e 'syscalls:sys_enter_mmap' \
    -e 'syscalls:sys_enter_clone' \
    -o perf.data ./syscalls

perf script -i perf.data
```

统计每种事件的数量：

```bash
perf report -i perf.data --stdio | head -20
```

---

## 📝 核心认知

### 事件的三个层次

```
硬件事件 (hardware event)      → cycles, instructions, cache-misses
                                   CPU 内部的 PMU 计数器

软件事件 (software event)      → context-switches, page-faults, cpu-migrations
                                   内核维护的计数器

追踪点 (tracepoint)            → syscalls:sys_enter_openat, block:block_bio_queue
                                   内核源码中预埋的静态追踪点
```

### perf trace 不是 strace 的替代品

```
strace 更适合：
  - "这个程序有没有报 ENOENT？"（一次性调试）
  - "为什么连接被拒绝？"（看错误的详细返回值）
  - 交互式排查

perf trace 更适合：
  - "生产服务器上这个服务每秒做了多少次 open？"（长期监控）
  - "哪些系统调用最慢？"（性能分析 + 统计）
  - 高吞吐场景（不能承受 strace 的性能损失）
```

---

## 🧪 试试看

1. 用 `perf trace -s ./syscalls` 看统计——哪个系统调用被调用次数最多？总耗时最长的是哪个？
2. 对比 `time strace ./syscalls` 和 `time perf trace ./syscalls` 的总耗时——perf trace 真的更快吗？
3. 追踪内核函数而非系统调用：`sudo perf record -e 'kmem:kmalloc' -o perf.data ./syscalls`——你的 WSL2 支持这个 tracepoint 吗？
4. 用 `perf trace --max-events 20 ./syscalls` 只显示前 20 个系统调用——适合只想"瞟一眼"的场景。

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `perf trace ./program` | 追踪所有系统调用（低开销） |
| `perf trace -e 'expr' ./program` | 按表达式过滤（支持通配符） |
| `perf trace -s ./program` | 统计模式（类似 strace -c） |
| `perf record -e 'tracepoint:名字' ./program` | 采集指定 tracepoint 事件 |
| `perf list tracepoint` | 列出所有可用的静态追踪点 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `perf trace` 报 `Permission denied` | 追踪其他进程或内核事件需要权限 | 对自己的程序不需要 sudo |
| tracepoint 事件 `<not supported>` | WSL2 内核可能缺少某些 CONFIG 选项 | 换一个可用的 tracepoint |
| `perf trace -e` 没有任何输出 | 表达式写错了 | 用 `perf trace ./program` 看全部输出，确认函数名 |

---

## 下一步

perf trace 让你从"系统调用"角度看程序。如果想看内核**怎么调度**你的程序（什么时候让出 CPU、迁移到哪个核心、等待了多久），下一步 [实验 08：调度与锁分析](../08-sched-analysis/)。
