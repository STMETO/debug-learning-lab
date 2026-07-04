# 实验 01：Hello perf —— 第一次看到 CPU 在"数什么"

> 🎯 **目标**：第一次运行 `perf stat`，看到 CPU 硬件计数器，理解 perf 的两种工作模式。
>
> ⏱️ 约 10 分钟

## 先看示例程序

[hello.c](hello.c) 设计了两种不同的工作负载——让你直观感受 CPU 计数器如何反映代码特征：

- **compute_heavy**：纯算术运算的紧循环（CPU 密集型——大量指令、高 IPC）
- **memory_heavy**：遍历一个大数组（内存密集型——多 cache-misses、低 IPC）

```bash
make && ./hello
```

输出：

```
=== Hello from perf lab! ===
Step 1: compute-heavy loop...
  [compute] sum = 150000055000000
Step 2: memory-heavy access...
  [memory] total = 285714714285
=== Done! ===
```

程序正常跑完。但你不知道 CPU 在里面做了什么——执行了多少条指令？CPU 周期花了多少？缓存命中率如何？

用 perf 来亲眼看看。

---

## 实验开始

### 第一步：第一次运行 perf stat

```bash
perf stat ./hello
```

你会看到程序正常输出，然后 perf 在最后追加了一段统计报告：

```
=== Hello from perf lab! ===
Step 1: compute-heavy loop...
  [compute] sum = 150000055000000
Step 2: memory-heavy access...
  [memory] total = 285714714285
=== Done! ===

 Performance counter stats for './hello':

             66.76 msec task-clock                #    0.983 CPUs utilized
                 0      context-switches          #    0.000 /sec
                 0      cpu-migrations            #    0.000 /sec
              2012      page-faults               #   30.137 K/sec
         136281635      cycles                    #    2.041 GHz
          15033631      stalled-cycles-frontend   #   11.03% frontend cycles idle
         140537614      instructions              #    1.03  insn per cycle
                                                  #    0.11  stalled cycles per insn
          16428617      branches                  #  246.074 M/sec
            657676      branch-misses             #    4.00% of all branches

       0.067929415 seconds time elapsed

       0.031879000 seconds user
       0.035864000 seconds sys
```

> 你刚才看到的是 **perf stat（计数模式）**——程序跑完后给你一份汇总报告。这和 `time` 命令有点像，但 `time` 只告诉你"花了 0.05 秒"，perf 告诉你"这 0.05 秒里 CPU 具体做了什么"。

### 第二步：perf stat 报告的解剖结构

perf stat 的每一行有固定结构：

```
      <原始计数>  <事件名>                    # <衍生指标>
```

把上面的报告拆开看：

| 行 | 原始计数 | 事件名 | 衍生指标 | 含义 |
|----|---------|--------|---------|------|
| 1 | 66.76 msec | task-clock | 0.983 CPUs | CPU 实际使用时间 66.76 毫秒，利用率 98.3% |
| 2 | 0 | context-switches | 0.000 /sec | 没有发生上下文切换 |
| 3 | 0 | cpu-migrations | 0.000 /sec | 没有跨 CPU 迁移 |
| 4 | 2012 | page-faults | 30.137 K/sec | 触发了 2012 次缺页（主要是加载代码和 libc） |
| 5 | 136,281,635 | cycles | 2.041 GHz | 消耗了 1.36 亿个 CPU 周期 |
| 6 | 15,033,631 | stalled-cycles-frontend | 11.03% | 11% 的周期在"等指令"（等内存、等译码） |
| 7 | 140,537,614 | instructions | 1.03 IPC | 执行了 1.4 亿条指令，**每个周期执行 1.03 条** |
| 8 | 16,428,617 | branches | 246 M/sec | 分支指令 1642 万次 |
| 9 | 657,676 | branch-misses | 4.00% | 分支预测失败率 4%（还不错） |

**关键发现**：

1. **IPC = 1.03**：每个 CPU 周期执行约 1 条指令。现代 CPU 超标量设计的目标是 > 2，1.03 意味着有一些停顿（stall），但不算太糟。

2. **stalled-cycles-frontend = 11%**：只有 11% 的周期在等指令解码——89% 的时间 CPU 在执行，说明这个程序是"计算为主"的。

3. **branch-misses = 4%**：分支预测准确率 96%，很好——循环的分支几乎完全被预测对了。

4. **0 context-switches**：程序执行期间没有被其他进程打断——它一直独占 CPU。

> 这些数字告诉你的不是"程序快不快"，而是"程序为什么快/慢"——这是 perf 的核心价值。

### 第三步：和 `time` 对比——perf 多给了什么？

```bash
time ./hello
```

输出：

```
real	0m0.051s
user	0m0.035s
sys	0m0.016s
```

| 工具 | 告诉你 | 不告诉你 |
|------|--------|---------|
| `time` | 总耗时 0.051s，用户态 0.035s，内核态 0.016s | 为什么花了 0.035s |
| `perf stat` | 同上 + 1.4 亿条指令、1.36 亿周期、IPC 1.03、缓存命中... | 哪个函数最耗时（那是实验 03 的内容） |

用 `perf stat` 跑 `ls` 试试，感受差别：

```bash
perf stat ls > /dev/null
```

> `ls` 是一个"启动快、跑完就退出"的程序，它的 `task-clock` 非常短（通常 < 5ms），`cycles` 和 `instructions` 都很少。和 `./hello`（计算密集）对比就很明显。

### 第四步：自定义事件——挑你想看的

