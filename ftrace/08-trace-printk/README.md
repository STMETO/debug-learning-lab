# 实验 08：trace_printk 陷阱与规范 —— 为什么生产环境禁用？

> 🎯 **目标**：理解 `trace_printk` 的运行时开销原理、高频打印阻塞内核的案例、生产禁用策略、`trace_marker` 替代方案、条件打印减少追踪负载。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 01](../01-hello-ftrace/)（trace_marker 基础）

## trace_printk 是什么？

`trace_printk` 是内核开发者用来向 ftrace 缓冲区写入调试信息的内核函数——类似于内核态的 `printf`。它只能在内核代码中调用（驱动程序、内核模块、内核子系统），用户态程序无法直接使用。

```
内核代码中：
  trace_printk("ext4_open: inode=%lu, flags=%x\n", inode->i_ino, flags);
                            ↓
                  写入 ftrace 环形缓冲区
                            ↓
              用户通过 cat /sys/kernel/tracing/trace 读取
```

> 用户态程序想要往 ftrace 写消息，用的是 `trace_marker`（实验 01 中已经用过了）。`trace_printk` 是内核版的 `trace_marker`。

---

## 先看示例程序

[marker_demo.c](marker_demo.c) 用 `trace_marker`（用户态等价物）模拟了四种不同的追踪模式，让你直观感受追踪开销：

| 阶段 | 追踪模式 | 模拟场景 |
|------|---------|---------|
| Phase 1 | 不写任何 marker | 无追踪——基线性能 |
| Phase 2 | 每次迭代都写 marker | 模拟 `trace_printk` 滥用（每行都打印） |
| Phase 3 | 只在函数边界写 | 最佳实践——只标记开始/结束 |
| Phase 4 | 条件打印 | 只在"有趣"的事件上打印 |

```bash
make && sudo ./marker_demo
```

输出：

```
Phase 1: Baseline (no tracing)
  No tracing:        14081 μs  (baseline)

Phase 2: Marker on every iteration (simulates trace_printk abuse)
  Marker every iter: 111833 μs  ← 慢了 8 倍！

Phase 3: Marker only at boundaries (best practice)
  Marker boundaries:  13204 μs  ← 几乎无开销

Phase 4: Conditional marker (only on interesting events)
  Conditional marker:  13406 μs  ← 几乎无开销
```

> **50000 次迭代中每次写 trace_marker → 14ms → 111ms**：这就是 trace_printk 滥用的后果——每次写操作都涉及内核态的系统调用、缓冲区检查和格式化。

---

## 实验开始

### 第一步：检查 trace_printk 是否启用

```bash
# 查看 trace_printk 选项（内核编译时决定是否支持）
cat /sys/kernel/tracing/options/trace_printk
# 1 = 允许 trace_printk 写入 ftrace 缓冲区
# 0 = trace_printk 输出被重定向到内核日志（printk）

# 如果设为 0，trace_printk 的消息会出现在 dmesg 中：
echo 0 | sudo tee /sys/kernel/tracing/options/trace_printk
dmesg | tail -5
echo 1 | sudo tee /sys/kernel/tracing/options/trace_printk  # 恢复
```

### 第二步：用 trace_marker 替代 trace_printk

用户态代码中，**永远不要尝试调用 trace_printk**（它根本不可用）。用 `trace_marker` 替代：

```c
// ❌ 内核中才能用，用户态不可用
// trace_printk("processing item %d\n", i);

// ✅ 用户态往 ftrace 写消息的唯一方式
int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);
dprintf(marker, "processing item %d\n", i);
```

**trace_marker vs trace_printk**：

| | trace_printk | trace_marker |
|------|------------|-------------|
| 调用位置 | 内核代码 | 用户态代码 |
| 系统调用开销 | 无（已经在内核中） | 有（write syscall） |
| 灵活性 | 格式化字符串+参数 | 任何字符串 |
| 生产安全性 | ⚠️ 可能阻塞内核 | ✅ 安全的用户态操作 |

### 第三步：识别生产环境中的 trace_printk 泄漏

trace_printk 最大的问题是**它可能被留在生产代码中**。如何检测？

```bash
# 1. 检查 ftrace 缓冲区中是否有 trace_printk 输出
sudo cat /sys/kernel/tracing/trace | grep -v 'tracing_mark_write'

# 如果出现不以 tracing_mark_write 为事件名的消息，
# 那就是内核中的 trace_printk 在输出！

# 2. 检查内核日志中是否有 trace_printk 输出
# (当 options/trace_printk = 0 时，输出会去 printk)
dmesg | grep -i 'trace'

# 3. 检查 /proc/kallsyms 确认内核是否有 trace_printk 调用者
sudo grep 'trace_printk' /proc/kallsyms
```

