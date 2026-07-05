# 实验 09：黑白名单组合滤波 —— 从 57000 个函数中精准筛选

> 🎯 **目标**：`set_ftrace_filter` 白名单、`set_ftrace_notrace` 反向黑名单、`set_graph_function` 限制 graph 追踪入口、模块过滤 `func:mod:*`、通配符批量筛选降噪。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 02](../02-function-tracer/)（filter 基础）、[实验 03](../03-function-graph/)（function_graph）

## 过滤的层次体系

ftrace 暴露了 57633 个可追踪函数。没有过滤，你一秒内就会溢出缓冲区。过滤机制有五个层次：

```
层次1: set_ftrace_pid        → 只看一个进程（最粗粒度）
层次2: set_ftrace_filter     → 只看匹配的函数名（白名单）
层次3: set_ftrace_notrace    → 排除匹配的函数名（黑名单）
层次4: set_graph_function    → 只展开特定函数的子调用图
层次5: mod: 模块过滤          → 只看特定内核模块的函数

它们可以组合使用——白名单选范围，黑名单去噪音，graph_function 控深度。
```

---

## 先看示例程序

[filter_demo.c](filter_demo.c) 执行四种不同子系统的操作：

| 阶段 | 操作 | 触发的主要函数族 |
|------|------|----------------|
| Phase 1 | 50 次文件 open/write/stat/close | `ext4_file_*`, `vfs_*` |
| Phase 2 | 5 次 fork/wait | scheduler 函数 |
| Phase 3 | 20 次大块 malloc/free | page allocator（`__alloc_pages` 等） |
| Phase 4 | 20000 次 getpid | `__x64_sys_getpid` |

```bash
make && ./filter_demo
```

---

## 实验开始

### 第一步：白名单 —— set_ftrace_filter

只追踪 ext4 文件操作相关的函数：

```bash
echo 'nop'      | sudo tee /sys/kernel/tracing/current_tracer
echo ""         | sudo tee /sys/kernel/tracing/trace
echo 'function' | sudo tee /sys/kernel/tracing/current_tracer

# 白名单：只看 ext4_file_ 开头的函数
echo 'ext4_file_*' | sudo tee /sys/kernel/tracing/set_ftrace_filter

echo 1 | sudo tee /sys/kernel/tracing/tracing_on
sudo ./filter_demo
echo 0 | sudo tee /sys/kernel/tracing/tracing_on

sudo cat /sys/kernel/tracing/trace | awk '{print $5}' | sort -u
```

输出只有 `ext4_file_open`、`ext4_file_write_iter`、`ext4_file_read_iter` 等函数——没有 scheduler、没有 syscall、没有 page allocator。

### 第二步：用 notrace 从白名单中排除

白名单选中了一组函数，但其中某个太吵了——用 notrace 排除：

```bash
echo 'nop'      | sudo tee /sys/kernel/tracing/current_tracer
echo ""         | sudo tee /sys/kernel/tracing/trace
echo 'function' | sudo tee /sys/kernel/tracing/current_tracer

# 白名单：追踪所有 ext4 操作
echo 'ext4_file_*' | sudo tee /sys/kernel/tracing/set_ftrace_filter

# 黑名单：但排除 read_iter（太频繁了）
echo 'ext4_file_read_iter' | sudo tee /sys/kernel/tracing/set_ftrace_notrace

echo 1 | sudo tee /sys/kernel/tracing/tracing_on
sudo ./filter_demo
echo 0 | sudo tee /sys/kernel/tracing/tracing_on

sudo cat /sys/kernel/tracing/trace | awk '{print $5}' | sort -u
# 应该看到 open、write_iter、getattr——但没有 read_iter
```

**白名单 + 黑名单的组合逻辑**：

```
1. 先应用 set_ftrace_filter：只有匹配的函数被追踪
2. 再应用 set_ftrace_notrace：在已追踪的函数中，移除匹配的

最终追踪 = set_ftrace_filter − set_ftrace_notrace
```

### 第三步：set_graph_function —— 控制调用图展开深度

白名单控制"追踪什么"，`set_graph_function` 控制"展开到什么程度"：

```bash
echo 'nop'            | sudo tee /sys/kernel/tracing/current_tracer
echo ""               | sudo tee /sys/kernel/tracing/trace
echo 'function_graph' | sudo tee /sys/kernel/tracing/current_tracer

# 大范围追踪（所有 ext4 函数）
echo 'ext4_*' | sudo tee /sys/kernel/tracing/set_ftrace_filter

# 但只对 file_open 展开子调用图——其他函数只记录入口+出口
echo 'ext4_file_open' | sudo tee /sys/kernel/tracing/set_graph_function

echo 1 | sudo tee /sys/kernel/tracing/tracing_on
sudo ./filter_demo
echo 0 | sudo tee /sys/kernel/tracing/tracing_on

sudo cat /sys/kernel/tracing/trace | head -20
```

**输出对比**：

```
# 有 set_graph_function 的（展开子调用）：
  ext4_file_open() {              ← 展开了！
    dquot_file_open() {
      generic_file_open() 0.45μs
    } 1.52μs
  } 4.73μs

# 没有 set_graph_function 的（只有一行）：
  ext4_file_write_iter() 3.12μs   ← 没有展开，只显示总耗时
```

> 这是生产环境最常用的组合——用宽 filter 看全局，用 graph_function 只在感兴趣的少数函数上深入。

### 第四步：模块过滤 —— func:mod:module_name

只追踪某个内核模块的函数——不需要知道具体函数名：

```bash
# 追踪 ext4 模块中的所有函数
echo 'func:mod:ext4' | sudo tee /sys/kernel/tracing/set_ftrace_filter

# 验证
cat /sys/kernel/tracing/set_ftrace_filter | wc -l
# 输出：500+（ext4 模块中有 500 多个可追踪函数）

# 重置
echo "" | sudo tee /sys/kernel/tracing/set_ftrace_filter
```

