# 实验 01：Hello ftrace —— 触碰内核的"飞行记录仪"

> 🎯 **目标**：浏览 tracefs 文件系统、用 `trace_marker` 写入自定义标记、读取 `trace` 输出、理解追踪条目的格式。
>
> ⏱️ 约 10 分钟

## ftrace 的接口：一切皆文件

和 perf 不同，ftrace 的**全部控制都在文件中**——不需要 CLI 参数、不需要子命令。你在 `/sys/kernel/tracing/` 目录下读写文件，就完成了追踪的配置、启动、停止和读取。

```
/sys/kernel/tracing/
├── current_tracer       ← 写入 "function" 或 "function_graph" 来选择追踪器
├── trace                ← 读取追踪结果（一次性）
├── trace_pipe           ← 流式读取追踪结果（类似 tail -f）
├── trace_marker         ← 用户态写入自定义标记
├── tracing_on           ← 1=开启追踪  0=暂停追踪
├── set_ftrace_filter    ← 过滤：只追踪这些函数
├── set_ftrace_pid       ← 只追踪这个 PID
└── available_tracers    ← 查看可用的追踪器列表
```

---

## 先看示例程序

[hello.c](hello.c) 是最简单的 ftrace 演示程序——它向 `trace_marker` 写入两条自定义标记：

```c
int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);
write(marker, ">>> DEMO START\n", 15);
// ... do some file I/O and syscalls ...
write(marker, "<<< DEMO END\n", 13);
```

```bash
make && ./hello
```

> ⚠️ 写 `trace_marker` 可能需要 sudo。如果报 `Permission denied`，用 `sudo ./hello`。

---

## 实验开始

### 第一步：探索 tracefs

```bash
# 查看有哪些可用的 tracer
cat /sys/kernel/tracing/available_tracers
# 输出：blk function_graph function nop

# 当前使用的是哪个 tracer？
cat /sys/kernel/tracing/current_tracer
# 输出：nop （nop = 不追踪，默认值）

# 查看可追踪的函数有多少个
cat /sys/kernel/tracing/available_filter_functions | wc -l
# 输出：57633（你的内核暴露了五万多个可追踪函数！）
```

### 第二步：写入第一个 trace_marker

先清空追踪缓冲区，然后运行程序：

```bash
# 清空缓冲区
echo "" | sudo tee /sys/kernel/tracing/trace

# 确保追踪已开启
echo 1 | sudo tee /sys/kernel/tracing/tracing_on

# 运行程序
sudo ./hello

# 读取追踪结果
sudo cat /sys/kernel/tracing/trace
```

输出：

```
# tracer: nop
#
# entries-in-buffer/entries-written: 2/2   #P:12
#
#                                _-----=> irqs-off/BH-disabled
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| / _-=> migrate-disable
#                              |||| /     delay
#           TASK-PID     CPU#  |||||  TIMESTAMP  FUNCTION
#              | |         |   |||||     |         |
           <...>-1001857 [009] ..... 817038.441427: tracing_mark_write: >>> DEMO START
           <...>-1001857 [009] ..... 817038.442403: tracing_mark_write: <<< DEMO END
```

### 第三步：解剖 trace 输出

```
# tracer: nop                  ← 当前使用的追踪器
# entries-in-buffer: 2/2       ← 缓冲区中有 2 条记录
# #P:12                        ← 12 个 CPU 核心

每一列的标题（从 `#` 开始的那几行）告诉你记录的上下文信息：
  irqs-off    → 中断是否关闭
  need-resched → 是否需要重新调度
  hardirq/softirq → 是否在硬中断/软中断上下文中
  preempt-depth → 内核抢占深度
  migrate-disable → 是否禁止 CPU 迁移
  delay        → 延迟标记

实际记录行：
           <...>-1001857 [009] ..... 817038.441427: tracing_mark_write: >>> DEMO START
           ^^^^^  ^^^^^^  ^^^^  ^^^^^ ^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^
           进程名   PID    CPU#  状态   时间戳        事件类型             你的自定义消息