### 第四步：条件打印模式

从 Phase 2 vs Phase 3/4 的对比可以看出——**不是 trace_marker 本身慢，而是每迭代一次就写一次的频率慢**。

```c
// ❌ 反模式：无条件打印（在热路径中每行都打印）
for (int i = 0; i < 1000000; i++) {
    dprintf(marker, "i=%d\n", i);  // 100 万次 write → 极慢！
    do_work(i);
}

// ✅ 模式 1：只在边界打标记
dprintf(marker, "batch start\n");
for (...) { do_work(i); }
dprintf(marker, "batch end\n");

// ✅ 模式 2：条件打印（只记录异常）
for (int i = 0; i < 1000000; i++) {
    long t = do_work(i);
    if (t > THRESHOLD) {  // 只在异常慢时打印
        dprintf(marker, "SLOW: i=%d took %ld μs\n", i, t);
    }
}
```

### 第五步：生产环境的 ftrace 追踪策略

| 环境 | 推荐做法 | 原因 |
|------|---------|------|
| **开发环境** | `trace_marker` 随意使用 | 开销量不重要，调试效率重要 |
| **测试/CI** | `trace_marker` 边界标记 + `function_graph` | 捕获完整的调用路径 |
| **生产环境** | 不用 `trace_marker`，用 `trace-cmd start/stop` | 按需开启，追踪完立刻关闭 |
| **生产内核模块** | **绝对不用** `trace_printk` | 会阻塞内核、影响所有进程 |

> 生产环境的黄金规则：**追踪默认是关闭的，只在排查问题时临时开启。** 不要把 trace_marker 或 trace_printk 留在热路径中。

---

## 📝 核心认知

### trace_printk 为什么危险？

```
1. 它在内核上下文中执行 → 没有"用户态/内核态"保护
2. 高频调用 → 每次都要格式化字符串 + 写入环形缓冲区
3. 如果缓冲区满了 → 要么丢弃，要么阻塞等待（取决于 overwrite 设置）
4. 在内核的热路径中（如调度器、中断处理）→ 可能导致整体系统卡顿
```

### trace_marker 为什么安全？

```
1. 它是用户态 → 内核态的系统调用 → 有固定开销但可预测
2. 内核可以速率限制（ratelimit）trace_marker 写入
3. 不影响其他进程
4. 用户态程序崩溃 → 只是 marker fd 关闭，不影响内核
```

---

## 🧪 试试看

1. 把 Phase 2 的 `dprintf(marker, ...)` 改成 `write(marker, buf, strlen(buf))`——性能有差别吗？`dprintf` 和 `write` 在 trace_marker 上的行为有什么不同？
2. 检查你的 WSL2 内核中是否有驱动在使用 trace_printk：`sudo cat /sys/kernel/tracing/trace | grep -v -E 'tracing_mark_write|^#' | head -10`——有意外输出吗？
3. 把 `options/trace_printk` 设为 0，然后用 `dmesg` 查看——之前写入 ftrace 的消息现在出现在内核日志中了吗？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `cat /sys/kernel/tracing/options/trace_printk` | 检查 trace_printk 是输出到 ftrace 还是 printk |
| `echo 0 \| sudo tee /sys/kernel/tracing/options/trace_printk` | 将 trace_printk 输出重定向到内核日志 |
| `dmesg \| grep -i trace` | 查看是否有内核 trace_printk 输出 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 在用户态 C 代码中写 `trace_printk(...)` | 这是内核函数，用户态不可用 | 改用 `open trace_marker` + `write/dprintf` |
| trace_marker 写入后 `cat trace` 看不到 | 缓冲区没被清空或 tracing_on 是 0 | 清空缓冲区、检查 tracing_on |
| `dprintf` 在大量迭代中导致性能严重下降 | 每次 `dprintf` 都是一次 write 系统调用 | 条件打印、减少频率或批量写入 |

---

## 下一步

trace_printk/trace_marker 的合理使用是"选择性输出"。但对 ftrace 的函数追踪来说，**选择性追踪**同样重要——下一步 [实验 09：黑白名单组合滤波](../09-filter-mastery/)——用白名单、黑名单和模块过滤从五万多个函数中精准选出你要追踪的目标。
