# 实验 03：SQL 查询追踪数据 —— 用 SQL 替代 grep

> 🎯 **目标**：掌握 `trace_processor_shell` 的核心 SQL 表结构；写出"找到耗时最长的系统调用"、"统计每个 CPU 的调度事件"这样的查询；用 SQL 做 perf 和 ftrace 做不到的跨事件关联分析。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 02](../02-ftrace-integration/)（ftrace 事件采集）

## 为什么 SQL？

在前面的实验中，你用 `grep` 和 `awk` 分析 ftrace 输出。但这有三个问题：

1. **不能跨表查询**——"找到 fork 事件前后 1ms 内的所有调度切换"需要复杂的 awk 脚本
2. **不能做聚合统计**——"每个 CPU 每秒的 context switch 数量"很难用 grep 计算
3. **不能按时间窗口分组**——"Phase A 和 Phase B 的事件量有什么不同？"

SQL 解决了所有这些问题——因为 Perfetto 把 trace 数据**导入为 SQLite 数据库**。

---

## 先看示例程序

[sql_demo.c](sql_demo.c) 有三个阶段，中间有 sleep 间隔方便在时间轴上区分：

```bash
make && ./sql_demo
```

---

## 实验开始

### 第一步：录制一个用于 SQL 练习的 trace

```bash
cat > sql_config.txt << 'EOF'
buffers: { size_kb: 4096 fill_policy: RING_BUFFER }
data_sources: {
    config {
        name: "linux.ftrace"
        ftrace_config {
            ftrace_events: "sched/sched_switch"
            ftrace_events: "sched/sched_waking"
            ftrace_events: "sched/sched_process_fork"
            ftrace_events: "syscalls/sys_enter_openat"
            ftrace_events: "syscalls/sys_enter_write"
            ftrace_events: "syscalls/sys_enter_close"
        }
    }
}
duration_ms: 4000
EOF

sudo lib/perfetto/tracebox -c sql_config.txt --txt -o sql_trace.perfetto-trace &
./sql_demo
wait
```

### 第二步：探索数据库结构

trace_processor 把 trace 数据映射为几十张 SQL 表。先看有哪些可用：

```bash
echo '.tables' | lib/perfetto/trace_processor_shell sql_trace.perfetto-trace
```

**核心表速查**：

| 表名 | 内容 | 典型查询 |
|------|------|---------|
| `ftrace_event` | 所有 ftrace 事件（名称、时间戳、CPU） | `SELECT name, count(*) FROM ftrace_event GROUP BY name` |
| `perf_sample` | CPU 采样（实验 04 会用） | `SELECT count(*) FROM perf_sample` |
| `slice` | 有开始和结束的事件（Binder、atrace） | `SELECT name, dur FROM slice ORDER BY dur DESC` |
| `counter` | 按时间变化的数值（CPU 频率、内存） | `SELECT ts, value FROM counter LIMIT 5` |
| `sched_slice` | 每个任务在 CPU 上的运行片段 | `SELECT dur FROM sched_slice WHERE dur > 10000000` |
| `thread` / `process` | 线程和进程信息 | `SELECT name FROM thread WHERE name != ''` |
| `raw` | 原始事件（通用表——ftrace 事件也在这里） | `SELECT name FROM raw GROUP BY name` |

### 第三步：基础查询练习

```bash
TP="lib/perfetto/trace_processor_shell sql_trace.perfetto-trace"
```

**查询 1：各事件的总数和占比**

```sql
SELECT
    name,
    count(*) as cnt,
    round(count(*) * 100.0 / (SELECT count(*) FROM ftrace_event), 1) as pct
FROM ftrace_event
GROUP BY name
ORDER BY cnt DESC;
```

**查询 2：每个 CPU 上的事件分布**

```sql
SELECT
    cpu,
    count(*) as events,
    count(DISTINCT name) as unique_types
FROM ftrace_event
GROUP BY cpu
ORDER BY events DESC;
```

> 如果某个 CPU 的事件数量远高于其他——可能是中断亲和性（IRQ affinity）导致的。

**查询 3：找到最繁忙的 1 毫秒**

```sql
SELECT
    (ts / 1000000000) as second,
    count(*) as events_per_ms
FROM ftrace_event
GROUP BY (ts / 1000000000)
ORDER BY events_per_ms DESC
LIMIT 10;
```

### 第四步：跨事件关联——SQL 的核心优势

