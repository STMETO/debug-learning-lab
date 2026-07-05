# 实验 10：动态与静态追踪机制抉择 —— 什么时候用什么工具

> 🎯 **目标**：静态 tracepoint vs 动态 kprobe/uprobe 选型对比；`function_graph` 全量追踪 vs `perf` 采样取舍；不同业务场景的选型决策流程。
>
> ⏱️ 约 22 分钟
>
> 📖 **前置**：ftrace 实验 01-09 + [perf 模块](../../perf/) 实验 01-10
>
> ⚠️ **环境**：kprobe 在 WSL2 内核中部分受限。本实验重点在概念对比和决策框架，perf probe 用户态示例在 WSL2 可运行。

## 全景：四种追踪机制

你对 Linux 程序的追踪能力来自四个层次——它们各有所长，互不替代：

```
┌─────────────────────────────────────────────────────────────────┐
│                    四种追踪机制全景                               │
│                                                                 │
│  静态 tracepoint    动态 kprobe      function_graph    perf采样  │
│  ──────────────    ────────────     ─────────────    ────────   │
│  内核预埋追踪点     运行时动态插桩    全量函数调用图    统计采样   │
│                                                                 │
│  开销: ★☆☆☆☆       开销: ★★☆☆☆      开销: ★★★★☆      开销: ★☆☆☆☆ │
│  覆盖: ★★★☆☆       覆盖: ★★★★★      覆盖: ★★★★★      覆盖: ★★★★☆ │
│  精度: ★★★★★       精度: ★★★★★      精度: ★★★★★      精度: ★★★☆☆ │
│  稳定: ★★★★★       稳定: ★★☆☆☆      稳定: ★★★★☆      稳定: ★★★★★ │
└─────────────────────────────────────────────────────────────────┘
```

---

## 先看示例程序

[compare.c](compare.c) 是一个简单的文件操作程序——你可以用四种不同的机制来追踪它：

```bash
make && ./compare
```

---

## 实验开始

### 第一步：静态 tracepoint —— 内核的"官方 API"

```bash
# 使用 syscalls tracepoint —— 所有系统调用的进入/退出
sudo trace-cmd record -e 'syscalls:sys_enter_openat' -e 'syscalls:sys_enter_write' \
    -e 'syscalls:sys_enter_close' -o tracepoint.dat ./compare

sudo trace-cmd report -i tracepoint.dat | grep 'syscalls:' | head -10
```

**tracepoint 的特征**：

| 优点 | 缺点 |
|------|------|
| ✅ 开销极低——每个 tracepoint 只是一条条件跳转 | ❌ 数量固定——内核中有就有，没有就没有 |
| ✅ ABI 稳定——内核保证 tracepoint 的字段名不变 | ❌ 只有内核开发者觉得"值得埋"的地方才有 |
| ✅ 生产环境安全——可以长期开启 | ❌ 粒度粗——只能看到"进入系统调用"，看不到内核函数细节 |

### 第二步：动态 kprobe —— "任何地方都能插探针"

```bash
# 在内核函数 ext4_file_open 上动态插入探针（需要原生 Linux）
sudo perf probe --add='ext4_file_open inode=%di' 2>&1

# WSL2 替代方案——在用户态函数上插探针
sudo perf probe -x ./compare --add='main' 2>&1

# 采集探针事件
sudo perf record -e 'probe_compare:main' -o probe.dat ./compare
sudo perf script -i probe.dat | head -5
```

**kprobe 的特征**：

| 优点 | 缺点 |
|------|------|
| ✅ 任何内核函数都能探测——57,633 个可选 | ❌ 不稳定——内核函数名和参数可能随版本变化 |
| ✅ 运行时可插拔——不需要重启、不需要重编译 | ❌ 开销比 tracepoint 高 |
| ✅ 能捕获函数参数和返回值 | ❌ WSL2 内核支持不完整 |
| ✅ 紧急排查时不需要重新部署 | ❌ 生产环境建议用完即删 |

### 第三步：function_graph 全量追踪 —— "不漏掉任何调用"

```bash
sudo trace-cmd record -p function_graph -g 'ext4_file_*' -o fgraph.dat ./compare
sudo trace-cmd report -i fgraph.dat | grep 'ext4_file_' | head -15
```

**function_graph 的特征**：

| 优点 | 缺点 |
|------|------|
| ✅ 100% 覆盖——每次调用都记录 | ❌ 高开销——每个函数入口/出口都要写缓冲区 |
| ✅ 精确耗时——微秒级 | ❌ 缓冲区压力大——高频函数容易溢出 |
| ✅ 完整调用栈——谁调用了谁一目了然 | ❌ 不适合长期运行——只能短期采样 |
| ✅ 不需要任何代码改动 | ❌ 输出量大——需要强大的过滤和后期处理 |

### 第四步：perf 采样 —— 统计推断

```bash
sudo perf record -g -o perf.dat ./compare
sudo perf report -i perf.dat --stdio | head -20
```

**perf 采样的特征**：

| 优点 | 缺点 |
|------|------|
| ✅ 极低开销——默认每 3500 cycles 采一次样 | ❌ 统计误差——小函数可能被漏掉 |
| ✅ 适合长期运行——生产环境可用的开销水平 | ❌ 看不到"每次调用的耗时"——只有统计分布 |
| ✅ 硬件事件——cache-miss、branch-miss 等与性能直接相关 | ❌ 看不到调用参数——只知道"这个函数热" |
| ✅ 火焰图可视化——宏观视角极好 | ❌ 调用链偶尔缺失——采样可能错过叶子函数 |

### 第五步：四种机制的耗时对比

对同一个程序（200 次文件操作），用四种机制追踪：