```

| 字段 | 含义 | 示例 |
|------|------|------|
| 进程名-PID | 触发事件的进程 | `<...>-1001857`（`<...>` 表示内核线程或者名字被截断） |
| [CPU#] | 在哪个 CPU 核心上 | `[009]` |
| 状态标志 | `.....` = 没有特殊状态 | `d..` = 中断关闭，`N..` = 需要重调度 |
| 时间戳 | 事件发生的时间（秒） | `817038.441427`（开机以来的秒数） |
| 事件类型 | `tracing_mark_write` | trace_marker 写入总是显示为 `tracing_mark_write` |
| 消息 | 你写入的文本 | `>>> DEMO START` |

> **关键认知**：ftrace 记录了 "谁、在哪个 CPU、什么时间、做了什么"——这就是内核级追踪的价值。

### 第四步：看一个更完整的 trace

trace_marker 只能看到你手动插入的标记。如果想看内核"自然发生"的事件，启用 tracepoint 事件：

```bash
# 清空缓冲区，启用 syscall 事件
echo "" | sudo tee /sys/kernel/tracing/trace
echo 1 | sudo tee /sys/kernel/tracing/events/syscalls/sys_enter_openat/enable

sudo ./hello

# 读取——除了 trace_marker，你还会看到 openat 系统调用
sudo cat /sys/kernel/tracing/trace | head -15

# 关闭事件
echo 0 | sudo tee /sys/kernel/tracing/events/syscalls/sys_enter_openat/enable
```

> 这就是 ftrace 的基本工作流：**写入某个文件来配置 → 运行程序 → 读取 trace 文件看结果**。你将在这个模块的每个实验中反复使用这个模式。

---

## 📝 核心认知

### ftrace 和 perf 的操作哲学差异

```
perf：  perf stat ./prog        ← 一个命令搞定：配置+运行+输出
        perf record ./prog
        perf report

ftrace：echo function > current_tracer   ← 写文件：配置
        echo 1 > tracing_on            ← 写文件：启动
        ./prog                          ← 运行
        cat trace                       ← 读文件：看结果
```

> ftrace 看起来更"原始"（echo + cat），但这种设计给了你**极致的灵活性**——你可以精确控制追踪的每一个细节。`trace-cmd`（实验 04）把这层文件操作封装成了更友好的命令。

---

## 🧪 试试看

1. 用 `sudo cat /sys/kernel/tracing/trace` 在**刚开机**时查看——缓冲区里有什么？是空的还是有系统启动的残留事件？
2. 写一个 bash 循环：每秒 `echo "tick" > /sys/kernel/tracing/trace_marker`——然后用 `cat /sys/kernel/tracing/trace_pipe` 实时观察。为什么 `trace_pipe` 会阻塞等待？
3. 启用 `events/syscalls/sys_enter_getpid/enable`，跑你之前写的任何 C 程序——`getpid` 系统调用出现了多少次？

---

## 命令小结

| 操作 | 命令 |
|------|------|
| 查看可用 tracer | `cat /sys/kernel/tracing/available_tracers` |
| 查看当前 tracer | `cat /sys/kernel/tracing/current_tracer` |
| 清空缓冲区 | `echo "" \| sudo tee /sys/kernel/tracing/trace` |
| 开启/暂停追踪 | `echo 1 \| sudo tee /sys/kernel/tracing/tracing_on` |
| 读追踪结果（一次性） | `sudo cat /sys/kernel/tracing/trace` |
| 读追踪结果（流式） | `sudo cat /sys/kernel/tracing/trace_pipe` |
| 启用 tracepoint 事件 | `echo 1 \| sudo tee /sys/kernel/tracing/events/<类别>/<事件>/enable` |
| 写入自定义标记 | `echo "message" \| sudo tee /sys/kernel/tracing/trace_marker` |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `Permission denied` 写 tracefs 文件 | 需要 root 权限 | 加 `sudo` |
| `trace` 文件是空的 | `tracing_on` 可能是 0，或缓冲区被清空了 | `echo 1 > tracing_on`，确保程序运行在 trace 之后 |
| `trace_marker: No such file or directory` | tracefs 未挂载 | `mount -t tracefs none /sys/kernel/tracing` |

---

## 下一步

trace_marker 让你能往 ftrace 缓冲区"插话"。但如果想看内核中**所有被调用的函数**——每一步都不遗漏——下一步 [实验 02：函数追踪](../02-function-tracer/)。