**可用的模块名**（部分）：

```bash
# 查看有哪些模块
lsmod | awk '{print $1}' | head -20
# ext4, jbd2, mbcache, ...
# 用 lsmod 输出的名字作为 mod: 参数

# 也可以组合使用
echo -e 'func:mod:ext4\nfunc:mod:jbd2' | sudo tee /sys/kernel/tracing/set_ftrace_filter
```

### 第五步：组合技实战 —— 生产级的过滤配置

一个典型的生产排查过滤配置：

```bash
#!/bin/bash
# 目标：追踪 ext4 文件操作的完整调用链，但排除噪音极大的 read_iter

# 1. 选 tracer
echo 'function_graph' > /sys/kernel/tracing/current_tracer

# 2. 白名单：ext4 文件操作族
echo 'func:mod:ext4' > /sys/kernel/tracing/set_ftrace_filter

# 3. 黑名单：排除高频但无用的函数
echo -e 'ext4_file_read_iter\next4_file_getattr' > /sys/kernel/tracing/set_ftrace_notrace

# 4. 只对 open 和 write 展开调用图
echo -e 'ext4_file_open\next4_file_write_iter' > /sys/kernel/tracing/set_graph_function

# 5. 只看目标进程
echo <PID> > /sys/kernel/tracing/set_ftrace_pid

# 效果：追踪范围大（整个 ext4 模块），但噪音小（排除了 read），
#       深度适中（只在 open/write 上展开），聚焦精准（只看目标进程）
```

### 第六步：过滤器的调试

```bash
# 查看当前设置了哪些 filter
cat /sys/kernel/tracing/set_ftrace_filter     # 白名单
cat /sys/kernel/tracing/set_ftrace_notrace    # 黑名单
cat /sys/kernel/tracing/set_graph_function    # graph 展开列表
cat /sys/kernel/tracing/set_ftrace_pid        # PID 过滤

# 查看哪些函数当前被追踪（白名单 - 黑名单的结果）
cat /sys/kernel/tracing/set_ftrace_filter | head -10
# 如果输出 "#### all functions enabled ####"，说明没有过滤
```

---

## 📝 核心认知

### 五个过滤层次的决策矩阵

| 要解决的问题 | 用哪个 |
|-------------|--------|
| "系统中有几十个进程，只想看我的程序" | `set_ftrace_pid` |
| "只想追踪 ext4 相关的函数" | `set_ftrace_filter = 'ext4_*'` |
| "ext4 函数里 read_iter 太吵了，排除它" | `set_ftrace_notrace = 'ext4_file_read_iter'` |
| "想看 ext4 的所有函数但只在 open 上深入" | `set_ftrace_filter = 'ext4_*'` + `set_graph_function = 'ext4_file_open'` |
| "不知道模块里有什么函数，但想全追踪" | `func:mod:ext4` |

### 过滤器的性能影响

```
无过滤：57000+ 函数追踪 → 系统几秒内卡死
宽通配符 (ext4_*)：500+ 函数 → ~5-10% 系统开销
精准过滤 (ext4_file_open)：1 个函数 → ~0.1% 系统开销
组合过滤 (mod + notrace + pid)：精细控制 → 可接受的追踪开销
```

---

## 🧪 试试看

1. 用 `func:mod:ext4` 追踪，然后数一数 trace 中出现了多少个不同的函数名——和 `ext4_file_*` 的结果有什么不同？
2. 试试 `echo 'vfs_*' | sudo tee /sys/kernel/tracing/set_ftrace_filter`——VFS 层的函数名称和 ext4 有什么不同？调用路径是什么？
3. 把 `set_ftrace_notrace` 设为 `*lock*`——锁操作从 trace 中消失了吗？这对调试"锁竞争"问题有什么影响？
4. 用 `echo '*lock*' > set_ftrace_filter` 只追踪锁操作——你的程序 50 次文件操作触发了多少次 `mutex_lock` 或 `spin_lock`？

---

## 命令小结

| 操作 | 命令 |
|------|------|
| 白名单：追踪匹配的函数 | `echo 'ext4_file_*' \| sudo tee /sys/kernel/tracing/set_ftrace_filter` |
| 黑名单：排除匹配的函数 | `echo 'ext4_file_read_iter' \| sudo tee /sys/kernel/tracing/set_ftrace_notrace` |
| 只展开特定函数 | `echo 'ext4_file_open' \| sudo tee /sys/kernel/tracing/set_graph_function` |
| 模块过滤 | `echo 'func:mod:ext4' \| sudo tee /sys/kernel/tracing/set_ftrace_filter` |
| 清除所有过滤器 | `echo "" \| sudo tee /sys/kernel/tracing/set_ftrace_filter` |
| 查看当前过滤器 | `cat /sys/kernel/tracing/set_ftrace_filter` |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `set_ftrace_filter: Invalid argument` | 通配符没匹配到任何函数，或模块名拼错了 | 用 `available_filter_functions` 验证函数名 |
| notrace 设了但函数还在 trace 中出现 | notrace 对 function_graph 子调用的行为不同 | 对 graph 的子调用需同时设 `set_graph_function` |
| `func:mod:模块名` 不生效 | 该模块可能编译进内核而非独立模块 | 用 `lsmod` 确认模块存在且已加载 |

---

## 下一步

你现在能从五万多个函数中精准筛选出目标。但到底选哪种追踪机制——静态 tracepoint？动态 kprobe？function_graph？perf 采样？下一步 [实验 10：机制抉择](../10-mechanism-choice/)——全景对比四种追踪机制，学会在不同场景下选对工具。
