# 实验 04：源码级分析 —— 时间花在哪一行？

> 🎯 **目标**：用 `perf annotate` 从热点函数深入到源代码/汇编级别，精确定位"哪一行代码、哪一条指令"最耗时。
>
> ⏱️ 约 15 分钟

## 先看示例程序

[hotspot.c](hotspot.c) 只有一个热点函数——但它内部包含四种不同开销的操作：

| 代码行 | 操作类型 | 预期开销 |
|--------|---------|---------|
| `val = data[i] * 7 + 13` | 乘法/加法运算 | 低（~1-3 个周期） |
| `val = val % 9973` | 取模（除法）运算 | **高**（~20-80 个周期） |
| `if (val > 5000)` | 条件分支 | 低（~1 个周期，预测准确） |
| `data[i] = val` | 内存写入 | 中（~几个周期，视缓存而定） |

> 这些"预期"是猜测——`perf annotate` 能给你**精确数据**而非猜测。

```bash
make && ./hotspot
```

---

## 实验开始

### 第一步：采样 → 找热点 → 钻进去

```bash
perf record -o perf.data ./hotspot
perf report -i perf.data --stdio | head -15
```

```
# Overhead  Command   Shared Object      Symbol
# ........  ........  .................  .............................
#
    98.72%  hotspot   hotspot            [.] hotspot
     0.58%  hotspot   [kernel.kallsyms]  [k] native_write_msr
     0.29%  hotspot   [kernel.kallsyms]  [k] _raw_spin_unlock_irqrestore
```

`hotspot` 函数占了 98.72%——它就是我们要分析的目标。在 `perf report` 交互界面中，光标移到 `hotspot` 上按 `a` 即可进入 annotate。或者直接用命令行：

```bash
perf annotate -i perf.data --stdio hotspot
```

### 第二步：阅读 annotate 输出

输出是**源代码 + 汇编 + 时间百分比**的三栏格式：

```
 Percent |	Source code & Disassembly of hotspot for cycles (33 samples, percent: local period)
---------------------------------------------------------------------------------------------------
         :
         : 13   // hotspot: the function we'll drill into with perf annotate
         : 14   void hotspot(long *data, int n) {
    0.00 :   11a9:   endbr64
    0.00 :   11ad:   push   %rbp
    0.00 :   11ae:   mov    %rsp,%rbp
        ...
         : 17   // Line A: arithmetic — cheap (few cycles)
         : 18   long val = data[i] * 7 + 13;
    6.05 :   11dd:   mov    -0x28(%rbp),%rax
    3.05 :   11e1:   add    %rdx,%rax
    3.02 :   11e7:   mov    %rdx,%rax
         :
         : 21   // Line B: modulo — expensive (many cycles)
         : 22   val = val % 9973;
    9.13 :   120d:   lea    (%rdx,%rcx,1),%rax
    9.10 :   1215:   mov    %rcx,%rdx
   15.17 :   1226:   mov    %rcx,%rax
   12.09 :   122c:   mov    %rax,-0x8(%rbp)
         :
         : 24   // Line C: conditional — branch
         : 25   if (val > 5000) {
    9.03 :   1230:   cmpq   $0x1388,-0x8(%rbp)
    6.06 :   123a:   mov    -0x8(%rbp),%rax
    9.11 :   1244:   mov    -0x8(%rbp),%rax
         :
         : 31   // Line D: store back (memory write)
         : 32   data[i] = val;
    9.14 :   1264:   mov    %rax,(%rdx)
```

**每列含义**：

| 列 | 含义 |
|----|------|
| **Percent** | 该指令占的**采样比例**（相对于本函数的总采样数）。越高 = 越耗时 |
| **地址** | 指令的虚拟地址（如 `11dd`） |
| **汇编指令** | x86-64 汇编 |
| **源代码行** | 穿插在汇编之间的 C 源码行 |

### 第三步：找出最贵的操作

把每行的百分比按 C 语义分组：

| C 代码 | 累计百分比 | 判断 |
|--------|-----------|------|
| `long val = data[i] * 7 + 13` | ~12% | 乘法+加法，开销尚可 |
| **`val = val % 9973`** | **~48%** | 🔴 **绝对热点！取模运算吃掉了近一半时间** |
| `if (val > 5000) { ... } else { ... }` | ~24% | 分支本身开销不高 |
| `data[i] = val` | ~12% | 内存写入 |

**结论一目了然**：`val = val % 9973` 这一行占了函数 48% 的时间。如果要优化这个函数，**换掉取模**或**用位运算替代**（如果模数是 2 的幂）就是最大的收益。

> 在没有 perf annotate 之前，你只能靠"直觉"猜测哪行最慢。有了 annotate，**数字告诉你答案**，不需要猜。

### 第四步：为什么取模这么慢？

