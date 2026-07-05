# 实验 06：实战排查 —— 诊断一个"不知道哪里慢"的程序

> 🎯 **目标**：综合使用 perf stat / record / report / annotate / flamegraph，从零开始诊断一个"没有报错、就是慢"的程序，找到**全部 3 个性能缺陷**并修复。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：实验 01–05（本模块的前 5 个实验）

---

## 场景设定

你接手了一个"数据分析工具"——[analyzer.c](analyzer.c)。同事说它"处理 600 万元素的数据要跑很久"，但程序没有崩溃、没有报错、也没有明显的 bug。

```
=== Data Analyzer v1.0 ===

Step 1: compute column stats (cache-unfriendly)...
Step 2: normalize data (repeated computation)...
Step 3: classify values (branch-unpredictable)...
Step 4: generate report...
  Grand total of column sums: 98289422261

=== Analysis complete. result = 508592 ===
```

你的任务：**找到为什么慢，修复它，验证修复效果**。

```bash
make && ./analyzer
```

> 💡 这个实验和前面 5 个不同——**不告诉你具体该用什么命令**。你拥有整个工具链：stat、record、report、annotate、flamegraph。自己决定诊断顺序。

当然，如果你卡住了，下面有完整的排查路径供参考。

---

## 排查路径（参考）

### 阶段一：看全局 → 判断"慢的类型"

第一步永远是 `perf stat`——了解程序的宏观特征。

```bash
perf stat ./analyzer
```

```
 Performance counter stats for './analyzer':

            177.17 msec task-clock                #    0.994 CPUs utilized
         511805367      cycles                    #    2.889 GHz
          84536725      stalled-cycles-frontend   #   16.52% frontend cycles idle
         526710605      instructions              #    1.03  insn per cycle
          45052755      branches                  #  254.286 M/sec
           4720714      branch-misses             #   10.48% of all branches

       0.178262552 seconds time elapsed
```

解读：

| 指标 | 值 | 判断 |
|------|-----|------|
| IPC | 1.03 | 正常范围，不是极端计算密集 |
| stalled-cycles | 16.52% | 有一些停顿，但不严重 |
| **branch-misses** | **10.48%** | 🔴 **偏高！** 正常应 < 5% |
| task-clock | 177 ms, 99.4% CPU | CPU 利用率很高——确实是 CPU 瓶颈 |

> 第一线索：branch-miss 比例偏高（10.48%），说明程序中有**分支预测困难**的代码。

再看 cache 情况：

```bash
perf stat -e cache-references,cache-misses ./analyzer
```

```
          16109880      cache-references
           1385017      cache-misses              #    8.597 % of all cache refs
```

> cache-miss 8.6%，虽然不算极高，但对于一个"处理数据"的程序来说偏高——可能有**缓存不友好的内存访问模式**。

**阶段一小结**：两条线索——branch-miss 偏高 + cache-miss 偏高。下一步找具体位置。

### 阶段二：找热点 → 定位"慢在哪里"

```bash
perf record -g -o perf.data ./analyzer
perf report -i perf.data --stdio
```

```
# Children      Self  Command   Shared Object         Symbol
# ........  ........  ........  ....................  ....................................
#
    95.69%     0.00%  analyzer  libc.so.6             [.] __libc_start_main
            |
            ---__libc_start_main
               main
               |
               |--29.76%--classify_values           ← 🔴 热点 1（~30%）
               |
               |--24.76%--compute_column_stats      ← 🔴 热点 2（~25%）
               |
               |--21.42%--normalize_data            ← 🔴 热点 3（~21%）
               |
                --7.36%--stats_report               ← 冷函数
```

三个热点函数的 CPU 时间分布相当均匀（各占 ~20-30%），都需要检查。我们逐个深入。

### 阶段三：逐个诊断 → perf annotate

#### 🔍 诊断 Bug 1：`compute_column_stats`

```bash
perf annotate -i perf.data --stdio compute_column_stats
```

看关键代码段（简化为源码视角）：

```c
// Line 15-19: compute_column_stats
for (int c = 0; c < cols; c++) {       // 外层遍历 COLUMNS
    for (int r = 0; r < rows; r++) {   // 内层遍历 ROWS
        sum += matrix[r * cols + c];   // ← 访问 matrix[r][c]
    }
}
```

**问题分析**：C 中的二维数组 `matrix[row][col]` 按行存储（row-major）。`matrix[r * cols + c]` 中 `r` 是内层循环变量——每次迭代 `r` 加 1，地址跳 `cols * sizeof(int)` = 3000 × 4 = **12KB**。

> 每次内存访问跳过 12KB → 跨越了 L1 缓存行（64 字节）→ **几乎每次访问都是 cache-miss**。这就是 cache-miss 8.6% 的来源。

**修复**：交换循环顺序，让内层遍历连续地址：

```c
// 修复后：内层遍历 rows，按行连续访问
for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
        sum_per_row[r] += matrix[r * cols + c];  // 连续访问，缓存友好
    }
}
```

#### 🔍 诊断 Bug 2：`normalize_data`

```bash
perf annotate -i perf.data --stdio normalize_data
```

看关键行：

```c
matrix[idx] = (matrix[idx] * 1000) / ((rows * cols) / 2 + 1);
//                                     ^^^^^^^^^^^^^^^^^^^^^^^
//                                     这个值在 6,000,000 次循环中从不改变，
//                                     但每次迭代都被重新计算！
```

**问题分析**：`(rows * cols) / 2 + 1` 在整个函数中是常量，但它在**内层循环的最内层被重复计算 6,000,000 次**。编译器可能优化了一部分（乘法），但除法的开销仍然可观。

在 annotate 中，你会看到除法相关的指令（`imul`、`sar`、`idiv`）周围采样百分比偏高。

