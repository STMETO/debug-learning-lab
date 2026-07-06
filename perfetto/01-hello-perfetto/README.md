# 实验 01：Hello Perfetto —— 录制第一条全栈追踪

> 🎯 **目标**：用 `tracebox` 录制第一条 Perfetto trace、理解 config 文件结构、用 SQL 查看事件统计、在 Perfetto UI 中打开并浏览时间轴。
>
> ⏱️ 约 15 分钟

## Perfetto 和已有工具的本质区别

你已经用过 `perf record` 和 `ftrace`。它们的输出是什么？

```
perf record → perf.data（二进制，只有 perf report 能读）
ftrace      → /sys/kernel/tracing/trace（文本，cat + grep）
```

Perfetto 的输出是一个 `.perfetto-trace` 文件——它包含**完整的时间轴**，可以用 **SQL 查询**，可以在**浏览器**中可视化。

```
tracebox（一行命令）→ .perfetto-trace → SQL 分析 / 浏览器 UI
```

---

## 先看示例程序

[hello.c](hello.c) 做文件 I/O——在 Perfetto 录制期间生成内核事件：

```bash
make && ./hello
```

---

## 实验开始

### 第一步：确认工具就绪

```bash
# tracebox 是 all-in-one 工具（内嵌 traced + traced_probes + perfetto CLI）
../../lib/perfetto/tracebox --version
# Perfetto v57.1

# trace_processor_shell 是 SQL 分析引擎
../../lib/perfetto/trace_processor_shell --version
# Perfetto v57.1
```

> `tracebox` 不需要单独启动守护进程——它把 `traced`、`traced_probes` 和 `perfetto` CLI 打包在一起，一行命令完成录制。

### 第二步：写第一个 trace config

Perfetto 用 **protobuf text format**（`.txt` 配置文件）描述"追踪什么"：

```bash
cat > ./hello_config.txt << 'EOF'
# Perfetto trace config —— 告诉 tracebox：追踪什么、追踪多久
buffers: {
    size_kb: 4096                     # 4 MB 环形缓冲区
    fill_policy: RING_BUFFER          # 满了覆盖旧数据
}
data_sources: {
    config {
        name: "linux.ftrace"          # 数据源：内核 ftrace 事件
        ftrace_config {
            ftrace_events: "sched/sched_switch"     # 调度切换
            ftrace_events: "sched/sched_waking"     # 任务唤醒
            ftrace_events: "syscalls/sys_enter_openat"  # 文件打开
            ftrace_events: "syscalls/sys_enter_write"   # 文件写入
            ftrace_events: "syscalls/sys_enter_close"   # 文件关闭
        }
    }
}
duration_ms: 5000                    # 录制 5 秒
EOF
```

**config 结构解读**：

```
buffers { ... }       ← 缓冲区配置（大小、策略）
data_sources { ... }  ← 数据源配置（每个数据源一个 block，可以多个）
duration_ms: 5000     ← 录制时长（毫秒），不指定则手动 Ctrl+C 停止
```

### 第三步：录制第一条 trace

```bash
# tracebox 在后台录制 5 秒，同时运行我们的程序
sudo ../../lib/perfetto/tracebox -c hello_config.txt --txt -o hello.perfetto-trace &
./hello
wait
```

输出关键行：

```
[531.008] Connected to the Perfetto traced service, TTL: 5s
[531.338] enabled ftrace in /sys/kernel/tracing/
=== Hello Perfetto ===
Batch 1: 20 file ops...
Batch 2: 100 file ops...
Batch 3: 50 file ops...
=== Done! ===
[536.081] Wrote 300932 bytes into hello.perfetto-trace
```

> **294 KB，5 秒录制**。这就是一条完整的内核追踪数据。

### 第四步：用 SQL 查看事件统计

```bash
echo 'SELECT name, count(*) as cnt
FROM ftrace_event
GROUP BY name
ORDER BY cnt DESC;' | ../../lib/perfetto/trace_processor_shell hello.perfetto-trace
```

输出：

```
name                 cnt
-------------------- --------------------
sched_switch                        11365
sched_waking                         5609
sys_enter_close                      1203
sys_enter_openat                      661
sys_enter_write                       355

Query executed in 2.794 ms
```

**SQL 结果解读**：

| 事件 | 数量 | 为什么这么多？ |
|------|------|---------------|
| `sched_switch` | 11,365 | 每次 CPU 调度切换都记录——系统中最频繁的事件 |
| `sched_waking` | 5,609 | 任务被唤醒的次数 |
| `sys_enter_close` | 1,203 | 170 次文件操作 × (open + 一些系统调用) |
| `sys_enter_openat` | 661 | 每次 `open()` 触发一次（加上一些库调用） |
| `sys_enter_write` | 355 | 每次 `write()` 触发一次 |

> 5 秒的录制、一个简单的程序，就产生了 19,193 个事件。Perfetto 把这些全部以微秒精度记录在时间轴上。

