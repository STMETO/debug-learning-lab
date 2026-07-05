# 实验 06：应用层自定义追踪 —— 在代码中插入 Perfetto 追踪点

> 🎯 **目标**：在 C 程序中插入自定义追踪点；理解 Perfetto 的 `B|E|I` 文本追踪协议；对比 trace_marker 方式 vs Perfetto C++ SDK（TRACE_EVENT）；让应用埋点、ftrace 事件和 CPU 采样出现在同一条时间轴上。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 02](../02-ftrace-integration/)（ftrace 事件）、[ftrace 模块实验 01](../../ftrace/01-hello-ftrace/)（trace_marker）

## 为什么需要应用层追踪？

`linux.ftrace` 告诉你"内核中发生了什么"，`linux.perf` 告诉你"CPU 在做什么"。但两者都**看不到应用内部的逻辑**——你不知道 "process_batch_3" 具体什么时候开始、什么时候结束、处理了多少数据。

```
内核 ftrace:  openat → write → close → ...  (只知道系统调用)
CPU perf:     compute_loop 占 60%            (只知道函数热点)

缺少的:       什么时候开始处理 batch_3？batch_3 和 batch_5 哪个慢？
              ↑ 这是应用层追踪的价值
```

---

## 先看示例程序

[trace_app.c](trace_app.c) 用 `trace_marker` 实现了类似 Perfetto TRACE_EVENT 的效果：

```c
trace_begin("load_config");        // 标记开始
// ... do work ...
trace_end("load_config");          // 标记结束

trace_begin("process_batch_3");    // 标记开始
// ... 50 万次计算 ...
trace_instant("batch_result","ok"); // 瞬时事件
trace_end("process_batch_3");      // 标记结束
```

```bash
make && sudo ./trace_app
```

---

## 实验开始

### 第一步：理解 Perfetto 的追踪协议

Perfetto 支持两种方式注入应用事件：

**方式 A：trace_marker 文本协议（Linux 通用，本实验使用）**

向 `/sys/kernel/tracing/trace_marker` 写入特定格式的文本：

```
B|<pid>|<name>        → 开始（Begin）一个追踪区间
E|<pid>|<name>        → 结束（End）一个追踪区间
I|<pid>|<name>=<val>  → 瞬时（Instant）事件
```

**方式 B：Perfetto C++ SDK（Android / 完整 Linux SDK）**

```cpp
#include <perfetto.h>
PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("app").SetDescription("Application events"));
PERFETTO_TRACK_EVENT("app", "load_config");
```

> 方式 B 需要 Perfetto SDK 头文件和链接 libperfetto。预编译的 `linux-amd64.zip` 不包含 SDK。在 Android 开发中 SDK 随 NDK 提供；在 Linux 桌面可从源码编译或使用 trace_marker 替代。

### 第二步：编写带追踪的代码

trace_app.c 的核心——三个追踪函数：

```c
static int trace_fd = -1;
static int my_pid;

void trace_init() {
    trace_fd = open("/sys/kernel/tracing/trace_marker", O_WRONLY);
    my_pid = getpid();
}

void trace_begin(const char *name) {
    dprintf(trace_fd, "B|%d|%s\n", my_pid, name);
}

void trace_end(const char *name) {
    dprintf(trace_fd, "E|%d|%s\n", my_pid, name);
}
```

**在业务代码中使用**：

```c
void process_batch(int batch_id, int items) {
    char name[64];
    snprintf(name, sizeof(name), "process_batch_%d", batch_id);
    trace_begin(name);                              // ← 开始追踪

    volatile long sum = 0;
    for (int i = 0; i < items; i++) {
        sum += (i * 7 + 13) % 9973;
    }

    trace_end(name);                                // ← 结束追踪
}
```

> 这和 Android 中用 `TRACE_EVENT("app", "process_batch")` 的效果等价——在 Perfetto trace 中产生一个从 begin 到 end 的 slice。

### 第三步：录制包含应用埋点的 trace

```bash
cat > custom_config.txt << 'EOF'
buffers: { size_kb: 8192 fill_policy: RING_BUFFER }
data_sources: {
    config {
        name: "linux.ftrace"
        ftrace_config {
            ftrace_events: "sched/sched_switch"
            ftrace_events: "syscalls/sys_enter_openat"
            ftrace_events: "syscalls/sys_enter_read"
            ftrace_events: "syscalls/sys_enter_close"
            # trace_marker 写入的内容作为 print 事件出现在 ftrace 中
            ftrace_events: "ftrace/print"
        }
    }
}
duration_ms: 4000
EOF

sudo lib/perfetto/tracebox -c custom_config.txt --txt -o custom_trace.perfetto-trace &
sudo ./trace_app
wait
```

