# 实验 07：全栈延迟排查 —— 三数据源联合诊断

> 🎯 **目标**：综合使用 linux.ftrace + linux.perf + 应用埋点（Perfetto 统一采集），在一条时间轴上对齐所有数据源，找出三层性能瓶颈的根因。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：实验 01–06

---

## 场景设定

你接手了一个数据处理工具——[fullstack.c](fullstack.c)。程序报告"整体偏慢"，但没有崩溃、没有明显 bug。你的任务：用 Perfetto 的三种数据源找到**所有性能瓶颈**。

```
=== Full-Stack Performance Debugging ===

Phase 1: compute_stats (CPU hotspot)...
Phase 2: sync_files (triggers fsync → block layer)...
Phase 3: fork_workers (scheduling pressure)...

=== Done! ===
```

```bash
make && ./fullstack
```

> 💡 这个实验不给你详细步骤。你拥有整个工具链：ftrace 事件、CPU 采样、应用埋点（trace_marker）、SQL 查询、Perfetto UI。自己决定诊断顺序。

当然，如果卡住了，下面有完整的排查路径。

---

## 排查路径（参考）

### 阶段一：配置全栈追踪

一次录制同时开启三种数据源：

```bash
cat > fullstack_config.txt << 'EOF'
buffers: { size_kb: 16384 fill_policy: RING_BUFFER }

# 数据源 1：内核事件
data_sources: {
    config {
        name: "linux.ftrace"
        ftrace_config {
            ftrace_events: "sched/sched_switch"
            ftrace_events: "sched/sched_process_fork"
            ftrace_events: "syscalls/sys_enter_openat"
            ftrace_events: "syscalls/sys_enter_write"
            ftrace_events: "syscalls/sys_enter_fsync"
            ftrace_events: "syscalls/sys_enter_close"
            ftrace_events: "ext4/ext4_da_write_begin"
            ftrace_events: "ext4/ext4_da_write_end"
        }
    }
}

# 数据源 2：CPU 采样
data_sources: {
    config {
        name: "linux.perf"
        perf_event_config {
            all_cpus: true
            sampling_frequency: 100
        }
    }
}
duration_ms: 6000
EOF
```

```bash
sudo ../../lib/perfetto/tracebox -c fullstack_config.txt --txt -o fullstack.perfetto-trace &
./fullstack
wait
```

### 阶段二：从全局统计入手

```bash
TP="../../lib/perfetto/trace_processor_shell fullstack.perfetto-trace"
```

**第一步——看事件分布**：

```sql
SELECT name, count(*) as cnt
FROM ftrace_event
GROUP BY name ORDER BY cnt DESC;
```

```
sched_switch                       201,208   ← 6 秒内 20 万次调度切换！正常吗？
sys_enter_close                      5,572
sys_enter_openat                     5,000
sys_enter_write                        821
sched_process_fork                     107   ← fork 了 107 次
ext4_da_write_end                        1
ext4_da_write_begin                      1
```

> 20 万次 sched_switch 看起来很多，但在 12 核系统上 6 秒内：201208 / 6 / 12 ≈ 2800 次/秒/核心 → **正常水平**。

**第二步——看 CPU 采样**：

```sql
SELECT count(*) as perf_samples FROM perf_sample;
-- 结果：6931（100Hz × 6s × 12核 ≈ 7200，接近）

-- 内核态 vs 用户态
SELECT cpu_mode, count(*) FROM perf_sample GROUP BY cpu_mode;
```

### 阶段三：定位三个瓶颈

#### 🔍 瓶颈 1（CPU）：compute_stats

在 Perfetto UI 中打开 trace，找到 Phase 1 的时间段（应用埋点 `B|0|phase_compute` 附近）。