**修复**：把常量提到循环外：

```c
int divisor = (rows * cols) / 2 + 1;  // 计算一次
for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
        int idx = i * cols + j;
        matrix[idx] = (matrix[idx] * 1000) / divisor;  // 直接使用
    }
}
```

#### 🔍 诊断 Bug 3：`classify_values`

```bash
perf annotate -i perf.data --stdio classify_values
```

```c
for (int i = 0; i < n; i++) {
    if (data[i] % 2 == 0) {   // ← 数据是伪随机生成的，奇偶各 50%
        evens++;               //    分支预测器猜不到，每次 ~50% 误预测
    } else {
        odds--;
    }
}
```

**问题分析**：`data[i]` 是伪随机数，`data[i] % 2 == 0` 的真假概率各 50%，且每次独立——分支预测器完全无法预测。这就是 branch-miss 10.48% 的根源。

**修复**：用无分支（branchless）写法，用位运算消除 if-else：

```c
// 修复后：无分支版本
// data[i] & 1  = 0 (偶数) or 1 (奇数)
// 用算术替代分支
for (int i = 0; i < n; i++) {
    int is_odd = data[i] & 1;
    evens += 1 - is_odd;     // 偶数时 +1
    odds += is_odd;           // 奇数时 +1
}
```

或者更简单——直接用查找表或数学等价变换：

```c
long diff = 0;
for (int i = 0; i < n; i++) {
    // 偶数 → +1, 奇数 → -1
    diff += 1 - 2 * (data[i] & 1);
}
```

### 阶段四：火焰图全景

生成火焰图看优化前后的对比：

```bash
# 优化前
perf record -g -o perf_before.data ./analyzer
perf script -i perf_before.data | stackcollapse-perf.pl | flamegraph.pl > before.svg

# 修改代码后
make
perf record -g -o perf_after.data ./analyzer
perf script -i perf_after.data | stackcollapse-perf.pl | flamegraph.pl > after.svg
```

在浏览器中并排打开 `before.svg` 和 `after.svg`，你会看到三个热点函数的宽度都明显变窄。

### 阶段五：验证修复效果

优化后重新跑 `perf stat`，对比关键指标：

```bash
perf stat ./analyzer        # 优化前
perf stat ./analyzer_fixed  # 优化后
```

| 指标 | 优化前 | 优化后（预期） | 改善原因 |
|------|--------|--------------|---------|
| **task-clock** | 177 ms | ~80-100 ms | 三项修复综合 |
| **cache-miss%** | 8.6% | < 3% | Bug 1 修复：连续访问 |
| **branch-miss%** | 10.48% | < 2% | Bug 3 修复：消除不可预测分支 |
| **IPC** | 1.03 | > 1.5 | 减少停顿 + 减少无用指令 |

---

## 📝 排查方法论回顾

这次实战中你练习的正是 perf 模块的"黄金法则"：

```
1. 看全局   → perf stat → IPC=1.03, branch-miss=10.48%, cache-miss=8.6%
               → 线索：分支预测有问题 + 内存访问不够高效

2. 找热点   → perf record + perf report → 3 个函数各占 20-30%
               → 都很重要，逐个分析

3. 钻进去   → perf annotate 对每个函数：
               → compute_column_stats：循环交换 → cache-miss ↓
               → normalize_data：常量外提 → 无用指令 ↓
               → classify_values：消除分支 → branch-miss ↓

4. 验修复   → perf stat 重新跑 → IPC↑、耗时↓、cache-miss↓、branch-miss↓
```

---

## 🧪 试试看

1. 只修复 Bug 1（循环交换），另外两个不动——cache-miss 降到多少？总共节省了多少时间？
2. 只修复 Bug 3（消除分支），另外两个不动——branch-miss 降到多少？
3. 比较三种修复的**独立收益**：哪一个修复带来的时间减少最多？这和 `perf report` 中哪个函数占比最高一致吗？
4. 优化后用 `perf stat -r 10 ./analyzer_fixed` 跑 10 次取平均——IPC 的方差比优化前大还是小？为什么？
5. 给优化后的版本生成火焰图——和优化前的火焰图并排看，哪个函数宽度变化最大？

---

## 命令小结

| 命令 | 在实战中用于 |
|------|-------------|
| `perf stat ./program` | 宏观诊断：IPC、cache-miss%、branch-miss% |
| `perf stat -e cache-references,cache-misses ./program` | 确认内存访问模式 |
| `perf record -g ./program` | 采样 + 调用栈 |
| `perf report --stdio` | 看热点函数排名 |
| `perf annotate 函数名` | 定位热点代码行 |
| `perf script \| stackcollapse \| flamegraph` | 可视化对比 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 修复后 perf 计数器没有变化 | 编译器自动优化了你的改动（比如循环交换） | 检查汇编确认代码真的变了；加 `-O0` 禁用优化对比 |
| 修复后反而变慢了 | 你的改动引入了新的问题（比如用了更慢的操作） | 用 `perf annotate` 检查新代码 |
| 看不懂 `perf annotate` 的汇编 | AT&T 语法不熟悉 | 只看百分比和源码行对应——哪行百分比最高就优化哪行 |
| 三个修复之间相互影响 | 改变了数据布局导致缓存状态不同 | 分别独立测试每个修复的收益 |

---

## 下一步

恭喜！你完成了 perf 模块的核心实验（01-06），能从 stat → record → report → annotate → flamegraph 完整诊断程序性能。

- 🟠 想学更多？继续 [阶段四：高级专项](../README.md#-阶段四高级专项实验-0710)——perf trace、perf sched、perf probe
- 🟣 或者直接跳到 [阶段五：系统资源分析](../README.md#-阶段五系统资源专项分析实验-1113)——块设备 IO、网络中断、内核内存泄漏
