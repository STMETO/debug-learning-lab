# 实验 04：CPU 耗时分析 —— 用 Perfetto 做 CPU Profiling

> 🎯 **目标**：用 `linux.perf` 数据源做 CPU 采样（底层调用 `perf_event_open`）；在时间轴上同时看到 CPU 热点函数和内核事件；和 `perf record` 对比输出。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 02](../02-ftrace-integration/)（ftrace 事件）、[perf 模块](../../perf/)（perf record 概念）

## linux.perf 和 perf record 的关系

```
perf record：直接调用 perf_event_open → perf.data → perf report（CLI/TUI）

Perfetto 的 linux.perf：perf_event_open → .perfetto-trace → SQL / UI
                              ↑
                        同一个系统调用！
```

**关键区别**：Perfetto 把 CPU 采样数据和 ftrace 事件放在**同一条时间轴**上。这意味着你可以回答"当 CPU 采样显示热点在 `compute_loop` 时，同一时刻的 ftrace 事件显示发生了什么"。

---

## 先看示例程序

[cpu_demo.c](cpu_demo.c) 有两个热点函数和一个冷函数：

| 函数 | 操作 | 预期热点？ |
|------|------|----------|
| `compute_loop()` | 5000 万次乘加取模 | ✅ 热点 1 (~60%) |
| `divide_loop()` | 800 万次取模运算 | ✅ 热点 2 (~35%) |
| `cold_setup()` | 100 个元素的数组 | ❌ 冷函数 (< 1%) |

```bash
make && ./cpu_demo
```

---

## 实验开始

### 第一步：配置 linux.perf 数据源

```bash
cat > cpu_config.txt << 'EOF'
buffers: {
    size_kb: 8192
    fill_policy: RING_BUFFER
}
data_sources: {
    config {
        name: "linux.perf"
        perf_event_config {
            all_cpus: true              # 所有 CPU 都采样
            sampling_frequency: 100     # 每秒 100 次采样
        }
    }
}
duration_ms: 5000
EOF
```

**关键参数**：

| 参数 | 含义 | 建议值 |
|------|------|--------|
| `all_cpus: true` | 采样所有 CPU 核心 | 生产环境推荐 |
| `sampling_frequency: N` | 每秒 N 次采样 | 99（避免和其他定时器共振）/ 100-1000 |
| `target_cmdline: "name"` | 只采样匹配进程名（比 all_cpus 更精准） | 本地测试推荐 |
| `target_installed_by: "name"` | 按包名过滤（Android） | Linux 桌面不需要 |

### 第二步：录制 CPU profile

```bash
sudo lib/perfetto/tracebox -c cpu_config.txt --txt -o cpu_trace.perfetto-trace &
./cpu_demo
wait
```

输出关键行：

```
[712.398] StartDataSource(ds 1, session 1, name linux.perf)
=== CPU Profiling Demo ===
Phase 1: compute_loop (30M iterations)...
Phase 2: divide_loop (8M iterations, modulo-heavy)...
Phase 3: compute_loop again (20M iterations)...
[717.500] Wrote 192106 bytes into cpu_trace.perfetto-trace
```

> **188 KB，5 秒，~500 次采样**（100Hz × 5s = 500）。

### 第三步：SQL 分析 CPU profile

```bash
TP="lib/perfetto/trace_processor_shell cpu_trace.perfetto-trace"
```

**查询 1：采样总数和调用栈覆盖率**

```sql
SELECT
    (SELECT count(*) FROM perf_sample) as total_samples,
    (SELECT count(*) FROM stack_profile_callsite) as resolved_callsites,
    (SELECT count(*) FROM stack_profile_frame) as unique_frames;
```

输出：

```
total_samples  resolved_callsites  unique_frames
           ~500                191             163
```

> 不是所有采样都能解析出调用栈——内核态的采样需要内核调试符号。WSL2 中约 40% 的采样被解析。

**查询 2：内核态 vs 用户态采样分布**

```sql
SELECT
    cpu_mode,
    count(*) as samples,
    round(count(*) * 100.0 / (SELECT count(*) FROM perf_sample), 1) as pct
FROM perf_sample
GROUP BY cpu_mode;
```

**查询 3：每个 CPU 的采样分布**

```sql
SELECT
    cpu,
    count(*) as samples
FROM perf_sample
GROUP BY cpu
ORDER BY cpu;
```

