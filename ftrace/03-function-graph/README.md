# 实验 03：调用图与延迟 —— 精确到微秒的"谁花了多久"

> 🎯 **目标**：`function_graph` tracer——看到函数入口+出口+耗时、`+` 标记（> 10μs）、`!` 标记（> 100μs）、用调用图找出"哪个子函数最慢"。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 02](../02-function-tracer/)（function tracer 基础）

## function_graph 和 function 的区别

```
function tracer：         function_graph tracer：

ext4_file_open()          ext4_file_open() {              ← 入口（缩进开始）
ext4_file_read_iter()     │  dquot_file_open() {          ← 子调用（更深缩进）
ext4_file_read_iter()     │  │  generic_file_open()  0.5μs│ ← 叶子函数+耗时
                          │  } 1.5μs                      ← 出口（缩进结束）+总耗时
                          } 4.7μs                         ← ext4_file_open 总耗时
                                                          ↑
                                          每个 `{` 和 `}` 包围一个函数的完整执行
```

> function tracer 只告诉你"被调用了"。function_graph 告诉你 **"谁调用了谁、每个花了多久"**——这是性能分析的核心信息。

---

## 先看示例程序

[graph_demo.c](graph_demo.c) 做了三组操作，刻意制造了快慢对比：

| 阶段 | 操作 | 预期耗时 |
|------|------|---------|
| Phase 1 | 10 次 open/write/close（无 fsync） | 每次 < 20μs |
| Phase 2 | 3 次 open/write/fsync/close（**有 fsync**） | 每次 >> 100μs |
| Phase 3 | 10 次 open/write/close（无 fsync） | 每次 < 20μs |

> `fsync()` 强制将脏页刷到磁盘——这会触发块层 I/O，在 function_graph 中表现为明显的长耗时。

```bash
make && ./graph_demo
```

---

## 实验开始

### 第一步：启用 function_graph

```bash
echo 'nop'            | sudo tee /sys/kernel/tracing/current_tracer
echo ""               | sudo tee /sys/kernel/tracing/trace
echo 'function_graph' | sudo tee /sys/kernel/tracing/current_tracer

# set_graph_function：只对选中的函数展开详细的调用图
# 没在 graph_function 中的函数调用 → 不显示子调用细节
echo 'ext4_file_*'    | sudo tee /sys/kernel/tracing/set_graph_function

echo 1 | sudo tee /sys/kernel/tracing/tracing_on
sudo ./graph_demo
echo 0 | sudo tee /sys/kernel/tracing/tracing_on
```

### 第二步：阅读 function_graph 输出

```bash
sudo cat /sys/kernel/tracing/trace
```

先看 Phase 1 的快操作（正常的嵌套调用，微秒级耗时）：

```
  8)               |  /* === PHASE 1: FAST OPS === */           ← trace_marker 写入
  8)               |  ext4_file_open() {                         ← ext4_file_open 入口
  8)               |    dquot_file_open() {                      ← 子调用
  8)   0.451 us    |      generic_file_open();                   ← 叶子函数，耗时 0.451μs
  8)   1.522 us    |    }                                        ← dquot_file_open 总耗时 1.522μs
  8)   4.729 us    |  }                                          ← ext4_file_open 总耗时 4.729μs
  8)               |  ext4_file_read_iter() {
  8)               |    generic_file_read_iter() {
  8)               |      filemap_read() {
  8)   0.421 us    |        __cond_resched();
  8)               |        filemap_get_pages() {
  8)               |          filemap_get_read_batch() {
  8)   0.421 us    |            __rcu_read_lock();
  8)   0.430 us    |            __rcu_read_unlock();
  8)   2.575 us    |          }
  8)   3.537 us    |        }
  8)               |        touch_atime() {
  8)               |          atime_needs_update() {
  8)   0.421 us    |            make_vfsuid();
  8)   0.431 us    |            make_vfsgid();
  8)   1.693 us    |          }
  8)   5.100 us    |        }
  8) + 12.143 us   |      }                 ← `+` 标记：generic_file_read_iter 耗时 > 10μs
  8) + 14.057 us   |    }                   ← `+` 标记：ext4_file_read_iter 耗时 > 10μs
```

### 第三步：解读嵌套和延迟标记

**缩进 = 调用深度**：

```
函数名前的缩进表示调用栈深度：
  ext4_file_open() {           ← 深度 0（我们的程序直接触发）
      dquot_file_open() {      ← 深度 1（ext4_file_open 调用了它）
          generic_file_open()  ← 深度 2（叶子函数）
      }                         ← 返回到深度 1
  }                             ← 返回到深度 0
```

**延迟标记**：

| 标记 | 含义 | 阈值 |
|------|------|------|
| `+` | 耗时超过 10 微秒 | > 10μs |
| `!` | 耗时超过 100 微秒 | > 100μs |
| `#` | 耗时超过 1000 微秒（1ms） | > 1000μs |

> 在 Phase 1 的快速操作中，`+` 标记已经出现了——`generic_file_read_iter` 花了 12μs。虽然"快"，但 function_graph 的精度足以捕捉这些微秒级的差异。

