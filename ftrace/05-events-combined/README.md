# 实验 05：事件与复合追踪 —— 同时看到"函数调用"和"系统事件"

> 🎯 **目标**：同时启用 tracepoint 事件和函数追踪、理解两者的互补关系、用事件过滤器减少噪音、构建"函数调用 + 系统事件"的完整视野。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 03](../03-function-graph/)（function_graph）、[实验 04](../04-trace-cmd/)（trace-cmd）

## 为什么需要复合追踪？

单独的 `function_graph` 只能看到**函数调用**，单独的 tracepoint 事件只能看到**系统事件**。但真实的问题往往跨层次——你想同时知道：

- "`ext4_file_write_iter` 花了 2ms → 在这 2ms 内，系统调用了多少次 `write`？"
- "`sched_process_fork` 事件触发时 → 内核中哪些函数被调用了？"
- "为什么 `open()` 有时快有时慢 → 是文件系统的问题还是调度的问题？"

**复合追踪 = function tracer + tracepoint events 同时运行。**

---

## 先看示例程序

[combined.c](combined.c) 有四种不同类型的操作，每种触发不同的追踪数据：

| 阶段 | 操作 | 触发什么 |
|------|------|---------|
| Phase 1 | 30 次文件 open/write/close | `ext4_file_*` 函数 + `syscalls:*` 事件 |
| Phase 2 | 3 次文件 fsync | 深层函数调用 + block layer 事件 |
| Phase 3 | fork + wait | `sched:sched_process_fork` 事件 |
| Phase 4 | 50000 次 getpid | 大量 `sys_enter_getpid` 事件 |

```bash
make && ./combined
```

---

## 实验开始

### 第一步：同时录制函数追踪 + tracepoint 事件

```bash
sudo trace-cmd record \
    -p function_graph \
    -g 'ext4_file_*' \
    -e 'syscalls:sys_enter_openat' \
    -e 'syscalls:sys_enter_write' \
    -e 'syscalls:sys_enter_close' \
    -e 'sched:sched_process_fork' \
    -o combined.dat \
    ./combined
```

**选项说明**：

| 选项 | 含义 | 追踪的层次 |
|------|------|-----------|
| `-p function_graph` | 选择 function_graph tracer | 函数调用（入口+出口+耗时） |
| `-g 'ext4_file_*'` | 对 ext4_file_* 展开调用图 | ext4 文件操作的具体路径 |
| `-e 'syscalls:sys_enter_*'` | 启用 syscall 进入事件 | 系统调用层面 |
| `-e 'sched:sched_process_fork'` | 启用 fork 事件 | 进程调度层面 |

> 这是一次追踪、三个层次（函数 + 系统调用 + 进程事件）的数据采集。`-e` 可以重复多次，每个 `-e` 添加一类事件。

### 第二步：阅读混合输出

```bash
sudo trace-cmd report -i combined.dat | head -40
```

```
cpus=12
   combined-1080520 [002] 820425.295738: funcgraph_entry:     |  ext4_file_open() {
   combined-1080520 [002] 820425.295740: funcgraph_entry:     |    dquot_file_open() {
   combined-1080520 [002] 820425.295740: funcgraph_entry: 0.441 us | generic_file_open();
   combined-1080520 [002] 820425.295741: funcgraph_exit:  1.493 us |    }
   combined-1080520 [002] 820425.295741: funcgraph_exit:  4.579 us |  }
   combined-1080520 [002] 820425.295768: funcgraph_entry:     |  ext4_file_read_iter() {
   combined-1080520 [002] 820425.295768: funcgraph_entry:     |    generic_file_read_iter() {
   combined-1080520 [002] 820425.295769: funcgraph_entry:     |      filemap_read() {
   ...
   combined-1080520 [002] 820425.295785: funcgraph_entry:     |      generic_update_time() {
   combined-1080520 [002] 820425.295786: funcgraph_entry:     |        inode_update_timestamps() {
```

**输出中同时包含**：
- `funcgraph_entry:` / `funcgraph_exit:` — 函数调用图
- `syscalls:sys_enter_openat:` — 系统调用事件（如果有 openat 被调用）
- `sched:sched_process_fork:` — fork 事件（如果有 fork 发生）
- `print: tracing_mark_write:` — trace_marker 标记

### 第三步：过滤复合输出 —— 只看事件

```bash
# 只看 tracepoint 事件，不看函数调用图
sudo trace-cmd report -i combined.dat | grep -v 'funcgraph'
```

