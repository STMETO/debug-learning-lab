# 实验 11：块设备 IO 性能分析 —— 磁盘在忙什么？

> 🎯 **目标**：用 `block:*` tracepoint 追踪块 IO 延迟；区分主/次缺页中断；排查存储慢查询的完整链路。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 02](../02-perf-stat/)（计数器）、[实验 07](../07-perf-trace/)（tracepoint 概念）
>
> ⚠️ **环境**：block tracepoint ✅ WSL2。`blktrace` 和 `iostat` 的扩展分析需原生 Linux。

## 块 IO 在系统中处于什么位置？

```
应用程序 → write() → VFS → 文件系统 → 块层(bio) → IO 调度器 → 磁盘驱动 → 磁盘
                                                                    ↑
                                              block tracepoint 在这里追踪
```

当程序调用 `write()` 时，数据不是立即写到磁盘的——内核先写入页缓存（page cache），标记为"脏页"，稍后由后台线程刷到磁盘。只有 `fsync()` 或页缓存压力足够大时，才真正触发块层 IO。

> **关键认知**：`write()` 返回 ≠ 数据已经落盘。`fsync()` 返回 ≈ 数据已经落盘。

---

## 先看示例程序

[blockio.c](blockio.c) 执行三种磁盘相关的操作：

| 操作 | 系统调用 | 触发的块层事件 |
|------|---------|-------------|
| 写 50MB + fsync | `write()` + `fsync()` | block_bio_queue → block_io_done |
| mmap + 触发缺页 | `mmap()` + 页面访问 | minor-faults / major-faults |
| 顺序读 vs 随机读 | `pread()` | block_bio_queue（数量和延迟不同） |

```bash
make && ./blockio
```

---

## 实验开始

### 第一步：用 page-fault 计数器看缺页类型

```bash
perf stat -e page-faults,minor-faults,major-faults ./blockio
```

```
              1804      page-faults      # 总缺页 1804 次
              1401      minor-faults     # 次缺页 1401 次（数据已在内存）
               403      major-faults     # 主缺页 403 次（需要读磁盘！）
```

| 类型 | 含义 | 性能影响 |
|------|------|---------|
| **minor fault** | 页已在内存（页缓存/共享库），只需更新页表 | 极快（~1μs） |
| **major fault** | 页不在内存，必须从磁盘读取 | 慢（~10ms，取决于磁盘） |

> 403 次 major fault = 如果每次 10ms → 总共 4 秒。如果程序频繁触发 major fault（比如内存映射了一个大文件但物理内存不够），性能会急剧下降。

**诊断信号**：如果 major-faults ÷ page-faults > 10%，说明磁盘 IO 是瓶颈——需要增加内存或减少工作集。

### 第二步：追踪块层 BIO 事件

```bash
sudo perf record -e 'block:block_bio_queue,block:block_io_done' -o perf.data ./blockio
sudo perf script -i perf.data | head -15
```

```
 blockio 50503 [002] 812012.074106:  block:block_bio_queue: 8,80 RA 55839464 + 24 [blockio]
 blockio 50503 [002] 812012.193168:  block:block_bio_queue: 8,80 WS 64159744 + 2192 [blockio]
 blockio 50503 [002] 812012.193527:  block:block_bio_queue: 8,80 WS 64161936 + 2208 [blockio]
```

**解读输出**：

| 字段 | 含义 | 示例值 |
|------|------|--------|
| `8,80` | 设备号（major,minor） | 8,80 = /dev/sda 的某个分区 |
| `RA` / `WS` | 操作类型 | RA=ReadAhead（预读），WS=WriteSync（同步写） |
| `55839464` | 扇区号（sector） | 磁盘上的起始位置 |
| `+ 24` | 扇区数 | 24 × 512 字节 = 12KB |
| `[blockio]` | 触发进程名 | 我们的程序 |

**关键发现**：
- `RA`（预读）→ 内核在主动预取数据，说明检测到了顺序访问模式
- `WS`（同步写）→ 来自 `fsync()`，数据正在写入磁盘
- 操作类型告诉你 IO 的"性质"——顺序还是随机、同步还是异步

### 第三步：测量块 IO 延迟

