# 实验 12：信号处理 —— 当程序收到信号时，GDB 该怎么做？

> 🎯 **目标**：理解 GDB 对信号的默认行为，学会用 `handle` 控制信号捕获、打印和转发。
>
> ⏱️ 约 10 分钟
>
> 📖 **前置**：[实验 01](../01-hello-gdb/) 即可

## 信号和调试有什么关系？

你的程序在运行中可能收到各种信号：

| 信号 | 典型场景 | GDB 默认行为 |
|------|---------|-------------|
| `SIGINT` | 你按了 `Ctrl-C` | **GDB 拦截**，不传给程序 |
| `SIGSEGV` | 程序段错误 | GDB 停下来让你分析 |
| `SIGUSR1` | 自定义信号（用户定义） | GDB 停下来**并传给程序** |
| `SIGPIPE` | 向已关闭的管道写数据 | GDB 拦截 |
| `SIGALRM` | `alarm()` 超时 | GDB 拦截 |

> 在 GDB 外，`Ctrl-C` 直接杀死程序。在 GDB 内，`Ctrl-C` 只是暂停程序——因为 GDB 拦截了 `SIGINT`。

---

## 先看示例程序

[sig.c](sig.c) 安装了 `SIGUSR1` 和 `SIGINT` 的自定义处理器：

- `SIGUSR1`：计数但不退出
- `SIGINT`：要按 3 次才退出（故意反直觉）

```bash
make && ./sig
```

程序会不断循环，按 `Ctrl-C` 试试——你会发现普通的 `Ctrl-C` **杀不掉它**（因为它自己捕获了 SIGINT）。

---

## 实验开始

```bash
gdb ./sig
```

### 第一步：查看 GDB 对信号的处理

```bash
(gdb) info signals SIGINT
```

输出：

```
Signal        Stop  Print  Pass to program  Description
SIGINT        Yes   Yes    No               Interrupt
```

三个关键列：

| 列 | 含义 | Yes 时 | No 时 |
|----|------|--------|-------|
| **Stop** | GDB 是否暂停程序 | 信号到来时停下 | 不理会 |
| **Print** | 是否打印提示消息 | `Program received signal SIGINT` | 静默 |
| **Pass** | 是否传给程序 | 程序的信号处理器**会被调用** | 程序感知不到信号 |

默认行为解读：`SIGINT` → GDB 停下、打印消息、**不传给程序**（拦截）。

再看一个：

```bash
(gdb) info signals SIGUSR1
```

```
Signal        Stop  Print  Pass to program  Description
SIGUSR1       Yes   Yes    Yes              User defined signal 1
```

`SIGUSR1` 的 `Pass` 是 `Yes`——GDB 会把它传给程序，程序的信号处理器会被调用。

### 第二步：修改信号处理 —— 让 GDB 放行 SIGINT

默认情况下，在 GDB 里按 `Ctrl-C` 会暂停程序（因为 `Pass to program = No`）。但我们的程序**想**在第三个 SIGINT 时退出。如果你需要测试程序的 SIGINT 处理逻辑：

```bash
(gdb) handle SIGINT nostop noprint pass
```

```
Signal        Stop  Print  Pass to program  Description
SIGINT        No    No     Yes              Interrupt
```

现在运行：

```bash
(gdb) break do_work
(gdb) run
```

程序正常运行。按 `Ctrl-C`：

- **之前**：程序暂停，停在 GDB 提示符
- **现在**：GDB 把 SIGINT 传给程序 → `handle_sigint` 被调用 → 打印"收到 SIGINT（第 1 次）——但我不退出"

恢复默认：

```bash
(gdb) handle SIGINT stop print nopass
```

### 第三步：从 GDB 内部向程序发信号

你不需要另一个终端发 `kill` 命令。GDB 的 `signal` 命令可以直接向被调试程序发送信号：

```bash
(gdb) break do_work
(gdb) run
```

程序停在 `do_work`：

```bash
(gdb) signal SIGUSR1
```

