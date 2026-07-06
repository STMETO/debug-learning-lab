# Perfetto —— 系统级全栈追踪与 SQL 分析

## 写给谁？

本模块假设你完成了 [perf 模块](../perf/) 和 [ftrace 模块](../ftrace/)，理解硬件性能计数器、tracepoint 事件、函数追踪等概念。

> perf 和 ftrace 各有所长——perf 擅长统计采样，ftrace 擅长精确追踪。Perfetto 把两者的数据**统一到一个时间线上**，并用 **SQL** 来查询分析。如果你对"写 SQL 查性能数据"这件事感到好奇，这个模块就是为你准备的。

## Perfetto 是什么？

**Perfetto** 是 Google 开发的开源全栈追踪系统，最初为 Android 设计，现已支持 Linux 桌面/服务器。它的核心设计理念是：**把所有追踪数据放入同一个时间线，用 SQL 统一查询**。

```
┌─────────────────────────────────────────────────────────────┐
│  ftrace（内核函数追踪）  ──┐                                  │
│  perf（CPU 采样）       ──┤                                  │
│  atrace（Android 应用）  ──┼──→ traced 守护进程              │
│  heapprofd（堆分析）     ──┤        │                        │
│  traced_perf（perf 桥接）──┘        │                        │
│                              ┌──────▼──────────┐             │
│                              │  .perfetto-trace │  统一格式   │
│                              └──────┬──────────┘             │
│                                     │                        │
│                              ┌──────▼──────────┐             │
│                              │ trace_processor  │  SQL 分析   │
│                              └──────┬──────────┘             │
│                                     │                        │
│                              ┌──────▼──────────┐             │
│                              │ Perfetto UI      │  可视化     │
│                              │ ui.perfetto.dev  │             │
│                              └─────────────────┘             │
└─────────────────────────────────────────────────────────────┘
```

### Perfetto 和已有工具的关系

```
┌─────────────────────────────────────────────────────────┐
│  perf：统计采样（~2% 开销）                               │
│  "哪个函数吃 CPU？" "IPC 多少？"  "缓存命中率如何？"      │
├─────────────────────────────────────────────────────────┤
│  ftrace：精确追踪（~20-50% 开销）                         │
│  "每个函数调用的耗时？" "谁调用了它？"                     │
├─────────────────────────────────────────────────────────┤
│  Valgrind/ASan：内存错误检测                              │
│  "哪里泄漏了？哪里越界了？"                                │
├─────────────────────────────────────────────────────────┤
│  Perfetto：统一平台 + SQL 分析                            │
│  "用 SQL 同时查询内核追踪、CPU 采样、应用埋点——             │
│   找到'当 CPU 采样显示热点在 func_a 时，                    │
│   同时刻的 ftrace 显示它在等 ext4_sync_file'。             │
│   也可以用 heapprofd 做在线内存分配分析——                   │
│   不需要重启程序，不需要重编译。"                           │
└─────────────────────────────────────────────────────────┘
```

### Perfetto 覆盖的三大分析维度

| 维度 | Perfetto 数据源 | 传统工具 | 实验 |
|------|----------------|---------|------|
| **内核事件** | `linux.ftrace` | ftrace / trace-cmd | 实验 02 |
| **CPU 耗时** | `linux.perf` | perf record / perf report | 实验 04 |
| **内存分配** | `heapprofd` | Valgrind Massif / heaptrack | 实验 05 |
| **应用埋点** | Perfetto C SDK | 自定义日志 / printf | 实验 06 |

---

## 环境评估

### WSL2 支持矩阵

| 组件 | 状态 | 说明 |
|------|------|------|
| ftrace 后端 | ✅ 可用 | tracefs 已挂载，4 种 tracer，2422 个事件 |
| perf_event_open | ✅ 可用 | `paranoid=1`，perf 桥接可用 |
| `/dev/binder` | ❌ 不适用 | Android IPC，Linux 桌面不需要 |
| traced 守护进程 | ⚠️ 需安装 | 从 GitHub releases 下载预编译二进制 |
| trace_processor | ⚠️ 需安装 | 同上，约 50-80MB |
| Perfetto UI | ✅ 免安装 | 浏览器打开 `https://ui.perfetto.dev` |
| 内核 tracepoint | ✅ 完备 | 2422 个事件可用 |

