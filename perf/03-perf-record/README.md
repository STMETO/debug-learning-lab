# 实验 03：采样与报告 —— 哪个函数在吃 CPU？

> 🎯 **目标**：用 `perf record` 采集性能样本，用 `perf report` 找出热点函数，理解调用链。
>
> ⏱️ 约 20 分钟

## 先看示例程序

[workload.c](workload.c) 是一个多函数的程序，每个函数做的事差别很大：

| 函数 | 角色 | 设计意图 |
|------|------|---------|
| `hot_calc()` | 绝对热点 | 3000 万次算术循环——吃掉 ~57% CPU |
| `medium_sort()` | 中等热点 | 5000 元素的冒泡排序——吃掉 ~41% CPU |
| `cold_init()` | 冷函数 | 只跑一次，100 次小循环——几乎不消耗 CPU |
| `helper()` | 辅助函数 | 被 cold_init 调用 1000 次——每次工作量很小，但累计会出现在 profile 中 |

```bash
make && ./workload
```

---

## 实验开始

### 第一步：第一次采样 —— perf record

```bash
perf record -o perf.data ./workload
```

输出：

```
=== Multi-function Workload ===
Step 1: cold_init (one-time setup)
  [cold_init] helper total = 416500
Step 2: hot_calc (the big hotspot)
  [hot_calc] sum = 150086538941
Step 3: medium_sort (moderate work)
  [medium_sort] arr[0]=1 arr[4999]=5000
=== Done! ===
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.034 MB perf.data (665 samples) ]
```

> **665 samples** — perf 在程序运行期间"拍了 665 张快照"。每张快照记录了一件事：**此刻 CPU 正在执行哪个函数、哪条指令**。

### 第二步：什么是采样？

```
时间线 →
CPU:  ═══hot_calc══╗  ═══medium_sort══╗  ══hot_calc══╗  ═══medium_sort══╗ ...
                    ║                  ║              ║                  ║
采    ↓     ↓  ↓      ↓   ↓    ↓  ↓  ↓    ↓     ↓  ↓   ↓    ↓  ↓  ↓  ↓    ↓
样点  ·     ·  ·      ·   ·    ·  ·  ·    ·     ·  ·   ·    ·  ·  ·  ·    ·
                    ↑                  ↑              ↑                  ↑
              hot_calc 占多数    medium_sort 占多数   ...

统计：hot_calc = 380 次采样 / 665 = 57.22%
      medium_sort = 272 次采样 / 665 = 40.86%
      helper = 1 次采样 / 665 = 0.12%
      ...
```

perf record **不是**在跟踪每一次函数调用（那是 ltrace 做的事），它是**每隔固定数量的 CPU 周期**拍一张快照。函数执行的 CPU 时间越长，被拍到的概率就越大。

> 这就是 perf 的核心洞察：**采样次数 ∝ CPU 时间占比**。hot_calc 消耗了 ~57% 的 CPU 时间，就被拍到了 ~57% 的样本。

### 第三步：perf report —— 看热点

```bash
perf report -i perf.data
```

（会进入交互式 TUI 界面——按 `q` 退出）

用 `--stdio` 以文本模式直接打印：

```bash
perf report -i perf.data --stdio
```

```
# Samples: 665  of event 'cycles'
# Event count (approx.): 507251762
#
# Overhead  Command   Shared Object      Symbol
# ........  ........  .................  .............................
#
    57.22%  workload  workload           [.] hot_calc
    40.86%  workload  workload           [.] medium_sort
     0.21%  workload  [kernel.kallsyms]  [k] check_preemption_disabled
     0.16%  workload  [kernel.kallsyms]  [k] hrtimer_interrupt
     0.16%  workload  workload           [.] main
     0.12%  workload  workload           [.] helper
     0.08%  workload  [kernel.kallsyms]  [k] __memcg_kmem_charge_page
```

**解读每列**：

| 列 | 含义 | 示例 |
|----|------|------|
| **Overhead** | 该函数占的 CPU 采样比例 | `hot_calc` 占 57.22% = 超过一半的 CPU 时间 |
| **Command** | 进程名 | `workload` = 我们的程序 |
| **Shared Object** | 该函数属于哪个文件 | `workload` = 我们编译的 ELF；`[kernel.kallsyms]` = 内核 |
| **Symbol** | 函数名 | `[.]` = 用户态函数；`[k]` = 内核函数 |

**关键发现**：

1. **hot_calc + medium_sort = 98%**：两个函数吃掉了几乎全部 CPU 时间。优化其他任何函数（包括 helper、main）都不会有明显效果。

2. **helper 只占 0.12%**：虽然被调用了 1000 次，但每次工作量很小——不要凭"调用次数"判断热点，要凭"CPU 时间占比"。

3. **内核函数也有少量采样**：`hrtimer_interrupt`、`check_preemption_disabled` 等是内核在处理定时器中断——每次 perf 采样本身也会触发少量内核开销。

### 第四步：调用链 —— 谁调用了热点函数？

热点函数找到了，但 **"是谁调用了它"** 同样重要。加 `-g` 启用调用链（call-graph）采集：