**"找到每次 fork 之后 1ms 内的 sched_switch 事件数量"**

```sql
SELECT
    f.ts as fork_time_ns,
    count(s.ts) as switches_within_1ms
FROM
    (SELECT ts FROM ftrace_event WHERE name = 'sched_process_fork') f
LEFT JOIN
    (SELECT ts FROM ftrace_event WHERE name = 'sched_switch') s
    ON s.ts BETWEEN f.ts AND f.ts + 1000000  -- 1ms = 1,000,000 ns
GROUP BY f.ts
ORDER BY switches_within_1ms DESC;
```

> 这种"按时间窗口关联两种事件"的查询，用 grep/awk 需要几十行代码。SQL 一行 `BETWEEN` 就搞定了。

**"按阶段划分事件数量"——利用时间戳分桶**

```sql
-- 把 4 秒的追踪按每秒分组，看事件模式
SELECT
    (ts - (SELECT min(ts) FROM ftrace_event)) / 1000000000 as second,
    count(*) as total,
    count(CASE WHEN name LIKE 'sys_enter%' THEN 1 END) as syscalls,
    count(CASE WHEN name = 'sched_process_fork' THEN 1 END) as forks
FROM ftrace_event
GROUP BY second
ORDER BY second;
```

预期输出类似：

```
second  total   syscalls  forks
0       1200    150       0      ← Phase A: 大量 syscall，无 fork
1       3800    20        45     ← Phase B: fork 集中，syscall 很少
2       2400    400       2      ← Phase C: syscall 恢复，fork 几乎没了
```

> 这个查询让你在时间维度上"看到"程序的三个阶段——不需要 UI，纯 SQL。

### 第五步：用 `.dump` 导出为 SQLite 数据库

```bash
echo '.dump sql_analysis.db' | lib/perfetto/trace_processor_shell sql_trace.perfetto-trace
```

导出后可以用任何 SQLite 工具分析：

```bash
sqlite3 sql_analysis.db "SELECT name, count(*) FROM ftrace_event GROUP BY name"
```

---

## 📝 核心认知

### trace_processor 的 SQL 方言

它是 SQLite 的超集，支持标准 SQL + Perfetto 特有的函数。关键点：

- **时间戳单位是纳秒**（1ms = 1,000,000 ns）
- **`ts` 是单调递增的**（从 trace 开始算起）
- **表结构看 `.schema 表名`**

### SQL vs grep/awk 分析 ftrace 数据

```
任务                               grep/awk           SQL
────────────────────────────────────────────────────────────
统计每种事件的数量                   awk '{print $5}'    SELECT name, count(*)
                                   | sort | uniq -c     FROM ftrace_event
                                                       GROUP BY name

找最慢的系统调用                    无法（需要配对          SELECT ... FROM slice
                                   enter/exit）         WHERE dur > threshold

跨事件类型关联                      难以实现              JOIN ... ON ts BETWEEN

按时间窗口聚合                      极为复杂              GROUP BY ts / interval
```

---

## 🧪 试试看

1. 写一个 SQL 查询，找出 `sys_enter_openat` 事件之间的平均时间间隔（提示：用 `LAG()` 窗口函数）
2. 用 `SELECT * FROM raw WHERE name='sched_switch' LIMIT 1` 看原始事件的完整字段——除了 name 和 ts，还有哪些列？
3. 把 `ftrace_event` 中的事件按 CPU 分组后计算每个 CPU 的"空闲比例"（`sched_switch` 事件多的 CPU 更忙）
4. 导出为 SQLite 后用 `sqlite3` 打开——和 `trace_processor_shell` 的体验有什么不同？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `echo 'SQL' \| trace_processor_shell trace.file` | 执行单条 SQL |
| `.tables` | 列出所有可用表 |
| `.schema 表名` | 查看表结构 |
| `.dump file.db` | 导出为 SQLite 数据库 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `no such table: xxx` | 表名写错或该数据源没在 trace 中 | `.tables` 确认可用表 |
| SQL 查询超慢 | 对未索引的列做了 JOIN | 先用 `WHERE` 缩小范围 |
| 时间戳看起来很怪（巨大数字） | 单位是纳秒 | 除以 1,000,000,000 得秒 |

---

## 下一步

SQL 让你能精确查询 trace 中的任何数据。但如果想知道"哪个函数吃了最多 CPU"——需要 CPU profiling。下一步 [实验 04：CPU 耗时分析](../04-cpu-profiling/)。
