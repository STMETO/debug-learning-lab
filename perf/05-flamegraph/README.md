# 实验 05：火焰图 —— 一眼看懂调用栈的时间分布

> 🎯 **目标**：用 Brendan Gregg 的 FlameGraph 工具把 `perf.data` 转成交互式 SVG，一眼看懂整个调用栈的 CPU 时间分布。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 03](../03-perf-record/)（perf record + call-graph）

## 为什么需要火焰图？

`perf report` 的文本输出对简单的调用链还行，但面对**多层嵌套**的调用栈就力不从心了：

```
# perf report 文本输出：你得在脑子里"画出"调用树
main
├── 56.88% hot_calc    ← 只有一层，还行
└── 42.60% medium_sort

# 真实项目的调用栈可能有 10-20 层深，几十上百个函数
# 用文本看 → 眼睛花、找热点慢、没法一眼看出"最宽的是谁"
```

火焰图用**宽度 = CPU 时间占比**、**颜色和层级 = 调用栈深度**的方式把这一切可视化，一张 SVG 图就能回答"程序把 CPU 时间花在了哪条调用路径上"。

```
┌──────────────────────────────────────────────────────────┐
│  ████████████████████████████  main                       │
│  ████████████████▌ ██████▌  ███▌                          │
│  ██████████▌███▌   ██▌ ██▌ ██▌  ← 宽度 = CPU 时间占比     │
│  ████▌████▌        █▌      █▌                             │
│  ██▌  ██▌                                                 │
│  █▌                                                       │
│  phase_compute   phase_sort  phase_report                 │
└──────────────────────────────────────────────────────────┘
   60%              25%           15%
```

---

## 先看示例程序

[calltree.c](calltree.c) 设计了一个三层调用树：

```
main (100%)
├── phase_compute (60%)
│   ├── multiply_arrays (25%)
│   │   ├── inner_product (15%)
│   │   └── accumulate (10%)
│   └── normalize (20%)
│       ├── find_max (10%)
│       └── scale_values (10%)
├── phase_sort (25%)
│   ├── quick_sort (15%)     ← 递归函数，火焰图中会堆叠
│   └── partition (10%)
└── phase_report (15%)
    ├── format_output (10%)
    └── checksum (5%)
```

```bash
make && ./calltree
```

---

## 实验开始

### 第一步：确认 FlameGraph 工具

FlameGraph 是 Brendan Gregg 开源的一套 Perl 脚本，已作为 git submodule 存放在项目的 `lib/FlameGraph/` 目录下。

```bash
# 克隆项目时一并拉取 submodule
git submodule update --init

# 确认两个核心脚本可用
ls lib/FlameGraph/stackcollapse-perf.pl lib/FlameGraph/flamegraph.pl

# 确保系统有 perl（已预装）
which perl
```

> 这两个脚本只有几百行 Perl，没有其他依赖。路径指向项目根目录的 `lib/FlameGraph/`，不需要全局安装。

### 第二步：采集带调用栈的样本

```bash
perf record -g -o perf.data ./calltree
```

```
=== FlameGraph Demo: Multi-Level Call Tree ===
Phase 1: compute (60% of CPU)...
Phase 2: sort (25% of CPU)...
Phase 3: report (15% of CPU)...
  [report] checksum = 0
=== Done! ===
[ perf record: Woken up 26 times to write data ]
[ perf record: Captured and wrote 6.425 MB perf.data (8054 samples) ]
```

> **`-g` 是关键**：它让 perf 在每次采样时记录完整调用栈（不是在采样点停住，而是回溯整个函数调用链）。没有 `-g`，火焰图就退化成扁平的柱状图。

### 第三步：生成火焰图的三步转换

```bash
# 步骤 1：perf.data → 文本格式（perf script）
perf script -i perf.data > out.perf-script

# 步骤 2：折叠调用栈（stackcollapse）
lib/FlameGraph/stackcollapse-perf.pl out.perf-script > out.folded

# 步骤 3：生成 SVG
lib/FlameGraph/flamegraph.pl out.folded > flamegraph.svg
```

每一步做了什么：

```
perf.data (二进制)                                    [8054 个样本]
    │  perf script
    ▼
out.perf-script (文本，一行一个样本的完整调用栈)         [8054 行]
    │  stackcollapse-perf.pl
    ▼
out.folded (折叠格式，一行一个唯一调用栈 + 计数)         [~50 行]
    │  flamegraph.pl
    ▼
flamegraph.svg (交互式 SVG，浏览器打开)                 [~200KB]
```

### 第四步：阅读火焰图

用浏览器打开 `flamegraph.svg`（或 `firefox flamegraph.svg`）：

```
┌────────────────────────────────────────────────────────────────┐
│  [搜索框: _________]  🔍  ← 可以搜索函数名，会高亮该函数          │
│                                                                │
│  ██████████████████████████████████████  main                   │
│  ███████████████████████████▌ ████████▌ ██████▌                 │
│  ████████████████▌████▌       ████▌██▌  ████▌██▌               │
│  ██████████▌████▌             ████▌     ████▌                  │
│  ██████▌    ████▌                                               │
│  phase_    multiply  normalize  phase_   phase_                 │
│  compute   _arrays             sort     report                 │
└────────────────────────────────────────────────────────────────┘
```

