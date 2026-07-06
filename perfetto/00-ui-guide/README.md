# 实验 00：读懂 Perfetto UI —— 可视化界面完全指南

> 🎯 **目标**：打开一条 trace 文件后，**看懂**时间轴上的每一行（track）、每一个色块（slice）、每一条竖线（counter）。知道怎么缩放、搜索、查看详情。
>
> ⏱️ 约 15 分钟
>
> 📌 **先读完这个实验，再去做其他实验**——UI 是所有实验的共同基础。

---

## 准备工作：生成一条用于学习的 trace

复制以下命令**整段粘贴**到终端执行（这是一条完整的录制+分析流水线）：

```bash
cd /home/debug-learning-lab/perfetto/00-ui-guide && make && cat > ui_demo_config.txt << 'CEOF'
buffers: { size_kb: 8192 fill_policy: RING_BUFFER }
data_sources: {
    config {
        name: "linux.ftrace"
        ftrace_config {
            ftrace_events: "sched/sched_switch"
            ftrace_events: "sched/sched_waking"
            ftrace_events: "syscalls/sys_enter_openat"
            ftrace_events: "syscalls/sys_enter_write"
            ftrace_events: "syscalls/sys_enter_close"
            ftrace_events: "ext4/ext4_da_write_begin"
            ftrace_events: "ext4/ext4_da_write_end"
        }
    }
}
data_sources: {
    config { name: "linux.perf" perf_event_config { all_cpus: true sampling_frequency: 100 } }
}
duration_ms: 5000
CEOF
sudo ../../lib/perfetto/tracebox -c ui_demo_config.txt --txt -o ui_demo.perfetto-trace & sleep 0.5 && ./ui_demo && wait && echo "=== Trace ready: $(du -h ui_demo.perfetto-trace | cut -f1) ===" && echo "=== Open in browser: https://ui.perfetto.dev ==="
```

然后在浏览器打开 https://ui.perfetto.dev ，拖入生成的 `ui_demo.perfetto-trace` 文件。

---

## UI 全景：你看到的每一块是什么

打开 trace 后，你会看到类似这样的布局：