```bash
# 只看函数调用图，不看事件
sudo trace-cmd report -i combined.dat | grep 'funcgraph'
```

```bash
# 看特定事件类型
sudo trace-cmd report -i combined.dat | grep 'syscalls:'
```

> 复合追踪的数据量大，但这些简单的 grep 就能将三个层次的数据分开分析。

### 第四步：关联函数和事件 —— 找因果关系

核心问题："`ext4_file_open` 花了 4.7μs，但**为什么**那次这么慢？"

在 `trace-cmd report` 的输出中，**按时间戳排列**的函数和事件天然就是因果链：

```
820425.295738  funcgraph_entry: ext4_file_open() {      ← 函数开始
820425.295740      funcgraph_entry: dquot_file_open() {   ← 子调用
820425.295740          generic_file_open() 0.44μs          ← 叶子函数
820425.295741      funcgraph_exit: 1.49μs                 ← 子调用结束
820425.295741  funcgraph_exit: 4.57μs                     ← 函数结束，总耗时 4.57μs
```

在这个 4.57μs 的窗口内，如果有 `syscalls:sys_enter_openat` 事件，它应该出现在 `820425.295738` 之前（系统调用是触发 `ext4_file_open` 的原因）。

### 第五步：事件丢失 —— 复合追踪的代价

注意 trace-cmd record 的输出末尾：

```
CPU 9:  336268 events lost
CPU 10: 391053 events lost
CPU 11: 399024 events lost
```

> **事件丢失了！** 复合追踪产生的事件量远超单一模式——函数追踪和 tracepoint 事件同时写入缓冲区，CPU 9-11 上的高速事件溢出丢弃了 30-40 万个事件。

这是复合追踪的最大挑战——**数据量太大，缓冲区不够用**。解决方法：
1. 增大缓冲区：`trace-cmd record -b 8192`（8MB，默认 ~1.4MB）
2. 绑定 CPU：`trace-cmd record -M 0-3`（只用 4 个核心，减少并发写入）
3. 缩小过滤范围：更精确的 `-g` 和 `-e` 模式

> 实验 07 专门讲环形缓冲区调优。

---

## 📝 核心认知

### tracepoint 事件和 function tracer 的区别

```
                     function tracer        tracepoint 事件
                    ─────────────────     ──────────────────
追踪对象             内核函数调用           内核中预埋的追踪点
触发方式             每次函数调用都触发      事件发生时触发
数量                 57633 个函数           2422 个事件
开销                 高（每个函数都要记录）   低（只有事件点才记录）
典型输出             ext4_file_open()       sys_enter_openat(fd=3, file="/tmp/...")
适合场景             "调用路径分析"          "事件计数和时序分析"
```

### 复合追踪 = 同时启用两者

trace-cmd 让这变得简单——`-p function_graph` 选 tracer，`-e` 重复选事件。它们在同一个缓冲区中按时间戳混排，天然提供了因果关系。

---

## 🧪 试试看

1. 在函数追踪输出的 `ext4_file_open` 入口前后，搜索 `syscalls:sys_enter_openat` 事件——它们的时间差是多少？这个差代表什么？
2. 把 `-e` 改成 `-e 'sched:sched_switch'`（调度切换事件）——你能看到 `combined` 程序何时被调度出 CPU、何时恢复吗？
3. 用 `trace-cmd report -i combined.dat | awk '{print $5}' | sort | uniq -c | sort -rn` 统计各类事件的数量——哪种最多？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `trace-cmd record -p function_graph -g 'pat' -e 'event' -e 'event2' ./prog` | 复合追踪：函数 + 事件 |
| `trace-cmd report -i trace.dat \| grep -v funcgraph` | 只看事件，过滤函数调用 |
| `trace-cmd report -i trace.dat \| grep funcgraph` | 只看函数调用 |
| `trace-cmd record -b 8192 ...` | 增大缓冲区（KB） |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `events lost` 很多 | 复合追踪事件量太大 | `-b 8192` 增大缓冲区、`-M 0-3` 限制 CPU |
| 函数和事件的时间戳不对齐 | 不同 CPU 的时钟可能略有偏差 | 正常——绑定到单 CPU 可以减少偏差 |
| `-e` 的事件没有出现在输出中 | 程序没有触发该事件 | 检查事件名称是否正确、程序是否真的触发了 |

---

## 下一步

复合追踪让你同时看到函数调用和系统事件。但面对"间歇性卡顿"这种棘手的实战问题，你需要的是完整的排查思路。下一步 [实验 06：实战——定位延迟抖动](../06-real-world/)。
