# 实验 10：动态探针与脚本 —— 不用改代码就能追踪任何函数

> 🎯 **目标**：用 `perf probe` 在运行中的内核/用户态函数上插入动态追踪点；用 `perf script` 做定制化输出。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 03](../03-perf-record/)（perf record）、[实验 07](../07-perf-trace/)（tracepoint 概念）

## 什么是动态探针？

前面用过的 `cycles`、`syscalls:sys_enter_openat` 都是**静态**的——CPU 硬件计数器或内核中预埋的 tracepoint。它们的数量和位置是固定的。

**`perf probe` 让你在任何函数的入口或出口处插入追踪点——不需要修改源码、不需要重新编译、不需要重启。**

```
静态事件：内核/CPU 已经定义好的 → 有就有，没有就没有
动态探针：你在我想要的函数上动态创建 → 任何函数都能追踪
```

> 这是 perf 中最强大也最灵活的功能——在生产环境中，你可以对一个正在运行的进程插入探针，查看它的内部行为，然后删除探针，一切恢复原样。

---

## 先看示例程序

[probe_demo.c](probe_demo.c) 有几个清晰的函数供追踪：

| 函数 | 作用 | 适合探针 |
|------|------|---------|
| `process_item(id, value)` | 处理单个数据项——被调用 500 次 | 追踪每次调用的参数 |
| `process_batch(data, n)` | 批量处理入口 | 追踪整个批次的耗时 |
| `my_allocate(size)` | 封装的内存分配 | 追踪分配大小和频率 |
| `compute_result(a, b, c)` | 计算操作 | 追踪参数和返回值 |

```bash
make && ./probe_demo
```

---

## 实验开始

### 第一步：列出可探测的函数

```bash
# 列出 probe_demo 中所有可探测的函数
sudo perf probe -x ./probe_demo --funcs
```

输出（部分）：

```
process_item
process_batch
my_allocate
my_deallocate
compute_result
main
```

> `-x ./probe_demo` 指定要探测的二进制文件。不加 `-x` 默认探测内核函数（`sudo perf probe --funcs` 列出内核中所有可探测函数）。

### 第二步：在用户态函数上插入探针

在 `process_item` 入口处插入探针，捕获它的两个参数：

```bash
sudo perf probe -x ./probe_demo --add='process_item id=%di value=%si'
```

```
Added new event:
  probe_probe_demo:process_item (on process_item in ./probe_demo
  with id=%di value=%si)

You can now use it in all perf tools, such as:

	perf record -e probe_probe_demo:process_item -aR sleep 1
```

**参数格式说明**：

| 格式 | 含义 | 示例 |
|------|------|------|
| `%di` | 第 1 个整数参数（x86-64 RDI 寄存器） | `id` |
| `%si` | 第 2 个整数参数（x86-64 RSI 寄存器） | `value` |
| `%dx` | 第 3 个整数参数（x86-64 RDX 寄存器） | |
| `%cx` | 第 4 个整数参数（x86-64 RCX 寄存器） | |
| `$retval` | 函数返回值（只能用在函数返回探针） | |
| `%ax` | 返回值（在函数出口） | |

### 第三步：采集探针事件

```bash
sudo perf record -e 'probe_probe_demo:process_item' -o perf.data ./probe_demo
```

```
=== perf probe Demo ===
Allocating work buffer...
Processing batch (500 items)...
Computing results...
  total = 50621550
=== Done! ===
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.051 MB perf.data (500 samples) ]
```

> **500 samples**——`process_item` 被调用了 500 次，每次调用被探针捕获一次。

### 第四步：用 perf script 查看原始数据

```bash
sudo perf script -i perf.data | head -15
```

```
 probe_demo 49888 [002] 811318.838356: probe_probe_demo:process_item: (60e9b9df91a9) id=0x0 value=0x7
 probe_demo 49888 [002] 811318.839310: probe_probe_demo:process_item: (60e9b9df91a9) id=0x1 value=0x14
 probe_demo 49888 [002] 811318.839318: probe_probe_demo:process_item: (60e9b9df91a9) id=0x2 value=0x21
 probe_demo 49888 [002] 811318.839323: probe_probe_demo:process_item: (60e9b9df91a9) id=0x3 value=0x2e
 probe_demo 49888 [002] 811318.839328: probe_probe_demo:process_item: (60e9b9df91a9) id=0x4 value=0x3b
```

每一行：

```
进程名  PID [CPU核心] 时间戳: 探针名: (地址) id=参数1 value=参数2
```

> 你可以看到 `process_item` 每次被调用时 `id` 从 0 递增到 499，`value` 是传入的实际值。这就是**"不修改一行代码，看到函数的所有参数"**。