```
┌── Perfetto UI ──────────────────────────────────────────────────────────┐
│  [🔍 search_________]  [⏮ ⏪ ▶ ⏩ ⏭]  [⏰ clock]  [📏 ruler]  [⚙ menu]│ ← 顶部工具栏
├── Tracks (左侧面板) ──┬── Timeline (右侧主区域) ─────────────────────────┤
│                       │                                                  │
│  📊 CPU Scheduling    │ ═══════════════════════════════════════════════  │
│    ├─ CPU 0           │ ██████████████████████████████████████████████  │
│    ├─ CPU 1           │ ████████████████████████                         │
│    ├─ CPU 2           │ ██████████████████████████████████████████████  │
│    └─ ...             │ ...                                              │
│                       │                                                  │
│  🔵 ftrace Events     │                                                  │
│    ├─ sched_switch    │ ────────┬──────┬──┬───┬────────┬───────┬──────  │
│    ├─ sched_waking     │ ───┬────┴──┬───┴──┴───┴────┬───┴───────┴──────  │
│    ├─ openat          │ ──┬┴┬─┬┴┬─┬┴┬─┬┴┬─┬┴┬─┬┴┬──                      │
│    └─ ...             │ ...                                              │
│                       │                                                  │
│  🟢 Process Tracks    │                                                  │
│    ├─ ui_demo [3021]  │ ════════════════  (我们的程序!)                    │
│    ├─ sshd [892]      │ ██                                               │
│    └─ init [1]        │ █                                                │
│                       │                                                  │
│  ⏱  0s        1s        2s        3s        4s        5s                │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 第一部分：顶部工具栏

```
🔍 搜索框      输入函数名或事件名，匹配的会在时间轴上高亮（比如搜 "openat"）
⏮ ⏪ ▶ ⏩ ⏭  导航按钮：跳到开头/上一点/播放/下一点/跳到结尾
⏰ 时间显示    当前位置的时间戳（点击可切换显示格式）
⚙  齿轮菜单    切换深色/浅色主题、显示选项
```

> **最常用**：搜索框。在大型 trace 中定位关键字比肉眼扫快得多。

---

## 第二部分：时间轴操作（最重要的技能）

| 操作 | 如何做 | 效果 |
|------|--------|------|
| **缩放** | 鼠标滚轮 或 按 `w` / `s` 键 | 放大 = 看到微秒级细节；缩小 = 看全局 |
| **平移** | 按住鼠标拖拽 或 按 `a` / `d` 键 | 左右移动时间轴 |
| **框选放大** | 按住鼠标在时间轴上拖出一个矩形 | 放大到选中的时间范围 |
| **点击事件** | 单击任意色块/圆点 | 展开详情面板（见下方） |
| **Reset Zoom** | 按 `m` 键或点击左下角 "Reset Zoom" | 恢复到全局视图 |

> 🔑 **先学会框选放大**：在时间轴上按住鼠标拖一个矩形 → 自动放大到那个区域。这是最直观的定位方式。

### 详情面板（点击事件后出现）

点击一个色块或圆点后，右侧或底部会弹出：

```
┌─ Event Detail ─────────────────────┐
│  Name:    sched_switch             │
│  Timestamp: 0.235172 s             │
│  Duration: 0.012 ms                │
│  CPU:     3                        │
│  Thread:  ui_demo [3021]           │
│  Args:    prev_state=R             │
└────────────────────────────────────┘
```

每个事件都有**名称、精确时间、持续时间、所在 CPU、所属进程**。Args 是事件特有的参数（如 `prev_state=R` 表示前一个进程处于 Running 状态）。

---

## 第三部分：Track 面板逐行详解

### 📊 Track 组 1：CPU Scheduling（CPU 调度轨道）

```
CPU 0  ██████████████████████████████████████████████████
CPU 1  ████████████████████████
CPU 2  ██████████████████████████████████████████████████
```

**这是什么**：每个 CPU 核心上"当前正在运行哪个进程/线程"的时间线。

**怎么看**：
- **色块颜色 = 不同的进程**（蓝色可能是 sshd，橙色可能是 ui_demo，灰色可能是 idle）
- **色块宽度 = 该进程在 CPU 上连续运行的时间**
- **空白/浅色 = CPU 空闲（idle）**
- **点击色块 → 看到进程名、PID、运行时长**

**实战意义**：
- "为什么程序慢？" → 找到你的程序的色块 → 它很窄/很少？→ CPU 被其他进程抢走了
- "为什么 CPU 100%？" → 看哪个进程的色块占满了整个 CPU 轨道的宽度
- "多核用上了吗？" → 看程序色块分布在几个 CPU 上 → 只在 1 个上 = 单线程

### 🔵 Track 组 2：ftrace Events（内核事件轨道）

```
sched_switch     ────────┬──────┬──┬───┬────────┬───────┬──────
sched_waking      ───┬────┴──┬───┴──┴───┴────┬───┴───────┴──────
sys_enter_openat  ──┬┴┬─┬┴┬─┬┴┬─┬┴┬─┬┴┬─┬┴┬──
sys_enter_close   ──┬┴┬─┬┴┬─┬┴┬─┬┴┬─┬┴┬─┬┴┬──
```

**这是什么**：每一个竖线/圆点 = 一次事件发生。这是你在 config 中 `ftrace_events` 列出的那些事件。

**怎么看**：
- **每个竖线 = 一个事件**（如一次 `sched_switch` 或一次 `openat`）
- **竖线密集 = 事件高频**（如 `sched_switch` 每秒数千次）
- **竖线稀疏 = 事件低频**（如 `ext4_da_write_begin` 可能只有几条）
- **点击竖线 → 看到事件名、时间戳、所属进程**

**实战意义**：
- 在 `sys_enter_openat` 轨道上框选 → 看到程序什么时候在做文件操作
- 对比 `sched_switch` 和你的程序色块——程序运行时 sched_switch 是否减少？

### ⏱ Track 组 3：Scheduler（调度器聚合轨道）

```
Scheduler
  ├─ Running     (哪个进程正在运行)
  ├─ Runnable    (哪些进程在等待 CPU)
  ├─ Sleeping    (哪些进程在休眠)
  └─ Uninterruptible Sleep  (D 状态——等 I/O)
