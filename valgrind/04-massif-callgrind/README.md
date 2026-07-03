# 实验 04：Massif & Callgrind —— Valgrind 的工具套件

> 🎯 **目标**：用 Massif 剖析堆内存使用曲线，用 Callgrind 分析函数调用性能。
>
> ⏱️ 约 30 分钟
>
> 📖 **前置**：[实验 01](../01-hello-valgrind/)

## Valgrind 不止有 Memcheck

Memcheck 是默认工具，但 Valgrind 是一个**工具套件**：

| 工具 | 分析什么 | 输出 |
|------|---------|------|
| Memcheck | 内存错误 + 泄漏 | 错误报告 |
| **Massif** | 堆内存使用曲线 | 图表 + 快照 |
| **Callgrind** | 函数调用次数 + CPU 指令数 | 调用图数据 |
| Helgrind | 线程错误 | 竞争报告 |
| DHAT | 堆分配热点 | 访问频率报告 |

本实验聚焦 Massif 和 Callgrind——性能分析中最常用的两个。

---

## 先看示例程序

[workload.c](workload.c) 有四个阶段：

| 阶段 | 操作 | Massif 看到 | Callgrind 看到 |
|------|------|-----------|--------------|
| 1 | `fibonacci(40)` | 栈递归（不涉及堆） | 🔥 极热——58 亿条指令 |
| 2 | 分配 20 个报告 | 堆内存上升 | 中等 |
| 3 | 释放 10 个报告 | 堆内存下降 | 少 |
| 4 | 分配 5MB 大块 | 堆内存猛增 | 少 |

```bash
make && ./workload
```

---

## 实验开始

### Massif：堆内存使用曲线

```bash
valgrind --tool=massif --massif-out-file=massif.out ./workload
ms_print massif.out
```

输出（ASCII 图表）：

```
    MB
5.009^                                                                       #
     |                                                                       #
     |                                                                       #
     |                                                                       #
     |                                                                       #
     |                                                                       #
     |                                                                       #
     |                                                                       #
     |                                                                       #
     |                                                                       #
     |                                                                       #
     |                                                                       #
     |                                                                       #
   0 +----------------------------------------------------------------------->Gi
     0                                                                   5.402
```

**解读**：

- **x 轴**：时间（Giga-instructions，即 GPU 指令数）
- **y 轴**：堆内存使用量（MB）
- **图表形状**：
  - 初始阶段：堆内存接近 0（Fibonacci 只用栈）
  - 上升阶段：20 个报告的分配 → 堆增长
  - 下降阶段：释放 10 个 → 堆回落
  - 飙升：5MB 大块分配 → 垂直上升
  - 结尾：未释放的 report[19] → 堆回到某条基线

> 这张图让你看到**程序的内存使用模式**——什么时候分配最多？有没有波浪形（泄漏）？峰值在哪里？

**详细快照**：

```
ms_print massif.out | grep -A5 '^  n'
```

Massif 定期拍"快照"，记录每个时间点的：
- 当前堆使用量
- 哪个函数分配了最多内存
- 分配了多少次

### Callgrind：函数调用性能分析

```bash
valgrind --tool=callgrind --callgrind-out-file=callgrind.out ./workload
callgrind_annotate callgrind.out
```

输出：

```
==...== Events    : Ir
==...== Collected : 5800728076

5,800,728,076  instructions total
```

58 亿条指令！几乎全部来自 `fibonacci(40)`——因为这个递归函数的复杂度是 O(2^n)，40 个深度调用了无数次。

**分析工具**：

```bash
# 命令行：看函数级别的指令数
callgrind_annotate callgrind.out | head -30

# 图形界面（推荐）：
# kcachegrind callgrind.out
# （需要 apt install kcachegrind）
```

KCachegrind 会显示：
- 每个函数的指令数（绝对值和百分比）
- 调用图——哪个函数调用了哪个
- 源码级别的热度（哪一行最"热"）

### Massif vs Callgrind —— 什么时候用哪个

| 问题 | 工具 | 看什么 |
|------|------|--------|
| 程序占用内存越来越多？ | Massif | 堆曲线是否持续上升不下降 |
| 哪个函数最占 CPU？ | Callgrind | 指令数最高的函数 |
| 一次请求分配了多少内存？ | Massif | 请求前后的堆快照差值 |
| 递归是否太深了？ | Callgrind | 调用图中的递归调用次数 |
| 内存峰值是多少？ | Massif | 图表的最高点 |

---

## 📝 命令小结

| 命令 | 作用 |
|------|------|
| `valgrind --tool=massif ./program` | 堆内存剖析 |
| `ms_print massif.out.PID` | 查看 Massif 报告（ASCII 图表） |
| `valgrind --tool=callgrind ./program` | 函数调用性能分析 |
| `callgrind_annotate callgrind.out.PID` | 查看 Callgrind 报告 |
| `kcachegrind callgrind.out.PID` | 图形界面查看（推荐） |

---

## 🧪 试试看

1. 用 Massif 追踪 `./workload`，在堆曲线中找出"分配 → 释放 → 再分配"的对应位置
2. 用 Callgrind 追踪 `./workload`，在 `callgrind_annotate` 输出中找 `compute_fibonacci`——它占了总指令数的多少？
3. 写一个程序故意循环泄漏（每次循环 malloc 但不 free），Massif 图表长什么样？
4. 安装 `kcachegrind`，用图形界面看 Callgrind 结果

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| Callgrind 跑得极慢 | Callgrind 在模拟 CPU 缓存，开销巨大 | 只对关注的部分用，或减小输入规模 |
| Massif 图表只有一条直线 | 程序堆内存使用量基本恒定 | 正常的——Massif 更适合分析有波动的程序 |
| `ms_print` 输出太长 | Massif 默认拍很多快照 | `--massif-out-file` 配合 `--depth=N` 限制 |

---

## 下一步

你已经能用 Massif 和 Callgrind 分析内存和 CPU 了。最后一个实验 [实验 05：Valgrind + GDB 联动](../05-gdb-integration/)——在 Valgrind 检测到内存错误时，用 GDB attach 到进程查看完整的变量和调用栈现场。