### 第五步：在函数返回处插探针

除了入口，还可以在函数**返回时**插探针——捕获返回值或计算函数耗时：

```bash
# 在 compute_result 返回处插探针，捕获返回值
sudo perf probe -x ./probe_demo --add='compute_result%return retval=$retval'

# 采集
sudo perf record -e 'probe_probe_demo:compute_result*' -o perf.data ./probe_demo

# 查看
sudo perf script -i perf.data | head -10
```

> `函数名%return` 表示探测函数的返回点。`$retval` 是特殊的伪变量——只存在于返回探针中。

### 第六步：内核函数探针

用户态函数用 `-x ./binary`，内核函数不需要：

```bash
# 探测内核的 kmalloc 函数（每次内核分配内存时触发）
sudo perf probe --add='kmalloc size=%di'

sudo perf record -e 'probe:kmalloc' -o perf.data ./probe_demo
sudo perf script -i perf.data | head -10

# 清理
sudo perf probe --del='probe:kmalloc'
```

> ⚠️ 内核探针需要 `sudo` 且需要内核调试符号（`CONFIG_DEBUG_INFO`）。WSL2 内核可能没有调试符号——如果 `perf probe --add='vfs_read'` 报 `No kprobe` 错误，说明内核不支持。

### 第七步：清理探针

```bash
# 列出当前所有动态探针
sudo perf probe --list

# 删除特定探针
sudo perf probe --del='probe_probe_demo:process_item'

# 删除所有探针
sudo perf probe --del='*'
```

---

## 📝 核心认知

### 动态探针的原理

```
perf probe --add='函数名 参数...'
         │
         ▼
内核用 kprobe（内核探针）或 uprobe（用户探针）机制
在目标函数的入口/出口处插入 INT 3 断点指令
         │
         ▼
每次函数被调用 → 触发断点 → perf 记录事件 → 恢复原指令继续执行
         │
         ▼
perf probe --del → 移除断点 → 函数恢复原样，零痕迹
```

> 插入和删除都是**在线**操作——不需要重启程序，不需要重新编译。

### 和 tracepoint 的区别

| | tracepoint | perf probe |
|------|-----------|-----------|
| 定义方式 | 内核源码中预埋（`trace_xxx()`） | 运行时动态创建 |
| 数量 | 固定——有就有 | 无限——任何函数都能探测 |
| 稳定性 | 高（内核 API 保证不变） | 低（函数名/参数可能随版本改变） |
| 适合场景 | 生产长期监控 | 临时调试、开发排查 |

---

## 🧪 试试看

1. 在 `process_batch` 的入口和返回处各插一个探针——用时间戳差计算这个函数的平均耗时。
2. 探测 `my_allocate` 函数——`%di` 参数是 `size`。你能看到 500 个元素的分配大小吗？
3. 尝试探测一个系统自带的命令：`sudo perf probe -x /bin/ls --add='main'`——能成功吗？为什么？（提示：strip 过的二进制没有符号表）
4. （需要内核调试符号）`sudo perf probe --add='schedule'`——探测内核调度函数。每次进程切换时都会触发。能采集到事件吗？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `sudo perf probe -x ./prog --funcs` | 列出可探测的用户态函数 |
| `sudo perf probe -x ./prog --add='func arg1=%di'` | 在用户态函数入口插入探针 |
| `sudo perf probe -x ./prog --add='func%return ret=$retval'` | 在用户态函数返回处插入探针 |
| `sudo perf probe --add='kfunc arg=%di'` | 在内核函数入口插入探针 |
| `sudo perf probe --list` | 列出所有动态探针 |
| `sudo perf probe --del='探针名'` | 删除探针 |
| `sudo perf script -i perf.data` | 以文本格式打印采集到的探针事件 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `No kprobe` 或 `No such file or directory` | WSL2 内核无调试符号，不支持 kprobe | 用用户态探针（`-x ./prog`）替代 |
| `Unknown argument '%di'` | 参数寄存器写错了 | x86-64 下第 N 个参数：`%di` `%si` `%dx` `%cx` `%r8` `%r9` |
| `perf probe -x /bin/ls --add='main': No symbols` | 二进制被 strip 了 | 需要未 strip 的调试版本 |
| `probe ... already exists` | 探针已经添加过了 | 先 `--del` 再 `--add` |

---

## 下一步

恭喜！你完成了 perf 模块的高级专项阶段（实验 07-10）——perf trace 追踪系统调用、perf sched 分析调度延迟、L1 cache 事件诊断内存模式、perf probe 动态插桩。

下一步 [阶段五：系统资源专项分析](../README.md#-阶段五系统资源专项分析实验-1113)——块设备 IO、网络中断、内核内存泄漏。
