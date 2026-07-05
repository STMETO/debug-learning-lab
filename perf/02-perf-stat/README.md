# 实验 02：读懂计数器 —— 用比值诊断性能瓶颈

> 🎯 **目标**：深入理解每个硬件计数器的含义，学会用**比值**快速判断程序性能瓶颈的类型。
>
> ⏱️ 约 20 分钟

## 先看示例程序

[bottleneck.c](bottleneck.c) 设计了三种不同瓶颈的函数——它们的 perf stat 计数器签名截然不同：

| 函数 | 瓶颈类型 | 特征 |
|------|---------|------|
| `compute_bound()` | CPU 计算密集 | 5000 万次算术运算，循环可预测，缓存友好 |
| `memory_bound()` | 内存访问密集 | 400 万元素的大跨度访问（步长 1024），每次访问不同的缓存行 |
| `branch_heavy()` | 分支预测密集 | 1000 万次 if-else 用随机数据驱动，分支预测器无法猜测 |

```bash
make && ./bottleneck
```

输出：

```
=== Three Bottleneck Patterns ===

A: compute-bound (tight arithmetic loop)
  [compute]  result = 249298699695

B: memory-bound (strided array access)
  [memory]   total = 7813499904

C: branch-heavy (unpredictable branches)
  [branch]   count = 68

=== Done! ===
```

---

## 实验开始

### 第一步：整体全景——先看默认输出

```bash
perf stat ./bottleneck
```

```
 Performance counter stats for './bottleneck':

            533.08 msec task-clock                #    0.996 CPUs utilized
         1053025969      cycles                    #    1.975 GHz
          191458489      stalled-cycles-frontend   #   18.18% frontend cycles idle
         1523752344      instructions              #    1.45  insn per cycle
         116361703       branches                  #  218.282 M/sec
           9515781       branch-misses             #    8.18% of all branches
```

整体看：**IPC = 1.45**（还不错），**stalled 只有 18%**（大部分时间在干活），**branch-miss 8.18%**（中等）。

但这是三个函数的**平均值**——掩藏了各自的特点。我们需要分别看每个函数。

### 第二步：IPC —— 诊断"CPU 在忙什么"

IPC（Instructions Per Cycle，每周期指令数）是性能分析的第一个切入口。

```bash
perf stat -e cycles,instructions ./bottleneck
```

```
         993693633      cycles
        1518837697      instructions              #    1.53  insn per cycle
```

**IPC 解读表**：

| IPC 范围 | 含义 | 常见原因 | 示例 |
|----------|------|---------|------|
| **> 2.0** | 超标量充分利用 | 多指令并行发射，计算密集 | 矩阵乘法、图像处理 |
| **1.0 ~ 2.0** | 正常，有些停顿 | 偶尔等内存、分支误预测 | 大部分程序 |
| **0.5 ~ 1.0** | 停顿较多 | 经常等内存、缓存不命中多 | 链表遍历、大数组随机访问 |
| **< 0.5** | 严重停顿 | 大量 cache-miss、锁等待、IO | 数据库全表扫描、频繁缺页 |

我们程序整体的 IPC = 1.53，属于"正常但不算顶尖"——因为 memory_bound 拖了后腿。

### 第三步：Cache-Miss 比例 —— 诊断"内存访问模式"

```bash
perf stat -e cycles,instructions,cache-references,cache-misses ./bottleneck
```

```
          8195300      cache-references
          1265729      cache-misses              #   15.445 % of all cache refs
```

**cache-miss 比例 = cache-misses ÷ cache-references**

| 比例 | 含义 | 常见原因 |
|------|------|---------|
| **< 3%** | 缓存命中率极好 | 连续访问、数据局部性好 |
| **3% ~ 10%** | 正常范围 | 适量随机访问 |
| **10% ~ 30%** | 偏高 | 大跨度访问、数据结构不够紧凑 |
| **> 30%** | 缓存效率低 | 随机访问大量数据、链表/哈希表遍历 |

> memory_bound 用步长 1024（4KB）跳跃访问——每次访问几乎必然 cache-miss。compute_bound 则是连续访问，几乎不会 miss。两者一平均得到 15.4%，而 memory_bound 单独跑可能 > 50%。

要验证这个判断，可以把步长改小再跑一次：

```bash
# 修改 bottleneck.c：把 step = 1024 改成 step = 1
# 重新 make && perf stat -e cache-references,cache-misses ./bottleneck
```

步长 = 1（连续访问）时，cache-miss 比例会大幅下降。

### 第四步：Branch-Miss 比例 —— 诊断"分支可预测性"

```bash
perf stat -e branch-instructions,branch-misses ./bottleneck
```

```
         115129980      branch-instructions
           8998803      branch-misses             #    7.82% of all branches
```

**branch-miss 比例 = branch-misses ÷ branch-instructions**

| 比例 | 含义 | 常见原因 |
|------|------|---------|
| **< 2%** | 极好的分支预测 | 循环、固定模式的 if-else |
| **2% ~ 5%** | 正常 | 大部分程序 |
| **5% ~ 15%** | 分支预测压力大 | 数据驱动的分支、错误处理、类型分发 |
| **> 15%** | 分支预测严重拖累 | 二分查找、随机数据排序、解析器 |