```bash
perf record -g -o perf.data ./workload
perf report -i perf.data --stdio
```

```
# Children      Self  Command   Shared Object      Symbol
# ........  ........  ........  .................  .....................................
#
    99.48%     0.00%  workload  workload           [.] main
            |
            ---main
               |
               |--56.88%--hot_calc
               |
                --42.60%--medium_sort

    56.88%    56.42%  workload  workload           [.] hot_calc
            |
             --56.42%--main
                       hot_calc

    42.60%    42.27%  workload  workload           [.] medium_sort
```

**新出现的两列**：

| 列 | 含义 |
|----|------|
| **Children** | 该函数**及其调用的所有子函数**总共占的采样比例 |
| **Self** | 该函数**自身代码**占的采样比例 |

**解读 main 那一行**：

- `Self = 0.00%`：main 自己的代码（几个 printf）几乎不消耗 CPU
- `Children = 99.48%`：但 main 调用的 `hot_calc` + `medium_sort` 占了 99.48%
- 调用树：`main → hot_calc (56.88%)` + `main → medium_sort (42.60%)`

> `Children` 告诉你函数的"总影响力"，`Self` 告诉你函数"自身的开销"。如果一个函数 Children 很高但 Self 很低，说明它是"组织者"——真正干活的是它调用的子函数。

### 第五步：perf report 交互式操作

不加 `--stdio` 进入交互式 TUI（推荐日常使用）：

```bash
perf report -i perf.data
```

常用按键：

| 按键 | 作用 |
|------|------|
| `↑` `↓` | 上下移动光标 |
| `Enter` | 展开/折叠调用链 |
| `+` | 展开当前项的一层调用链 |
| `e` | 展开全部调用链 |
| `c` | 折叠全部调用链 |
| `/` | 搜索函数名 |
| `a` | 对当前函数运行 `perf annotate`（实验 04 会讲） |
| `q` | 退出 |
| `?` | 显示所有快捷键 |

> 试试在 `hot_calc` 上按 `Enter` 展开调用链，你会看到它的调用者（callee tree）。

---

## 📝 核心认知

### perf stat vs perf record

```
perf stat   → "程序总共跑了 1.5 亿个周期，IPC = 1.45"
              汇总报告——告诉你"总体的快慢"

perf record → "hot_calc 吃了 57% 的 CPU，medium_sort 吃了 41%"
              采样报告——告诉你"哪些函数在吃 CPU"
```

**工作流**：先用 `perf stat` 判断"慢的类型"（计算？内存？分支？），再用 `perf record/report` 找到"慢的具体位置"。

### 什么是"样本"？

每个样本不是一次函数调用，而是 perf 在特定时刻拍的快照。默认每 3500 个 cycles 拍一张（受内核频率上限控制）。函数运行的时间越长，拍到的次数越多，Overhead 越高。

> ⚠️ 采样有统计误差。0.12% 的 helper 可能是采样噪声，不值得优化。重点关注 > 5% 的热点。

---

## 🧪 试试看

1. 把 `hot_calc` 的循环次数从 3000 万改成 100 万——`hot_calc` 和 `medium_sort` 的 Overhead 排名会反转吗？采样总数会变吗？
2. 找出本实验 `perf report` 中 kernel 函数占的总比例是多少——用户的 CPU 时间有多少花在了内核里？正常吗？
3. 用 `perf record -F 99 ./workload`（每秒 99 次采样，而不是按周期采样）——采样数有什么变化？什么场景下用 `-F` 比默认更好？
4. 跑 `perf record -g -- ./workload && perf report --stdio`，在调用链输出中找到 `helper` 是被谁调用的（提示：在 `cold_init` 的 Children 下面找）。
5. 试试 `perf report -i perf.data --sort=dso,symbol`——按共享库排序，看 libc 消耗了多少？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `perf record ./program` | 采样模式——采集 CPU 使用样本到 perf.data |
| `perf record -g ./program` | 采样 + 调用链（call-graph） |
| `perf record -o 文件 ./program` | 指定输出文件（默认 perf.data） |
| `perf record -F N ./program` | 以 N Hz 频率采样（替代默认的事件周期方式） |
| `perf report -i 文件` | 交互式查看采样报告 |
| `perf report -i 文件 --stdio` | 文本模式打印报告 |
| `perf report -i 文件 --sort=dso,symbol` | 按指定列排序 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `perf.data` 中只有内核符号 | 编译时忘了 `-g` | 加 `-g` 重新编译 |
| 所有 Symbol 显示为地址（`0x0000...`） | 没加 `-g` 或 strip 了二进制 | 重新编译并保留调试符号 |
| 采样数很少（< 10） | 程序运行太短 | 增加循环次数，或用 `-F 999` 提高采样频率 |
| `failed to write perf.data: Permission denied` | 输出目录不可写，或 perf.data 被 root 拥有 | 用 `-o /tmp/perf.data` 或 `sudo rm perf.data` |

---

## 下一步

你现在能用 `perf record/report` 找到热点函数了。但如果想知道"热点函数里**具体哪一行代码**最耗时"，需要钻进去：下一步 [实验 04：源码级分析](../04-perf-annotate/)。