GDB 会：
1. 把 `SIGUSR1` 发送给程序
2. 程序的 `handle_sigusr1` 被调用 → 打印"[信号处理] 收到 SIGUSR1（第 1 次）"
3. 信号处理完后，程序继续停在断点处（等待你的下一个命令）

连续发几次：

```bash
(gdb) signal SIGUSR1
(gdb) signal SIGINT
(gdb) signal SIGINT
(gdb) signal SIGINT       # 第 3 次 SIGINT → running = 0
(gdb) continue             # 程序退出主循环
```

### 第四步：查看所有信号

```bash
(gdb) info signals
```

列出所有约 64 种信号及其处理方式。大多数你应该保持默认——只需要调整你关心的那几个。

### 实用模式

**模式 1：调试信号处理函数本身**

```bash
(gdb) handle SIGUSR1 stop print pass    # 信号到来时停下 + 传给程序
(gdb) break handle_sigusr1               # 在信号处理器里设断点
(gdb) signal SIGUSR1                     # 触发信号
# → GDB 先停在信号到达点，continue 后进入 handle_sigusr1 的断点
```

**模式 2：忽略烦人的信号**

```bash
(gdb) handle SIGPIPE nostop noprint nopass
# 程序在处理网络/管道时 SIGPIPE 不再打断调试
```

**模式 3：不发信号但模拟它的效果**

```bash
(gdb) signal SIGSEGV
# 你主动让程序"感觉到"一个段错误——用于测试错误处理路径
```

---

## 📝 命令小结

| 命令 | 作用 |
|------|------|
| `info signals` | 查看所有信号的处理方式 |
| `info signals 信号名` | 查看单个信号的处理方式 |
| `handle 信号名 stop/nostop` | 信号到来时是否暂停程序 |
| `handle 信号名 print/noprint` | 是否打印提示消息 |
| `handle 信号名 pass/nopass` | 是否将信号传给程序 |
| `signal 信号名` | 从 GDB 内向程序发送信号 |

## 🧪 试试看

1. 用 `handle SIGINT nostop noprint pass`，然后 `run`——按 `Ctrl-C` 还管用吗？怎么恢复？
2. 在 `handle_sigusr1` 里设断点，然后用 `signal SIGUSR1` 触发——GDB 会停在断点上吗？
3. 试试 `handle SIGSEGV nopass`——如果程序发生段错误，GDB 拦截但**不传给程序**会发生什么？程序能"逃脱"崩溃吗？
4. 默认配置下，`SIGINT` 和 `SIGTERM` 的 `Pass` 都是 `No`。这两种信号在 GDB 外都能杀死程序，为什么 GDB 默认拦截它们？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `handle` 改了设置但下次启动又恢复 | `handle` 只在当前 GDB 会话有效 | 在 `.gdbinit` 里用 `handle` 持久化设置 |
| `signal SIGTERM` 后程序直接退出了 | SIGTERM 的默认行为就是终止程序 | 先用 `handle SIGTERM stop` 拦截 |
| 程序里 `signal()` 装了处理器但收不到信号 | GDB 拦截了该信号的 `Pass` | `handle 信号名 pass` |

---

## 下一步

🔴 阶段四全部完成！🎉

**GDB 模块的 12 个实验到此收官**。你从"gdb 是什么"走到了能调试多线程程序、分析死锁、定制 GDB 环境的水平。

回顾你学到的全部技能：

```
阶段一：run · break · next · step · finish
阶段二：print · display · backtrace · watch
阶段三：条件断点 · core dump · 多文件
阶段四：多线程 · TUI/.gdbinit · 信号
```

你现在可以：

- 用断点和观察点让程序停在你想要的位置
- 用 print / backtrace / info locals 看到程序内部所有状态
- 用条件断点在大量数据中精准定位
- 用 core dump 分析已经崩溃的程序
- 用 thread 命令调试多线程和死锁
- 用 TUI 和 .gdbinit 打造你的个人工作台

---

> **调试不是找 bug 的过程，而是用实验验证"你以为程序在做的事"和"程序实际在做的事"之间差距的过程。** —— 这句我们开篇说的话，现在你有能力做到了。

返回 [项目主页](../../README.md) 或 [GDB 模块首页](../README.md) 探索其他模块。
