# ftrace —— 内核函数追踪器

## 写给谁？

本模块假设你完成了 [perf 模块](../perf/)（至少实验 01-06），理解"采样"和"tracepoint"的概念，知道内核中有大量可追踪的事件。

> perf 每隔 N 个周期拍一张快照——它告诉你"哪些函数比较热"，但看不到"每个函数被调用的精确时机和耗时"。ftrace 是**全量追踪**——不漏掉任何一次函数调用、任何一个事件。

## ftrace 是什么？

**ftrace**（Function Tracer）是 Linux 内核内置的追踪基础设施。它不像 perf 那样依赖硬件采样，而是在**每个被追踪的函数入口/出口**插入追踪代码。

```
perf record（采样）：        ftrace function_graph（全量追踪）：
                            
时间 →                      时间 →
 ·  ·  ·  ·  ·  ·          ├─ func_a() 0.5μs
 ·  ·  ·  ·  ·  ·          │  ├─ func_b() 1.2μs
 ·  ·  ·  ·  ·  ·          │  │  └─ func_c() 0.3μs
 ·  ·  ·  ·  ·  ·          │  └─ func_d() 0.8μs
 ·  ·  ·  ·  ·  ·          ├─ func_e() 3.1μs
   ↑ 统计推断              │  └─ func_f() 2.1μs
   热点占用 ~60%？          └─ func_g() 0.2μs
                              ↑ 精确：每个调用、每次耗时
```

### ftrace 和 perf 的互补关系

```
┌─────────────────────────────────────────────────────────┐
│  perf：宏观性能分析                                      │
│  "哪个函数吃 CPU？" "IPC 多少？" "缓存命中率如何？"       │
│  优势：低开销（~2%）、可长期运行                           │
├─────────────────────────────────────────────────────────┤
│  ftrace：微观时序分析                                    │
│  "这个函数被谁调用了？每次耗时多少？为什么有时慢？"         │
│  优势：100% 覆盖、微秒级精度                               │
│  代价：高开销（~20-50%）、不适合长期运行                    │
└─────────────────────────────────────────────────────────┘

使用决策：
  "程序慢" → perf stat 看宏观 → perf record/report 找热点
  "热点函数有时慢有时快，不知道为什么" → ftrace function_graph
  "想知道内核中完整的调用路径" → ftrace + trace-cmd
```

### ftrace 的核心机制：一切皆文件

```
┌───────────────────────────────────────────────────┐
│  /sys/kernel/tracing/  (tracefs)                  │
│                                                   │
│  current_tracer    ← 选择 tracer（function/       │
│                       function_graph/nop）         │
│  set_ftrace_filter ← 设置要追踪的函数              │
│  set_ftrace_pid    ← 只追踪指定进程                │
│  trace             ← 读取追踪结果                  │
│  trace_pipe        ← 实时流式读取                  │
│  trace_marker      ← 用户态写入自定义标记          │
│  events/           ← 启用/禁用 tracepoint 事件     │
│  options/          ← 微调控追踪行为                 │
└───────────────────────────────────────────────────┘
```

> 你不需要安装任何软件——ftrace 是内核的一部分。你直接读写这些文件就能控制追踪。当然，`trace-cmd` 让这个操作更友好。

---

## 学习路线图

### 🟢 阶段一：初次接触（实验 01–02）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [01](01-hello-ftrace/) | Hello ftrace | 浏览 tracefs、用 `trace_marker` 写自定义标记、读取 `trace` 输出、理解追踪条目的格式 | ~10min |
| [02](02-function-tracer/) | 函数追踪 | 启用 `function` tracer、用 `set_ftrace_filter` 过滤函数、只追踪一个进程（`set_ftrace_pid`）、理解"全量追踪"的开销 | ~15min |

> 学完这两个实验，你能看到"内核中发生了什么"——每一个被追踪的函数调用都被记录。

