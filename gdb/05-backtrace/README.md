# 实验 05：调用栈 —— 你的程序是怎么来到这里的？

> 🎯 **目标**：用 `backtrace` 看清函数调用链，并在不同层级之间跳转查看各自的参数和变量。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 04](../04-inspecting/)（你需要知道 `print` 和 `info locals`）

## 什么是"调用栈"？

每当一个函数被调用，计算机会在内存里分配一块空间给这个函数——存放它的参数、局部变量、以及"调用完该回哪里去"。这块空间叫**栈帧（stack frame）**。

```
函数调用链：                      栈（从上到下生长）：
                                 
main()                            ┌──────────┐
  └─ layer_a(1)                   │ main     │ ← 最外层
       └─ layer_b(1)              │ layer_a  │
            └─ layer_c(6)         │ layer_b  │
                 └─ layer_d(16)   │ layer_c  │
                                  │ layer_d  │ ← 当前位置（栈顶）
                                  └──────────┘
```

`backtrace` 让你在任意时刻看清整个塔的结构。

---

## 先看示例程序

[stack.c](stack.c) 有两个场景：

1. **递归**——`factorial(5)` 调用 `factorial(4)` 调用 `factorial(3)` ...
2. **深层调用链**——`main → layer_a → layer_b → layer_c → layer_d`

```bash
make && ./stack
```

---

## 实验开始

### 场景一：观察递归栈

```bash
gdb ./stack
```

```bash
(gdb) break factorial
(gdb) run
```

第一次停在 `factorial(n=5)`：

```
Breakpoint 1, factorial (n=5) at stack.c:11
11      if (n <= 1) {
```

```bash
(gdb) backtrace
```

输出：

```
#0  factorial (n=5) at stack.c:11
#1  0x... in main () at stack.c:40
```

目前只有两层：`factorial`（#0，当前）被 `main`（#1）调用。

> **#0 永远是你当前所在的函数**。数字越大，离你越远（越外层）。

连续 `continue` 几次，让递归来到 `n=2`：

```bash
(gdb) continue        # 第一停：n=5
(gdb) continue        # 第二停：n=4
(gdb) continue        # 第三停：n=3
(gdb) continue        # 第四停：n=2
```

现在看调用栈：

```bash
(gdb) backtrace
```

输出（简化）：

```
#0  factorial (n=2) at stack.c:11    ← 你在这一层
#1  factorial (n=3) at stack.c:14    ← #1 调用了 #0
#2  factorial (n=4) at stack.c:14    ← #2 调用了 #1
#3  factorial (n=5) at stack.c:14
#4  main () at stack.c:40            ← 最外层
```

**每一行就是一个栈帧**。读法：
- `#0` 是当前正在执行的 `factorial(2)`
- `#1` 是调用 `#0` 的 `factorial(3)`——它正在第 14 行，等待 `factorial(2)` 返回

### 场景二：在栈帧之间跳转

栈不只是用来"看"的——你可以跳到任意一层，查看那层的参数和变量：

```bash
(gdb) frame 2
```

输出：

```
#2  0x... in factorial (n=4) at stack.c:14
14      unsigned long long result = n * factorial(n - 1);
```

现在你在 `factorial(n=4)` 这一层。执行：

```bash
(gdb) print n
$1 = 4
```

你看到了 `#2` 层的参数，而不是 `#0` 层的。跳回最新一层：

```bash
(gdb) frame 0
(gdb) print n
$2 = 2
```

> 💡 **这就是调试的核心能力**：程序在深层崩溃了，但问题的根因可能在十几层之前的调用者那里——你可以在栈帧之间跳转，一层层追溯。

### 简化导航：up 和 down

```bash
(gdb) up         # 向调用方移动（数字 +1，往"外层"走）
(gdb) down       # 向被调用方移动（数字 -1，往"内层"走）
```

例：你在 #2，`up` 到 #1，再 `up` 到 #0，`down` 回到 #1。

### 限制 backtrace 的行数

有时候调用栈非常深（几百层），你只需要看顶部几层：

```bash
(gdb) backtrace 3        # 只看最近的 3 层
(gdb) backtrace full     # 显示所有局部变量（非常详细）
```

### 场景三：深层调用链

先把递归断点删掉，在调用链最深处停下：

```bash
(gdb) delete
(gdb) break layer_d
(gdb) run
```

（程序跑完递归部分后，会在 `layer_d` 停下）

```bash
(gdb) backtrace
```

输出：

```
#0  layer_d (x=16) at stack.c:23
#1  layer_c (x=6) at stack.c:27
#2  layer_b (x=1) at stack.c:31
#3  layer_a (x=1) at stack.c:35
#4  main () at stack.c:44
```

这就是整个调用链的可视化。现在跳转看看每一层收到的参数：

```bash
(gdb) frame 0          # layer_d，x = 16
(gdb) frame 1          # layer_c，x = 6
(gdb) frame 2          # layer_b，x = 1
```

注意参数的传递：
- `main` 调用 `layer_a(1)`——参数是 1
- `layer_a` 调用 `layer_b(x)`——参数还是 1
- `layer_b` 调用 `layer_c(x + 5)`——参数变成 6（1+5=6）
- `layer_c` 调用 `layer_d(x + 10)`——参数变成 16（6+10=16）

> 你可以在 `frame 0` 里 `print x` 得到 16，跳到 `frame 2` 再 `print x` 得到 1——**时间旅行般的体验**。

---

## 📝 命令小结

| 命令 | 简写 | 作用 |
|------|------|------|
| `backtrace` | `bt` | 显示完整调用栈 |
| `backtrace N` | `bt 3` | 只显示最近 N 层 |
| `backtrace full` | `bt full` | 显示调用栈 + 每层的局部变量 |
| `frame N` | `f 2` | 跳转到第 N 号栈帧 |
| `up` | — | 向调用方移动一层（往外） |
| `down` | — | 向被调用方移动一层（往内） |
| `info locals` | `i lo` | 当前栈帧的局部变量 |
| `info args` | `i ar` | 当前栈帧的函数参数 |
| `info frame` | `i f` | 当前栈帧的详细信息 |

## 🧪 试试看

1. 在 `factorial` 递归到 `n=1` 时，`backtrace` 能看到多少层？
2. 在 `layer_d` 的 `return x * x` 那行设断点，用 `info args` 查看每一层的 `x` 分别是多少
3. 为什么递归太深（比如 `factorial(100000)`）会导致程序崩溃？（提示：和调用栈有关）

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `backtrace` 只看到 `??` 和地址 | 编译时没加 `-g` 或栈被破坏 | 检查 Makefile 的 `-g`，或在更高层设断点 |
| `frame N` 报 `No frame N` | N 超过了栈的层数 | 先 `backtrace` 看有多少层 |
| 变量值看起来"不对" | 你可能在错误的 `frame` 里 | 用 `info frame` 确认当前所在层 |

---

## 下一步

你现在能"看见"程序的内部状态（实验 04），也能"导航"函数的调用关系（本实验）。但如果有个变量**在你不知道的地方被意外修改**了该怎么办？下一步学习 [实验 06：观察点](../06-watchpoints/)——让 GDB 帮你监控某个变量，值一变就自动停下，精确追踪修改来源。
