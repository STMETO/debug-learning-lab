# 实验 04：trace-cmd 实战 —— 把 ftrace 变成命令行工具

> 🎯 **目标**：用 `trace-cmd record` 录制、`trace-cmd report` 回放、`trace-cmd stream` 实时查看、`trace-cmd start/stop` 后台追踪。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 03](../03-function-graph/)（function_graph 概念）

## 为什么需要 trace-cmd？

前三个实验中，你手动操作了这些文件：

```bash
echo 'function_graph' > current_tracer
echo 'ext4_file_*' > set_graph_function
echo 1 > tracing_on
./program
echo 0 > tracing_on
cat trace
```

每一步都是写文件、读文件——繁琐、容易出错、难以自动化。

**`trace-cmd` 把这些操作封装成了直觉式的命令行**：

```
trace-cmd record -p function_graph -g 'ext4_file_*' ./program
trace-cmd report
```

> trace-cmd 是 ftrace 的"用户界面"——生产环境中几乎都通过它来使用 ftrace。

---

## 先看示例程序

[workload.c](workload.c) 和实验 03 类似——快速文件操作 + fsync 操作的混合负载。trace-cmd 让我们能更优雅地追踪它：

```bash
make && ./workload
```

---

## 实验开始

### 第一步：trace-cmd record —— 录制

```bash
# -p function_graph：选择 tracer（等价于 echo function_graph > current_tracer）
# -g ext4_file_*：   graph function filter（等价于 echo ... > set_graph_function）
# ./workload：       要运行并追踪的程序
sudo trace-cmd record -p function_graph -g 'ext4_file_*' ./workload
```

输出：

```
  plugin 'function_graph'
CPU0 data recorded at offset=0x830000
    4096 bytes in size
CPU1 data recorded at offset=0x831000
    4096 bytes in size
...
```

> `trace-cmd record` 自动处理了 tracefs 的所有文件操作——选择 tracer、设置 filter、开启追踪、运行程序、停止追踪。追踪结果保存在 `trace.dat`（二进制格式）。

### 第二步：trace-cmd report —— 回放

```bash
sudo trace-cmd report | head -35
```

输出：

```
cpus=12
   workload-1005036 [005] 817168.783324: funcgraph_entry:    |  ext4_file_open() {
   workload-1005036 [005] 817168.783326: funcgraph_entry:    |    dquot_file_open() {
   workload-1005036 [005] 817168.783326: funcgraph_entry:   0.451 us | generic_file_open();
   workload-1005036 [005] 817168.783327: funcgraph_exit:    1.522 us |    }
   workload-1005036 [005] 817168.783327: funcgraph_exit:    4.729 us |  }
   workload-1005036 [005] 817168.783360: funcgraph_entry:    |  ext4_file_read_iter() {
   workload-1005036 [005] 817168.783361: funcgraph_entry:    |    generic_file_read_iter() {
   workload-1005036 [005] 817168.783361: funcgraph_entry:    |      filemap_read() {
   workload-1005036 [005] 817168.783373: funcgraph_exit:  + 12.143 us |      }
   workload-1005036 [005] 817168.783374: funcgraph_exit:  + 13.075 us |    }
   workload-1005036 [005] 817168.783374: funcgraph_exit:  + 14.057 us |  }
```

**格式解读**：

| 字段 | 含义 | 示例 |
|------|------|------|
| `workload-1005036` | 进程名-PID | 我们的 workload 程序 |
| `[005]` | CPU 核心号 | CPU 5 |
| `817168.783324` | 时间戳 | 开机以来的秒数 |
| `funcgraph_entry:` | 函数入口 | `{` 表示进入 |
| `funcgraph_exit:` | 函数出口 | `}` 表示退出，前面有总耗时 |
| `\|` 缩进 | 调用深度 | 更多 `\|` = 更深嵌套 |

> 这和实验 03 中直接 `cat trace` 看到的内容等价，但 trace-cmd report 的输出更规整——适合 grep、awk 等文本处理。

### 第三步：trace-cmd stream —— 实时查看

```bash
# 终端 1：启动实时追踪（像 tail -f trace_pipe）
sudo trace-cmd stream -p function_graph -g 'ext4_file_*'

# 终端 2：运行 workload
sudo ./workload
```

> `trace-cmd stream` 是实时流式输出——不需要等程序结束，事件一到缓冲区就显示。适合观察**正在发生**的内核行为。

按 `Ctrl+C` 停止。

### 第四步：trace-cmd start/stop —— 后台追踪

对于已运行的程序，不需要重新启动它：

```bash
# 启动后台追踪（不阻塞终端）
sudo trace-cmd start -p function_graph -g 'ext4_file_*'

# ... 此时运行任何程序，被追踪的函数都会被记录 ...

sudo ./workload

# 停止并保存
sudo trace-cmd stop
sudo trace-cmd extract -o trace.dat

# 查看
sudo trace-cmd report | head -20

# 清理
sudo trace-cmd reset
```