```bash
sudo perf record -e 'block:block_bio_queue,block:block_io_done' -o perf.data ./blockio
sudo perf script -i perf.data | awk '
/block_bio_queue/ {start[$5]=$4}
/block_io_done/   {if(start[$5]) {print $4-start[$5], $0; delete start[$5]}}'
```

> 这个 awk 脚本配对每个 block_bio_queue 和对应的 block_io_done，计算两者的时间差 = 块层延迟。在你的 WSL2 环境中，延迟通常在 0.x ms（虚拟磁盘），但原生 Linux 中可能是 5-20ms（HDD）或 0.1-1ms（SSD）。

### 第四步：区分"内存不够"还是"磁盘慢"

用 `perf stat` 的两个指标快速判断：

```bash
perf stat -e page-faults,major-faults -I 1000 ./blockio
```

| 现象 | 诊断 | 下一步 |
|------|------|--------|
| major-faults / sec > 100 | 内存不够，大量缺页从磁盘读 | 增加物理内存，或减少 mmap 大小 |
| major-faults 很少但程序还是慢 | 磁盘本身慢（HDD 随机访问） | 用 SSD，或优化 IO 模式为顺序访问 |
| block_bio_queue 数量极大 | 程序在做大量小 IO | 合并 IO（用更大的 buffer，或 `setvbuf`） |

### 第五步：（原生 Linux）blktrace 和 iostat

在原生 Linux 中，还有两个互补工具：

```bash
# blktrace：比 block tracepoint 更底层的块层追踪
sudo blktrace -d /dev/sda -o - | blkparse -i -

# iostat：块设备级别的统计
iostat -x 1
```

> ❌ WSL2 中 `blktrace` 不可用（需要 `blktrace` 内核模块）。`block:*` tracepoint 是 WSL2 中块 IO 分析的主要工具。

---

## 📝 核心认知

### 块 IO 延迟 = 排队 + 寻道 + 传输

```
一次磁盘 IO 的总延迟 ≈
  排队时间（IO 调度器队列中等待）
  + 寻道时间（磁头移动到正确轨道，HDD: ~5-10ms, SSD: ~0ms）
  + 传输时间（实际读写数据，取决于带宽）
```

`perf` 的 block tracepoint 测量的是**从 bio 入队到完成**的端到端延迟。

### page-fault 的完整分类

```
page-faults = minor-faults + major-faults

minor-fault: 页面在物理内存中，只需更新页表
  例：访问 mmap 的文件，数据已在 page cache

major-fault: 页面不在内存，必须从磁盘读取
  例：mmap 一个从未访问过的文件区域
  例：swap-in（被换出到 swap 的匿名页）
```

---

## 🧪 试试看

1. 把 `blockio.c` 中 `FILE_SIZE` 从 50MB 改成 500MB——major-faults 数量变化了吗？block_bio_queue 事件多了多少？
2. 注释掉 `system("sync; echo 1 > ... /drop_caches")` 那行——第二次 mmap 的 major-faults 数量有什么变化？（提示：数据还在 page cache，不会触发 major fault）
3. 把 `direct_write_test` 中的 `CHUNK_SIZE` 从 4096 改成 64——block_bio_queue 事件数量变了吗？为什么小 IO 更糟糕？
4. （需要原生 Linux）对比 HDD 和 SSD 上的 `block_io_done - block_bio_queue` 延迟差异。

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `perf stat -e page-faults,minor-faults,major-faults ./prog` | 查看缺页类型分布 |
| `perf record -e 'block:block_bio_queue,block:block_io_done' ./prog` | 追踪块层 BIO 入队和完成 |
| `perf script -i perf.data` | 查看块 IO 事件的时间线 |
| `perf stat -e page-faults -I 1000 ./prog` | 每秒报告缺页频率 |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| block tracepoint 没有事件 | 程序 IO 太小，全被 page cache 吸收了 | 加大文件（> 内存），或加 `fsync` 强制刷盘 |
| major-faults = 0 | 数据全部在 page cache 中 | 先 `echo 1 > /proc/sys/vm/drop_caches` 清缓存再跑 |
| `block_bio_queue` 字段看不懂 | 输出中的数值是扇区号/扇区数 | 扇区 × 512 = 字节 |

---

## 下一步

块设备分析告诉你"磁盘在忙什么"。下一步 [实验 12：网络中断与软中断](../12-network-irq/)——分析网络 IO 的中断处理和软中断延迟。
