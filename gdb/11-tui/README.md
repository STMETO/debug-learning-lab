# 实验 11：TUI 与效率 —— 让 GDB 更好看、更好用

> 🎯 **目标**：打开 GDB 内置文本界面（TUI）、配置 `.gdbinit`、编写自定义命令。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：任意一个已完成的实验

## 为什么 GDB 需要"界面"？

用了一个暑假的 GDB，你每次都是这样：

```
(gdb) break something
(gdb) run
(gdb) next         ← 现在执行到第几行了？不记得了……
(gdb) list         ← 看一眼源码
(gdb) next         ← 又忘了……
(gdb) list         ← 再看一眼
```

TUI（Text User Interface）在 GDB 内部给你一个**分屏界面**：上面是源码（带光标指示当前位置），下面是指令输入区。

---

## 先看示例程序

[demo.c](demo.c) 是一个有层次的计算程序，适合在 TUI 下观察函数调用。

```bash
make && ./demo
```

---

## 实验开始

### 第一步：进入 TUI 模式

```bash
gdb ./demo
```

```bash
(gdb) tui enable
```

或者直接在启动时进入：

```bash
gdb -tui ./demo
```

你会看到屏幕被分为上下两部分：

```
┌─────────────────────────────────────┐
│  20  void analyze_number(int n) {   │  ← 源码窗口
│  21      long fib = fibonacci(n);   │
│  22      int prime = is_prime(n);   │    ▶ 光标指示当前位置
│  23                                │
│  ...                               │
├─────────────────────────────────────┤
│  (gdb)                             │  ← 命令窗口
└─────────────────────────────────────┘
```

> 退出 TUI：`tui disable` 或 `Ctrl-X A`

### 第二步：切换布局

GDB TUI 有四种预设布局：

```bash
(gdb) layout src        # 只看源码（默认）
(gdb) layout asm        # 只看汇编指令
(gdb) layout split      # 源码 + 汇编上下分屏
(gdb) layout regs       # 源码 + 寄存器值
```

试试设个断点后在 `layout split` 下观察：

```
┌────────── 源码 ──────────┐
│  21  fib = fibonacci(n); │
├────────── 汇编 ──────────┤
│  call  fibonacci         │
│  mov   QWORD PTR [rbp-8] │
└──────────────────────────┘
├──────── 命令 ────────────┤
│  (gdb)                   │
└──────────────────────────┘
```

> `layout next` 可以循环切换四种布局。

### 第三步：在 TUI 中操作

在 TUI 模式下，焦点可以在窗口间切换：

| 快捷键 | 作用 |
|--------|------|
| `Ctrl-X O` | 切换到下一个窗口 |
| `Ctrl-X A` | 进入/退出 TUI 模式 |
| `Ctrl-L` | 刷新屏幕（TUI 有时显示混乱） |
| `↑` / `↓` | 在源码窗口中滚动 |
| `PgUp` / `PgDn` | 源码窗口翻页 |

> 源码窗口中会有一个 `▶` 或高亮行标记当前 PC（程序计数器）的位置，断点会显示为 `B+`。

全流程体验：

```bash
(gdb) break analyze_number
(gdb) run
# 源码窗口自动跳到 analyze_number 函数
# 当前行高亮显示
(gdb) next          # 光标向下移动一行
(gdb) step          # 进入 fibonacci，源码窗口自动切换
(gdb) finish        # 回到 analyze_number
```

### 第四步：用 .gdbinit 定制你的 GDB

你发现了吗？每次启动 GDB 你可能会重设相同的配置。把这些写进 `.gdbinit` 文件，GDB 启动时自动加载。

```bash
# 创建项目级 .gdbinit（在本目录生效）
cp gdbinit.example .gdbinit
```

打开 [gdbinit.example](gdbinit.example) 看一下，关键配置有：

```gdb
# 关闭烦人的版权信息
set startup-quietly on

# 记住输入过的命令
set history save on

# 好看的打印格式
set print pretty on
set print array-indexes on

# Intel 风格的汇编语法（如果你看汇编的话）
set disassembly-flavor intel
```

> `.gdbinit` 的加载顺序：先加载全局 `~/.gdbinit`，再加载项目目录下的 `.gdbinit`。

### 第五步：自定义命令 —— define

你可以把常用的操作组合定义成新命令：

```bash
(gdb) define pinfo
Type commands for definition of "pinfo".
End with a line saying just "end".
>printf "── 参数 ──\n"
>info args
>printf "── 局部变量 ──\n"
>info locals
>end
```

现在 `pinfo` 就是一个新命令了：

```bash
(gdb) pinfo
── 参数 ──
n = 5
── 局部变量 ──
fib = 5
prime = 1
```

gdbinit.example 里预定义了 `pinfo`、`tinfo`、`bsave`、`bload`。

### 第六步：断点自动执行 —— commands

你可以在断点触发时自动执行一串命令，然后自动继续：

```bash
(gdb) break analyze_number
(gdb) commands
Type commands for breakpoint(s) 1, one per line.
End with a line saying just "end".
>printf "n = %d\n", n
>continue
>end
(gdb) run
```

现在每次进入 `analyze_number`，GDB 自动打印 `n` 的值，然后继续运行。你只需坐着看输出。

> 这就是"printf debugging without modifying code"——在不下 `printf`、不重新编译的情况下实现了自动日志输出。

---

## 📝 命令小结

| 命令 | 作用 |
|------|------|
| `tui enable` / `tui disable` | 进入/退出 TUI 模式 |
| `layout src/asm/split/regs` | 切换布局 |
| `layout next` | 循环切换到下一个布局 |
| `set print pretty on` | 美化结构体打印 |
| `set pagination off` | 长输出不暂停 |
| `define 命令名 ... end` | 定义自定义命令 |
| `commands ... end` | 断点触发时自动执行的命令 |
| `save breakpoints 文件` | 保存断点到文件 |

## 🧪 试试看

1. 在 `layout split` 下做一次完整的 `break → run → step → print`，对比纯命令行模式，哪个更直观？
2. 复制 `gdbinit.example` 为 `.gdbinit`，重启 GDB——`pinfo` 命令能直接用吗？
3. 用 `commands` 在 `is_prime` 上加一个自动日志：每次进入时打印 `n` 并继续
4. 把 `set print pretty on` 加到 `.gdbinit` 里，然后用 demo 验证结构体打印是否变好看了

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| TUI 界面显示混乱 | GDB 重绘和终端输出冲突 | 按 `Ctrl-L` 刷新 |
| `.gdbinit` 不生效 | GDB 出于安全原因不自动加载 | 在 `~/.gdbinit` 里加 `set auto-load local-gdbinit on` |
| 在 TUI 里输入命令不响应 | 焦点在源码窗口 | 按 `Ctrl-X O` 切到命令窗口 |
| `commands` 配了 `continue` 但程序停了 | 条件断点未满足 | `continue` 只在断点**触发后**才执行 |

---

## 下一步

你已经把 GDB 打造成了一个舒适的工作环境。最后一个实验 [实验 12：信号处理](../12-signals/)——学习如何在 GDB 中控制 Linux 信号（SIGINT/SIGSEGV/SIGUSR1），在信号和调试之间自如切换。