### 🟡 阶段二：核心技能（实验 03–04）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [03](03-function-graph/) | 调用图与延迟 | `function_graph` tracer——看到函数入口+出口+耗时、`+` 标记（耗时 > 10μs）、`!` 标记（耗时 > 100μs）、用调用图找出"哪个子函数最慢" | ~20min |
| [04](04-trace-cmd/) | trace-cmd 实战 | `trace-cmd record` 录制、`trace-cmd report` 回放、`trace-cmd stream` 实时查看、`trace-cmd start/stop` 后台追踪、`KernelShark`（可选 GUI） | ~20min |

> 学完这阶段，你能用 ftrace 找出内核路径中的延迟热点——"为什么 `open()` 有时要 50ms？因为 `ext4_lookup` 那次花了 48ms。"

### 🟠 阶段三：生产技能（实验 05–07）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [05](05-events-combined/) | 事件与复合追踪 | 启用 tracepoint 事件（`events/sched/`、`events/syscalls/`）、事件过滤器（`set_event_filter`）、同时追踪函数调用 + tracepoint 事件、`trace_printk` 的条件输出 | ~20min |
| [06](06-real-world/) | 实战：定位延迟抖动 | 给定一个"间歇性慢"的程序，综合使用 function_graph + trace-cmd + events——找出"为什么 1000 次调用里有一次慢了 100 倍" | ~20min |
| [07](07-ring-buffer/) | 环形缓冲区调优 | `buffer_size_kb` 扩容、多核独立缓冲、`tracing_cpumask` 绑定指定 CPU、识别缓冲区溢出丢包特征、长时间追踪防丢失配置方案 | ~18min |

> 到这里，你能独立排查内核路径中的延迟抖动，并正确配置缓冲区防止丢数据。

### 🔴 阶段四：深度进阶（实验 08–10）

| # | 实验 | 你会学到 | 时长 | 环境要求 |
|---|------|---------|------|----------|
| [08](08-trace-printk/) | trace_printk 陷阱与规范 | `trace_printk` 运行时开销原理、高频打印阻塞内核案例、生产禁用策略、`trace_marker` 替代方案、条件打印减少追踪负载 | ~15min | ✅ WSL2 / 原生 Linux |
| [09](09-filter-mastery/) | 黑白名单组合滤波 | `set_ftrace_filter` 白名单、`set_ftrace_notrace` 反向黑名单、`set_graph_function` 限制 graph 追踪入口、模块过滤 `func:mod:*`、通配符批量筛选降噪 | ~20min | ✅ WSL2 / 原生 Linux |
| [10](10-mechanism-choice/) | 动态与静态追踪机制抉择 | 静态 tracepoint（编译埋点低开销）vs 动态 kprobe/uprobe（无需重编内核）选型对比；`function_graph` 全量追踪 vs `perf` 采样取舍；不同业务场景选型决策流程 | ~22min | ⚠️ WSL2 / 原生 Linux（kprobe 仅原生完整支持） |

> 这四个实验是"从会用工具到理解原理"的跃迁——你知道的不只是"怎么用 ftrace"，还有"什么时候该用 ftrace，什么时候不该用"。

---

## 开始之前

### ⚠️ 环境要求

ftrace 是内核功能，**不需要安装任何软件包**（trace-cmd 可选但强烈推荐）：

| 环境 | 状态 | 说明 |
|------|------|------|
| **WSL2 原生 shell** | ✅ 推荐 | tracefs 已挂载，57,633 个函数 + 2,422 个事件可用；实验 01-09 全部支持，实验 10 的 kprobe 部分需原生 Linux |
| **Docker 容器** | ❌ 不支持 | 容器内无法访问 `/sys/kernel/tracing/` |
| **原生 Linux / 虚拟机** | ✅ 全功能 | 所有 tracer、事件和 kprobe 可用 |

### 你需要什么

```bash
# 1. 确认 tracefs 已挂载
mount | grep tracefs
# 应该看到：tracefs on /sys/kernel/tracing type tracefs (rw,...)

# 2. 确认可用的 tracer
cat /sys/kernel/tracing/available_tracers
# 至少应该看到：function_graph function nop

# 3. 安装 trace-cmd（强烈推荐，但不是必须）
sudo apt install trace-cmd

# 4. 大部分操作需要 sudo（追踪内核需要 root 权限）
sudo -i  # 或者每条命令前加 sudo
```

