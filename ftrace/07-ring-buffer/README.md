# 实验 07：环形缓冲区调优 —— 别让数据在你眼皮底下丢失

> 🎯 **目标**：`buffer_size_kb` 扩容、理解多核独立缓冲、`tracing_cpumask` 绑定指定 CPU、识别缓冲区溢出丢包特征、长时间追踪防丢失配置方案。
>
> ⏱️ 约 18 分钟
>
> 📖 **前置**：[实验 04](../04-trace-cmd/)（trace-cmd 基础）

## 为什么需要关心缓冲区？

ftrace 的环形缓冲区（ring buffer）是内核中一块固定大小的内存。事件以**每秒百万次**的速度写入，而你在读取时是**事后批量读取**。如果写入速度超过读取速度，旧事件会被新事件**覆盖**——数据就丢了。

```
环形缓冲区工作方式：

写入指针 →  │ event_N │ event_N+1 │ event_N+2 │  ......  │
读取指针 →  └──────────────────────────────────────────────┘
                                                          ↑
                                                缓冲区满了 → 覆盖最旧的事件
```

> 在前几个实验中你可能已经看到了 `CPU X: NNNNN events lost` 的警告——这就是缓冲区溢出的信号。

---

## 先看示例程序

[flood.c](flood.c) 是一个"事件洪水"生成器——快速连续地创建文件、写入、关闭，以最高速度触发内核函数调用：

```bash
make && ./flood
```

---

## 实验开始

### 第一步：查看当前缓冲区配置

```bash
# 缓冲区总大小（KB）
cat /sys/kernel/tracing/buffer_size_kb
# 输出：1410（约 1.4 MB）

# 每个 CPU 的缓冲区大小 = buffer_size_kb / CPU 数量
# 12 个 CPU × ~117 KB = 约 1.4 MB 总缓冲区

# 查看 buffer_size_kb 的允许范围
cat /sys/kernel/tracing/buffer_size_kb
# 通常在 7 KB 到数 GB 之间
```

> 默认的 ~1.4MB 对轻度追踪（几十个函数、几秒）足够了。但对于高事件率的场景——如同时追踪大量函数和 tracepoint 事件——这个值远远不够。

### 第二步：故意制造溢出

先把缓冲区缩小到极致，然后跑 flood 程序：

```bash
# 缩小缓冲区到 128 KB（强制溢出）
echo 128 | sudo tee /sys/kernel/tracing/buffer_size_kb

echo 'nop'      | sudo tee /sys/kernel/tracing/current_tracer
echo ""         | sudo tee /sys/kernel/tracing/trace
echo 'function' | sudo tee /sys/kernel/tracing/current_tracer
echo 'ext4_file_*' | sudo tee /sys/kernel/tracing/set_ftrace_filter

echo 1 | sudo tee /sys/kernel/tracing/tracing_on
sudo ./flood
echo 0 | sudo tee /sys/kernel/tracing/tracing_on
```

查看 trace 文件头部：

```
# tracer: function
#
# entries-in-buffer/entries-written: 3085/3085   #P:12
```

> `entries-written` = 总共产生的事件数；如果缓冲区溢出，`entries-in-buffer` < `entries-written`。但 128KB 在小 flood 下可能还不够溢出——在实验 05 中我们看到了真正的溢出（30 万事件丢失）。

`trace-cmd` 的等效操作——在 `record` 的输出中直接看到溢出：

```bash
sudo trace-cmd record -p function_graph -g 'ext4_file_*' -e 'syscalls:sys_enter_*' -b 128 ./flood
```

输出末尾：

```
CPU 9:  336268 events lost     ← 单核就丢了 33 万个事件！
CPU 10: 391053 events lost
CPU 11: 399024 events lost
```

### 第三步：增大缓冲区

```bash
# 增大到 8 MB
echo 8192 | sudo tee /sys/kernel/tracing/buffer_size_kb

# 或用 trace-cmd 的 -b 选项
sudo trace-cmd record -b 8192 -p function_graph -g 'ext4_file_*' ./flood
```

**buffer_size_kb 选多大？**

| 追踪强度 | 推荐 buffer_size_kb | 说明 |
|---------|-------------------|------|
| 轻量（< 10 个函数） | 默认 1408 | 日常调试 |
| 中等（10-50 函数 + 少量事件） | 4096-8192 | 函数+事件复合追踪 |
| 重度（大量函数 + 高频事件） | 16384-65536 | 生产环境临时追踪 |
| 极端（所有函数 + 所有事件） | 131072+ | 可能影响系统稳定性 |

> ⚠️ buffer_size_kb 分配的是内核内存——不能设置得太大。在 16GB RAM 的系统上，65536（64MB）是安全上限。

### 第四步：多核缓冲区的工作原理

ftrace 为**每个 CPU 核心**分配独立缓冲区：

```
CPU 0: ┌──────────────────┐  ← buffer_size_kb/N per CPU
CPU 1: ├──────────────────┤
CPU 2: ├──────────────────┤
...
CPU11: └──────────────────┘  ← 总大小 = buffer_size_kb
```

这意味着**一个 CPU 的溢出不影响其他 CPU 的数据**——在输出中你会看到 `CPU 9: events lost` 而 CPU 0-8 完好。

### 第五步：tracing_cpumask —— 只追踪特定 CPU

如果不需要追踪所有核心，用 `tracing_cpumask` 限制：