```

**这是什么**：不是单个事件，而是**按时间聚合后的"系统中有多少进程在 XX 状态"**。这是 trace_processor 自动计算的衍生轨道。

**怎么看**：
- **Running 宽 = 系统繁忙**
- **Runnable 宽 = 有进程在排队等 CPU**（调度瓶颈！）
- **Uninterruptible Sleep 宽 = 有进程在等磁盘 I/O**（存储瓶颈！）

### 📈 Track 组 4：Perf Counters（CPU 性能计数器）

```
cpu-clock     ────────────────────  (线条 = 计数器值随时间变化)
```

**这是什么**：`linux.perf` 数据源采集的硬件计数器，以**连续线条**（而非离散事件）展示。

**怎么看**：线条的值随时间波动 → 点击线条可以看到具体数值。

### 🟢 Track 组 5：Process Tracks（进程/线程轨道）

```
ui_demo [3021]   ════════════════
sshd [892]       ██
init [1]         █
```

**这是什么**：每个进程/线程的"生命周期 + 运行片段"。**你的程序会在这里出现**。

**怎么看**：
- 你的程序（如 `ui_demo`）的轨道 = 它从开始到结束的时间范围
- 展开进程轨道（点击 ▶）→ 看到该进程内部的线程
- **如果你在代码中用了 trace_marker（实验 06 的 B/E 协议），那些 slice 会出现在这里**——标记"load_config"的开始和结束

---

## 第四部分：三种事件的视觉形态

Perfetto UI 中有三种基本视觉元素：

| 视觉形态 | 叫什么 | 例子 | 含义 |
|---------|--------|------|------|
| **彩色矩形**（有宽度） | **slice** | 进程在 CPU 上的连续运行、应用埋点的区间 | 有**开始时间**和**持续时间**的事件 |
| **竖线/圆点**（无宽度） | **instant** | `sched_switch`、`sys_enter_openat` | 在**某个时刻**发生的瞬时事件 |
| **连续线条** | **counter** | `cpu-clock`、`cpu-frequency` | 随时间**连续变化**的数值 |

> 🔑 **slice 有宽度，instant 没有，counter 是线条**。记住这三种形态，所有 track 都是它们的组合。

---

## 第五部分：实战演练 —— 5 分钟读懂一条 trace

用我们刚录制的 `ui_demo.perfetto-trace`，按以下顺序操作：

### Step 1：全局缩放 —— 看到 5 秒的全貌

打开文件 → 按 `m` 键（Reset Zoom）→ 看到 0s 到 5s 的完整时间轴。

**观察**：
- CPU 0-11 都有色彩（系统在正常工作）
- `sched_switch` 轨道上竖线覆盖了整个时间轴（调度一直在发生）
- `sys_enter_openat` 轨道上有三簇密集的竖线（对应程序的三个 Batch）

### Step 2：框选 Batch 2 的时间窗口

在 `sys_enter_openat` 轨道上，找到第二簇竖线 → 按住鼠标拖一个矩形框选它 → UI 自动放大。

**观察**：
- 放大后，你看到 `openat` 和 `close` 的竖线是**配对出现**的（open 一个文件，close 一个文件）
- 放大后的 `sched_switch` 竖线间距变大了——你能看到每条之间的间隔

### Step 3：点击一个 sched_switch 事件

**观察详情面板**：
- `prev_state=R` → 前一个进程是 Running（时间片用完才切换的）
- `prev_state=S` → 前一个进程是 Sleeping（自己主动让出 CPU）

> `prev_state=S` 的切换说明进程在等什么东西（I/O、锁、定时器）。

### Step 4：找到我们的程序

在左侧 Tracks 面板中，找到名为 `ui_demo` 或匹配 PID 的 Process Track。如果它被折叠在右侧面板中，展开 "Processes" 列表。

**观察**：程序的进程轨道从 ~0.3s 开始，到 ~4s 结束。三个 Batch 之间有 `usleep(500000)` 产生的间隙。

### Step 5：用搜索定位特定事件

按 Ctrl+F → 输入 `ext4_da_write` → 匹配的事件被高亮。

**观察**：高亮的事件只有 22 个（全部在三个 Batch 内）。这说明程序的文件写入大部分被 page cache 吸收了——和实验 02 讲的一致。

---

## 📝 核心认知

### Track 的层次从粗到细

```
系统级（最粗）：
  CPU Scheduling → "哪个进程在用 CPU？"
  Scheduler     → "系统中有多少进程在等待？"

事件级（中等）：
  ftrace Events → "内核中发生了什么事件？"
  Perf Counters → "CPU 性能计数器值是多少？"

进程级（最细）：
  Process Tracks → "我的程序在做什么？"
  应用埋点       → "程序内部的业务逻辑阶段"
```

### 三种最常见的"看不懂"及解答

| 你看到的 | 是什么 | 需要关心吗 |
|---------|--------|----------|
| `sshd`、`init`、`systemd` 等进程 | 系统守护进程——一直在后台运行 | 一般不需要关心，除非它们占了大量 CPU |
| `<idle>` 或大片灰色区域 | CPU 空闲状态——没有任务可运行 | 正常——除非你的程序在此期间需要 CPU |
| 大量 `sched_switch` 竖线 | 正常的调度切换——12 核系统每秒数千次 | 正常——关注"你的程序被切出去"的比例 |

---

## 试试看

1. 在 UI 中分别点击 `openat` 和 `close` 的一个事件——它们的 `Args` 里有什么？能看到文件路径吗？
2. 搜索 `waking` → 在时间轴上高亮 → `sched_waking` 和 `sched_switch` 的关系是什么？（提示：waking 在先，switch 在后——唤醒后才调度）
3. 框选一个 Batch 的时间窗口，看这个窗口内的 CPU Scheduling——`ui_demo` 在哪个 CPU 上运行？是固定在某个核还是跨核迁移？

---

## 下一步

UI 的基本操作和轨道含义都清楚了。现在可以进入 [实验 01：Hello Perfetto](../01-hello-perfetto/)——自己写 config、录制 trace、用 SQL 分析。
