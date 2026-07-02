# 实验 03：单步执行 —— next vs step，学会"看穿"函数调用

> 🎯 **目标**：掌握 `next`（不进入函数）、`step`（进入函数）、`finish`（跳出函数）的区别。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 01](../01-hello-gdb/)、[实验 02](../02-breakpoints/)

## 先看示例程序

[calc.c](calc.c) 用了一个故意拆分得很细的函数链来演示：

```
main()
  └─ calculate(x)
       ├─ multiply(x, 2)   ← 第 1 次调用
       ├─ multiply(x, 3)   ← 第 2 次调用
       └─ add(a, b)        ← 把两个结果加起来
```

```bash
make && ./calc
```

输出：

```
input=4, result=20
```

（推导：4×2 + 4×3 = 8 + 12 = 20 ✓）

---

## 核心问题：next 和 step 到底有什么区别？

很多初学者会搞混。一句话说清楚：

> - `next`：执行当前行，**跳过**里面的函数调用，直接到下一行
> - `step`：执行当前行，如果这行有函数调用，就**跳进去**到函数的第一行

用图表示：

```
程序当前停在 →  18  int doubled = multiply(x, 2);

  next → 停在 19（不进入 multiply）
  step → 停在 11（进入 multiply 函数内部）
```

---

## 实验开始

```bash
gdb ./calc
```

```bash
(gdb) break calculate
(gdb) run
```

程序停在 `calculate` 函数的第一行：

```
Breakpoint 1, calculate (x=4) at calc.c:18
18      int doubled = multiply(x, 2);   // 进入 multiply
```

> 此时 `x=4`，第 18 行还没执行。

### 场景一：用 `next` —— 跳过函数内部

```bash
(gdb) next
```

输出：

```
19      int tripled = multiply(x, 3);   // 再次进入 multiply
```

第 18 行执行完毕！`multiply(4, 2)` 被调用了，但 GDB **没有带你进入 `multiply` 函数内部**——它直接算完结果返回到第 19 行。

你可以验证：

```bash
(gdb) print doubled
$1 = 8
```

> `next` 的思维模型：**"我对这个函数内部不感兴趣，直接告诉我下一行就好"**。

再来一次 `next`：

```bash
(gdb) next
```

```
20      int sum = add(doubled, tripled); // 进入 add
```

```bash
(gdb) print tripled
$2 = 12
```

两次 `next`，两次跳过了 `multiply` 内部。

### 场景二：用 `step` —— 进入函数内部

现在你对 `add` 函数的内部实现感兴趣了：

```bash
(gdb) step
```

输出：

```
add (a=8, b=12) at calc.c:12
12      int result = a + b;
```

你**进入了 `add` 函数**！能看到参数 `a=8`, `b=12`。现在你在 `add` 函数内部，可以一步步看它的执行：

```bash
(gdb) next
```

```
13      return result;
```

```bash
(gdb) print result
$3 = 20
```

> `step` 的思维模型：**"这个函数里发生了什么？带我进去看看"**。

### 场景三：用 `finish` —— 从函数中跳出来

你现在在 `add` 函数里面，想直接回到调用方（`calculate`）：

```bash
(gdb) finish
```

输出：

```
Run till exit from #0  add (a=8, b=12) at calc.c:13
0x... in calculate (x=4) at calc.c:20
20      int sum = add(doubled, tripled);
Value returned is $4 = 20
```

GDB 执行完 `add` 的剩余部分，返回 `calculate`，并且**告诉了你返回值是多少**（`$4 = 20`）。

> `finish` 的思维模型：**"这个函数我已经看够了，让我回到调用方"**。

---

## 三种单步方式对比

```
                           step →
 ┌──────────┐     ┌───────────────────────┐
 │ 调用方    │     │  被调用函数内部         │
 │          │     │                       │
 │  f(); ───┼─────┼→ 第 1 行              │
 │           │     │  第 2 行              │
 │  ←───────┼─────┼   return              │
 │ 下一行    │     │                       │
 └──────────┘     └───────────────────────┘
     ↑      next →           finish →
```

| 命令 | 简写 | 行为 | 什么时候用 |
|------|------|------|-----------|
| `next` | `n` | 执行当前行，**不进入**函数 | "这函数应该没问题，跳过" |
| `step` | `s` | 执行当前行，**进入**函数内部 | "这个函数里面可能有问题，我要仔细看" |
| `finish` | `fin` | 执行到当前函数 `return` 并返回调用方 | "我看够了，让我出去" |

### 一个判断法则

停在一行代码上时，问自己一个问题：

> **"bug 可能在这个函数里面吗？"**
>
> 可能 → `step`
> 不可能 → `next`
> 已经看清楚了 → `finish`

---

## 📝 命令小结

| 命令 | 简写 | 作用 |
|------|------|------|
| `next` | `n` | 执行当前行，**跨过**函数调用 |
| `step` | `s` | 执行当前行，**进入**函数调用 |
| `finish` | `fin` | 执行到当前函数返回，**跳出**函数 |
| `print 变量` | `p x` | 查看变量当前值 |
| `continue` | `c` | 继续运行到下一个断点 |

---

## 🧪 试试看

1. 这次在 `main` 而不是 `calculate` 设断点，看看 `calculate(input)` 这行用 `next` 和 `step` 分别有什么不同？
2. 停在 `calculate` 的第一行时，连续用 `step`，观察你会依次进入 `multiply` → `multiply` → `add` 的完整路径
3. 在 `multiply` 函数内部时，用 `finish` 会发生什么？返回值是多少？
4. 如果不小心 `step` 进了一个你不想看的函数（比如 `printf`），怎么最快出来？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `step` 和 `next` 看起来一样 | 当前行没有函数调用 | 两者等效——没有函数可进入时行为相同 |
| `step` 进入了奇怪的函数 | 当前行有库函数调用（如 `printf`） | 用 `finish` 快速出来，这种函数通常没问题 |
| `finish` 后程序直接结束了 | 你当时在 `main` 里 | `finish` 在 `main` 中等同于让程序跑完 |

---

## 下一步

现在你掌握了调试的"移动三件套"——`next`、`step`、`finish`。接下来学习 [实验 04：查看变量](../04-inspecting/)——如何更深入地检查程序状态：结构体、数组、指针，以及 `display` 自动显示。