看 `val % 9973` 对应的汇编——它不是一条 `div` 指令，而是编译器的"除常数优化"展开：

```
    9.13 :   120d:   lea    (%rdx,%rcx,1),%rax     ← 地址计算
    9.10 :   1215:   mov    %rcx,%rdx              ← 数据移动
    3.03 :   1218:   sar    $0x3f,%rdx             ← 算术右移
   15.17 :   1226:   mov    %rcx,%rax              ← 🔴 最热单指令
   12.09 :   122c:   mov    %rax,-0x8(%rbp)        ← 🔴 存储结果
```

编译器用乘法+移位+减法来模拟取模（因为 `div` 指令更慢——~80 个周期），但即便如此，这组指令加起来仍然是整个函数最昂贵的部分。

> 注意 `15.17%` 和 `12.09%`——它们只是普通的 `mov` 指令，但因为**取模运算被展开成多条指令**，总开销分摊到了这些看似简单的指令上。annotate 帮你看到的是"整体热度"，而非单条指令的绝对成本。

### 第五步：annotate 的交互式操作

不加 `--stdio` 进入交互模式：

```bash
perf annotate -i perf.data hotspot
```

| 按键 | 作用 |
|------|------|
| `↑` `↓` | 上下滚动 |
| `Enter` | 跳转到跳转目标/调用目标 |
| `Esc` | 返回上一级 |
| `t` | 切换百分比显示模式（周期/采样数） |
| `H` | 在源代码和汇编之间切换 |
| `o` | 在新窗口中打开（如果终端支持） |
| `q` | 退出 |

> 在 `hotspot` 的输出中，找到 `jle` 那一行按 `Enter`——你会跳到跳转目标，直观看到 if-else 的分支结构。

---

## 📝 核心认知

### perf report → perf annotate 的分析链路

```
perf stat ./program          → "这程序整体慢，IPC < 1，cache-miss 高"
perf record ./program        → 采样，生成 perf.data
perf report                  → "hot_func() 占了 60%，先看它"
perf annotate hot_func       → "原来第 23 行的取模运算占了 48%，考虑用位运算替代"
改代码 → 重新验证
```

### 百分比的含义

annotate 中的百分比是 **本地占比**（local period）——该指令的采样数 ÷ 本函数总采样数。它回答的是"这个函数的时间里，百分之多少花在了这条指令上"，而不是"整个程序的百分之多少"。

> `15.17%` 在 annotate 中，对应 `hotspot` 占了 98% 总开销——所以这条指令的实际全局占比是 `15.17% × 98% ≈ 14.9%`。

---

## 🧪 试试看

1. 在 `hotspot.c` 中把 `val % 9973` 改成 `val & 0x3FF`（位运算替代取模）——重新 `perf record && perf annotate hotspot`，热点分布怎么变？哪行变成最热的了？
2. 对实验 03 的 `workload` 程序跑 `perf annotate hot_calc`——那个函数里最热的是哪条指令？和本实验的 `hotspot` 有什么相似之处？
3. 在 annotate 交互界面中按 `t` 切换百分比模式——`Period` 和 `Samples` 两种模式有什么不同？
4. 用 `perf annotate --source hotspot`（只显示源码+百分比，不显示汇编）——是不是更易读？你更喜欢哪个模式？
5. 试试 `perf annotate --stdio --no-source hotspot`——只显示纯汇编。如果函数很大，这个模式有时更快定位热点指令。

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `perf annotate -i perf.data 函数名` | 交互式查看函数的源码/汇编级热点 |
| `perf annotate --stdio 函数名` | 文本模式打印——适合复制粘贴 |
| `perf annotate --source 函数名` | 只显示源码行+百分比（隐藏汇编） |
| `perf annotate --no-source 函数名` | 只显示汇编+百分比（隐藏源码） |
| 在 `perf report` 中按 `a` | 从 report 直接跳转到当前函数的 annotate |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `The perf.data data has no samples!` | perf.data 是空的或只记录了非当前函数的事件 | 重新 `perf record`，确保程序运行足够长 |
| annotate 中看不到源码 | 编译时没加 `-g` | 加 `-g` 重新编译 |
| annotate 中所有百分比都是 0.00% | 采样数太少（只有个位数样本） | 增加程序运行时间或循环次数 |
| annotate 显示 `<error: ...>` | perf 找不到源文件 | 在编译目录下运行 `perf annotate`，或用 `--symfs` 指定路径 |
| annotate 中的汇编看不懂 | 不熟悉 AT&T 语法 | 这是 x86-64 默认语法——没关系的，**只需关注百分比最高的那几行** |

---

## 下一步

你现在能从源码行级定位性能瓶颈了——perf stat 看宏观，perf record/report 找热点函数，perf annotate 定位热点指令。下一步学习 [实验 05：火焰图](../05-flamegraph/)——用可视化工具一眼看懂整个调用栈的时间分布。
