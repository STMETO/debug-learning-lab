# 实验 12：网络中断与软中断追踪 —— 网络为什么延迟？

> 🎯 **目标**：用 `irq:` 和 `net:` tracepoint 追踪网络包的处理流程；定位中断漂移、软中断瓶颈；用 `perf trace` 监控 socket 系统调用延迟。
>
> ⏱️ 约 18 分钟
>
> 📖 **前置**：[实验 07](../07-perf-trace/)（tracepoint）、[实验 08](../08-sched-analysis/)（调度概念）

## 一个网络包的内核之旅

当一个 TCP 包到达网卡时，内核需要经过多层处理才能交给你的 `recv()`：

```
网卡硬件中断 → 硬中断处理（极短）→ 触发软中断(NET_RX) → NAPI 收包
 → 网络栈(TCP/IP) → socket 缓冲区 → 应用程序 recv()
```

```
时间线：
IRQ  ┤  ████                                    ████
softirq┤        ████████████████████████                ████████████
应用   ┤                                    ██████████████
       ↑                                    ↑
    硬中断                                recv() 返回
    (< 10μs)                             (> 几十 μs)
```

> 用 `perf` 的 `irq:` 和 `net:` tracepoint，你可以看到这个流程的每一步。

---

## 先看示例程序

[netdemo.c](netdemo.c) 创建了一个 TCP echo 服务器 + 客户端，在本地回环（loopback）上跑 500 次请求/响应：

```bash
make && ./netdemo
```

---

## 实验开始

### 第一步：观察软中断事件

```bash
sudo perf record -e 'irq:softirq_entry,irq:softirq_exit' -o perf.data ./netdemo
sudo perf script -i perf.data | head -20
```

```
 netdemo 50467 [010] 811986.121098:  irq:softirq_entry: vec=7 [action=SCHED]
 netdemo 50467 [010] 811986.121107:  irq:softirq_entry: vec=9 [action=RCU]
 netdemo 50467 [010] 811986.173227:  irq:softirq_entry: vec=3 [action=NET_RX]
```

**软中断类型速查**：

| vec | action | 含义 | 触发场景 |
|-----|--------|------|---------|
| 3 | NET_RX | 网络接收 | 网卡收到数据包 |
| 6 | TASKLET | 小任务 | 驱动程序推迟执行 |
| 7 | SCHED | 调度 | 调度器唤醒/抢占 |
| 9 | RCU | 读-复制-更新 | 内核 RCU 回调 |

> 看到 `NET_RX` 了吗？它就是处理网络包的软中断。每次你的程序 `recv()` 之前，网络数据包就是被 `NET_RX` 软中断处理的。

### 第二步：追踪网络包的内核路径

```bash
sudo perf record -e 'net:net_dev_queue,net:net_dev_start_xmit,net:netif_receive_skb' -o perf.data ./netdemo
sudo perf script -i perf.data | head -15
```

```
 netdemo 50467 [010] 811986.173217:  net:net_dev_queue: dev=lo skbaddr=0xffff91ef9e7af8e8 len=74
 netdemo 50467 [010] 811986.173222:  net:net_dev_start_xmit: dev=lo queue_mapping=0 skbaddr=... len=74 protocol=0x0800
 netdemo 50467 [010] 811986.173227:  irq:softirq_entry: vec=3 [action=NET_RX]
 netdemo 50467 [010] 811986.173244:  net:net_dev_queue: dev=lo skbaddr=0xffff91ef83910300 len=74
```

这个序列显示了一个网络包的完整旅程：

```
1. net_dev_queue      → 包进入发送队列（应用刚 send() 完）
2. net_dev_start_xmit → 包开始传输（进入网卡驱动）
3. softirq_entry(NET_RX) → 对端收到包，触发软中断
4. net_dev_queue      → 回包进入队列（echo server send 响应）
```

**关键字段**：

| 字段 | 含义 | 示例值 |
|------|------|--------|
| `dev=lo` | 网络设备名 | loopback（本实验）或 `eth0`、`wlan0` |
| `len=74` | 数据包长度（字节） | 74 字节 = 20(IP头) + 20(TCP头) + 34(数据) |
| `protocol=0x0800` | IPv4 | 0x0800=IPv4, 0x86DD=IPv6 |
| `skbaddr=...` | socket buffer 内核地址 | 可用于追踪同一个包的多个事件 |

