# 实验 06：实战 —— 定位"为什么有时快有时慢"

> 🎯 **目标**：综合使用 function_graph + trace-cmd + events——给定一个间歇性卡顿的程序，找出"为什么 500 次操作里总有那么几次慢了 100 倍"。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：实验 01–05

---

## 场景设定

你接手了一个文件处理工具——[latency_test.c](latency_test.c)。用户反馈："大部分时候很快，但偶尔会卡一下，不知道在干什么。"

```
=== Intermittent Latency Test ===
Total ops: 500, fsync every 50 ops

  op   0 [SYNC]  took   3763 μs  <<<
  op   7 [SLOW]  took   7219 μs  <-- unexpected!  ← 没有 fsync，为什么也这么慢?!
  op  50 [SYNC]  took   2696 μs  <<<
  op 100 [SYNC]  took   2487 μs  <<<
  op 160 [SLOW]  took    826 μs  <-- unexpected!  ← 又出现了!
  op 473 [SLOW]  took   3047 μs  <-- unexpected!  ← 第三次异常!
```

程序每 50 次做一次 fsync（慢是预期的），但在不 fsync 的迭代中偶尔也会出现几百到几千微秒的延迟。**这些"意外的慢"就是我们要排查的。**

```bash
make && ./latency_test
```

> 💡 这个实验不再给详细步骤——你自己决定用哪个 tracer、什么 filter、怎么分析。原则：先缩小范围（perf stat / 代码审查），再用 ftrace 精确采样。

当然，如果你卡住了，下面有完整排查路径供参考。

---

## 排查路径（参考）

### 阶段一：确认问题特征

首先确认这些"意外的慢"不是测量误差——多跑几次：

```bash
for i in $(seq 1 5); do
    ./latency_test 2>&1 | grep 'SLOW'
done
```

> 如果 `SLOW` 出现的位置每次都不同（op 7 有时慢有时不慢），说明不是固定位置的 bug，而是**时机依赖**的问题——可能是缓存状态、调度、或后台内核活动。

### 阶段二：用 function_graph 捕获慢操作

```bash
sudo trace-cmd record -p function_graph \
    -g 'ext4_file_*' \
    -o latency.dat \
    ./latency_test
```

在 report 中找 `FSYNC` marker 附近的函数调用——比较 "fast iteration" 和 "SLOW iteration" 的调用栈差异：

```bash
sudo trace-cmd report -i latency.dat | grep -A20 "FSYNC at iteration 0" | head -25
```

观察 `ext4_file_write_iter` 的总耗时。在 SYNC 迭代中，`ext4_file_write_iter` 会深入到日志层（jbd2）和块层——耗时从微秒跳到毫秒。

### 阶段三：找出"意外慢"的原因

对于标记为 `SLOW` 但没有 fsync 的迭代（如 op 7，7219μs），搜索对应时间窗口的函数调用：

```bash
# 找到 SLOW 操作发生的大致时间
./latency_test 2>&1 | grep SLOW

# 在 trace-cmd report 中搜索对应时间区域
sudo trace-cmd report -i latency.dat | grep -E 'ext4_file_(open|write_iter)' | \
    awk '{if ($4 > START_TIME && $4 < END_TIME) print}'
```

**常见原因**：

| 现象 | 可能原因 | 在 ftrace 中的表现 |
|------|---------|-------------------|
| `ext4_file_open` 偶尔慢 | dcache（目录项缓存）miss → 需要扫描目录 | `ext4_lookup` 出现在 open 的调用栈中 |
| `ext4_file_write_iter` 偶尔慢 | 页缓存压力大 → 触发了回写 | `ext4_writepages` 或 `writeback` 相关函数出现 |
| 整个操作慢但 ext4 不慢 | 调度延迟——被其他进程抢占了 CPU | 在慢操作的时间窗口中有 `sched_switch` 事件 |

### 阶段四：加 tracepoint 事件找调度问题

如果 ext4 函数调用本身不慢，但操作总时间慢，可能是**被内核调度出去了**。加上调度事件再看：

```bash
sudo trace-cmd record -p function_graph \
    -g 'ext4_file_*' \
    -e 'sched:sched_switch' \
    -o latency2.dat \
    ./latency_test
```

在 SLOW 操作的时间戳窗口中搜索 `sched_switch`——如果 latency_test 被切换出去，说明其他进程在抢 CPU。

### 阶段五：验证修复

如果发现慢的原因是 page cache 回写（`ext4_writepages` 出现在非 fsync 迭代中），可能的修复包括：

1. 增大系统 dirty page 阈值（`sysctl vm.dirty_ratio`）
2. 在程序中主动 `posix_fadvise()` 告诉内核不要缓存这些文件
3. 使用 `O_DIRECT` 绕过 page cache

修复后重新跑，确认 SLOW 标记消失或大幅减少。

---

## 📝 排查方法论

```
间歇性延迟的排查流程：

1. 确认特征   → 多跑几次，看延迟是否可复现、是否和固定位置相关
2. 缩小范围   → 是 I/O 慢？调度慢？锁竞争？缓存 miss？
3. 精确追踪   → function_graph + 关键函数的 filter
4. 加事件     → 如果函数调用不慢但操作慢 → 加 sched 事件
5. 找因果     → 按时间戳对齐函数调用和事件，建立因果链
6. 验证修复   → 改代码或调整配置，重新追踪确认
```

> 这个流程不仅适用于本实验的程序——任何"间歇性慢"的问题都可以用同样的思路排查。

---

## 🧪 试试看

1. 修改 `SYNC_INTERVAL` = 1（每次都 fsync）——所有操作的延迟都到毫秒级了吗？最快的和最慢的差多少倍？
2. 在追踪的同时，另一个终端跑 `stress -c 4`（CPU 压力）——`latency_test` 的 SLOW 标记变多了吗？用 sched_switch 事件验证是不是被抢占。
3. 在 latency_test 中加 `posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED)` 在 close 之前——SLOW 标记减少了吗？为什么？

---

## 命令小结

| 步骤 | 命令 |
|------|------|
| 抓取慢操作 | `trace-cmd record -p function_graph -g 'ext4_file_*' -o trace.dat ./prog` |
| 加调度事件 | `trace-cmd record -p function_graph -g 'ext4_file_*' -e 'sched:sched_switch' -o trace.dat ./prog` |
| 找慢函数 | `trace-cmd report -i trace.dat \| grep -E 'ext4_file_.*\\| *[!+]'` |
| 看时间窗口 | `trace-cmd report -i trace.dat \| awk '/START_TIME/,/END_TIME/'` |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| SLOW 操作在 trace 中找不到 | 没有在该时间窗口设 filter | 扩大 filter 范围或用更宽松的时间搜索 |
| trace 中所有操作都是 `+` 标记 | filter 太宽，追踪了太多函数导致系统整体变慢 | 缩小 `-g` 范围 |
| 加了 sched_switch 后缓冲区溢出 | sched_switch 事件非常高频（每次调度都触发） | 增大 `-b 16384` 或只追踪一个 CPU |

---

## 下一步

本次实战用了 function_graph + events 组合排查间歇性延迟。你注意到 trace-cmd 输出中的 `events lost` 提示了吗？下一步 [实验 07：环形缓冲区调优](../07-ring-buffer/)——如何配置缓冲区避免丢数据，以及 CPU 绑定的正确姿势。