### 第四步：在时间轴上看到应用的三个阶段

在 Perfetto UI 中打开 trace 文件，你会看到应用埋点和内核事件在同一时间轴上：

```
┌─────────────────────────────────────────────────────────────┐
│  [App Slices - 从 trace_marker B/E 解析]                     │
│  ╔══════════════╗   ╔═════╗╔═════╗╔═════╗╔═════╗╔═════╗    │
│  ║ load_config  ║   ║ B_0 ║║ B_1 ║║ B_2 ║║ B_3 ║║ B_4 ║    │
│  ╚══════════════╝   ╚═════╝╚═════╝╚═════╝╚═════╝╚═════╝    │
│                                                             │
│  [Ftrace events]                                             │
│  openat close read   openat close  openat close              │
│                                                             │
│  时间 →                                                      │
└─────────────────────────────────────────────────────────────┘
```

> 应用埋点让你**精确知道**每个业务操作的起止时间——这是内核事件和 CPU 采样都提供不了的。

### 第五步：三种追踪层次的全景

现在你有三个层次的追踪数据在同一时间轴上：

```
层次 3 - 应用埋点： load_config(), process_batch_0...4, save_results()
         ↑ "业务操作"的起止时间和顺序
         
层次 2 - 系统调用：  openat, read, write, close, fsync
         ↑ 应用操作触发了哪些内核交互？
         
层次 1 - CPU 采样：   compute_loop 占用 X% CPU，等待 I/O 期间 CPU 在做什么？
         ↑ CPU 微架构层面的热点
```

> 这是 Perfetto 的核心设计理念——**把不同来源的数据对齐到同一条时间轴上**，消除"盲人摸象"式的性能分析。

---

## 📝 核心认知

### trace_marker 的限制（Linux 桌面环境）

| 能力 | trace_marker | Perfetto C++ SDK |
|------|-------------|-----------------|
| 标记开始/结束 | ✅ (B/E 协议) | ✅ (TRACE_EVENT_BEGIN/END) |
| 自定义参数 | ❌ (只有字符串) | ✅ (key-value pairs) |
| 时间戳精度 | 微秒（系统调用延迟） | 纳秒（用户态时间戳） |
| 动态启用/禁用 | ❌ | ✅ (按 category 开关) |
| 跨进程追踪 | ❌ (需要同步 PID) | ✅ (自动) |
| Linux 桌面可用性 | ✅ 开箱即用 | ⚠️ 需编译 SDK |

### 生产建议

```
开发/测试：trace_marker（简单，零依赖）
Android 生产：Perfetto C++ SDK（TRACE_EVENT，完整功能）
Linux 生产：考虑 OpenTelemetry + Perfetto exporter
```

---

## 🧪 试试看

1. 在 `process_batch` 内部加额外的 `trace_begin/end` 对（嵌套追踪）——Perfetto UI 能正确显示嵌套的 slice 吗？
2. 把 `trace_begin("X")` 和 `trace_end("X")` 故意配对错误（两个 begin 一个 end）——Perfetto 怎么处理？时间轴上怎么显示？
3. 对比本实验的 trace 和 [ftrace 模块实验 01](../../ftrace/01-hello-ftrace/) 的纯 `write(marker, "msg")`——两种写入方式在 trace 中的表现有什么不同？

---

## 命令小结

| 操作 | 方式 |
|------|------|
| 标记开始 | `dprintf(fd, "B\|%d\|%s\n", pid, name)` |
| 标记结束 | `dprintf(fd, "E\|%d\|%s\n", pid, name)` |
| 瞬时事件 | `dprintf(fd, "I\|%d\|%s=%s\n", pid, key, val)` |
| 配置中启用 | `ftrace_events: "ftrace/print"` |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| trace 中看不到应用埋点 | `ftrace/print` 事件没在 config 中启用 | 加 `ftrace_events: "ftrace/print"` |
| `Permission denied` 写 trace_marker | 需要 root | 加 `sudo` |
| B/E 配对错误导致 UI 显示异常 | begin 和 end 的名字或 PID 不匹配 | 确保 `trace_begin("X")` 和 `trace_end("X")` 严格配对 |

---

## 下一步

应用埋点 + ftrace 事件 + CPU 采样——三个层次的追踪数据已就绪。下一步 [实验 07：全栈延迟排查](../07-real-world/)——给你一个"不知道哪里慢"的程序，用这三种数据源找出根因。
