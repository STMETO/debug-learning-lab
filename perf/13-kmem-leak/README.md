# 实验 13：内核内存与泄漏定位 —— 谁在吃内核内存？

> 🎯 **目标**：用 `kmem:*` tracepoint 追踪内核内存分配（slab/page）；区分用户态 `malloc` 与内核态内存开销；定位长时间运行服务的内存持续上涨根因。
>
> ⏱️ 约 22 分钟
>
> 📖 **前置**：[实验 07](../07-perf-trace/)（tracepoint）、[ASan 模块](../../asan/)（用户态内存泄漏概念）
>
> ⚠️ **环境**：`kmem:kmalloc` / `kmem:kfree` tracepoint ✅ WSL2。`perf kmem record` 的聚合分析需原生 Linux。

## 为什么需要内核内存分析？

用户态的内存泄漏（`malloc` 不 `free`）ASan/Valgrind 能检测。但内核态的内存问题呢？

```
你的程序 → malloc(100) → glibc 内存池 → 不够了 → brk/mmap → 内核分配虚拟内存

你的程序 → open("/tmp/f") → 内核分配：
  - struct file（文件对象）
  - struct dentry（目录项缓存）
  - struct inode（索引节点）
  - page cache pages（页缓存页）

这些内核对象在程序退出时会被回收，但如果是长期运行的服务：
  - 频繁打开/关闭文件 → dentry/inode 缓存增长
  - 频繁 fork → task_struct + 内核栈增长
  - 网络连接泄漏 → socket 内核对象不释放
```

> **`kmem:*` tracepoint 让你看到所有这些内核分配。**

---

## 先看示例程序

[kmem_demo.c](kmem_demo.c) 执行四种触发内核分配的操作：

| 操作 | 触发的内核分配 |
|------|-------------|
| 5000 次小 malloc/free（含泄漏） | brk/mmap + page 分配 |
| 20 次大 malloc（1-20MB） | 直接 mmap，触发 page 分配 |
| 10 次 fork + malloc + exit | task_struct, kernel stack, mm_struct |
| 10 万次 getpid + malloc/free | 高频 kmalloc/kfree（slab 分配器） |

```bash
make && ./kmem_demo
```

---

## 实验开始

### 第一步：观察内核内存分配事件

```bash
sudo perf record -e 'kmem:kmalloc,kmem:kfree' -o perf.data ./kmem_demo
sudo perf script -i perf.data | head -15
```

```
 kmem_demo 50407 [011] 811952.923711: kmem:kmalloc:
   call_site=perf_event_mmap+0x164 ptr=0xffff91ee15c0e000
   bytes_req=4096 bytes_alloc=4096 gfp_flags=GFP_KERNEL node=-1

 kmem_demo 50407 [011] 811952.923718: kmem:kfree:
   call_site=perf_event_mmap+0x28c ptr=0xffff91ee15c0e000
```

**解读 kmalloc 事件字段**：

| 字段 | 含义 | 示例值 |
|------|------|--------|
| `call_site` | 调用 kmalloc 的内核函数 | `perf_event_mmap+0x164` — perf 自己为了采样分配的内存！ |
| `ptr` | 分配的内核虚拟地址 | `0xffff91ee15c0e000` |
| `bytes_req` | 请求的字节数 | 4096（4KB） |
| `bytes_alloc` | 实际分配的字节数 | 4096（slab 可能向上取整到 2^n） |
| `gfp_flags` | 分配标志 | GFP_KERNEL = 普通内核分配（可睡眠） |

> `call_site` 是最重要的字段——它告诉你**内核中谁在分配内存**。如果 `call_site` 指向你的程序的系统调用处理函数，那就是你的程序触发的内核分配。

### 第二步：区分"谁分配了什么"

```bash
sudo perf script -i perf.data | grep kmem:kmalloc | \
    awk -F'call_site=' '{print $2}' | awk '{print $1}' | \
    sort | uniq -c | sort -rn | head -10
```

输出类似：

```
    230 perf_event_mmap+0x164
    180 __alloc_file+...
     50 copy_process+...
     30 sock_alloc_inode+...
     10 anon_vma_prepare+...
```

> `perf_event_mmap+0x164` 排第一——这是 perf 自己的采样缓冲区。排除 perf 自身开销，看剩下的 `call_site` 分布就知道程序的内核内存消耗模式。

### 第三步：找出"分配了但不释放"的模式

配对 kmalloc 和 kfree，找不会被释放的地址：

```bash
sudo perf script -i perf.data | awk '
/kmem:kmalloc/ {
    split($0, a, "ptr=")
    split(a[2], b, " ")
    alloc[b[1]] = $0
}
/kmem:kfree/ {
    split($0, a, "ptr=")
    split(a[2], b, " ")
    delete alloc[b[1]]
}
END {
    print "Unfreed allocations:"
    for (p in alloc) print alloc[p]
}' | head -20
```

> 这个脚本输出"分配了但从未 kfree"的内核地址——它们可能是：正在使用中的内存、一次性的永久分配、或者真正的泄漏。

### 第四步：用 page-fault 看用户态内存增长

内核分配用 `kmem:*`，用户态的内存增长用 `page-faults`：

```bash
perf stat -e page-faults -I 1000 ./kmem_demo
```