### 第四步：找出慢路径

Phase 2 的 `fsync` 操作会触发大量块层 I/O。在 trace 中找到 Phase 2 的区域，观察延时的剧变：

```bash
# 搜索延迟标记 ! 和 + —— 它们指向最慢的操作
sudo cat /sys/kernel/tracing/trace | grep -E '! |\+ ' | head -20
```

Phase 2 的关键调用链（`write + fsync` 路径）：

```
 11)               |  ext4_file_write_iter() {             ← write 系统调用入口
 11)               |    ext4_buffered_write_iter() {
 11)               |      ext4_generic_write_checks() {
 11)               |        ext4_dirty_inode() {           ← 标记 inode 为脏
 11)               |          __ext4_journal_start_sb() {  ← 开始日志事务
 11)   0.431 us    |            ext4_journal_check_start();
 11)               |          __ext4_mark_inode_dirty() {
 11)               |            ext4_reserve_inode_write() {
 11)               |              ext4_get_inode_loc() {
 11)               |                __ext4_get_inode_loc() {
 11)               |                  ext4_get_group_desc() {
 11)   0.431 us    |                    ext4_inode_table();
 11)               |                  __ext4_journal_get_write_access() {
 11)               |                ext4_mark_iloc_dirty() {
 11)               |                  ext4_fc_track_inode();
 11)               |                  ext4_fill_raw_inode() {
 11)               |                    ext4_inode_csum_set() {
```

> Phase 2 的调用栈比 Phase 1 深得多——`write + fsync` 经过了 VFS → ext4 → jbd2（日志）→ block layer，每一层都有耗时记录。

### 第五步：对比分析——快 vs 慢

```
操作类型          典型的函数调用深度      典型总耗时
─────────────────────────────────────────────────
open/write/close （无 fsync）   3-5 层     < 20μs
open/write/fsync/close         8-12 层    > 500μs（甚至 ms 级）
                                         ↑
                              块层 I/O 占主导——等待磁盘
```

**这个对比就是 function_graph 的核心价值**：它不只告诉你"慢"，它告诉你**慢在哪一层、哪个子函数**。如果 `fsync` 慢了，你可以往下钻——是 `ext4_sync_file` 慢？还是 `jbd2_log_wait_commit` 慢？还是 `blk_mq_submit_bio` 慢？

---

## 📝 核心认知

### set_graph_function vs set_ftrace_filter

```
set_ftrace_filter：     控制"哪些函数被追踪"
                        用于 function tracer

set_graph_function：    控制"哪些函数的子调用被展开"
                        用于 function_graph tracer
                        
二者可以同时使用：
  set_ftrace_filter = 'ext4_*'      → 只记录 ext4_ 开头的函数
  set_graph_function = 'ext4_file_*' → 但只对 ext4_file_* 展开子调用图
```

### function_graph 的性能成本

function_graph 比 function tracer 开销更大——因为每个函数入口和出口都要记录，还要计算时间。在 WSL2 的虚拟环境中，建议：

- **`set_graph_function` 限制在 < 20 个函数**
- **`set_ftrace_pid` 限制在一个进程**
- 如果系统变卡，立刻 `echo 'nop' > current_tracer`

---

## 🧪 试试看

1. 把 `set_graph_function` 从 `ext4_file_*` 改成 `ext4_*`——trace 输出变多了多少？嵌套深度增加了多少？
2. 在 `graph_demo.c` 中把 `fsync` 改成 `fdatasync`——延迟有变化吗？调用栈少了哪些函数？
3. 搜索 trace 中的 `!` 和 `#` 标记——哪个函数耗时最长？它在哪一层？
4. 同时用 `set_graph_function` 和 `set_ftrace_notrace`——把 `ext4_file_read_iter` 排除在外。trace 中还有它吗？

---

## 命令小结

| 操作 | 命令 |
|------|------|
| 选择 function_graph | `echo 'function_graph' \| sudo tee /sys/kernel/tracing/current_tracer` |
| 设置展开哪些函数的子调用 | `echo 'ext4_file_*' \| sudo tee /sys/kernel/tracing/set_graph_function` |
| 搜索最慢的调用 | `sudo cat /sys/kernel/tracing/trace \| grep '!'` |
| 清除 graph function 列表 | `echo "" \| sudo tee /sys/kernel/tracing/set_graph_function` |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 只有入口没有子调用 | `set_graph_function` 没设或设错了 | 确认函数名和 `available_filter_functions` 中的一致 |
| 全部是 `+` 和 `!` 标记 | 追踪了太多函数，系统整体变慢 | 缩小 filter 范围 |
| 输出中看不到 Phase 标记 | trace_marker 和 function_graph 输出混在一起 | 正常的——trace_marker 作为注释行穿插在调用图中 |

---

## 下一步

function_graph 让你精确看到每个内核函数的耗时。但每次都要手动写 tracefs 文件——下一步 [实验 04：trace-cmd 实战](../04-trace-cmd/)——把繁琐的文件操作封装成友好的命令行工具。