```bash
# 只追踪 CPU 0 和 CPU 4
echo 0,4 | sudo tee /sys/kernel/tracing/tracing_cpumask

# 用十六进制（推荐）：bit 0 = CPU0, bit 4 = CPU4 → 0x11 = 17
echo 11 | sudo tee /sys/kernel/tracing/tracing_cpumask

# 查看当前设置
cat /sys/kernel/tracing/tracing_cpumask
# 输出：011（只追踪 CPU 0 和 CPU 4）

# 恢复追踪所有 CPU
echo ff | sudo tee /sys/kernel/tracing/tracing_cpumask
```

**trace-cmd 等效操作**：

```bash
# -M 指定 CPU 掩码
sudo trace-cmd record -M 0,4 -p function_graph -g 'ext4_file_*' ./flood
```

**什么时候绑定 CPU？**

| 场景 | 建议 |
|------|------|
| 程序只在特定核心上跑 | 只追踪那些核心——减少不必要的事件 |
| 需要精确的时间戳对比 | 绑定到单核——消除跨核时钟偏差 |
| 缓冲区频繁溢出 | 减少追踪的核心数 = 减少写入并发 |
| NUMA 系统 | 只追踪程序所在的 NUMA 节点上的核心 |

### 第六步：溢出检测与防丢失配置

**识别溢出的三种方式**：

```bash
# 1. trace-cmd record 输出末尾直接显示
sudo trace-cmd record ... ./flood
# CPU 7: 215623 events lost  ← 溢出信号

# 2. trace 文件头部
sudo cat /sys/kernel/tracing/trace | head -5
# entries-in-buffer/entries-written: 5034/5034  ← 相等=无溢出
# entries-in-buffer/entries-written: 5034/38291  ← 不相等=溢出了！

# 3. per_cpu 溢出统计
sudo cat /sys/kernel/tracing/per_cpu/cpu0/stats
# overrun: 0        ← 该核心没有溢出
# overrun: 152341    ← 溢出了 15 万次！
```

**防丢失配置清单**：

```bash
# 1. 增大缓冲区
echo 8192 | sudo tee /sys/kernel/tracing/buffer_size_kb

# 2. 绑定到程序所在的 CPU
echo <cpumask> | sudo tee /sys/kernel/tracing/tracing_cpumask

# 3. 只追踪目标进程
echo <PID> | sudo tee /sys/kernel/tracing/set_ftrace_pid

# 4. 缩小 filter 范围（更少的函数/事件）
echo 'ext4_file_*' | sudo tee /sys/kernel/tracing/set_ftrace_filter

# 5. 使用 overwrite 模式（不停机，最旧的数据被覆盖但不报错）
echo 1 | sudo tee /sys/kernel/tracing/options/overwrite
```

---

## 📝 核心认知

### 缓冲区的两个关键属性

```
buffer_size_kb：    总缓冲区大小（跨所有 CPU 平分）
                    增大 → 更久不溢出，但占用更多内核内存

tracing_cpumask：   哪些 CPU 参与追踪
                    限制 → 减少并发写入，但可能丢失在其他 CPU 上的事件
```

### 溢出不是"错误"，而是"信号"

生产环境中，偶尔的溢出是**正常的**——说明你追踪的范围大、数据量大。关键是要：
1. **知道溢出了**——检查 trace-cmd 输出的 `events lost`
2. **评估溢出量**——丢失了 5% 还是 50%？如果 > 20%，需要调整配置
3. **针对性调整**——增大缓冲区、缩小 filter、限制 CPU

---

## 🧪 试试看

1. 把 `buffer_size_kb` 设成最小值（`cat /sys/kernel/tracing/buffer_size_kb` 的下限），跑 `./flood`。trace 文件中有多少条目？丢失了多少？
2. 绑到单 CPU（`tracing_cpumask=1`），跑 `./flood`。和追踪所有 CPU 相比，事件丢失率下降了多少？
3. 用 `trace-cmd record -b 64 -p function_graph -g 'ext4_file_*' -e 'syscalls:sys_enter_*' ./flood`——同时减小缓冲区并增加事件类型。溢出量是多少？你能通过 `-M` 缓解吗？

---

## 命令小结

| 操作 | 命令 |
|------|------|
| 查看缓冲区大小 | `cat /sys/kernel/tracing/buffer_size_kb` |
| 设置缓冲区大小 | `echo 8192 \| sudo tee /sys/kernel/tracing/buffer_size_kb` |
| 绑定 CPU | `echo 0-3 \| sudo tee /sys/kernel/tracing/tracing_cpumask` |
| 查看 CPU 掩码 | `cat /sys/kernel/tracing/tracing_cpumask` |
| 查看每 CPU 统计 | `sudo cat /sys/kernel/tracing/per_cpu/cpu0/stats` |
| trace-cmd 指定缓冲区 | `trace-cmd record -b 8192 ...` |
| trace-cmd 指定 CPU | `trace-cmd record -M 0-3 ...` |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `buffer_size_kb: Invalid argument` | 设置的值超出允许范围 | 先 `cat` 当前值，在附近范围内调整 |
| 绑定 CPU 后程序的事件完全没出现 | 程序跑在其他 CPU 上 | 用 `taskset -c <cpumask> ./prog` 把程序也绑到同一 CPU |
| `events lost` 但 `buffer_size_kb` 已经很大 | 事件产生速度远超写入速度 | 缩小 filter、减少 `-e` 事件类型、缩短追踪时间 |

---

## 下一步

你现在理解了 ftrace 的缓冲区机制——大小、CPU 分布、溢出防护。下一步 [实验 08：trace_printk 陷阱与规范](../08-trace-printk/)——深入理解 `trace_printk` 的开销原理，以及为什么在生产环境中应该避免它。
