# 实验 02：内核事件追踪 —— 用 Perfetto 采集 ftrace 数据

> 🎯 **目标**：用 `linux.ftrace` 数据源采集多种内核事件；理解 tracepoint 事件 vs 函数追踪的区别；用 SQL 对比 Perfetto 和原生 ftrace 的输出。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 01](../01-hello-perfetto/)（Perfetto 基础）、[ftrace 模块](../../ftrace/)（ftrace 概念）

## Perfetto 的 ftrace 集成和原生 ftrace 的区别

在 ftrace 模块中，你直接操作 tracefs 文件：

```bash
echo 'function_graph' > current_tracer
echo 'ext4_file_*' > set_ftrace_filter
cat trace
```

Perfetto 的 `linux.ftrace` 数据源**封装了同样的底层机制**（读写 tracefs），但做了三件事：
1. **统一配置**——所有事件类型写在一个 config 文件中
2. **统一时间线**——ftrace 事件和其他数据源（CPU 采样、堆分析）在同一个时间轴上
3. **统一查询**——用 SQL 代替 `cat | grep | awk`

---

## 先看示例程序

[ftrace_demo.c](ftrace_demo.c) 分三个阶段触发不同类型的内核事件：

| 阶段 | 操作 | 触发的事件类型 |
|------|------|-------------|
| Phase 1 | 80 次文件创建/写入/关闭 | syscalls（openat, write, close）+ ext4（da_write_*） |
| Phase 2 | 10 次 fork/wait | sched（process_fork, sched_switch） |
| Phase 3 | 60 次文件操作 | 同 Phase 1 |

```bash
make && ./ftrace_demo
```

---

## 实验开始

### 第一步：写一个多事件 config

```bash
cat > ftrace_config.txt << 'EOF'
buffers: {
    size_kb: 4096
    fill_policy: RING_BUFFER
}
data_sources: {
    config {
        name: "linux.ftrace"
        ftrace_config {
            # 调度事件（高频——5 秒内会有数千条）
            ftrace_events: "sched/sched_switch"
            ftrace_events: "sched/sched_waking"
            ftrace_events: "sched/sched_process_fork"

            # 系统调用事件（中频——每次 syscall 触发一次）
            ftrace_events: "syscalls/sys_enter_openat"
            ftrace_events: "syscalls/sys_enter_close"

            # 文件系统事件（低频——只在真正写磁盘时触发）
            ftrace_events: "ext4/ext4_da_write_begin"
            ftrace_events: "ext4/ext4_da_write_end"
        }
    }
}
duration_ms: 4000
EOF
```

### 第二步：录制并查看事件分布

```bash
sudo lib/perfetto/tracebox -c ftrace_config.txt --txt -o ftrace_trace.perfetto-trace &
./ftrace_demo
wait
```

用 SQL 统计每种事件的数量：

```bash
echo 'SELECT name, count(*) as cnt
FROM ftrace_event
GROUP BY name
ORDER BY cnt DESC;' | lib/perfetto/trace_processor_shell ftrace_trace.perfetto-trace
```

输出：

```
name                 cnt
-------------------- --------------------
sched_switch                         8252
sched_waking                         4053
sys_enter_close                       939
sys_enter_openat                      465
sched_process_fork                     84
ext4_da_write_end                       1
ext4_da_write_begin                     1
```

### 第三步：解读事件的频率差异

| 事件 | 数量 | 为什么是这个数量？ |
|------|------|-------------------|
| `sched_switch` | 8,252 | **系统中最频繁的事件**——每次 CPU 从一个任务切换到另一个任务就触发一次。4 秒内 8252 次 = 每秒 ~2000 次调度切换（12 核系统） |
| `sched_waking` | 4,053 | 任务被唤醒（从睡眠队列移到就绪队列） |
| `sys_enter_close` | 939 | 每次 `close()` 触发一次——140 个文件 × open + close，加上系统自身的 close |
| `sys_enter_openat` | 465 | 每次 `open()` 触发一次 |
| `sched_process_fork` | 84 | 每次 `fork()` 触发一次——10 次 fork × 加上 tracebox 自身的子进程 |
| `ext4_da_write_begin` | **1** | 🔍 为什么只有 1 次？ |

### 第四步：为什么 ext4 事件这么少？

**关键发现**：你的程序调用了 140 次 `write()`，但 ext4 的 da_write 事件只触发了 1 次。原因：

```
程序：  write(fd, "data", 4)  × 140
              ↓
       页缓存（page cache）——数据写到了内存，不是磁盘！
              ↓
       内核说："数据在内存里，不用急着写磁盘"
              ↓
       第一个 write 触发 ext4_da_write_begin/end（分配磁盘空间）
       后续 139 个 write 直接写到已有的页缓存页——不触发 ext4 事件
```

> 这和 [ftrace 模块实验 03](../../ftrace/03-function-graph/) 中讨论的 `fsync` 场景一致——只有数据真正落到磁盘时才会触发块层和文件系统层事件。

**验证**：把 `write(fd, "data", 4)` 改成写 1MB 数据（超出页缓存阈值），再跑一次——ext4 事件数量会大幅增加。