> 如果某个 CPU 的采样数远低于其他——说明你的程序被调度到了特定核心。这和实验 02 中 `sched_switch` 的 CPU 分布可以交叉验证。

### 第四步：Perfetto UI 中的 CPU profile

在 `ui.perfetto.dev` 中打开 `cpu_trace.perfetto-trace`，你会看到：

```
┌─────────────────────────────────────────────────────────────┐
│  CPU 0  ██████████████████████████████████████████████████   │
│  CPU 1  ████████████████████████████                         │
│  ...                                                        │
│                                                             │
│  perf_samples  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·   │
│  (每个点 = 一次 CPU 采样)                                     │
│                                                             │
│  点击采样点 → 展开调用栈面板                                   │
└─────────────────────────────────────────────────────────────┘
```

### 第五步：linux.perf vs perf record —— 对比

对同一个 `cpu_demo` 程序，分别用两种工具分析：

```bash
# 方式 A：perf record（传统方式）
perf record -g -o perf.data ./cpu_demo
perf report --stdio | head -15

# 方式 B：Perfetto linux.perf
sudo lib/perfetto/tracebox -c cpu_config.txt --txt -o cpu_trace.perfetto-trace &
./cpu_demo && wait
```

| 维度 | perf record | Perfetto linux.perf |
|------|-----------|-------------------|
| 输出格式 | perf.data（perf 专用） | .perfetto-trace（protobuf，可跨工具） |
| 分析方式 | `perf report`（TUI） | SQL + Perfetto UI |
| 与其他数据融合 | 不可 | ✅ ftrace 事件可在同一时间轴上 |
| 调用栈解析 | 成熟（fp + dwarf） | 依赖内核符号，WSL2 部分受限 |
| 适合场景 | 专注 CPU 热点分析 | 多数据源关联分析 |

---

## 📝 核心认知

### 为什么 Perfetto 的 CPU profile 要和 ftrace 一起用？

单独看 CPU profile："`compute_loop` 占了 60% CPU"——这 `perf record` 也能告诉你。

但加上 ftrace 事件："`compute_loop` 占 60% CPU，而且**在它运行期间，没有 sched_switch 事件**——说明它一直独占 CPU，没有被调度出去"——这才是 Perfetto 的独特价值。

### linux.perf 的三个适用场景

```
1. 纯 CPU 热点 → perf record 就够了（更快更成熟）
2. CPU 热点 × 内核事件关联 → Perfetto linux.perf + linux.ftrace
3. CPU 热点 × 应用埋点关联 → Perfetto linux.perf + TRACE_EVENT（实验 06）
```

---

## 🧪 试试看

1. 把 `sampling_frequency` 从 100 改成 1000——采样数翻了 10 倍吗？trace 文件变大了多少？
2. 把 `cpu_demo.c` 中的 `divide_loop` 迭代次数从 8M 改成 80M——CPU profile 中 `divide_loop` 的采样占比变化了吗？
3. 同时采集 `linux.perf` + `linux.ftrace`（两个 data_sources block）——能在 SQL 中关联"`compute_loop` 的采样时间戳"和"同时间的 sched_switch 事件"吗？
4. 和 `perf record -F 100 ./cpu_demo && perf report` 的输出对比——两种工具对 CPU 热点的排名一致吗？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `perf_event_config { all_cpus: true sampling_frequency: 100 }` | 全核采样，100Hz |
| `SELECT cpu_mode, count(*) FROM perf_sample GROUP BY cpu_mode` | 内核态 vs 用户态 |
| `SELECT * FROM stack_profile_mapping` | 查看被采样的二进制文件 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `perf_samples_skipped: N` | 某些采样未能匹配目标进程 | 用 `target_cmdline` 缩小范围或调高频率 |
| 调用栈全是地址没有函数名 | 二进制被 strip 或缺少调试符号 | 加 `-g` 编译且不要 strip |
| `perf_sample` 表为空 | 采样频率太低或目标进程没运行 | 提高 `sampling_frequency`，确保程序在录制期间运行 |

---

## 下一步

CPU profiling 告诉你"哪些函数在吃 CPU"。如果是内存问题呢？下一步 [实验 05：内存分配分析](../05-heapprofd/)——用 heapprofd 追踪 `malloc/free`，不需要重启程序。