每秒的 page-fault 速率反映虚拟内存的增长速度——内存持续上涨的服务，page-fault 速率居高不下。

### 第五步：（原生 Linux）perf kmem

```bash
# 记录内核 slab 分配
sudo perf kmem record ./kmem_demo

# 查看 slab 分配统计
sudo perf kmem stat

# 查看调用栈
sudo perf kmem report
```

> ❌ **WSL2 不支持** `perf kmem record`（`kmem:kmalloc_node` tracepoint 不存在）。替代方案：使用 `kmem:kmalloc` 和 `kmem:kfree` tracepoint 手动采集和分析。

### 第六步：内存问题的报告模板

对于"程序内存持续上涨"的排查，可以生成这样一份报告：

```bash
#!/bin/bash
# 运行 30 秒，每秒采样一次内存相关指标
echo "timestamp,page_faults,kmallocs,kfrees"
for i in $(seq 1 30); do
    faults=$(perf stat -e page-faults -p $1 --timeout 1000 2>&1 | \
             grep page-faults | awk '{print $1}' | tr -d ',')
    echo "$(date +%s),$faults"
done
```

如果 `page-faults` 持续增长且不回落 → 用户态内存在持续分配
如果 `kmallocs` 持续大于 `kfrees` → 内核态内存在累积

---

## 📝 核心认知

### 内存泄漏的三个层次

```
第一层：用户态 malloc 不 free       → ASan / Valgrind 检测
第二层：用户态分配被全局容器持有      → 看起来"用了"，其实不需要
第三层：内核对象不释放               → kmem tracepoint 检测
  例：socket 不 close → socket 内核对象泄漏
  例：打开文件不 close → file/dentry/inode 泄漏
  例：频繁 fork 不 wait → 僵尸进程 + task_struct 残留
```

### `call_site` 的黄金价值

`kmem:kmalloc` 的 `call_site` 字段告诉你**内核中的调用者**。结合 `/proc/kallsyms`：

```bash
# 查找 call_site 对应的内核函数
sudo grep perf_event_mmap /proc/kallsyms
```

> 如果 `call_site` 指向你的程序名字的函数或某个系统调用（如 `__x64_sys_open`），你就知道是哪个系统调用触发了内核分配。

---

## 🧪 试试看

1. 把 `kmem_demo.c` 的 `LEAK_INTERVAL` 从 20 改成 5（多泄漏）——`kfree` 的数量减少了吗？`kmalloc - kfree` 的差值变大了吗？
2. 注释掉 `fork_and_alloc` 的 `waitpid`——僵尸进程会导致哪些内核对象无法释放？
3. 用 `perf stat -e page-faults -I 1000 ./kmem_demo` 观察每秒缺页速率——哪个阶段（small/large/fork/syscall）page-fault 最多？
4. （需要原生 Linux）在同一程序上用 `perf kmem stat`——看到的调用者分布和手动 `perf script` 分析一致吗？

---

## 命令小结

| 命令 | 作用 | WSL2 支持 |
|------|------|----------|
| `perf record -e 'kmem:kmalloc,kmem:kfree' ./prog` | 追踪内核分配/释放 | ✅ |
| `perf script -i perf.data \| grep kmalloc` | 分析 call_site 分布 | ✅ |
| `perf stat -e page-faults -I 1000 ./prog` | 每秒用户态内存增长 | ✅ |
| `perf kmem record ./prog` | 聚合 slab 分配统计 | ❌ |
| `perf kmem stat` | 查看 slab 分配汇总 | ❌ |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| kmem:kmalloc 事件全部是 perf 自己的 | perf record 本身分配采样缓冲区 | 正常——过滤掉 `call_site=perf_event_mmap` 即可 |
| `kmem:kmalloc_node` 不存在 | WSL2 内核未开启此 tracepoint | 用 `kmem:kmalloc` + `kmem:kfree` 替代 |
| 用户态 `malloc` 没触发 kmem:kmalloc | glibc 内存池命中，不需要向内核申请 | 大量分配或首次分配才会触发 brk/mmap → kmalloc |

---

## 下一步

🎉 **恭喜！你完成了 perf 模块的全部 13 个实验！**

回顾你的能力栈：

```
🟢 perf stat           → "程序慢在哪种类型？"（IPC / cache-miss / branch-miss）
🟡 perf record/report  → "哪个函数在吃 CPU？"
🟡 perf annotate       → "具体哪一行代码/哪条指令？"
🟠 FlameGraph          → "调用栈的 CPU 时间全貌（一张图）"
🟠 实战排查            → "综合工具链诊断真实程序"
🔴 perf trace          → "系统调用追踪（比 strace 更轻量）"
🔴 perf sched          → "调度延迟和 CPU 迁移分析"
🔴 L1 cache 事件       → "数据缓存命中率和伪共享"
🔴 perf probe          → "动态追踪任意函数（不改代码）"
🟣 block tracepoint    → "块设备 IO 延迟和缺页类型"
🟣 irq/net tracepoint  → "网络中断处理和包路径"
🟣 kmem tracepoint     → "内核内存分配/释放追踪"
```

> 📚 回到 [模块总览](../README.md) 查看完整的学习路线图和下一步建议。
