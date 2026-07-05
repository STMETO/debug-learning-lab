# 实验 09：内存与缓存分析 —— 数据为什么堵在路上？

> 🎯 **目标**：用 L1 缓存事件诊断内存访问模式；理解伪共享（false sharing）及其对多线程性能的影响。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 02](../02-perf-stat/)（计数器解读）、[实验 03](../03-perf-record/)（perf record）
>
> ⚠️ **环境**：本实验中 `perf mem` 和 `perf c2c` 需要原生 Linux。WSL2 中可用 L1-dcache 事件替代。

## 为什么缓存这么重要？

现代 CPU 的速度瓶颈不在计算，在**等数据**。L1 缓存访问 ~1ns，主内存访问 ~100ns——差了 100 倍。

```
CPU 寄存器 ── <1ns ──→ L1 缓存 (32KB) ── ~10ns ──→ L2 缓存 (256KB)
    ── ~30ns ──→ L3 缓存 (8MB) ── ~100ns ──→ 主内存 (GB)

每往下一层，延迟增加 ~10 倍。
所谓"性能优化"，很多时候就是"让数据尽量待在 L1/L2 里"。
```

---

## 先看示例程序

[cache_demo.c](cache_demo.c) 演示两种缓存相关的性能问题：

1. **跨步访问 vs 连续访问**——stride 大小对缓存命中率的影响
2. **伪共享（False Sharing）**——两个线程写同一个缓存行的不同变量

```bash
make && ./cache_demo
```

---

## 实验开始

### 第一步：用 L1-dcache 事件看访问模式

```bash
perf stat -e L1-dcache-loads,L1-dcache-load-misses,L1-dcache-prefetches ./cache_demo
```

```
 Performance counter stats for './cache_demo':

        1119180527      L1-dcache-loads
          10597809      L1-dcache-load-misses     #    0.95% of all L1-dcache accesses
           1102053      L1-dcache-prefetches

       0.709482209 seconds time elapsed
```

**解读**：

| 指标 | 值 | 含义 |
|------|-----|------|
| L1-dcache-loads | 11.2 亿次 | 程序执行期间 L1 数据缓存被访问了 11 亿次 |
| L1-dcache-load-misses | 1059 万次 (0.95%) | L1 缓存未命中率——99% 命中了，不错！ |
| L1-dcache-prefetches | 110 万次 | CPU 的硬件预取器主动加载了 110 万次——说明访问模式有一定的可预测性 |

> L1 miss rate 0.95% 听起来很低，但**绝对值是 1000 万次**——每次 miss 成本 ~100ns → 总计 ~1 秒，在 0.7 秒的总运行时间里是不可能的……因为 L2/L3 命中比主内存快得多。**miss rate 低但 miss 次数绝对值大仍可能影响性能**。

### 第二步：连续 vs 跨步 —— spatial locality

`cache_demo.c` 里有两个函数：

```c
// 好：stride = 1 → 每个缓存行的 16 个 int 都被使用
for (int i = 0; i < n; i++) {
    sum += arr[i];
}

// 差：stride = 64 → 每隔 256 字节取一个 int，只用了缓存行的 1/16
for (int i = 0; i < n; i += 64) {
    sum += arr[i];
}
```

分别测试（注释掉另一个函数调用，重新编译后分别跑）：

```bash
# 连续访问版本
perf stat -e L1-dcache-loads,L1-dcache-load-misses ./cache_demo_seq 2>&1 | grep L1

# 跨步访问版本
perf stat -e L1-dcache-loads,L1-dcache-load-misses ./cache_demo_strided 2>&1 | grep L1
```

跨步版本的 L1 miss rate 应该**远高于**连续版本——因为每个缓存行（64 字节 = 16 个 int）只用了 1 个 int，另外 15 个被浪费了。

### 第三步：伪共享 —— 多线程的隐性杀手

两个线程各写各的变量——看起来互不影响？不一定：

```
struct shared_counters {
    volatile long counter_a;  // Thread A 写这里
    volatile long counter_b;  // Thread B 写这里
};  // ← 两个变量在同一个 64 字节缓存行内！
```

**伪共享的过程**：

```
1. Thread A 写 counter_a → CPU 0 的缓存行被标记为"脏"（modified）
2. Thread B 写 counter_b → CPU 1 需要这个缓存行的最新版本
3. 缓存一致性协议 → CPU 0 的缓存行失效 → CPU 1 从内存/L3 加载
4. Thread A 再写 counter_a → 又需要重新加载……
5. 循环往复 → 缓存行在两个 CPU 之间"乒乓"
```

**修复**：用 padding 把两个变量推到不同的缓存行：

```c
struct padded_counters {
    volatile long counter_a;
    char _pad1[64 - sizeof(long)];  // 填充到 64 字节
    volatile long counter_b;
    char _pad2[64 - sizeof(long)];
};
```

测试对比：

