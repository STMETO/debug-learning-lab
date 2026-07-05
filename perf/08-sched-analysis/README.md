# 实验 08：调度与锁分析 —— 程序在等待什么？

> 🎯 **目标**：用 `perf sched` 分析任务调度延迟、CPU 迁移和上下文切换，理解"程序在等 CPU 还是在等别的"。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 03](../03-perf-record/)（perf record）

## 为什么需要调度分析？

`perf stat` 告诉你"程序花了多少 CPU 周期"，`perf report` 告诉你"哪些函数在吃 CPU"。但它们都回答不了：

- "程序是不是经常被内核踢出 CPU？"
- "每次等待调度花了多长时间？"
- "程序在不同的 CPU 核之间漂移了吗？"

**`perf sched` 回答的就是这些"调度"层面的问题。**

---

## 先看示例程序

[sched_demo.c](sched_demo.c) 创建了 5 个子进程，模拟两种典型负载：

| 子进程 | 类型 | 行为 |
|--------|------|------|
| 3 个 cpu-worker | CPU 密集型 | 200ms 不间断运算——用满时间片 |
| 2 个 mixed-worker | 混合型 | 运算 + sleep(1ms) 交替 20 轮——频繁让出/获取 CPU |

```bash
make && ./sched_demo
```

```
=== Scheduling Analysis Demo ===
  [cpu-worker-0] done (99000000 iterations)
  [cpu-worker-1] done (99000000 iterations)
  [cpu-worker-2] done (99000000 iterations)
  [mixed-worker-0] done
  [mixed-worker-1] done
=== Done! ===
```

---

## 实验开始

### 第一步：采集调度事件

```bash
sudo perf sched record -o perf.sched ./sched_demo
```

```
=== Scheduling Analysis Demo ===
  [cpu-worker-0] done (99000000 iterations)
  ...
=== Done! ===
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.325 MB perf.sched (1401 samples) ]
```

> ⚠️ `perf sched record` 可能需要 sudo（因为它追踪内核调度事件）。如果 `perf_event_paranoid=1`，追踪自己的进程通常不需要 sudo，但追踪**所有 CPU 的调度事件**需要。WSL2 环境建议加上 `sudo`。

### 第二步：查看调度延迟

```bash
sudo perf sched latency -i perf.sched
```

```
 ----------------------------------------------------------------------------------
  Task                  |   Runtime ms  | Switches | Avg delay ms    | Max delay ms
 ----------------------------------------------------------------------------------
  :864955:864955        |    193.542 ms |        1 | avg:   0.069 ms | max:   0.069 ms
  :864958:864958        |      0.932 ms |        1 | avg:   0.064 ms | max:   0.064 ms
  :864956:864956        |      4.220 ms |        1 | avg:   0.059 ms | max:   0.059 ms
  sleep:(3)             |      0.000 ms |      188 | avg:   0.052 ms | max:   0.527 ms
  :864953:864953        |    193.105 ms |        4 | avg:   0.043 ms | max:   0.088 ms
  :864954:864954        |    193.269 ms |        3 | avg:   0.033 ms | max:   0.061 ms
  :864941:864941        |      3.276 ms |        1 | avg:   0.000 ms | max:   0.000 ms
```

**解读各列**：

| 列 | 含义 | 示例解读 |
|----|------|---------|
| **Task** | 进程名:TID:PID 或 :TID:PID（内核线程） | `:864955` 是我们的 cpu-worker 子进程 |
| **Runtime ms** | 该任务在 CPU 上运行的总时长 | 第 1 个 cpu-worker 跑了 193.5ms |
| **Switches** | 该任务被调度进 CPU 的次数 | `1` = 只被调度了一次（一直在跑，没被打断） |
| **Avg delay ms** | 平均等待调度延迟 | 0.069ms = 几乎立即被调度 |
| **Max delay ms** | 最大等待调度延迟 | 0.527ms（sleep 的某次唤醒等待了半毫秒） |

**关键发现**：

1. **cpu-worker 们 Switches=1**：CPU 密集型任务几乎不被打断——它们用满整个时间片。
2. **sleep 进程 Switches=188**：`usleep(1000)` 导致频繁的 sleep/wakeup——每次 1ms 的睡眠都触发一次调度。
3. **Avg delay 都很小（<0.07ms）**：系统不忙，任务几乎立即获得 CPU。如果 delay > 10ms，说明存在调度竞争。

### 第三步：时间线视图

```bash
sudo perf sched timehist -i perf.sched
```