### 安装方案

Perfetto 工具链已放置在项目的 `lib/perfetto/` 目录下（不会被 git 追踪——已加入 `.gitignore`）。教程中所有命令使用项目根相对路径 `lib/perfetto/`。

> 💡 如果你在其他地方使用这个项目，需要自己下载 perfetto 工具：
>
> ```bash
> # 从 GitHub Releases 下载 linux-amd64.zip（~13MB）
> # https://github.com/google/perfetto/releases
> # 解压到项目的 lib/perfetto/ 目录
> unzip linux-amd64.zip -d lib/perfetto/
> chmod +x lib/perfetto/*
> ```

**可选：加到 PATH 方便日常使用**：

```bash
# 临时（当前 shell）
export PATH="$PWD/lib/perfetto:$PATH"

# 永久（加到 ~/.bashrc）
echo 'export PATH="$HOME/debug-learning-lab/lib/perfetto:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

**验证安装**：

```bash
lib/perfetto/perfetto --version
# Perfetto v57.1-c59f57dd4

lib/perfetto/traced --version
lib/perfetto/trace_processor_shell --version
```

---

## 学习路线图

> 📌 **先完成 [实验 00：UI 完全指南](00-ui-guide/)**——学会看 Perfetto 可视化界面。所有后续实验都依赖 UI。

### 🟢 阶段一：初次接触（实验 00–02）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [00](00-ui-guide/) | 🔑 UI 完全指南 | 读懂 CPU Scheduling / ftrace Events / Process Tracks / Perf Counters 五种轨道；缩放、搜索、框选、点击详情；slice vs instant vs counter 三种视觉形态 | ~15min |
| [01](01-hello-perfetto/) | Hello Perfetto | 安装工具链、用 `tracebox` 录制第一条 trace、在 Perfetto UI 中打开 `.perfetto-trace` 文件 | ~15min |
| [02](02-ftrace-integration/) | 内核事件追踪 | 用 Perfetto 采集 ftrace 事件（对比 ftrace 模块的实验 03）、理解 Perfetto 的 trace config 文件格式、用 SQL 查询 ftrace 事件 | ~20min |

> 学完这两个实验，你能用 Perfetto 录制内核事件并在时间轴上查看——和 ftrace 互补，但可视化更好。

### 🟡 阶段二：核心技能（实验 03–05）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [03](03-trace-processor/) | SQL 查询追踪数据 | `trace_processor_shell` 启动、用标准 SQL 查询 trace 表（`slice`、`sched`、`counter`）、写出"找到耗时最长的 10 个函数"这样的查询 | ~25min |
| [04](04-cpu-profiling/) | CPU 耗时分析（linux.perf） | 用 Perfetto 的 `linux.perf` 数据源做 CPU 采样（底层调用 `perf_event_open`）、在时间轴上同时看到 CPU 热点函数和内核事件、和 `perf record` 对比输出 | ~20min |
| [05](05-heapprofd/) | 内存分配分析（heapprofd） | 用 `heapprofd` 追踪用户态 `malloc/free` 调用栈和分配量、识别内存泄漏热点、和 Valgrind Massif 对比——Perfetto 的优势是"不需要重启程序即可开启" | ~22min |

> 学完这阶段，你能用 SQL 分析追踪数据，并且能用 Perfetto 同时做 CPU profiling 和 heap profiling——覆盖了 perf 和 Valgrind 的核心场景。

### 🟠 阶段三：实战与选型（实验 06–08）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [06](06-custom-tracing/) | 应用层自定义追踪 | 用 Perfetto C SDK 在程序中插入追踪点（`TRACE_EVENT`）、对比应用埋点 vs ftrace 事件 vs perf 采样的覆盖范围 | ~20min |
| [07](07-real-world/) | 实战：全栈延迟排查 | 给定一个慢程序，同时用 perf 采样 + ftrace 事件 + 应用埋点（Perfetto 统一采集），在时间轴上对齐所有数据源，找到瓶颈根因 | ~25min |
| [08](08-compare-tools/) | 三工具全景对比 | perf vs ftrace vs Perfetto 的综合对比矩阵；给出 5 个真实场景，选择正确的工具组合并说明理由 | ~15min |

> 到这里，你能在 perf、ftrace、Perfetto 三者之间正确选型——知道什么时候该用什么工具，以及什么时候该组合使用。

---

## 开始之前

### 你需要什么

- WSL2 原生 shell（本模块的实验环境）
- 能访问 GitHub（下载预编译二进制）或从源码编译的环境
- 现代浏览器（打开 `ui.perfetto.dev`）
- 前置知识：perf 模块（01-06）和 ftrace 模块（01-04）

### Perfetto vs perf vs ftrace 全景定位

```
                      | perf        | ftrace      | Perfetto
