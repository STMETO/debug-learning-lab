# 实验 02：断点 —— 让程序停在你想要的地方

> 🎯 **目标**：在任意位置设置、查看、删除、开关断点。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 01](../01-hello-gdb/)（你至少要知道 `run`、`next`、`quit`）

## 先看示例程序

[loop.c](loop.c) 是一个计算 1²+2²+3²+4²+5² 的程序：

```c
// 三件我们感兴趣的事：
//   1. 循环跑了多少次、每次 i 和 sq 是多少
//   2. square() 函数被调用时传入了什么参数
//   3. is_even() 的判断逻辑是否按预期工作
```

先正常编译运行，知道正确输出是什么：

```bash
make && ./loop
```

输出：

```
i=1, i²=1 (奇数), total=0
i=2, i²=4 (偶数), total=1
i=3, i²=9 (奇数), total=5
i=4, i²=16 (偶数), total=14
i=5, i²=25 (奇数), total=30
最终结果: 1²+2²+3²+4²+5² = 55
```

---

## 实验开始

```bash
gdb ./loop
```

### 第一步：在多个位置设断点

GDB 提供了多种指定断点位置的方式。我们来设三个：

```bash
(gdb) break 17          # 在第 17 行设断点
(gdb) break square       # 在 square 函数入口设断点
(gdb) break is_even      # 在 is_even 函数入口设断点
```

输出：

```
Breakpoint 1 at 0x1193: file loop.c, line 17.
Breakpoint 2 at 0x1154: file loop.c, line 5.
Breakpoint 3 at 0x1167: file loop.c, line 10.
```

> 💡 三种定位方式：`break 行号`、`break 函数名`、也可以 `break 文件名:行号`（跨文件调试时有用）

### 第二步：查看已设置的断点

```bash
(gdb) info breakpoints
```

输出：

```
Num     Type           Disp Enb Address            What
1       breakpoint     keep y   0x0000000000001193 in main at loop.c:17
2       breakpoint     keep y   0x0000000000001154 in square at loop.c:5
3       breakpoint     keep y   0x0000000000001167 in is_even at loop.c:10
```

你会看到一个表格，每列的含义：

| 列 | 含义 | 说明 |
|----|------|------|
| Num | 编号 | 每个断点的唯一 ID，后续操作用这个号码 |
| Type | 类型 | `breakpoint`（普通断点）、`watchpoint`（观察点，后面实验会学） |
| Disp | 触发后行为 | `keep`=保持不删除，`del`=触发一次后自动删除 |
| Enb | 是否启用 | `y`=启用中，`n`=已禁用 |
| Address | 内存地址 | 断点在可执行文件中的位置 |
| What | 源码位置 | 对应哪个文件的哪一行/哪个函数 |

### 第三步：运行，看断点如何触发

```bash
(gdb) run
```

程序停在第一个断点——第 17 行：

```
Breakpoint 1, main () at loop.c:17
17          int sq = square(i);
```

此时 `i = 1`（循环第一次迭代），第 17 行还没执行。让我们继续：

```bash
(gdb) continue
```

这次触发了哪个断点？

```
Breakpoint 2, square (n=1) at loop.c:5
5       return n * n;
```

是断点 2——在 `square` 函数入口。`continue` 继续：

```
Breakpoint 3, is_even (n=1) at loop.c:10
10      return n % 2 == 0;
```

是断点 3——在 `is_even` 函数入口。

> 有没有一种 **"在程序内部装了很多监控摄像头"** 的感觉？你可以看到程序在各个关键位置来回穿梭。

### 第四步：删除不需要的断点

假设我们对第 17 行不再感兴趣了，想删掉它：

```bash
(gdb) delete 1          # 删除断点 1（就是第 17 行那个）
(gdb) info breakpoints   # 确认它已经没了
```

输出：

```
Num     Type           Disp Enb Address            What
2       breakpoint     keep y   ... in square at loop.c:5
3       breakpoint     keep y   ... in is_even at loop.c:10
```

> 断点编号 1 消失了。注意：编号**不会重新排列**——原来的 2 和 3 保持编号不变。

```bash
(gdb) continue
```

现在只剩下 `square` 和 `is_even` 的断点，程序在它们之间穿梭。反复按 `continue` 观察循环的每一次迭代。

### 第五步：临时关闭断点（不删除）

有时候你不想删断点，只是想"暂时不用它"：

```bash
(gdb) disable 2         # 关闭断点 2（square）
(gdb) info breakpoints
```

现在断点 2 的 `Enb` 列显示 `n`：

```
Num     Type           Disp Enb Address            What
2       breakpoint     keep n   ... in square at loop.c:5
3       breakpoint     keep y   ... in is_even at loop.c:10
```

```bash
(gdb) continue
```

程序**不再**停在 `square` 了，但 `is_even` 的断点仍然有效。想恢复？

```bash
(gdb) enable 2          # 重新启用断点 2
(gdb) delete             # 删除所有断点（不带编号 = 全删）
```

GDB 会确认：

```
Delete all breakpoints? (y or n) y
```

```bash
(gdb) continue
```

程序直接跑到结束。

---

## 📝 命令小结

| 命令 | 简写 | 作用 |
|------|------|------|
| `break 17` | `b 17` | 在当前文件的第 17 行设断点 |
| `break 函数名` | `b square` | 在函数入口设断点 |
| `break 文件:行号` | `b loop.c:10` | 在指定文件的某行设断点 |
| `info breakpoints` | `i b` | 列出所有断点 |
| `delete 编号` | `d 1` | 删除指定断点 |
| `delete` | `d` | 删除**全部**断点（会提示确认） |
| `disable 编号` | `dis 2` | 暂时关闭断点（保留但不触发） |
| `enable 编号` | `en 2` | 重新启用断点 |
| `continue` | `c` | 继续运行到下一个断点（或程序结束） |

## 🧪 试试看

1. 试一下 `break 16`（for 循环那行），每次 `continue` 看到 `i` 的变化是什么样的？
2. 再试试 `break 17` 呢？你发现了什么？for循环~
4. 同时设 3 个断点，用 `disable` 轮流开关，观察哪些断点触发了
5. 如果对一个不存在的行号设断点（比如 `break 999`），GDB 会怎么提示？
6. 试试 `break loop.c:5 if n == 3`——这设置了"条件断点"，我们会在实验 07 深入

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `No line 17 in current file` | GDB 的"当前文件"不是你想要的 | `break loop.c:17` 明确指定文件 |
| 断点触发了但值看起来不对 | 断点停在那行**执行之前** | 用 `next` 执行后再用 `print` 看 |
| `disable` 后还在触发 | 可能关错了编号 | `info breakpoints` 确认 `Enb` 列为 `n` |

---

## 下一步

你已经能精确控制程序的暂停位置了。下一步学习 [实验 03：单步执行](../03-stepping/)——程序停住后，怎么让它**按要求一步一步走**：跳进函数、跳过函数、或直接跑完当前函数。