```bash
# 跑两次，比较耗时
perf stat -e L1-dcache-load-misses,cache-misses ./cache_demo 2>&1 | grep -E "(cache|elapsed)"
```

无 padding 版本（伪共享）应该：
- 耗时更长
- cache-miss 比例更高（因为缓存行不断失效）

> ⚠️ 注意：这个 demo 中两个函数各跑 5000 万次迭代，耗时差异可能在 10-30% 范围。在生产代码中，伪共享的线程数越多，影响越显著。

### 第四步：perf mem 和 perf c2c（原生 Linux 专属）

`perf mem` 和 `perf c2c` 是分析内存问题的专用工具：

```bash
# perf mem：记录内存访问的延迟分布
perf mem record ./cache_demo
perf mem report

# perf c2c：检测 cache-to-cache 传输（伪共享的精确检测）
perf c2c record ./cache_demo
perf c2c report
```

> ❌ **WSL2 不支持**：这两个命令需要 CPU 的 PEBS（Precise Event-Based Sampling）内存事件支持，虚拟化环境目前不暴露这些事件。在原生 Linux 或标准 KVM 虚拟机中可以正常使用。

在 WSL2 中的替代方案：
1. 用 `perf stat -e L1-dcache-*` 间接观察缓存行为
2. 用 `perf record -e cache-misses` 采样缓存 miss 位置
3. 对比有/无 padding 版本的运行时间差异

### 第五步：识别伪共享的间接信号

即便没有 `perf c2c`，也可以通过以下方式识别伪共享：

```bash
# 1. 检查程序在多核上的扩展性
# 单线程运行 → 记录耗时
# 多线程运行 → 如果耗时几乎不减少（甚至增加），可能是伪共享

# 2. 检查 L1-dcache 事件的绝对值
perf stat -e L1-dcache-load-misses,cache-misses -p <PID> sleep 2

# 如果 cache-miss 计数很高且持续增长，说明缓存行在不断失效
```

---

## 📝 核心认知

### 缓存行 = 数据运输的基本单位

```
int arr[16] → 16 × 4 = 64 字节 = 1 个缓存行
访问 arr[0] → 整个缓存行被加载到 L1
访问 arr[1] → 已经在 L1 里了，极快（L1 hit）

int arr[10000]
访问 arr[0], arr[1024], arr[2048]... → stride = 4096 字节 = 64 个缓存行
每次访问都是 L1 miss → ~100ns × 10000 = ~1ms → 慢 100 倍
```

### 内存性能问题的三个信号

| 信号 | 工具 | 诊断 |
|------|------|------|
| L1 miss rate > 5% | `perf stat -e L1-dcache-*` | 数据局部性差 → 检查访问模式 |
| cache-miss rate > 20% | `perf stat -e cache-misses` | 工作集太大或访问随机 → 考虑数据布局优化 |
| 多线程扩展性差 | 对比运行时间 | 可能是伪共享 → 加 padding 或 `__cacheline_aligned` |

---

## 🧪 试试看

1. 修改 `sequential_sum` 和 `strided_sum` 的数组大小为 80,000,000（~320MB）——L1 miss rate 变化了吗？L2/L3 命中呢？
2. 把 `strided_sum` 的步长从 64 改成 1, 8, 16, 32, 64, 256——记录每次的 L1 miss rate，画一条曲线。拐点在哪里？
3. 在 `struct padded_counters` 中把 `char _pad1[64]` 改成 `char _pad1[8]`（只有 8 字节 padding，不跨缓存行）——还能消除伪共享吗？
4. （需要原生 Linux）找一台原生 Linux 跑 `perf c2c record ./cache_demo && perf c2c report`——`Shared Data Cache Line Table` 显示了什么？

---

## 命令小结

| 命令 | 作用 | WSL2 支持 |
|------|------|----------|
| `perf stat -e L1-dcache-loads,L1-dcache-load-misses` | L1 数据缓存命中率 | ✅ |
| `perf stat -e cache-references,cache-misses` | 最后一级缓存（LLC）命中率 | ✅ |
| `perf record -e cache-misses` | 采样缓存 miss 位置 | ✅ |
| `perf mem record` | 记录内存访问延迟 | ❌ |
| `perf c2c record` | 检测 cache-to-cache 伪共享 | ❌ |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| L1-dcache 事件显示 `<not supported>` | WSL2 或旧 CPU 不支持 | 用 `cache-references,cache-misses` 替代（LLC 级别） |
| perf mem/c2c 报 `memory events not supported` | WSL2 不暴露 PEBS 事件 | 切换到原生 Linux |
| 多线程程序耗时和单线程几乎一样 | 可能是伪共享或锁竞争 | 先用 padding 消除伪共享，再用 perf sched 排查锁 |

---

## 下一步

`perf stat -e` 和 `perf mem` 分析的是**已知的**事件和函数。如果你想要追踪**任意函数**（内核或用户态）的调用和参数——不需要修改源代码，不需要重新编译——下一步 [实验 10：动态探针与脚本](../10-probe-script/)。
