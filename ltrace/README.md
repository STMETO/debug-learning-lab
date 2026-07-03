# ltrace 库函数调用追踪

## 写给谁？

本模块假设你完成了 [strace 模块](../strace/)，知道什么是系统调用，理解"程序通过调用函数来使用操作系统服务"这个概念。

> 如果 strace 让你看到了程序和内核的"底层对话"，ltrace 让你看到程序和库函数之间的"中层对话"。

## ltrace 是什么？

**ltrace** 拦截并打印一个程序对**共享库函数**的每一次调用。

对比 strace 才能理解 ltrace 的定位：

```
你的 C 代码                     ltrace 看到这里
    │
    │ printf("hello")     ←─── 库函数调用（libc）
    │ malloc(100)         ←─── 库函数调用（libc）
    │ strlen(str)         ←─── 库函数调用（libc）
    ↓
  共享库 (libc.so)             
    │
    │ write(1, ...)       ←─── 系统调用             strace 看到这里
    │ brk(...)            ←─── 系统调用
    ↓
  Linux 内核
```

| | ltrace | strace |
|------|--------|--------|
| 追踪对象 | **库函数调用**（`printf`、`malloc`、`strlen`...） | **系统调用**（`write`、`brk`、`openat`...） |
| 所见即所得 | `printf("hello")` ← 和你的 C 代码一致 | `write(1, "hello", 5)` ← 被翻译成了内核接口 |
| 适用场景 | "这个函数被调用了多少次？参数是什么？" | "程序在向内核请求什么？为什么卡住了？" |
| 性能开销 | 中高（~20-50% 减速） | 中（~10-30% 减速） |

---

## 学习路线图

### 🟢 阶段一：初次接触（实验 01–02）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [01](01-hello-ltrace/) | Hello ltrace | `ltrace ./program`、看到库函数调用的第一条输出流 | ~10min |
| [02](02-read-output/) | 读懂输出 | 返回值格式、嵌套调用、`<unfinished ...>` 和 `<... resumed>` | ~15min |

> 学完这两个实验，你能对任何程序跑 `ltrace` 并看懂输出。

### 🟡 阶段二：核心技能（实验 03–04）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [03](03-filtering/) | 过滤库调用 | `-e` 按函数名或库名过滤、聚焦你关心的函数 | ~10min |
| [04](04-timing-stats/) | 时间与统计 | `-t`/`-T`/`-c`、找出最耗时的库函数调用 | ~15min |

> 学完这阶段，你能高效定位库调用层面的性能问题。

### 🟠 阶段三：多工具联动（实验 05–06）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [05](05-ltrace-strace/) | ltrace vs strace | 同一个程序——ltrace 看到的 vs strace 看到的、理解调用栈的三个层级 | ~15min |
| [06](06-ltrace-gdb/) | ltrace + GDB 联动 | 库调用视角 + 源码变量视角、三层调试闭环 | ~15min |

> 到这里，你拥有"源码→库调用→系统调用"的三层全视角。

---

## 开始之前

### 你需要什么

- Linux 系统（WSL2 / 虚拟机 / Docker 均可）
- ltrace 已在本项目的环境中预装

```bash
# 手动安装（Ubuntu/Debian）
sudo apt install ltrace

# 验证
ltrace --version
```

### ltrace vs strace —— 我该用哪个？

```
你要排查的问题是...

"程序有没有调用 free()？调了几次？"      → ltrace
"free() 最终触发 brk 还是 munmap？"      → strace
"malloc 返回的指针是不是正确的？"         → GDB + ltrace
"为什么程序读文件这么慢？"               → strace（看 read 系统调用的耗时）
"printf 格式化对不对？"                  → ltrace（直接看到 printf 的参数）
```

### 关键概念速览

| 术语 | 一句话解释 |
|------|-----------|
| **库函数** | `libc` 等共享库提供的函数——`printf`、`malloc`、`strlen` 都是库函数 |
| **PLT（Procedure Linkage Table）** | 动态链接时，库函数的实际地址在 PLT 中解析。ltrace 在 PLT 入口处拦截 |
| **ltrace 输出一行** | `函数名(参数...) = 返回值`——格式和 strace 几乎一样 |
| **动态库 vs 静态函数** | ltrace 只能追踪**动态链接**的库函数。编译进程序的静态函数（包括你的 `main`）不会被追踪 |

---

## 实验目录结构

```
03-filtering/
├── README.md      # 实验教程
├── demo.c         # 示例源码
└── Makefile
```

---

## 追踪三层次全景

完成本模块后，你将对程序的"调用栈"有三个层次的理解：

```
第三层：你的源代码         ← GDB 看这里
   │  printf("x=%d", x)
   │  char *p = malloc(100)
   ↓
第二层：库函数调用         ← ltrace 看这里
   │  printf("x=%d", 42) = 6
   │  malloc(100) = 0x555...
   ↓
第一层：系统调用           ← strace 看这里
   │  write(1, "x=42", 4)
   │  brk(0x...) = 0x...
   ↓
   Linux 内核
```

---

<p align="center">
  <b>准备好了吗？从 <a href="01-hello-ltrace/">实验 01：Hello ltrace</a> 开始 👇</b>
</p>