| 机制 | 运行耗时（典型） | 相对开销 | 数据量 | 得到的信息 |
|------|---------------|---------|--------|----------|
| 无追踪（基线） | ~50ms | 0% | 0 | — |
| tracepoint (`syscalls:*`) | ~55ms | ~10% | ~2,000 条 | 系统调用次数和时序 |
| kprobe (1 个探针) | ~60ms | ~20% | ~200 条 | 特定函数的每次调用参数 |
| function_graph (`ext4_file_*`) | ~200ms | ~300% | ~50,000 条 | 每个函数调用的完整嵌套图 + 耗时 |
| perf record | ~55ms | ~10% | ~500 样本 | 热点函数分布 + 调用栈统计 |

> 这不是一个"哪个更好"的问题——是**用对场景**的问题。

### 第六步：选型决策框架

```
排查问题的决策树：

1. 先判断问题的类型：
   ├── 程序偶尔崩溃？
   │   → GDB + core dump（能看到崩溃现场的所有变量）
   │
   ├── 内存问题？
   │   → ASan / Valgrind（heap overflow, use-after-free, leak）
   │
   ├── 功能问题（"为什么返回 -1？"）？
   │   → strace / perf trace（看系统调用的返回值）
   │
   └── 性能问题（"为什么慢？"）？
       ├── 宏观："哪个函数吃 CPU？" → perf record/report（~10% 开销）
       │
       ├── 中观："这个函数为什么慢？" → perf annotate（看源码/汇编占比）
       │
       ├── 微观："每次调用分别花了多久？" → function_graph（~300% 开销）
       │   └── "谁调用了它？" → set_graph_function + trace_marker
       │
       ├── 统计："缓存命中率、IPC？" → perf stat（几乎无开销）
       │
       ├── 事件："系统调用在哪些时间点发生？" → tracepoint events
       │
       └── 临时："内核里某个特定函数被调用了多少次？"
           → kprobe（可运行时插拔，用完即删）
```

### 第七步：真实场景的选型示例

| 场景 | 推荐工具链 | 原因 |
|------|----------|------|
| "新上线的服务 CPU 突然飙到 100%" | `perf record -g` → 火焰图 | 低开销，能跑在生产环境，能快速定位热点函数 |
| "本地测试时某个操作偶尔卡 100ms" | `trace-cmd record -p function_graph -g 'ext4_file_*'` | 高精度，全量覆盖，不怕漏掉偶发事件 |
| "不知道为什么 `open()` 有时候返回慢" | ftrace function_graph + `ext4:*` tracepoint events | 同时看到函数调用和系统事件，按时间戳建立因果链 |
| "想知道生产环境有没有内存泄漏" | `perf stat -e page-faults -I 1000` + 长时间监控 | 极低开销，可以跑几天 |
| "怀疑自研内核模块中有 bug" | `perf probe` 动态插桩 → `perf script` | 不需要重编内核，用完即删 |
| "Locust 压测时发现某 API p99 突然升高" | `perf record` 缩小范围 → `function_graph` 精确采样 | 先用低开销工具找热点，再用高精度工具深入 |

---

## 📝 核心认知

### ftrace 和 perf 的关系——不是替代而是互补

```
perf 擅长回答：                    ftrace 擅长回答：
─────────────────────            ─────────────────────
"谁在吃 CPU？"                   "每次调用花了多久？"
"缓存命中率如何？"                "谁调用了这个函数？"
"哪个指令最热？"                  "为什么有时快有时慢？"
"长期趋势是怎样的？"              "慢的那次调用走了哪条路径？"

perf = 宏观统计（快、粗、安全）     ftrace = 微观分析（慢、细、精确）
```

### 一句话选型口诀

> **统计用 perf，精确用 ftrace，事件用 tracepoint，临时用 kprobe，交互用 GDB。**

---

## 🧪 试试看

1. 对 `compare.c` 分别用 perf record 和 function_graph 追踪——perf report 的热点函数排名和 function_graph 中耗时最长的函数一致吗？
2. 用 `perf probe -x ./compare --add='work_loop tag=%di count=%si'`（WSL2 可行）——捕获到了 `work_loop` 的每次调用参数吗？和 tracepoint 看到的有什么不同？
3. 假设你要在**生产环境**排查一个间歇性问题（每 1000 次出现 1 次的延迟尖峰）——从四种机制中选一种并说明理由。

---

## 命令小结

| 机制 | 代表命令 |
|------|---------|
| 静态 tracepoint | `trace-cmd record -e 'syscalls:sys_enter_*' ./prog` |
| 动态 kprobe | `perf probe --add='kfunc arg=%di'; perf record -e 'probe:kfunc' ./prog` |
| function_graph | `trace-cmd record -p function_graph -g 'func_*' ./prog` |
| perf 采样 | `perf record -g ./prog; perf report` |

---

## 📚 模块完结

🎉 **你完成了 ftrace 模块的全部 10 个实验！**

回顾你在此模块中获得的能力：

```
🟢 基础：tracefs 文件系统、trace_marker、trace 输出格式
🟢 进阶：function tracer、set_ftrace_filter/PID、全量追踪概念
🟡 核心：function_graph 调用图、微秒延迟、+!# 标记
🟡 工具：trace-cmd record/report/stream/start/stop
🟠 实战：函数+事件复合追踪、间歇性延迟排查
🟠 运维：环形缓冲区调优、CPU 绑定、溢出防丢失
🔴 深度：trace_printk 规范、黑白名单组合滤波
🔴 架构：四种追踪机制全景对比、场景选型决策
```

> 📚 返回 [ftrace 模块总览](../README.md) 或 [项目首页](../../README.md)。