> `trace-cmd start/stop` 适合"先启动追踪，再复现问题"的场景——当你能复现 bug 但不知道它什么时候发生时。

### 第五步：常用选项速查

```bash
# 限制追踪时长（5 秒后自动停止）
sudo trace-cmd record -p function_graph -g 'ext4_*' sleep 5

# 只追踪特定 PID
sudo trace-cmd record -p function_graph -P <PID> ./workload

# 同时追踪函数 + tracepoint 事件
sudo trace-cmd record -p function_graph -g 'ext4_*' \
    -e 'syscalls:sys_enter_openat' \
    -e 'syscalls:sys_enter_write' \
    ./workload

# 输出到指定文件
sudo trace-cmd record -p function_graph -g 'ext4_*' -o my_trace.dat ./workload
sudo trace-cmd report -i my_trace.dat

# 只看特定 CPU 的追踪
sudo trace-cmd record -p function_graph -g 'ext4_*' -M 0 ./workload
```

### 第六步：trace-cmd vs 手动 tracefs

| 维度 | 手动操作 tracefs | trace-cmd |
|------|-----------------|-----------|
| 学习价值 | 高——理解 ftrace 原理 | 中——掌握工具用法 |
| 使用便利性 | 低——多次 echo/cat | 高——一行命令 |
| 自动化 | 差——脚本需要管理多个文件 | 好——命令行参数标准化 |
| 数据持久化 | 差——缓冲区是临时的 | 好——trace.dat 可保存、传输 |
| 适合场景 | 学习、调试 | 生产、自动化 |

> **建议**：学习阶段用 tracefs 手动操作（理解原理），生产环境用 trace-cmd（效率和可靠性）。

---

## 📝 核心认知

### trace-cmd 子命令速查

```
record   → 录制追踪（运行程序 + 收集数据）
report   → 回放追踪数据（文本格式）
stream   → 实时流式输出（类似 tail -f）
start    → 启动后台追踪（不阻塞）
stop     → 停止后台追踪
extract  → 提取追踪数据到文件
reset    → 重置所有 ftrace 设置
listen   → 网络监听（远程追踪）
```

### trace.dat 的结构

trace-cmd 的 `trace.dat` 是每个 CPU 核心独立缓冲区的合并——它保留了每个 CPU 的独立时间线。`trace-cmd report` 默认按时间戳交叉排列。

---

## 🧪 试试看

1. 用 `trace-cmd record -p function_graph -g 'ext4_file_*' -o before.dat ./workload` 录制。然后修改 `workload.c`（比如去掉 fsync），录制 `after.dat`。比较两份报告的延迟差异。
2. 在一个终端用 `trace-cmd stream -p function_graph -g 'ext4_*'`，在另一个终端用 `find /usr -name "*.so"` ——实时看到 `ext4_file_open` 事件吗？find 和 workload 产生的调用路径有什么不同？
3. 用 `trace-cmd record -p function_graph --help` 看看还有哪些选项——`-b`（buffer size）、`-M`（CPU mask）、`-T`（timer interval）分别是什么？
4. 试试 `trace-cmd record -p function -l 'ext4_file_*' -F ./workload`——`-l`（函数过滤器）和 `-g`（graph 展开）有什么不同？输出有什么区别？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `sudo trace-cmd record -p function_graph -g 'func_pattern' ./prog` | 录制追踪 |
| `sudo trace-cmd report` | 回放追踪数据 |
| `sudo trace-cmd stream -p function_graph -g 'func_pattern'` | 实时查看 |
| `sudo trace-cmd start -p function_graph -g 'func_pattern'` | 后台启动 |
| `sudo trace-cmd stop && sudo trace-cmd extract -o trace.dat` | 后台停止+保存 |
| `sudo trace-cmd reset` | 重置所有 ftrace 设置 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `CPU X: NNNNN events lost` | 缓冲区太小，高速事件溢出 | 加 `-b 8192`（8MB 缓冲区） |
| `Permission denied` | 需要 sudo | 加 `sudo` |
| `trace-cmd: command not found` | 未安装 | `sudo apt install trace-cmd` |
| `report` 为空 | `record` 时没有函数匹配 filter | 检查 `-g` 或 `-l` 的模式是否正确 |

---

## 下一步

trace-cmd 让 ftrace 从"文件操作"变成了"命令行工具"。现在你拥有 ftrace 核心技能的四件套——tracefs 基础操作 → function tracer → function_graph → trace-cmd。

下一步 [实验 05：事件与复合追踪](../05-events-combined/)——同时使用 tracepoint 事件和函数追踪，构建"既知道谁调用了什么、也知道系统事件何时发生"的完整视野。
