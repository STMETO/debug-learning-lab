# 实验 01：Hello GDB

> 🎯 **目标**：第一次启动 GDB，运行一个程序，然后优雅地退出。
>
> ⏱️ 约 10 分钟

## 先跑起来

不看任何概念，先让程序在 GDB 里跑一遍：

```bash
# 1. 确认你在本实验目录下
pwd    # 应该看到 .../gdb/01-hello-gdb

# 2. 编译（注意 -g 参数，它生成调试信息）
make

# 3. 启动 GDB
gdb ./hello
```

你会看到 GDB 的欢迎信息，最后一行是 `(gdb)`——这是 GDB 在等你输入命令。

```bash
# 4. 在 GDB 里运行程序
(gdb) run
```

输出：

```
Starting program: .../01-hello-gdb/hello
x = 10, y = 20, sum = 30
[Inferior 1 (process ...) exited normally]
```

程序正常运行完毕。现在退出：

```bash
# 5. 退出 GDB
(gdb) quit
```

恭喜！你刚刚完成了人生中第一次 GDB 调试。🎉

---

## 刚刚发生了什么？

让我们拆解每一步：

```
┌──────────────────────────────────────────────────┐
│  终端                    GDB 进程                 │
│                                                     │
│  $ gdb ./hello    ─────►  GDB 启动，加载 hello      │
│                          等你下命令                  │
│                                                     │
│  (gdb) run        ─────►  GDB 启动 hello 子进程     │
│                          hello 正常执行完毕          │
│                                                     │
│  (gdb) quit       ─────►  GDB 关闭                  │
│                          回到终端                    │
└──────────────────────────────────────────────────┘
```

这里有一个关键角色：[Makefile](Makefile) 里的 `-g` 参数。

### 为什么需要 `-g`？

来做个小实验对比：

```bash
# 不带 -g 编译
gcc -o hello_nodb hello.c

# 带 -g 编译  
gcc -g -o hello_dbg hello.c

# 比较文件大小
ls -lh hello_nodb hello_dbg
```

你会发现 `hello_dbg` 更大。多出来的不是垃圾，而是 **调试符号（debug symbols）**——一张"源代码地图"，告诉 GDB：

- 哪段机器码对应 `hello.c` 的第几行
- 变量 `x` 存放在内存的哪个位置
- 函数 `add` 的入口地址在哪

> 💡 **关键认知**：`-g` 不会让你的程序变慢，它只是在可执行文件里多放了元数据。调试时用 `-g`，发布时去掉，这是标准做法。

---

## 你的第一个调试操作

这次我们不只是运行，而是让程序 **停在某个位置**，看看里面的状态。

```bash
gdb ./hello
```

### 1. 在 main 函数入口设一个断点

```bash
(gdb) break main
```

输出类似：

```
Breakpoint 1 at 0x1149: file hello.c, line 10.
```

> GDB 告诉你：断点 1 设在地址 `0x1149`，对应 `hello.c` 第 10 行。

**"断点"** 是什么？就像在书里夹一张书签——程序运行到这里时会自动停下来，把控制权交给你。

### 2. 运行程序

```bash
(gdb) run
```

这次程序没有一口气跑完，而是停在 `main` 的第一行：

```
Breakpoint 1, main () at hello.c:10
10      int x = 10;
```

注意！第 10 行 **还没执行**——GDB 停在这一行**之前**。

### 3. 让程序执行一行

```bash
(gdb) next
```

输出：

```
11      int y = 20;
```

`next` 执行了第 10 行（`int x = 10;`），然后停在下一行（第 11 行）之前。

此时变量 `x` 已经存在了且值为 `10`。再执行一次 `next`，`y` 也会被创建。

### 4. 查看变量值

连续执行三次 `next`，到 `printf` 之前，然后：

```bash
(gdb) print x
$1 = 10

(gdb) print y
$2 = 20

(gdb) print sum
$3 = 30
```

**这就是调试的核心**——你不需要加 `printf`，直接在 GDB 里就能看到任何变量的当前值。

`$1`、`$2`、`$3` 是 GDB 为你每次 `print` 结果分配的编号，后面实验会用到。

### 5. 让程序跑完

```bash
(gdb) continue
```

程序执行完剩下的部分，正常退出。

---

## 📝 命令小结

这是你目前学到的全部命令——只有 6 个：

| 命令 | 简写 | 作用 |
|------|------|------|
| `gdb ./程序名` | — | 用 GDB 加载一个程序 |
| `run` | `r` | 运行（或重新运行）程序 |
| `break 函数名` | `b main` | 在函数入口设断点 |
| `next` | `n` | 执行当前行，**不进入**函数内部 |
| `print 变量名` | `p x` | 打印变量的当前值 |
| `continue` | `c` | 继续运行直到下一个断点（或程序结束） |
| `quit` | `q` | 退出 GDB |

> 💡 GDB 中几乎所有命令都可以简写：`r` = `run`, `n` = `next`, `p` = `print`, `c` = `continue`, `q` = `quit`。熟练之后你会爱上简写。

---

## 🧪 试试看

1. 把 `break main` 换成 `break add`，看看程序会停在哪里？
2. 停在 `add` 函数里的时候，用 `print a` 和 `print b` 能看到什么？
3. 如果把 Makefile 里的 `-g` 去掉再编译，`break main` 还能用吗？

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `No symbol table is loaded` | 编译时没加 `-g` | 检查 Makefile，确保有 `-g` |
| `No executable file specified` | 启动 GDB 时没指定程序 | `gdb ./hello` 而不是 `gdb` |
| `The program is not being run` | 还没执行 `run` 就想 `next` | 先 `run`，等程序停在断点后再 `next` |

---

## 下一步

你已经掌握了 GDB 最基本的工作流：**设断点 → 运行 → 单步 → 看变量 → 继续**。

下一实验会深入学习断点：[实验 02：断点](../02-breakpoints/)——如何在任意位置设断点、查看所有断点、删除断点。