### 第五步：在 Perfetto UI 中可视化

```bash
# 1. 把 .perfetto-trace 文件传到你本机（如果你在 WSL2 中）
#    → 复制到 Windows 侧：
cp hello.perfetto-trace /mnt/c/Users/<你的用户名>/Desktop/

# 2. 打开浏览器，访问 https://ui.perfetto.dev
# 3. 点击 "Open trace file" 或直接拖入 hello.perfetto-trace
```

在 UI 中你会看到：

```
┌─────────────────────────────────────────────────────────────┐
│ 时间轴（可缩放、拖拽）                                        │
│                                                             │
│  CPU 0  ██████████████████████████████████████████████████   │
│  CPU 1  ████████████████████████████████████                 │
│  ...                                                        │
│                                                             │
│  sched_switch  ────────────────────────────────────────     │
│  sched_waking  ────────────────────────────────────────     │
│  sys_enter_openat  ──  ──  ──  ──  ──  ──  ──              │
│                                                             │
│  按 w/s 缩放 ｜ 按 a/d 平移 ｜ 点击事件看详情                  │
└─────────────────────────────────────────────────────────────┘
```

**UI 基本操作**：

| 操作 | 快捷键 |
|------|--------|
| 缩放 | `w` / `s` 或鼠标滚轮 |
| 左右平移 | `a` / `d` 或拖拽 |
| 点击事件 | 展开详情面板（时间戳、CPU、线程、参数） |
| 搜索事件 | Ctrl+F → 输入事件名 |
| SQL 查询 | 左侧边栏 → "Query (SQL)" 标签 |

---

## 📝 核心认知

### Perfetto 的三层架构

```
┌──────────────────────────────────────────┐
│  tracebox                                │  ← 录制（一行命令）
│  (内嵌 traced + probes + CLI)            │
├──────────────────────────────────────────┤
│  .perfetto-trace 文件                    │  ← 传输/存储
├──────────────────────────────────────────┤
│  trace_processor_shell / Perfetto UI     │  ← 分析/可视化
└──────────────────────────────────────────┘
```

### Perfetto vs ftrace 的操作对比

```
ftrace：                               Perfetto：
echo function_graph > current_tracer   → 写在 config 文件里
echo ext4_* > set_ftrace_filter        → ftrace_events: "ext4/ext4_file_open"
echo 1 > tracing_on                    → duration_ms: 5000（自动启停）
./program                              → ./program（一样）
cat trace                              → SQL: SELECT * FROM ftrace_event
                                       → UI: 时间轴拖拽缩放
```

---

## 🧪 试试看

1. 把 `duration_ms` 从 5000 改成 10000——事件数量翻倍了吗？为什么 sched_switch 的增长不是线性的？
2. 把 `ftrace_events` 列表中加入 `"sched/sched_wakeup_new"`——你能在 SQL 结果中看到这个新事件吗？它的数量和 `sched_waking` 有什么不同？
3. 在 Perfetto UI 中打开 trace 文件，用 Ctrl+F 搜索 `sys_enter_openat`——你能在时间轴上找到我们的程序执行文件操作的时间窗口吗？
4. 比较 `tracebox`（all-in-one）和分开使用 `traced` + `perfetto` 的区别——为什么教程推荐 tracebox？（提示：看实验开始时的背景测试结果）

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `../../lib/perfetto/tracebox --version` | 确认工具版本 |
| `sudo ../../lib/perfetto/tracebox -c config.txt --txt -o output.trace` | 录制 trace |
| `echo 'SELECT ...' \| ../../lib/perfetto/trace_processor_shell trace.file` | SQL 查询 |
| `echo '.tables' \| ../../lib/perfetto/trace_processor_shell trace.file` | 列出所有表 |
| Perfetto UI `ui.perfetto.dev` | 浏览器可视化 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `perfetto_cmd.cc:676 The trace config is invalid` | 缺少 `--txt` 标志 | 加上 `--txt`（表示 config 是文本格式） |
| `.perfetto-trace` 只有几百字节、SQL 查询为空 | `traced` 没有正确启动或权限不足 | 用 `sudo tracebox`（all-in-one 最可靠） |
| `Failed to setup event triggers` | WSL2 不支持某些 ftrace trigger | 无视——不影响事件采集 |
| Perfetto UI 打不开 trace | 文件路径包含非 ASCII 或浏览器安全限制 | 拖入文件，或用 `--httpd` 本地服务 |
| `tracebox: command not found` | 不在 PATH 中 | 用 `../../lib/perfetto/tracebox`（相对/绝对路径） |

---

## 下一步

你录下了第一条 Perfetto trace，用 SQL 统计了事件，在 UI 中看到了时间轴。下一步 [实验 02：内核事件追踪](../02-ftrace-integration/)——深入配置 `linux.ftrace` 数据源，精确筛选事件，对比 Perfetto 和原生 ftrace 的输出。
