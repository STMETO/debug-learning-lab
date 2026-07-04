# perf —— CPU 性能分析

## 写给谁？

本模块假设你完成了 [strace 模块](../strace/) 和 [ltrace 模块](../ltrace/)，知道什么是系统调用和库函数调用，理解"程序运行时会消耗 CPU 资源"这个概念。

> 如果 strace 让你看到了程序和内核的"对话"，ltrace 让你看到了程序和库的"对话"，perf 让你看到 CPU 在"想什么"——它把时间花在哪些函数上，缓存是否命中，分支是否预测正确。

## perf 是什么？

**perf** 是 Linux 内核自带的性能分析工具。它利用 CPU 内置的**硬件性能计数器（Performance Monitoring Unit, PMU）**来回答一个核心问题：**程序的 CPU 时间花在了哪里？**

打个比方：

```
┌─────────────────────────────────────────────────┐
│  GDB：你看程序内部的变量和代码执行                 │
│        像一个显微镜——放大代码的内部状态            │
├─────────────────────────────────────────────────┤
│  ASan：在内存被破坏时自动报警                      │
│        像一个烟雾探测器——只在出问题时叫            │
├─────────────────────────────────────────────────┤
│  strace/ltrace：你看程序和外部世界的每一次交互       │
│        像一个窃听器——监听所有外部通信              │
├─────────────────────────────────────────────────┤
│  perf：你看 CPU 在"哪里"花了时间                   │
│        像一个计时器+计数器——精确测量性能热点        │
└─────────────────────────────────────────────────┘
```

### perf 的两大工作模式

```
┌─────────────────────────────────────────────────────┐
│  perf stat：计数模式                                  │
│  ┌─────────────────────────────────────────────┐    │
│  │  程序运行 → perf 在后台数数                    │    │
│  │  程序结束 → perf 打印汇总：                    │    │
│  │    10,000,000 cycles                         │    │
│  │     8,500,000 instructions                   │    │
│  │       150,000 cache-misses                   │    │
│  │        50,000 branch-misses                  │    │
│  └─────────────────────────────────────────────┘    │
│  "这个程序执行了多少条指令？缓存命中率如何？"           │
├─────────────────────────────────────────────────────┤
│  perf record：采样模式                                │
│  ┌─────────────────────────────────────────────┐    │
│  │  程序运行 → perf 每秒拍几千张"快照"             │    │
│  │  每张快照记录：CPU 正在执行哪个函数              │    │
│  │  程序结束 → perf report：                      │    │
│  │    60% main.c:15 (slow_function)             │    │
│  │    20% main.c:28 (fast_function)             │    │
│  │    10% libc:printf                           │    │
│  └─────────────────────────────────────────────┘    │
│  "哪些函数消耗了最多的 CPU 时间？"                     │
└─────────────────────────────────────────────────────┘
```

### 为什么不用 `time` 或 `printf(gettimeofday())` 就够了？

`time` 只能告诉你"总共花了 3 秒"，不能告诉你：
- 这 3 秒是 CPU 在忙，还是在等磁盘？
- 哪个函数占用了最多时间？
- 为什么慢——是指令太多，还是缓存不命中？分支预测失败？

**perf 回答的不只是"慢不慢"，而是"为什么慢"。**

---

## 学习路线图

整个模块分为五个阶段，由浅入深。**强烈建议按顺序学习**，每个实验都是下一个的基础。

### 🟢 阶段一：基础入门（实验 01–02）

| # | 实验 | 你会学到 | 时长 | 环境要求 |
|---|------|---------|------|----------|
| [01](01-hello-perf/) | Hello perf | `perf stat ls`、看到第一批硬件计数器、理解 perf 的两种模式 | ~10min | ✅ WSL2 |
| [02](02-perf-stat/) | 读懂计数器 | cycles、instructions、IPC、cache-misses、branch-misses、用比值诊断性能问题 | ~20min | ✅ WSL2 |

> 学完这两个实验，你能对任何程序跑 `perf stat` 并判断它"为什么慢"。

### 🟡 阶段二：核心技能（实验 03–04）

| # | 实验 | 你会学到 | 时长 | 环境要求 |
|---|------|---------|------|----------|
| [03](03-perf-record/) | 采样与报告 | `perf record` 采集样本、`perf report` 查看热点函数、理解调用链（callchain） | ~20min | ✅ WSL2 |
| [04](04-perf-annotate/) | 源码级分析 | `perf annotate` 在源代码/汇编级别定位热点指令、理解"时间花在哪一行" | ~15min | ✅ WSL2 |

> 学完这阶段，你能精确定位程序的热点函数甚至热点代码行。

### 🟠 阶段三：生产技能（实验 05–06）

