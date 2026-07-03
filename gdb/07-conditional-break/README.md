# 实验 07：条件断点 —— 不要让我按 500 次 continue

> 🎯 **目标**：学会用条件断点，只在满足特定条件时才停下来。
>
> ⏱️ 约 10 分钟
>
> 📖 **前置**：[实验 02](../02-breakpoints/)、[实验 04](../04-inspecting/)

## 痛点场景

```c
for (int j = 0; j < 1000; j++) {
    counter += j % 7;
    // 已知：当 counter == 126 时程序出 bug
    // 问题：你怎么在那一刻停下来？
}
```

如果设普通断点在这一行，你会停 **1000 次**，手动按 1000 次 `continue`。

这就是条件断点的用武之地——**只在条件成立时才停**。

---

## 先看示例程序

[find.c](find.c) 有两个场景：
1. 在 1~200 中寻找特殊的质数（正常流程）
2. 一个大循环，模拟"需要条件断点"的场景

```bash
make && ./find
```

---

## 实验开始

```bash
gdb ./find
```

### 第一步：设条件断点

语法：在 `break` 后面加 `if 条件`：

```bash
(gdb) break 50 if counter == 126
```

输出：

```
Breakpoint 1 at 0x12f2: file find.c, line 50.
```

看起来和普通断点一模一样——条件并没有显示在输出里。怎么确认条件生效了？

```bash
(gdb) info breakpoints
```

输出：

```
Num     Type           Disp Enb Address            What
1       breakpoint     keep y   0x... in main at find.c:50
        stop only if counter == 126          ← 注意这一行！
```

GDB 在 `What` 列下面多了 `stop only if counter == 126`。

### 第二步：运行，验证条件触发

```bash
(gdb) run
```

程序跑完第一段质数输出（没有断点），进入大循环，然后在 `counter == 126` 的那一刻停住：

```
Breakpoint 1, main () at find.c:50
50          counter += j % 7;
```

验证：

```bash
(gdb) print counter
$1 = 126

(gdb) print j
$2 = 42
```

> 第 42 次迭代时 `counter` 恰好等于 126。你不用手动 `continue` 41 次——GDB 帮你跳过了所有不相关的迭代。

```
"大海捞针" → 条件断点帮你直接跳到针的位置
```

### 第三步：给已有断点加/改条件

假设你已经开始调试，设了一个普通断点，后来发现需要一个条件：

```bash
(gdb) break 50                 # 先设一个普通断点（假设是 #2）
(gdb) condition 2 counter == 126   # 给 2 号断点加上条件
```

改条件：

```bash
(gdb) condition 2 counter == 200   # 换成新条件
```

去掉条件（变回普通断点）：

```bash
(gdb) condition 2              # 不加条件 = 删除条件
```

### 第四步：条件可以是任意 C 表达式

条件断点支持任何返回 0（假）或非 0（真）的 C 表达式：

```bash
(gdb) break 36 if i > 100           # i 大于 100 时才停
(gdb) break 11 if n % 2 == 0        # n 是偶数时停
(gdb) break gcd if a < b            # a < b 时停（多文件场景）
(gdb) break 50 if counter > 200     # counter 超过 200 时停
(gdb) break 50 if j % 100 == 0      # 每 100 次迭代停一次
```

---

## 📝 命令小结

| 命令 | 作用 |
|------|------|
| `break 行号 if 条件` | 设条件断点 |
| `condition 断点号 条件` | 给已有断点加/改条件 |
| `condition 断点号` | 移除断点的条件（变为普通断点） |
| `info breakpoints` | 查看所有断点及其条件 |

## 🧪 试试看

1. 设 `break 50 if j == 500`，程序会在这条断点停下吗？那时 `counter` 是多少？
2. 设 `break 36 if i % 10 == 0`，观察哪些质数在 10 的倍数位置出现
3. 设条件断点 `break 50 if j == 999`——这是最后一次迭代，和 `break 50 if counter == 2997` 等价吗？
4. 条件断点和普通断点可以共存吗？设一个普通断点在 50 行，再设一个条件断点在 50 行——谁会先触发？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 条件断点从来没触发 | 条件从未成立（比如 `counter == 137` 被跳过） | 用 `print` 先验证条件值是否会出现 |
| 程序变慢了 | 条件断点每次都要计算表达式 | 正常——GDB 每执行一行都要暂停评估条件，大循环时用 `j % 100 == 0` 减少触发频率 |
| `condition` 报 `No breakpoint number N` | 断点编号不存在 | 先 `info breakpoints` 确认编号 |

---

## 下一步

条件断点让你在循环中精准定位。但有些 bug 更棘手——程序在**你不在场的时候**崩溃了（比如服务器上的定时任务半夜崩了）。这时你需要 [实验 08：Core Dump 分析](../08-core-dump/)——学会分析程序崩溃后留下的"尸体"，回溯案发现场。