> branch_heavy 函数用随机数据（50% 概率是 0，50% 概率是 1）来驱动 `if (data[i])`——分支预测器完全猜不到，必然 ~50% 误预测。compute_bound 的循环分支可以 99% 预测。

### 第五步：三种瓶颈的计数器签名对比

| 指标 | compute_bound（计算密集） | memory_bound（内存密集） | branch_heavy（分支密集） |
|------|--------------------------|--------------------------|--------------------------|
| **IPC** | 高（> 1.5） | 低（< 0.8） | 中等（~1.0） |
| **cache-miss%** | 低（< 5%） | 高（> 30%） | 低（< 5%） |
| **branch-miss%** | 低（< 2%） | 低（< 2%） | 高（> 15%） |
| **stalled%** | 低（< 10%） | 高（> 30%） | 中等（~15%） |

现在你可以做一个"盲测"：给一个不认识的程序跑 `perf stat`，只看计数器就能猜出它的瓶颈类型。

动手验证——分别跑三个函数（注释掉另外两个，重新编译），看看每个函数单独的计数器是否匹配上表。

### 第六步：诊断公式速查

拿到一份 `perf stat` 输出后，按这个流程判断：

```
1. IPC 低吗？（< 1.0）
   ├── 是 → 看 cache-miss 比例
   │       ├── 高（> 20%）→ 内存瓶颈 → 优化数据布局、减少随机访问
   │       └── 低 → 看 stalled-cycles
   │               ├── 高（> 30%）→ 前端停顿 → 指令获取/解码有问题
   │               └── 低 → 可能是 I/O 等待（不是 CPU 问题）
   └── 否（IPC > 1.0）→ 看 branch-miss 比例
           ├── 高（> 10%）→ 分支预测瓶颈 → 优化 if/switch，用查找表替代
           └── 低 → IPC > 2.0？计算密集型 → 算法层面优化
```

---

## 📝 核心认知

### 读计数器的第一原则

**不要看绝对值，要看比值。** `1,000,000,000 cycles` 没有任何意义——不知道这个程序运行了多久就是空谈。但 `IPC = 0.5` 就知道它在等内存，`cache-miss = 30%` 就知道数据布局有问题。

### 三个最常用的比值

| 比值 | 公式 | 诊断价值 |
|------|------|---------|
| **IPC** | instructions ÷ cycles | "CPU 是否在高效执行？" |
| **cache-miss rate** | cache-misses ÷ cache-references | "数据在缓存中的命中率？" |
| **branch-miss rate** | branch-misses ÷ branch-instructions | "分支预测是否在拖后腿？" |

---

## 🧪 试试看

1. 把 `memory_bound` 的步长从 1024 改成 1（连续访问）、64（每 256 字节跳一次）、4096（每 16KB 跳一次），cache-miss 比例分别怎么变？
2. 把 `branch_heavy` 的数据改成全 0（`data[i] = 0`）——分支全走同一方向。branch-miss 变成多少？为什么预测器能完美预测？
3. 在三个函数间加一些 `sleep(1)`——`task-clock` 会增加吗？`cycles` 会增加吗？为什么？
4. `perf stat -e L1-dcache-load-misses,L1-dcache-loads ./bottleneck`——L1 缓存的事件你的环境支持吗？（`<not supported>` 说明被 WSL2 虚拟化限制）
5. 用 `perf stat -r 5 ./bottleneck` 跑 5 次——计算 IPC 的平均值和方差。波动有多大？为什么会有波动？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `perf stat ./program` | 默认事件集：task-clock, cycles, instructions, branches, branch-misses |
| `perf stat -e 事件列表 ./program` | 自定义事件 |
| `perf stat -d ./program` | 详细模式：额外显示 L1 缓存事件 |
| `perf stat -r N ./program` | 跑 N 次取平均 |
| `perf stat -p <PID>` | 附加到已运行的进程 |
| `perf list hw` | 列出硬件事件 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `<not supported>` | WSL2 虚拟化 / CPU 不支持该事件 | 正常——用支持的事件代替 |
| `cycles` 和 `instructions` 数值波动大 | CPU 频率动态调整（Turbo Boost） | 用 `-r 5` 取平均 |
| IPC 远小于 1 但程序"感觉不慢" | 时间花在 I/O 或 sleep——它们不消耗 cycles | 看 `task-clock`：如果 CPU 利用率低，瓶颈不是计算 |
| `cache-misses` 很高但没法优化 | 工作集大于 CPU 缓存——只能减少数据量 | 考虑分块处理（blocking/tiling） |

---

## 下一步

你现在能用计数器比值判断瓶颈类型了。但计数器只告诉你"全局情况"——如果你想问"**哪个函数**消耗了最多的 CPU 时间？"，那是 `perf record` 的领域。下一步 [实验 03：采样与报告](../03-perf-record/)。

---