| # | 实验 | 你会学到 | 时长 | 环境要求 |
|---|------|---------|------|----------|
| [05](05-flamegraph/) | 火焰图 | 用 FlameGraph 脚本把 `perf.data` 转成可视化 SVG、一眼看懂调用栈的时间分布 | ~20min | ✅ WSL2 |
| [06](06-real-world/) | 实战排查 | 综合使用 perf stat/record/report/annotate/火焰图——从一个"不知道哪里慢"的程序到精确定位瓶颈 | ~20min | ✅ WSL2 |

> 到这里，你拥有"从宏观到微观"的完整性能分析能力——stat 看全局指标 → record/report 找热点函数 → annotate 定位热点指令 → 火焰图可视化汇报。

### 🔴 阶段四：高级专项（实验 07–10）

| # | 实验 | 你会学到 | 时长 | 环境要求 |
|---|------|---------|------|----------|
| [07](07-perf-trace/) | 事件定制与 perf trace | 使用 `-e` 监控特定事件；用 `perf trace` 低开销追踪系统调用（比 strace 更轻量） | ~15min | ✅ WSL2 |
| [08](08-sched-analysis/) | 调度与锁分析 | 使用 `perf sched` 分析任务调度延迟、CPU 迁移和上下文切换 | ~20min | ✅ WSL2 |
| [09](09-mem-cache/) | 内存与缓存分析 | `perf mem` 分析内存访问延迟；`perf c2c` 检测伪共享（false sharing） | ~25min | ❌ 原生 Linux |
| [10](10-probe-script/) | 动态探针与脚本 | `perf probe` 动态追踪任意函数；`perf script` + Python 定制化分析 | ~20min | ✅ WSL2 |

> 实验 09 需要硬件 PMU 的内存事件支持（`perf mem` / `perf c2c`），WSL2 虚拟化环境不支持。其他三个实验在 WSL2 均验证可用。

### 🟣 阶段五：系统资源专项分析（实验 11–13）

| # | 实验 | 你会学到 | 时长 | 环境要求 |
|---|------|---------|------|----------|
| [11](11-block-io/) | 块设备 IO 性能分析 | `perf record -e block:*` 追踪块 IO 延迟；结合 `blktrace` 定位磁盘排队、sync/flush 瓶颈；page-fault 区分主缺页/次缺页；存储慢查询完整排查链路 | ~20min | ⚠️ WSL2 部分 |
| [12](12-network-irq/) | 网络中断与软中断追踪 | `-e irq/softirq` 事件采集；定位网卡中断漂移、NAPI 瓶颈；`perf trace` 监控 socket 系统调用（connect/send/recv），排查网络延迟 | ~18min | ✅ WSL2 |
| [13](13-kmem-leak/) | 内核内存与泄漏定位 | `perf kmem` 追踪 slab 分配/释放；监控内核内存泄漏、缓存占用；区分用户态 `malloc` 与内核态内存开销；定位长时间运行服务内存持续上涨根因 | ~22min | ⚠️ WSL2 部分 |

> 阶段五聚焦系统级资源分析。WSL2 中 block/irq/net/kmem 的 tracepoint 事件均可用，但 `perf kmem record` 和 `blktrace` 需原生 Linux。

---

## 环境分级

```
WSL2 可用（实验 01–08, 10, 12）：    需要原生 Linux（实验 09）：
┌─────────────────────────┐    ┌──────────────────────────────┐
│ perf stat               │    │ perf mem                     │
│ perf record / report    │    │ perf c2c                     │
│ perf annotate           │    └──────────────────────────────┘
│ perf trace              │
│ FlameGraph              │    WSL2 部分可用（实验 11, 13）：
│ perf sched              │    ┌──────────────────────────────┐
│ perf probe              │    │ block: tracepoint ✅          │
│ perf script             │    │ kmem: tracepoint ✅           │
│ -e 硬件/软件事件         │    │ blktrace ❌                  │
│ -e tracepoint 事件      │    │ perf kmem record ❌           │
│ irq/softirq/net tp      │    └──────────────────────────────┘
└─────────────────────────┘
```

> 💡 以上环境分级基于 WSL2 (kernel 6.6.87.2, perf 5.15.200) 实际验证结果。只有实验 09（perf mem/c2c）完全不可用，实验 11/13 的核心 tracepoint 可用但部分工具受限。

---

## 开始之前

### ⚠️ 重要：环境限制

perf 是内核态工具，**无法在 Docker 容器内运行**（容器限制 `perf_event_open()` 系统调用）。你需要在以下环境中操作：

| 环境 | 状态 | 说明 |
|------|------|------|
| **WSL2 原生 shell** | ✅ 推荐 | 支持 11/13 个实验；仅实验 09 完全不可用 |
| **Docker 容器** | ❌ 不支持 | `perf_event_open()` 被限制 |
| **原生 Linux / 虚拟机** | ✅ 全功能 | 支持全部 13 个实验 |

### 你需要什么