- **现象**：Phase 1 期间某个 CPU 核的利用率接近 100%
- **原因**：`compute_stats(80000000)` —— 8000 万次取模运算，纯 CPU 密集型
- **perf 数据**：CPU 采样中 compute_stats 是绝对热点
- **修复**：改用更高效的算法（如查表法替代取模），或并行化

#### 🔍 瓶颈 2（Kernel I/O）：sync_files

在 Phase 2（`B|0|phase_sync` 标记附近）：

- **现象**：`sys_enter_fsync` 事件 + `sys_enter_write` 事件出现
- **ftrace 数据**：虽然 ext4 事件只有 1-2 次，但 `fsync` 系统调用本身导致了明显的延迟
- **原因**：每次 `fsync` 强制将脏页刷到磁盘 → 等待块 I/O 完成 → 进程阻塞
- **修复**：减少 fsync 频率，或用 `fdatasync`（只刷数据不刷元数据）

#### 🔍 瓶颈 3（调度压力）：fork_workers

在 Phase 3（`B|0|phase_fork` 标记附近）：

- **现象**：`sched_process_fork` 事件 107 次 + `sched_switch` 大量增加
- **ftrace 数据**：30 次 fork 导致 107 次调度事件——每个 fork 触发 3-4 次调度相关事件
- **原因**：fork 创建新进程 → 内核分配 task_struct + 内核栈 → 调度器决定谁先运行
- **修复**：用线程池替代频繁 fork，或预创建 worker 进程

### 阶段四：验证修复

对每个瓶颈单独修复后，重新录制对比：

| 指标 | 修复前 | 修复 compute_stats | 修复 sync_files | 修复 fork_workers |
|------|--------|-------------------|----------------|------------------|
| 总耗时 | ~1.5s | ~0.5s | ~0.3s 减少 | ~0.2s 减少 |
| sched_process_fork | 107 | 未变 | 未变 | ~10 |
| sys_enter_fsync | ~10 | 未变 | 0 | 未变 |
| CPU 热点 | compute_stats | 消失 | 未变 | 未变 |

---

## 📝 排查方法论

```
全栈延迟排查 = 三个数据源 × 一个时间轴

1. 先看应用埋点 → 定位"哪个业务阶段慢"
   (phase_compute? phase_sync? phase_fork?)

2. 在该阶段内看 ftrace 事件 → "内核在做什么？"
   (在等 fsync? 在 fork? 在调度?)

3. 同时在时间轴上对齐 CPU 采样 → "CPU 在忙什么？"
   (热点函数是什么？内核态 vs 用户态比例?)

4. 确定根因 → 修复 → 重新录制验证
```

---

## 🧪 试试看

1. 注释掉 `fullstack.c` 中的 `fsync(fd)`——Phase 2 的耗时减少多少？`sys_enter_fsync` 事件消失了吗？
2. 把 `fork_workers(30)` 改成 `fork_workers(5)`——`sched_process_fork` 数量变化了吗？总耗时变化了吗？
3. 用 Perfetto UI 的 SQL 面板查询 `SELECT ts, dur FROM slice ORDER BY dur DESC LIMIT 5`——最慢的 slice 是什么？和你的预期一致吗？
4. 对比只用 `perf record` vs 只用 `ftrace` vs Perfetto 全栈——哪种方式让你最快找到根因？

---

## 命令小结

| 要点 | 内容 |
|------|------|
| 多数据源 config | 多个 `data_sources { }` block 并列 |
| 应用埋点 | `write(marker, "B\|0\|phase_name")` |
| ftrace 事件 | `ftrace_events: "sched/...", "syscalls/...", "ext4/..."` |
| CPU 采样 | `perf_event_config { sampling_frequency: 100 }` |
| SQL 事件统计 | `SELECT name, count(*) FROM ftrace_event GROUP BY name` |

---

## 下一步

perf、ftrace、Perfetto——你现在能独立使用这三种工具排查性能问题了。下一步 [实验 08：三工具全景对比](../08-compare-tools/)——系统化的选型决策框架。
