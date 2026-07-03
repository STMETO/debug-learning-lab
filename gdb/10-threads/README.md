# 实验 10：多线程调试 —— 五个线程，一个计数器

> 🎯 **目标**：学会查看所有线程、在它们之间切换、控制调度，以及排查死锁。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 05](../05-backtrace/)（backtrace）、[实验 09](../09-multifile/)

## 多线程调试图什么？

单线程程序是一条线从头走到尾。多线程程序是多条线同时跑，新的问题来了：

> 程序卡住了——是哪个线程卡住的？
> 一个变量值不对——是哪个线程改的？
> 两个线程都不动了——是不是死锁了？

---

## 先看示例程序

[threads.c](threads.c) 启动 5 个线程 + 一个"死锁模式"：

```
正常模式：2 个不安全线程 + 2 个安全线程 + 1 个监控线程
死锁模式：./threads --deadlock
```

```bash
make && ./threads
```

> ⚠️ 不安全线程没有用互斥锁保护，最终 `shared_counter` 可能不等于 20——这就是竞态条件（race condition）。

---

## 实验开始

```bash
gdb ./threads
```

### 第一步：看到所有线程

```bash
(gdb) break worker_unsafe
(gdb) run
```

GDB 会打印新线程的创建：

```
[New Thread 0x... (LWP 159530)]
[New Thread 0x... (LWP 159531)]
[New Thread 0x... (LWP 159532)]
...
```

当某个线程命中断点后，查看当前所有线程：

```bash
(gdb) info threads
```

输出：

```
  Id   Target Id                                    Frame
  1    Thread 0x... (LWP 159523) "threads"  clone () at clone.S:83
* 2    Thread 0x... (LWP 159530) "threads"  worker_unsafe (...) at threads.c:14
  3    Thread 0x... (LWP 159531) "threads"  worker_unsafe (...) at threads.c:14
  4    Thread 0x... (LWP 159532) "threads"  clone () at clone.S:83
```

每一列的含义：

| 列 | 含义 |
|----|------|
| `*` | **当前线程**——你的 `next`/`step`/`print` 都在这个线程上操作 |
| Id | GDB 内部编号（`thread` 命令用这个号） |
| Target Id | 操作系统级别的线程标识（LWP = Light Weight Process） |
| Frame | 这个线程当前正在执行什么 |

> 线程 #4 还在 `clone()` 里——它刚被创建，还没进入工作函数。

### 第二步：在多个线程之间切换

```bash
(gdb) thread 3                 # 切换到 3 号线程
(gdb) backtrace                # 看 3 号线程的调用栈
```

输出：

```
#0  worker_unsafe (arg=0x...) at threads.c:14
#1  start_thread (...) at pthread_create.c:442
```

```bash
(gdb) print id                 # 这个线程收到的 id 参数
(gdb) print shared_counter     # 线程间共享的全局变量
```

切回主线程（#1）：

```bash
(gdb) thread 1
```

> 💡 GDB 不会自动显示你切换到了哪个线程——每次切线程后看一眼 `info threads` 确认 `*` 在正确的位置。

### 第三步：一览所有线程的状态

这是多线程调试中最强大的命令：

```bash
(gdb) thread apply all backtrace
```

输出（简化）：

```
Thread 4 (LWP 159532):
#0  clone () ...
#1  start_thread ...

Thread 3 (LWP 159531):
#0  worker_unsafe (arg=...) at threads.c:14
#1  start_thread ...

Thread 2 (LWP 159530):
#0  worker_unsafe (arg=...) at threads.c:14
#1  start_thread ...

Thread 1 (LWP 159523):
#0  clone () ...
#1  pthread_join ...
#2  main () at threads.c:92
```

**一个命令看清所有线程在做什么**：

- #1（主线程）：在 `pthread_join` 中等待其他线程结束
- #2、#3：在 `worker_unsafe` 里干活
- #4：刚创建，还在初始化

也可以只对部分线程执行命令：