```bash
# 安装 perf（Ubuntu/Debian）
sudo apt install linux-tools-common linux-tools-generic linux-tools-$(uname -r)

# 如果内核版本对应的包不存在（WSL2 常见情况），安装最接近的版本：
sudo apt install linux-tools-5.15.0-185  # 或你的系统上可用的版本

# 验证安装
perf --version

# 检查权限级别
cat /proc/sys/kernel/perf_event_paranoid
# 0 或 1 = 大部分功能可用（推荐设为 1）
# 2 = 限制较多（默认值，需要 sudo 才能采样其他进程）
# 3 = 完全禁用
```

```bash
# 如果 paranoid 不是 1，降低权限要求：
sudo sysctl kernel.perf_event_paranoid=1

# 永久设置（重启后保留）：
echo 'kernel.perf_event_paranoid=1' | sudo tee -a /etc/sysctl.conf
```

### perf 和其他工具的关系

| | perf | strace | ltrace | Valgrind/Callgrind |
|------|------|--------|--------|------|
| 追踪对象 | CPU 硬件事件 | 系统调用 | 库函数调用 | 模拟 CPU 执行 |
| 输出内容 | 热点函数、缓存命中率、CPU 周期分布 | `open()`, `read()`, `write()`... | `printf()`, `malloc()`... | 调用图 + 指令计数 |
| 性能开销 | 低（stat：~0%；record：~2-5%） | 中（~10-50%） | 中高（~20-50%） | 高（~20x） |
| 适合场景 | "为什么慢？CPU 时间花在哪了？" | "程序在向内核请求什么？" | "库函数调用有没有问题？" | "最详尽的调用图分析" |
| 需要重编译 | 否（但加 `-g` 才能看到源码） | 否 | 否 | 否 |

### 关键概念速览

| 术语 | 一句话解释 |
|------|-----------|
| **PMU（Performance Monitoring Unit）** | CPU 内置的硬件计数器，记录 cycles、instructions、cache-misses 等事件 |
| **采样（sampling）** | perf record 每隔 N 个事件（如每 100000 次 cycle）拍一张"快照"，记录 CPU 正在执行哪个函数 |
| **热点（hotspot）** | 占用 CPU 时间最多的代码位置——采样次数越多 = 越热 |
| **IPC（Instructions Per Cycle）** | 每个 CPU 时钟周期执行了多少条指令。> 1 = 高效（超标量执行），< 0.5 = 低效（在等内存） |
| **调用链（callchain）** | 从 `main` 到当前函数的一整条调用路径。perf 可以帮你回溯出"是谁调用了这个热点函数" |
| **火焰图（Flame Graph）** | 把调用链可视化为彩色矩形——宽度 ∝ CPU 时间占比。x 轴 = 按函数名排序，y 轴 = 调用栈深度 |
| **tracepoint** | 内核源码中预埋的静态追踪点（如 `block:block_rq_issue`、`syscalls:sys_enter_openat`），perf 可以直接采集 |
| **动态探针（kprobe/uprobe）** | 不需要修改内核或重启，perf probe 可以在运行中的内核/用户态函数入口/出口处插入追踪点 |

---

## 实验目录结构

每个实验目录都遵循同样的结构：

```
03-perf-record/
├── README.md      # 本实验的详细教程
├── demo.c         # 示例源码（一个可以压测的 C 程序）
└── Makefile       # make 编译（CFLAGS = -g -Wall -Wextra）
```

---

## 阅读 perf 报告的黄金法则

你会在每个实验中反复看到 perf 输出。拿到输出后按这个顺序分析：

```
1. 看全局     → perf stat：IPC 是多少？cache-miss 比例高吗？
                 High IPC (> 1.5) + 许多 cycles → 计算密集（算法问题）
                 Low IPC (< 0.5) + 许多 cache-misses → 内存瓶颈（数据结构问题）

2. 找热点     → perf report：哪个函数占的 Overhead 最高？
                 Overhead 60% → 一半以上的时间都在这个函数里——重点分析它

3. 钻进去     → perf annotate：在这个热点函数里，具体是哪几行代码最耗时？
                 是循环条件？是内存访问？是函数调用？

4. 验修复     → 改代码 → 重新 perf stat → IPC 提升了吗？cycles 减少了吗？
```

---

## 遇到问题？

- 每个实验的 README 末尾有"常见错误"小节
- `man perf` 包含所有子命令的完整文档
- `perf help <子命令>` 查看具体子命令的帮助（如 `perf help stat`）
- WSL2 中如果 `perf stat` 报 `perf_event_open() failed`，检查 `perf_event_paranoid` 设置
- 如果某些事件（如 `cache-misses`）显示 `<not supported>`，说明当前 CPU 不支持该事件——WSL2 的虚拟化环境可能限制部分硬件计数器
- 阶段四/五的实验如无法运行，切换到原生 Linux 虚拟机即可

---

<p align="center">
  <b>准备好了吗？从 <a href="01-hello-perf/">实验 01：Hello perf</a> 开始 👇</b>
</p>
