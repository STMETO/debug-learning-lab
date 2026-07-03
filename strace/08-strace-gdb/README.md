# 实验 08：strace + GDB 联动 —— 同时拥有"内"和"外"两个视角

> 🎯 **目标**：学会把 strace（外部系统调用视角）和 GDB（内部变量和执行流视角）结合起来诊断问题。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：strace 实验 01-06 + GDB 实验 01-06

## 双视角 —— 完整的调试能力

```
                    ┌─────────────────────┐
  strace 看到：     │  你的程序            │    GDB 看到：
                    │                     │
  openat() = 3      │  int fd = open(...) │   fd = 3
  read(3,...,127)=90│  read(fd,buf,127)   │   buf = "Hello..."
  write(1,...) = 306│  printf(...)        │   格式化字符串的值
  close(3) = 0      │  close(fd)          │   fd 已关闭
                    │                     │
  外部：程序在和内核   └─────────────────────┘  内部：程序的变量和流程
  说什么、说了多久                              是什么、为什么会这样
```

| | strace | GDB |
|------|--------|-----|
| 看到什么 | 系统调用名称、参数、返回值、耗时 | 变量值、调用栈、源代码、内存 |
| 不需要 | 重新编译（-g 有帮助但非必须） | 必须加 `-g` 编译 |
| 适合问题 | "程序卡住了？读了什么文件？哪个调用慢？" | "变量值怎么变了？执行到了哪一行？NULL 指针从哪来的？" |

---

## 先看示例程序

[dual.c](dual.c) 有文件操作成功和失败的情况，还有一个带延迟的循环：

```bash
make && ./dual
```

---

## 实验开始

### 场景一：排查"打开文件失败"——strace 先上，GDB 深入

**先用 strace 看外部**：

```bash
strace -e trace=openat ./dual 2>&1 | grep -E 'dual_test|nonexistent|=-1'
```

输出：

```
openat(AT_FDCWD, "/tmp/dual_test.txt", ...)       = 3     ← 成功
openat(AT_FDCWD, "/tmp/nonexistent.txt", O_RDONLY) = -1 ENOENT  ← 失败
```

strace 告诉你：`/tmp/nonexistent.txt` 不存在 → `ENOENT`。但为什么程序传了这个路径？

**再用 GDB 看内部**：

```bash
gdb ./dual
```

```bash
(gdb) break process_file
(gdb) run
(gdb) print path
$1 = 0x... "/tmp/dual_test.txt"     # 第一次调用：正常的路径
(gdb) continue
(gdb) print path
$2 = 0x... "/tmp/nonexistent.txt"   # 第二次调用：这个路径哪来的？
(gdb) backtrace
#0  process_file (path=...) at dual.c:16
#1  main (...) at dual.c:88          # ← 在 main 的第 88 行调用的
```

> **strace 答"发生了什么"**：`openat` 返回了 `ENOENT`
> **GDB 答"为什么"**：`path` 参数是 `/tmp/nonexistent.txt`，调用来自 `dual.c:88`

### 场景二：排查"间歇性变慢"——strace 看时间，GDB 看状态

**先用 strace 找到慢的调用**：

```bash
strace -T -e trace=clock_nanosleep ./dual 2>&1
```

输出（关键行）：

```
clock_nanosleep(..., {tv_sec=0, tv_nsec=500000000}, ...) = 0 <0.500301>
```

> strace 告诉你：程序在某处 sleep 了 **0.5 秒**。但你不知道**为什么会 sleep 0.5 秒，在哪个循环条件下触发的**。

**再用 GDB 看到内部状态**：

```bash
gdb ./dual
```

```bash
(gdb) break dual.c:51          # usleep 那行
(gdb) run
(gdb) continue                 # 前 3 次循环正常
(gdb) continue
(gdb) continue
# 第 4 次：停在 usleep 前
(gdb) print i
$3 = 3                         # i == 3 → 触发了延迟分支
(gdb) print count
$4 = 5                         # 总共 5 次循环，第 4 次（i=3）是慢的
```

> **strace 答"哪个调用慢"**：`clock_nanosleep` 花了 0.5 秒
> **GDB 答"为什么会调用它"**：`i == 3` 时进入了 `if (i == 3)` 分支

---

## 📝 双工具决策流程

```
程序出问题了
    │
    ├─ 知道是什么类型的 bug？
    │   ├─ 内存越界/UAF/泄漏 → ASan
    │   ├─ 文件/网络/卡住/慢 → strace
    │   └─ 逻辑错误/值不对 → GDB
    │
    ├─ strace 发现了异常的系统调用
    │   └─ 需要知道为什么传了这个参数？
    │       └─ GDB：break 对应函数 → print 参数 → backtrace
    │
    ├─ GDB 发现了异常的变量值
    │   └─ 需要知道对外部有什么影响？
    │       └─ strace：追踪这个操作 → 看返回值
    │
    └─ 都不确定？
        └─ strace 先上（无侵入，快速扫一遍）
           └─ 发现线索 → GDB 深入
```

---

## 🧪 试试看

1. 用 strace 追踪 `./dual`，找到所有 `= -1` 的调用，然后在 GDB 里逐一设断点确认
2. 在 GDB 的 `process_file` 里设断点，`step` 到 `open` 那行——然后用 `print path` 看到的值和 strace 输出中的路径对比
3. 写一个小程序，它根据命令行参数决定打开哪个文件。先 strace 看它尝试打开了什么，再 GDB 看参数是怎么传到 `open` 的
4. strace 的 `<unfinished ...>` 和 GDB 的 `info threads` 结合——能用两个工具同时追踪多线程程序吗？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| strace 和 GDB 同时用会互相干扰 | 两个工具都通过 ptrace 附加到进程 | 不要同时对同一个进程使用——先用 strace 收集数据，再用 GDB 分析 |
| strace 看到返回 -1 但 GDB 中 `fd` 不是 -1 | 你看的不是同一次运行 | strace 和 GDB 的数据必须来自**同一执行路径** |
| GDB 中 `print` 的值和 strace 参数不一致 | 参数在系统调用和用户态之间可能被修改 | 在系统调用**之前**设断点查看 |

---

## 下一步

🟠 strace 模块全部完成！🎉

回顾 8 个实验：

```
阶段一：Hello strace → 读懂输出格式
阶段二：过滤调用 → 时间分析 → 追踪子进程
阶段三：统计模式 → 实战调试 → strace+GDB
```

你现在可以：

- 用 `strace ./program` 看任何程序的内核交互
- 用 `-e trace=` 精确过滤系统调用
- 用 `-tt -T` 找到性能瓶颈
- 用 `-f` 追踪整个进程树
- 用 `-c` 生成系统调用统计画像
- 用 strace + GDB 双视角诊断复杂 bug

### 🛠️ 三工具全景

```
          │  内存安全  │  内部状态  │  外部交互
──────────┼───────────┼───────────┼──────────
  工具     │   ASan    │   GDB     │  strace
  检测方式  │  自动报警  │  手动调试  │  被动观察
  核心输出  │  错误报告  │  变量/栈  │  系统调用
  实验数    │   10      │   12      │   8
```

返回 [模块首页](../README.md) 或 [项目主页](../../README.md)。