```bash
(gdb) thread apply 2-3 print id        # 只对 2-3 号线程
(gdb) thread apply 2-3 backtrace       # 只看 worker 线程的栈
```

### 第四步：控制调度 —— scheduler-locking

默认情况下，`next` / `step` 时**所有线程同时运行**。这意味着：
- 你在 #2 单步的时候，#3 可能已经修改了 `shared_counter`
- 连续两次 `print shared_counter` 可能看到不同的值

`set scheduler-locking` 控制单步时是否让其他线程运行：

| 模式 | 行为 |
|------|------|
| `off`（默认） | 所有线程同时运行 |
| `on` | **只有当前线程**运行，其他线程冻结 |
| `step` | `next`/`step` 时锁定，`continue` 时放行 |

```bash
(gdb) set scheduler-locking on
(gdb) next                      # 只有当前线程执行这一步
(gdb) print shared_counter      # 值不会"莫名其妙"变化了
```

> `scheduler-locking on` 让你能像调试单线程程序一样调试多线程程序——隔离性是你的朋友。

恢复默认：

```bash
(gdb) set scheduler-locking off
```

### 第五步：死锁排查

我们准备了一个故意死锁的场景：

```bash
# 在另一个终端或退出 GDB 后重新启动
gdb ./threads
```

```bash
(gdb) break deadlock_a
(gdb) break deadlock_b
(gdb) run --deadlock
```

GDB 停在两个死锁线程的入口。放它们继续：

```bash
(gdb) continue
```

等几秒后按 `Ctrl-C` 打断：

```bash
^C
Program received signal SIGINT, Interrupt.
```

现在用 **一个命令** 诊断：

```bash
(gdb) thread apply all backtrace
```

你会看到类似：

```
Thread 3: deadlock_b → pthread_mutex_lock(lock_a)  ← 等 lock_a
Thread 2: deadlock_a → pthread_mutex_lock(lock_b)  ← 等 lock_b
Thread 1: pthread_join                              ← 等 Thread 2 和 3
```

> **经典死锁模式**：A 持有 lock_a 等 lock_b，B 持有 lock_b 等 lock_a。

你不需要靠肉眼读代码排查——`thread apply all bt` 一秒钟揭示死锁全貌。

---

## 📝 命令小结

| 命令 | 简写 | 作用 |
|------|------|------|
| `info threads` | `i th` | 列出所有线程 |
| `thread N` | `t 3` | 切换到 N 号线程 |
| `thread apply all bt` | `t a a bt` | 查看所有线程的调用栈 |
| `thread apply N-M cmd` | `t a 2-4 bt` | 对 N-M 号线程执行命令 |
| `set scheduler-locking on/off/step` | — | 控制单步时其他线程是否运行 |

## 🧪 试试看

1. 在 `worker_safe` 里设断点，用 `thread apply all bt` 看看所有线程的"全景快照"
2. 打开 `scheduler-locking on`，`next` 5 次，然后 `print shared_counter`——它变了吗？和默认模式对比
3. 运行死锁模式，用 `thread apply all bt` 找到死锁线程。你能看出哪个线程持有哪个锁吗？（提示：看 `__pthread_mutex_lock` 的参数）
4. 在 `monitor` 线程里设断点，切到它后用 `bt` 查看——监控线程和 worker 线程的视角有什么不同？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `thread N` 报 `No such thread` | 线程已经退出了 | `info threads` 确认线程还存在 |
| 单步时变量一直在变 | `scheduler-locking` 默认 `off`，其他线程在修改 | `set scheduler-locking on` |
| `thread apply all bt` 太长 | 线程太多 | `thread apply 1-5 bt` 限制范围 |
| 断点被多个线程反复触发 | 所有线程共享断点 | 正常现象——看 `info threads` 中 `*` 在哪个线程 |

---

## 下一步

你已经掌握了 GDB 在单线程和多线程中的所有核心调试技能。下一步学习 [实验 11：TUI 与效率](../11-tui/)——打开 GDB 的内置文本界面，让调试过程可视化，并配置 `.gdbinit` 定制属于你的 GDB 环境。
