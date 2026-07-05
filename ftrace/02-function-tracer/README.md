# 实验 02：函数追踪 —— 看见内核中的每一次调用

> 🎯 **目标**：启用 `function` tracer、用 `set_ftrace_filter` 过滤函数、只追踪一个进程（`set_ftrace_pid`）、理解全量追踪的开销。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 01](../01-hello-ftrace/)（tracefs 基础操作）

## function tracer 是什么？

`function` tracer 是 ftrace 最基础的追踪器——它在**每个被追踪的内核函数入口**记录一行：

```
function tracer 输出：
  CPU#  TIMESTAMP    FUNCTION
  [003] 817068.19:   ext4_file_open <- do_dentry_open
                      ^^^^^^^^^^^^^    ^^^^^^^^^^^^^^
                      被调用的函数      调用者（谁调用了它）
```

> 和 `perf record` 的采样不同，function tracer **不遗漏任何一次调用**——如果 `ext4_file_open` 被调用了 100 次，trace 中就有 100 行。

---

## 先看示例程序

[trace_funcs.c](trace_funcs.c) 做了两件事：

1. 100 次文件打开/写入/关闭 —— 触发大量的 `ext4_file_*` 内核函数调用
2. 10000 次 `getpid()` —— 触发 `__x64_sys_getpid` 系统调用函数

```bash
make && ./trace_funcs
```

---

## 实验开始

### 第一步：启用 function tracer 但不过滤——感受开销

```bash
# 清空 + 选择 function tracer
echo 'nop'      | sudo tee /sys/kernel/tracing/current_tracer
echo ""         | sudo tee /sys/kernel/tracing/trace
echo 'function' | sudo tee /sys/kernel/tracing/current_tracer

# 不设任何 filter——追踪所有 57000+ 个函数！
# 只追踪我们自己的进程，避免追踪整个系统
echo $(pgrep trace_funcs) | sudo tee /sys/kernel/tracing/set_ftrace_pid

echo 1 | sudo tee /sys/kernel/tracing/tracing_on
sudo ./trace_funcs
echo 0 | sudo tee /sys/kernel/tracing/tracing_on

# 看看缓冲区的状况
sudo cat /sys/kernel/tracing/trace | wc -l
```

> 没有 filter 的 function tracer 会记录**每一次函数调用**——几毫秒就能填满缓冲区。这就是为什么过滤（filtering）是使用 ftrace 的必备技能。

恢复默认状态：

```bash
echo 'nop' | sudo tee /sys/kernel/tracing/current_tracer
echo ""    | sudo tee /sys/kernel/tracing/set_ftrace_pid
```

### 第二步：用 set_ftrace_filter 聚焦目标函数

只看 `ext4_file_*` 函数——程序的主要内核活动：

```bash
echo 'nop'      | sudo tee /sys/kernel/tracing/current_tracer
echo ""         | sudo tee /sys/kernel/tracing/trace
echo 'function' | sudo tee /sys/kernel/tracing/current_tracer

# 过滤：只看 ext4_file_ 开头的函数
echo 'ext4_file_*' | sudo tee /sys/kernel/tracing/set_ftrace_filter

echo 1 | sudo tee /sys/kernel/tracing/tracing_on
sudo ./trace_funcs
echo 0 | sudo tee /sys/kernel/tracing/tracing_on

sudo cat /sys/kernel/tracing/trace | head -20
```

输出：

```
# tracer: function
#
# entries-in-buffer/entries-written: 1394/1394   #P:12
#
#           TASK-PID     CPU#  |||||  TIMESTAMP  FUNCTION
            sudo-1002599 [003] ..... 817068.191199: ext4_file_open <-do_dentry_open
            sudo-1002599 [003] ..... 817068.191210: ext4_file_getattr <-vfs_getattr_nosec
            sudo-1002599 [003] ..... 817068.191219: ext4_file_read_iter <-vfs_read
            sudo-1002599 [003] ..... 817068.191239: ext4_file_read_iter <-vfs_read
            sudo-1002599 [003] ..... 817068.191256: ext4_file_read_iter <-vfs_read
            sudo-1002599 [003] ..... 817068.191543: ext4_file_open <-do_dentry_open
   rs:main Q:Reg-905     [006] ..... 817068.191718: ext4_file_write_iter <-vfs_write
```

### 第三步：解读 function tracer 输出

```
ext4_file_open <-do_dentry_open
^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^
被调用的函数     调用者：谁调用了 ext4_file_open？
                 ← do_dentry_open 调用了 ext4_file_open
                  （do_dentry_open 是 VFS 层的"打开文件"入口）
```

**关键发现**：

1. **1394 条记录**——100 次文件操作产生了 1394 次 `ext4_file_*` 函数调用。每次文件操作不仅仅是 `open` + `write` + `close`，还有 `getattr`、`read_iter` 等辅助操作。
2. **调用者揭示了调用路径**——`ext4_file_open <- do_dentry_open` 说明文件打开走的是 VFS → ext4 路径。
3. **不同 CPU 核心上的并发**——`sudo` 进程在 CPU 3 上，`rs:main Q:Reg` 进程在 CPU 6 上。