默认输出已经不少，但你也可以只选几个事件：

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses ./hello
```

输出精简为：

```
 Performance counter stats for './hello':

         135604872      cycles
         139856714      instructions              #    1.03  insn per cycle
           1607177      cache-references
            306556      cache-misses              #   19.074 % of all cache refs
          16257237      branch-instructions
            621232      branch-misses             #    3.82% of all branches

       0.052316537 seconds time elapsed
```

> 用 `-e` 后面列出你感兴趣的事件名（逗号分隔），perf 就只统计这些。

看看软件事件也能告诉我们有用的信息：

```bash
perf stat -e task-clock,context-switches,cpu-migrations,page-faults ./hello
```

输出：

```
 Performance counter stats for './hello':

             46.51 msec task-clock                #    0.974 CPUs utilized
                 1      context-switches          #   21.500 /sec
                 0      cpu-migrations            #    0.000 /sec
              2014      page-faults               #   43.301 K/sec

       0.047729227 seconds time elapsed
```

### 第五步：看看有哪些事件可用

```bash
perf list
```

输出（前几行）：

```
  branch-instructions OR branches                    [Hardware event]
  branch-misses                                      [Hardware event]
  cache-misses                                       [Hardware event]
  cache-references                                   [Hardware event]
  cpu-cycles OR cycles                               [Hardware event]
  instructions                                       [Hardware event]
  stalled-cycles-frontend OR idle-cycles-frontend    [Hardware event]
  alignment-faults                                   [Software event]
  context-switches OR cs                             [Software event]
  cpu-migrations OR migrations                       [Software event]
  page-faults OR faults                              [Software event]
  task-clock                                         [Software event]

  L1-dcache-load-misses                              [Hardware cache event]
  L1-dcache-loads                                    [Hardware cache event]
  L1-icache-load-misses                              [Hardware cache event]
  ...
```

事件分三类：

| 类型 | 标签 | 来源 | 示例 |
|------|------|------|------|
| **Hardware event** | `[Hardware event]` | CPU 内置 PMU 硬件计数器 | `cycles`, `instructions`, `cache-misses` |
| **Software event** | `[Software event]` | 内核软件计数器 | `context-switches`, `page-faults`, `cpu-migrations` |
| **Hardware cache event** | `[Hardware cache event]` | CPU 缓存专用计数器 | `L1-dcache-load-misses`, `L1-icache-loads` |

> `perf list` 列出的可用事件取决于你的 CPU 和内核版本。WSL2 的虚拟化环境可能不支持某些硬件缓存事件（如 `L1-dcache-*`）——跑 `perf stat -e L1-dcache-load-misses ls` 试试看就知道你的环境是否支持。

---

## 📝 核心认知

### perf 的两种模式

```
perf stat   = 计数模式 → "执行了多少条指令？缓存命中率？"
              程序跑完才出结果，像一个"超级 time"

perf record = 采样模式 → "CPU 时间花在哪个函数？"
              每秒拍几千张快照，记录 CPU 正在哪一行代码
              （实验 03 会讲）
```

### 两个最重要的指标

| 指标 | 公式 | 含义 |
|------|------|------|
| **IPC** | instructions ÷ cycles | 每个周期执行几条指令。> 1.5 = 高效，< 0.5 = 停顿严重 |
| **cache-miss ratio** | cache-misses ÷ cache-references | 缓存未命中比例。> 10% = 内存访问模式可能需要优化 |

---

## 🧪 试试看

1. 用 `perf stat ls` 和 `perf stat find /usr -name "*.so"` 对比——`ls` 和 `find` 的 IPC 有什么不同？为什么？
2. 把 `hello.c` 中 `compute_heavy()` 的循环次数从 10,000,000 改成 100,000,000——IPC 和 branch-misses 变了吗？
3. `perf stat -e cycles,instructions,cache-references,cache-misses find /usr -name "*.so" > /dev/null`——`find` 的 cache-miss 比例比 `./hello` 高还是低？为什么？
4. 试试 `perf stat -d ./hello`（`-d` 是详细模式）——比默认输出多了哪些事件？
5. 对比 `perf stat ls` 和 `perf stat -e task-clock,context-switches ls`——哪个 `task-clock` 更短？为什么两次跑的数值不一样？（提示：perf 每次运行都会受系统当前负载影响）

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `perf stat ./program` | 统计模式——程序跑完后打印硬件计数器汇总 |
| `perf stat -e 事件1,事件2 ./program` | 只统计指定事件 |
| `perf stat -d ./program` | 详细模式——比默认多显示 L1 缓存等事件 |
| `perf list` | 列出当前系统支持的所有事件 |
| `perf list hw` | 只列出硬件事件 |
| `perf list sw` | 只列出软件事件 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `perf not found` | perf 未安装 | `sudo apt install linux-tools-common linux-tools-generic` |
| `perf_event_open() failed` | `perf_event_paranoid` 太高 | `sudo sysctl kernel.perf_event_paranoid=1` |
| `<not supported>` 出现在某些事件旁 | 当前 CPU/内核不支持该事件 | 正常——WSL2 虚拟化会限制部分硬件事件 |
| 每次跑 `perf stat` 数值都不一样 | CPU 频率变化、其他进程干扰、缓存状态不同 | 正常——多跑几次取平均，或固定 CPU 频率 |
| `perf stat` 输出和程序输出混在一起 | perf 报告也打到 stderr | 正常——都输出到终端所以混在一起，用 `-o` 写到文件或 `2>&1` 重定向 |

---

## 下一步

你现在能用 `perf stat` 看到程序运行时的 CPU 计数器了。下一步学习 [实验 02：读懂计数器](../02-perf-stat/)——深入理解每个计数器的含义，以及如何用它们的**比值**快速诊断性能问题。