### 第三步：用 perf trace 监控网络系统调用

在另一个终端（或前台）跑 netdemo，用 `perf trace` 看 socket 系统调用的耗时：

```bash
perf trace -e 'socket*,connect*,send*,recv*' -- ./netdemo 2>&1 | head -30
```

重点关注耗时 `>` 标记的慢调用——如果 `connect()` 花了 >100ms，说明网络有问题。

### 第四步：统计事件的分布

```bash
sudo perf record -e 'net:net_dev_queue,net:net_dev_start_xmit,irq:softirq_entry' -o perf.data ./netdemo
sudo perf script -i perf.data | awk '{print $5}' | sort | uniq -c | sort -rn
```

```
  14111  irq:softirq_entry:
   7161  net:net_dev_queue:
      1  net:net_dev_start_xmit:
```

> 软中断占了大部分事件（14,111 次）——说明网络处理触发了大量软中断。每个包从发送到接收至少触发两次 NET_RX（客户端和服务端各一次）。

### 第五步：诊断中断漂移

中断漂移（IRQ imbalance）是指网卡中断被固定到一个 CPU 核心上，导致该核心成为瓶颈。用 `perf stat` 检查：

```bash
sudo perf stat -e 'irq:irq_handler_entry' -a -I 1000 sleep 3
```

观察每个核心上的中断数量。如果某个核心的中断数远高于其他核心，说明存在中断漂移——需要调整 IRQ affinity（`/proc/irq/*/smp_affinity`）。

---

## 📝 核心认知

### 网络延迟的三个来源

```
网络延迟 ≠ 网络传输时间

1. 中断处理延迟 — 从中断到达 CPU 到软中断开始执行
2. 协议栈延迟   — TCP/IP 处理、socket 缓冲区操作
3. 应用延迟     — 从数据到达 socket 到 recv() 被调用

perf 能帮你测量 1 和 2，3 需要应用层埋点。
```

### softirq 的关键属性

- **softirq 运行在中断上下文中**：不能被用户态进程抢占，也不能睡眠
- **NET_RX 受 `net.core.netdev_budget` 限制**：一次软中断最多处理 300 个包（默认值），避免饥饿其他软中断
- **高网络吞吐下 softirq 可能成为瓶颈**：如果 `softirq_entry(NET_RX)` 的频率极高，考虑启用 NAPI 或 RPS/RFS

---

## 🧪 试试看

1. 把 `N_REQUESTS` 从 500 改成 5000——`net_dev_queue` 事件数量线性增长吗？softirq 事件呢？
2. 在跑 netdemo 的同时，用另一个终端 `ping -f 127.0.0.1`（flood ping）——irq 事件数量大幅增加了吗？
3. `perf stat -e 'irq:softirq_entry' -a -I 1000 sleep 3`——你的系统每秒有多少次软中断？空闲时的基线是多少？
4. 把 `netdemo.c` 的 `usleep(50000)`（给服务器的启动时间）改成 1ms——会发生什么？connect 失败了吗？perf trace 能看到错误码吗？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `perf record -e 'irq:softirq_entry' ./prog` | 追踪软中断事件 |
| `perf record -e 'net:net_dev_queue,net:net_dev_start_xmit' ./prog` | 追踪网络包的发送路径 |
| `perf record -e 'net:netif_receive_skb' ./prog` | 追踪网络包的接收路径 |
| `perf trace -e 'socket*,connect*,send*,recv*' ./prog` | 追踪 socket 系统调用耗时 |
| `perf stat -e 'irq:irq_handler_entry' -a -I 1000` | 每秒报告各核心的中断数 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `net:net_dev_queue` 几乎没有事件 | 走的是 loopback，不走真实网卡 | loopback 也触发 net tracepoint——如果没有，检查 `perf record` 是否用了 sudo |
| `net_dev_start_xmit` 很少 | 数据包被合并了（TSO/GSO） | 正常的——网卡硬件卸载会合并小包 |
| 软中断事件全是 SCHED/RCU，没有 NET_RX | 没有网络流量 | 用 `ping` 或 `curl` 制造一些网络流量 |

---

## 下一步

网络中断分析告诉你"网络包在内核里经历了什么"。下一步 [实验 13：内核内存与泄漏定位](../13-kmem-leak/)——用 `kmem:*` tracepoint 追踪内核内存分配/释放，定位长期运行服务的内存持续上涨问题。