### 第四步：set_ftrace_pid —— 只追踪一个进程

当系统中有几十个进程时，全局追踪会充满噪音。`set_ftrace_pid` 只追踪指定的 PID：

```bash
echo 'nop'      | sudo tee /sys/kernel/tracing/current_tracer
echo ""         | sudo tee /sys/kernel/tracing/trace
echo 'function' | sudo tee /sys/kernel/tracing/current_tracer
echo 'ext4_file_*' | sudo tee /sys/kernel/tracing/set_ftrace_filter

# 程序自己报告了 PID——我们只追踪这个 PID
# 在 trace_funcs.c 中：printf("PID = %d\n", getpid());
sudo ./trace_funcs &
PID=$!
echo $PID | sudo tee /sys/kernel/tracing/set_ftrace_pid

echo 1 | sudo tee /sys/kernel/tracing/tracing_on
wait $PID
echo 0 | sudo tee /sys/kernel/tracing/tracing_on

# 现在 trace 中只有 PID 匹配的进程
sudo cat /sys/kernel/tracing/trace | awk '{print $1}' | sort -u
# 输出应该只包含 trace_funcs 的 PID（没有其他进程的噪音）

echo "" | sudo tee /sys/kernel/tracing/set_ftrace_pid
echo 'nop' | sudo tee /sys/kernel/tracing/current_tracer
echo ""    | sudo tee /sys/kernel/tracing/set_ftrace_filter
```

### 第五步：通配符过滤技巧

`set_ftrace_filter` 支持多种匹配模式：

```bash
# 1. 前缀匹配
echo 'ext4_file_*' | sudo tee ...      # ext4_file_open, ext4_file_read_iter, ...

# 2. 后缀匹配
echo '*_open' | sudo tee ...           # ext4_file_open, vfs_open, ...

# 3. 精确匹配
echo 'ext4_file_open' | sudo tee ...   # 只要这一个函数

# 4. 模块过滤（func:mod:module_name）
echo 'func:mod:ext4' | sudo tee ...   # ext4 模块中的所有函数

# 5. 多个模式（换行分隔）
echo -e 'ext4_file_open\next4_file_write_iter' | sudo tee ...
```

---

## 📝 核心认知

### function tracer 的开销

```
追踪 0 个函数    → ~0% 开销
追踪 10 个函数   → ~1-2% 开销
追踪 100 个函数  → ~5-10% 开销
追踪所有 57000+  → ~50-80% 开销（几乎不可用）
```

> **黄金法则**：永远先设 `set_ftrace_filter` 再启用追踪。先追踪所有函数再过滤 = 系统几秒内可能卡死。

### function tracer vs function_graph

```
function：只记录函数名 + 调用者——轻量，适合"谁调用了这个函数？"
function_graph：记录入口+出口+耗时+子函数嵌套——信息量大，适合"为什么这么慢？"

本实验用的是 function。实验 03 讲 function_graph。
```

---

## 🧪 试试看

1. 把 `set_ftrace_filter` 设为 `*_lock`——看看你的程序触发了哪些锁操作？有多少次 `mutex_lock`？
2. 改 `trace_funcs.c`：把 100 次文件操作改成 1 次——trace 中 `ext4_file_*` 的条目从 1394 减少到了多少？每次文件操作平均触发多少次内核函数调用？
3. 把 filter 设为 `__x64_sys_getpid`——10000 次 `getpid()` 产生了多少条记录？为什么每个 syscall 只产生一条 `function` 记录？

---

## 命令小结

| 操作 | 命令 |
|------|------|
| 选择 function tracer | `echo 'function' \| sudo tee /sys/kernel/tracing/current_tracer` |
| 设置函数过滤器 | `echo 'ext4_file_*' \| sudo tee /sys/kernel/tracing/set_ftrace_filter` |
| 只看指定 PID | `echo <PID> \| sudo tee /sys/kernel/tracing/set_ftrace_pid` |
| 清除过滤器 | `echo "" \| sudo tee /sys/kernel/tracing/set_ftrace_filter` |
| 关闭追踪（恢复 nop） | `echo 'nop' \| sudo tee /sys/kernel/tracing/current_tracer` |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `set_ftrace_filter: Invalid argument` | 通配符没有匹配到任何函数 | 检查 `available_filter_functions` 中是否存在该函数 |
| trace 输出全是其他进程的 | 没设 `set_ftrace_pid` | 设置 PID 过滤 |
| 系统变卡/终端无响应 | function tracer 追踪了太多函数 | 立刻 `echo 'nop' > current_tracer` 关闭 |

---

## 下一步

function tracer 告诉你"哪些函数被调用了"。下一步 [实验 03：调用图与延迟](../03-function-graph/)——`function_graph` 不仅显示调用，还显示每次调用的**微秒级耗时**和完整的子函数嵌套。