```
           time    cpu  task name                       wait time  sch delay   run time
                        [tid/pid]                          (msec)     (msec)     (msec)
--------------- ------  ------------------------------  ---------  ---------  ---------
  811268.022175 [0000]  perf[49786]                         0.000      0.000      0.000
  811268.022240 [0000]  sleep                               0.000      0.000      0.064
  811268.022487 [0001]  perf[49786]                         0.000      0.000      0.000
  811268.022720 [0001]  sleep                               0.246      0.000      0.233
  811268.023000 [0002]  perf[49786]                         0.000      0.000      0.000
  811268.023009 [0001]  <idle>                              0.000      0.000      0.288
  811268.023046 [0001]  sleep                               0.288      0.000      0.037
```

**解读各列**：

| 列 | 含义 |
|----|------|
| **time** | 调度事件发生的时间戳（秒） |
| **cpu** | 在哪个 CPU 核心上运行 |
| **wait time** | 该任务在等待队列中等了多久（被唤醒到真正运行之间的时间） |
| **sch delay** | 调度延迟（调度器从决定调度到实际切换的时间） |
| **run time** | 这次实际在 CPU 上运行了多久 |

> `<idle>` 出现在 cpu 0001 ——这个核心在等新任务，当时没有可运行的进程。

### 第四步：诊断信号

用三个指标判断调度是否健康：

| 指标 | 健康值 | 警告值 | 诊断 |
|------|--------|--------|------|
| **Avg delay** | < 1ms | > 10ms | CPU 过载——太多进程抢同一个 CPU |
| **Switches（低频任务）** | 接近 1 | > 10 | 任务频繁被打断——可能是锁竞争或 I/O 等待 |
| **wait time** | < 1ms | > 10ms | 长时间等待——CPU 被其他任务抢占 |

### 第五步：上下文切换的全局视角

用软件事件看整体的上下文切换量：

```bash
perf stat -e context-switches,cpu-migrations ./sched_demo
```

```
          X      context-switches
          Y      cpu-migrations
```

> **cpu-migrations**：一个任务从 CPU 核 A 被移动到 CPU 核 B 的次数。迁移过多（> 每秒 100 次）意味着调度器在频繁抖动——任务的 CPU 缓存（L1/L2）每次都失效，性能下降。

对于长时间运行的服务，可以持续监控：

```bash
# 每秒报告一次系统的上下文切换率（需要另一终端运行 sched_demo）
perf stat -e context-switches -a -I 1000 sleep 5
```

---

## 📝 核心认知

### 三种"慢"

```
"程序慢"有三种可能：
1. CPU 在干活（cycles 高，IPC 正常）        → 算法慢，用 perf record/report 找热点函数
2. CPU 在等内存（stalled-cycles 高）        → 缓存问题，用实验 09 的 cache 事件分析
3. CPU 在等调度（wait time 长，Switches 多） → 调度问题，用 perf sched 分析
```

### perf sched 的数据来源

`perf sched record` 采集的是内核中 `sched:sched_switch`、`sched:sched_wakeup` 等 tracepoint 事件。你可以直接用这些 tracepoint：

```bash
perf record -e 'sched:sched_switch,sched:sched_wakeup' -o perf.data ./sched_demo
perf script -i perf.data | head -15
```

---

## 🧪 试试看

1. 把 `cpu_worker` 的 `duration_ms` 从 200 改成 1000——`Switches` 有变化吗？为什么 CPU 密集型任务的 Switches 总是接近 1？
2. 把 `mixed_worker` 的 `usleep(1000)` 改成 `usleep(1)`——调度延迟变化吗？Switches 变化吗？
3. 在另一个终端用 `stress -c 4` 制造 CPU 压力，同时跑 `perf sched record ./sched_demo`——Avg delay 变大了吗？
4. 用 `perf stat -e cpu-migrations -a -I 1000` 持续 5 秒监控系统的迁移率——平时有多少？跑 `./sched_demo` 时增加了吗？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `sudo perf sched record ./program` | 记录调度事件 |
| `sudo perf sched latency -i perf.sched` | 报告每个任务的调度延迟 |
| `sudo perf sched timehist -i perf.sched` | 时间线视图——看"谁在哪个核心跑了多久" |
| `perf stat -e context-switches,cpu-migrations ./program` | 快速查看上下文切换和 CPU 迁移数量 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `perf sched record` 报 Permission denied | 需要追踪内核调度事件 | 加 `sudo` |
| 输出中任务名全是 `:TID` | 子进程没有独立的进程名 | 正常——fork 出的子进程继承父进程名字但 perf 可能显示为原始内核线程格式 |
| `Samples do not have callchains` | sched 事件默认不带调用栈 | 正常——`perf sched timehist` 不需要调用栈 |

---

## 下一步

调度分析告诉你"程序在不在 CPU 上"。下一步 [实验 09：内存与缓存分析](../09-mem-cache/)——当程序在 CPU 上时，查看它的内存访问效率（缓存命中率、伪共享、访问延迟）。