### 第五步：用 SQL 关联事件 —— 找到 fork 的时间窗口

Perfetto 的杀手功能是**用 SQL 跨事件类型查询**。比如——找到所有 fork 事件前后 1ms 内的调度事件：

```bash
echo "
SELECT
    f.ts as fork_ts,
    s.name as sched_event,
    (s.ts - f.ts) / 1000.0 as delta_us
FROM ftrace_event f
JOIN ftrace_event s
    ON s.ts BETWEEN f.ts AND f.ts + 1000000  -- 1ms = 1,000,000 ns
WHERE f.name = 'sched_process_fork'
  AND s.name LIKE 'sched_%'
LIMIT 10;
" | lib/perfetto/trace_processor_shell ftrace_trace.perfetto-trace
```

> 这就是 Perfetto 的核心价值——**SQL 让你跨事件类型做关联分析**，这在本机 ftrace 中需要手动写复杂的 awk/sed 脚本。

### 第六步：和原生 ftrace 对比

对于同一组事件，用原生 ftrace 采集做对比：

```bash
# 原生 ftrace 方式（等价于 Perfetto 的 linux.ftrace 配置）
echo 'nop' | sudo tee /sys/kernel/tracing/current_tracer
echo 1 | sudo tee /sys/kernel/tracing/events/sched/sched_switch/enable
echo 1 | sudo tee /sys/kernel/tracing/events/sched/sched_waking/enable
echo 1 | sudo tee /sys/kernel/tracing/events/syscalls/sys_enter_openat/enable
echo 1 | sudo tee /sys/kernel/tracing/tracing_on
./ftrace_demo
echo 0 | sudo tee /sys/kernel/tracing/tracing_on
sudo cat /sys/kernel/tracing/trace | head -20
```

| 维度 | 原生 ftrace | Perfetto `linux.ftrace` |
|------|-----------|------------------------|
| 配置方式 | 逐个 `echo 1 > .../enable` | 一个 config 文件集中管理 |
| 输出格式 | 文本（每行一条） | protobuf 二进制 |
| 分析方式 | `cat` / `grep` / `awk` | SQL / Perfetto UI |
| 多数据源融合 | 不支持 | 核心能力 |
| 时间精度 | 微秒 | 纳秒 |

---

## 📝 核心认知

### ftrace 事件的三个层级（按频率）

```
高频事件（每秒数千次）：
  sched_switch, sched_waking, irq_handler_entry
  → 适合调度分析、CPU 时间线重建
  → 4 秒就能产生上万条——注意缓冲区大小

中频事件（每秒数十到数百次）：
  sys_enter_*, sys_exit_*（系统调用）
  → 适合追踪程序的外部行为（I/O、网络、内存）

低频事件（按需触发）：
  ext4_da_write_*, block_bio_queue, kmem_kmalloc
  → 只在特定条件下触发——不要期望每个 write 都触发一次
```

### Perfetto event 命名规则

```
ftrace 路径                            Perfetto config 中的写法
/sys/kernel/tracing/events/            ftrace_events:
  sched/sched_switch           →         "sched/sched_switch"
  syscalls/sys_enter_openat    →         "syscalls/sys_enter_openat"
  ext4/ext4_da_write_begin     →         "ext4/ext4_da_write_begin"
```

---

## 🧪 试试看

1. 在 config 中加入 `"syscalls/sys_enter_read"` 和 `"syscalls/sys_enter_write"`——它们的数量和 close/openat 的数量能对上吗？为什么 close 比 open 多那么多？（提示：系统启动时打开的文件也在这次 4 秒窗口内关闭了）
2. 把 `duration_ms` 改成 10000，不运行 `ftrace_demo`（只录系统空闲状态）——sched_switch 还有多少？这些"背景"事件说明了什么？
3. 用 SQL 查询 `ext4_da_write_begin` 的时间戳，然后在 Perfetto UI 中定位到那个时间点——你能看到和它相邻的其他事件吗？
4. 注释掉 `ftrace_config` 中的 `sched_switch`（减少高频噪音），重新录制——trace 文件小了多？SQL 查询变快了吗？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `tracebox -c cfg.txt --txt -o trace.file` | 录制 ftrace 事件 |
| `SELECT DISTINCT name FROM ftrace_event` | 查看 trace 中所有事件类型 |
| `SELECT name, count(*) ... GROUP BY name` | 统计各事件数量 |
| `SELECT ... FROM ftrace_event WHERE name='...'` | 过滤特定事件 |
| `JOIN ... ON ts BETWEEN ...` | 按时间关联不同事件 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 某些事件数量为 0 | 事件名拼写错误或内核不支持 | 检查 `/sys/kernel/tracing/events/<类别>/` 确认路径 |
| ext4 事件几乎没有 | write 被 page cache 吸收，没真正写磁盘 | 写更大的数据或加 `fsync` |
| `Failed to setup event triggers` | WSL2 不支持 synthetic events | 无视——不影响常规事件采集 |

---

## 下一步

你现在能用 Perfetto 采集和分析 ftrace 事件了。下一步 [实验 03：SQL 查询追踪数据](../03-trace-processor/)——深入学习 trace_processor 的 SQL 表结构，写出更复杂的性能分析查询。