### ftrace vs perf vs strace 定位速查

```
你要排查的问题是...

"哪个函数吃 CPU？"                         → perf record/report
"函数每次调用的耗时分布？"                   → ftrace function_graph
"系统调用成功了还是失败了？"                 → strace / perf trace
"为什么有时 open() 很快有时很慢？"            → ftrace + ext4:* events
"长时间追踪担心缓冲区溢出丢数据？"              → 实验 07 环形缓冲区调优
"trace_printk 会影响性能吗？生产能用吗？"       → 实验 08 trace_printk 陷阱与规范
"追踪结果噪音太多，只想看关键函数？"             → 实验 09 黑白名单组合滤波
"什么时候用 ftrace，什么时候用 perf，什么时候用 kprobe？" → 实验 10 机制抉择
"内核中是谁调用了这个函数？"                  → ftrace function_graph + func_stack_trace
"生产环境持续监控，不能有高开销"              → perf（~2% 开销）
"开发/测试环境，需要 100% 精确的调用轨迹"     → ftrace（~20-50% 开销，但完整）
```

### 关键概念速览

| 术语 | 一句话解释 |
|------|-----------|
| **tracefs** | ftrace 的文件系统接口——所有控制都在 `/sys/kernel/tracing/` 目录下 |
| **tracer** | 追踪引擎——`function`（只记录函数名）、`function_graph`（记录调用图+耗时）、`nop`（关闭） |
| **trace_marker** | 用户态向 trace 缓冲区写入标记的文件——类似代码中的 `printf` 但打到 ftrace 缓冲区 |
| **set_ftrace_filter** | 白名单——只追踪列表中的函数。支持通配符：`ext4_*`、`*_lock` |
| **set_ftrace_pid** | 只追踪指定 PID——避免追踪整个系统的噪音 |
| **trace vs trace_pipe** | `trace` 是一次性读取，`trace_pipe` 是阻塞式流式读取（类似 `tail -f`） |
| **trace-cmd** | ftrace 的命令行前端——把文件操作封装成 `trace-cmd record/report/stream` |

---

## 实验目录结构

```
03-function-graph/
├── README.md      # 详细教程
├── demo.c         # 触发内核函数调用的示例程序
└── Makefile
```

> ⚠️ ftrace 实验的特殊性：大部分操作在 shell 中直接进行（读写 tracefs 文件、运行 trace-cmd）。C 示例程序是用来"触发内核行为"的——比如打开文件、分配内存——让 ftrace 有东西可追踪。

---

## 阅读 ftrace 输出的黄金法则

```
1. 看格式     → function tracer：  CPU#  [PID]  TIMESTAMP:  FUNCTION <- ARGS
               → function_graph：   DURATION |                  FUNCTION() {  (入口)
                                    DURATION |                  } /* FUNCTION */ (出口)

2. 看标记     → + 表示耗时 > 10μs（注意）
               → ! 表示耗时 > 100μs（异常！）
               → # 表示耗时 > 1000μs（严重！）

3. 找异常     → 在 function_graph 输出中搜索 `!` 和 `#`
               → 找到后往上看——它的子函数中谁占比最大？

4. 验假设     → 修改代码 → 重新追踪 → 延迟抖动消失了吗？
```

---

## 遇到问题？

- 每个实验的 README 末尾有"常见错误"小节
- `cat /sys/kernel/tracing/README` — 内核自带的 ftrace 文档（~300 行，非常详尽）
- ftrace 的内核文档：https://www.kernel.org/doc/html/latest/trace/ftrace.html
- 注意：ftrace 的高开销意味着它不适合在生产环境中长期运行——先用 perf 缩小范围，再用 ftrace 精确分析

---

<p align="center">
  <b>准备好了吗？从 <a href="01-hello-ftrace/">实验 01：Hello ftrace</a> 开始 👇</b>
</p>