----------------------|-------------|-------------|----------------
数据来源               | CPU PMU     | 内核函数调用 | ftrace + perf + 应用埋点
采集方式               | 统计采样     | 全量追踪     | 全量追踪 + 采样
开销                   | 低 (~2%)    | 高 (~50%)   | 中 (~5-30%)
分析方式               | CLI/TUI     | cat/grep    | SQL + Web UI
时间线可视化            | 无          | 无          | ✅ 核心能力
多数据源关联            | 无          | 无          | ✅ 核心能力
应用层埋点              | 无          | trace_marker| ✅ C SDK / Java SDK
适合场景               | 宏观热点     | 微观延迟    | 复杂系统关联分析
```

### 关键概念速览

| 术语 | 一句话解释 |
|------|-----------|
| **traced** | Perfetto 的守护进程——所有追踪数据都通过它收集 |
| **trace config** | 一个 protobuf 配置文件，描述"追踪什么、追踪多久、缓冲区多大" |
| **.perfetto-trace** | 追踪数据的输出文件——包含所有数据源的完整时间线 |
| **trace_processor** | 命令行 SQL 引擎——用 SQL 查询 `.perfetto-trace` 文件 |
| **slice** | Perfetto 的基本数据单元——一段有开始时间、结束时间、名称的事件（类似 ftrace function_graph 的 `{...}`） |
| **counter** | 计数器——按时间采样的数值（类似 perf stat 的计数器值） |
| **Perfetto UI** | 浏览器中的可视化工具——时间轴 + SQL 查询面板 |

---

## 实验目录结构

每个实验目录遵循项目统一结构：

```
03-trace-processor/
├── README.md      # 详细教程
├── demo.c         # 示例源码（需要追踪的程序）
└── Makefile
```

> 💡 Perfetto 实验和 ftrace 类似——C 程序是"被追踪的目标"，大部分操作在 shell 中进行（`perfetto` 命令行、`trace_processor_shell` SQL 查询、浏览器 UI）。

---

## 阅读 Perfetto trace 的黄金法则

```
1. 录下来   → perfetto -c config.txt -o trace.perfetto-trace
              或 traced + perfetto CLI

2. 看时间轴 → 打开 ui.perfetto.dev，拖入 trace 文件
              缩放、拖拽、点击 slice 看详情

3. 写 SQL   → trace_processor_shell trace.perfetto-trace
              SELECT * FROM slice ORDER BY dur DESC LIMIT 10;

4. 做关联   → 在时间轴上同时看 ftrace 事件 + CPU 采样 + 应用埋点
              找到"CPU 热点"和"内核延迟"在时间上的因果关系
```

---

## 遇到问题？

- 每个实验的 README 末尾有"常见错误"小节
- Perfetto 官方文档：https://perfetto.dev/docs/
- Perfetto UI：https://ui.perfetto.dev/
- `trace_processor_shell` 内置帮助：`.help` 命令列出所有 SQL 表
- 下载慢？在宿主机下载 → 通过 `/mnt/c/` 复制到 WSL2

---

<p align="center">
  <b>准备好了吗？从 <a href="01-hello-perfetto/">实验 01：Hello Perfetto</a> 开始 👇</b>
</p>