**火焰图阅读法则**：

| 维度 | 含义 | 怎么看 |
|------|------|--------|
| **x 轴（宽度）** | CPU 时间占比 | 越宽 = 占用时间越多。**找最宽的矩形** |
| **y 轴（高度）** | 调用栈深度 | 越往上 = 调用层次越深。底层是调用者，顶层是被调用者 |
| **颜色** | 随机（无特殊含义） | 只是为了区分不同函数，便于视觉辨识 |
| **排列顺序** | 按函数名字母排序 | 不是按时间排列！x 轴从左到右是 a → z |

**从我们的火焰图能看到**：

1. 最下面一层是 `main`，占满整个宽度（100%）
2. `phase_compute` 占了 ~60% 宽度 → 最大热点
3. 在 `phase_compute` 上方，`multiply_arrays` 和 `normalize` 各占一部分
4. `quick_sort` 上方有自己（递归）→ 火焰图中表现为同一函数名的多个堆叠矩形
5. 点击任意矩形可以**放大**到那个函数及其子调用 → 这就是交互式的价值

### 第五步：交互式功能

在浏览器中打开 `flamegraph.svg`，试试这些操作：

| 操作 | 效果 |
|------|------|
| **点击矩形** | 放大该函数（水平填充窗口），显示它和它调用的子函数 |
| **搜索函数名** | 匹配的函数会高亮显示，背景函数变灰 |
| **鼠标悬停** | 显示函数名 + 采样数 + 百分比 |
| **点击 `Reset Zoom`** | 恢复全局视图 |

> 试试搜索 `quick_sort`——你会看到它在 `phase_sort` 区域中出现多次（递归层），每次都是同一个宽度但不同高度。

---

## 📝 核心认知

### 火焰图 vs perf report

| | perf report (文本) | FlameGraph (SVG) |
|------|-------------------|-------------------|
| 信息密度 | 高（数字精确） | 极高（一眼看全局） |
| 适合场景 | 快速定位热点函数名 | 理解调用关系和占比 |
| 可分享性 | 差（终端截图） | 好（SVG 可以发给任何人用浏览器看） |
| 搜索 | `/` 搜索函数名 | 搜索框 + 高亮 |
| 缩放 | 无 | 点击放大子调用 |

**最佳实践**：先 `perf report` 找热点函数名，再生成火焰图理解调用关系和汇报。

### 火焰图不只是"好看"

它是 Brendan Gregg 在 Netflix 做性能分析时的核心工具，原因：
- **一眼看出"意外的调用者"**——某个底层函数被一个意料之外的路径频繁调用
- **对比优化前后**——跑两次，生成两张火焰图，并排看宽度变化
- **发现"平板"（plateau）**——如果某层只有一个函数占满整个宽度，说明这一层没有分支，可能是瓶颈集中点

---

## 🧪 试试看

1. 把 `calltree.c` 中 `phase_compute` 的循环从 50 次改成 200 次——火焰图中 `phase_compute` 的宽度变化了吗？
2. 把 `quick_sort` 改成非递归的排序（如 bubble sort）——火焰图中还有递归的"堆叠"效果吗？
3. 生成两张火焰图：优化前（calltree.c）和优化后（自己改代码让某个函数跑更快）。在浏览器中并排打开——哪个函数的宽度明显变窄了？
4. 试试 `perf script -i perf.data | head -20`——看懂 `perf script` 输出的原始格式。调用栈中为什么有很多 `[unknown]` 和内核地址？
5. 用 `--call-graph dwarf` 替代默认的 `fp`（frame pointer）模式：`perf record --call-graph dwarf -o perf.data ./calltree`。生成的火焰图有什么不同？（如果 `dwarf` 报错，说明你的环境需要 `libunwind`）

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `perf record -g ./program` | 采样 + 调用栈（生成火焰图的第一步） |
| `perf script -i perf.data > out.perf-script` | 把 perf.data 转成折叠脚本的输入格式 |
| `stackcollapse-perf.pl out.perf-script > out.folded` | 折叠相同调用栈 |
| `flamegraph.pl out.folded > flamegraph.svg` | 生成火焰图 SVG |
| `firefox flamegraph.svg` | 在浏览器中打开（也可以用任何浏览器） |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 火焰图中只有 `[unknown]` | 编译时没加 `-g` | 加 `-g` 重新编译 |
| 火焰图很"矮"（只有 1-2 层） | `perf record` 没加 `-g` | 重新 `perf record -g` |
| `stackcollapse-perf.pl: not found` | submodule 未拉取 | `git submodule update --init` |
| SVG 在浏览器中是空白 | 生成过程中某步文件为空 | 检查 `out.perf-script` 是否有内容 |
| `--call-graph dwarf` 报错 | WSL2 中 libunwind 可能未安装 | 用默认的 fp 模式（`-g`） |
| `perf script` 输出中有 `[unknown]` | 内核或没有调试符号的库 | 正常——关注用户态函数即可 |

---

## 下一步

火焰图是性能分析的"可视化终极武器"。现在你拥有完整的工具链：stat → record → report → annotate → flamegraph。下一步 [实验 06：实战排查](../06-real-world/)——给你一个"不知道哪里慢"的程序，用这五个工具完整诊断。
