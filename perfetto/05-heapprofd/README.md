# 实验 05：内存分配分析 —— 用 heapprofd 追踪 malloc/free

> 🎯 **目标**：理解 `heapprofd` 的工作原理（LD_PRELOAD 拦截 malloc/free）；写出 heapprofd 的 trace config；识别内存泄漏热点；和 Valgrind Massif 对比——Perfetto 的优势是"不需要重启即可开启"。
>
> ⏱️ 约 22 分钟
>
> 📖 **前置**：[实验 01](../01-hello-perfetto/)（Perfetto 基础）、[Valgrind 模块](../../valgrind/)（内存分析概念）
>
> ⚠️ **环境**：heapprofd 需要原生 Linux（内核需支持 `memfd_create` + `/proc/pid/maps` 的正确权限）。**WSL2 中 heapprofd 进程连接正常但不产生数据**——可使用原生 Linux 虚拟机完成本实验。

## heapprofd 是什么？

`heapprofd` 是 Perfetto 的堆内存分析器。它通过 `LD_PRELOAD` 拦截 `malloc/free/realloc/calloc` 调用，记录每次分配的**大小、调用栈和时间戳**。

```
┌──────────────────────────────────────────────────────────┐
│  Valgrind Massif：模拟 CPU 执行，拦截所有内存操作          │
│  优点：不需要重编译，覆盖全面                              │
│  缺点：~20x 减速，需要重启程序                             │
├──────────────────────────────────────────────────────────┤
│  heapprofd：LD_PRELOAD 拦截 malloc 系列函数               │
│  优点：~2-5% 开销，可以在运行中开启（Android）             │
│  缺点：只追踪 libc 堆分配（不追踪 mmap、内核分配）         │
└──────────────────────────────────────────────────────────┘
```

---

## 先看示例程序

[heap_demo.c](heap_demo.c) 有两种不同的分配模式：

| 阶段 | 操作 | heapprofd 应该看到的 |
|------|------|-------------------|
| Phase 1 | 泄漏 10 × 1MB | 10 次 1MB 分配，0 次 free → 泄漏热点 |
| Phase 2 | 分配+释放 5 × 10MB | 5 次大块分配，5 次 free → 正常 |
| Phase 3 | 100 轮大小递增的 alloc/free | 高频小分配 → 分配热点 |
| Phase 4 | 再泄漏 5 × 1MB | 额外泄漏 → 总计 15MB 泄漏 |

```bash
make && ./heap_demo
```

---

## 实验开始

### 第一步：理解 heapprofd 的工作方式

heapprofd 不是独立的程序——它通过 `LD_PRELOAD` 注入到目标进程：

```
1. tracebox 启动（内嵌 traced + heapprofd producer）
2. config 中指定 target process（process_cmdline: "heap_demo"）
3. LD_PRELOAD=libheapprofd_glibc_preload.so ./heap_demo
4. 预加载库拦截 malloc/free → 通过 socket 发给 heapprofd
5. heapprofd 写入 traced 的共享内存缓冲区
6. 录制结束 → .perfetto-trace 包含所有分配记录
```

### 第二步：写 heapprofd config

```bash
cat > heap_config.txt << 'EOF'
buffers: {
    size_kb: 32768                  # 32MB 缓冲区（堆追踪数据量大）
    fill_policy: RING_BUFFER
}
data_sources: {
    config {
        name: "android.heapprofd"
        heapprofd_config {
            sampling_interval_bytes: 4096   # 每 4KB 采样一次
            shmem_size_bytes: 33554432      # 共享内存 32MB
            block_client: true              # 阻塞程序直到 heapprofd 就绪
            block_client_timeout_us: 30000000  # 30 秒超时
            process_cmdline: "heap_demo"    # 目标进程名
            heaps: "libc.malloc"            # 追踪 libc 堆
        }
    }
}
duration_ms: 8000
EOF
```

**关键参数解读**：

| 参数 | 含义 | 调优建议 |
|------|------|---------|
| `sampling_interval_bytes` | 每分配 N 字节采样一次 | 4096 = ~0.02% 开销；1 = 100% 采样（高开销） |
| `shmem_size_bytes` | 共享内存缓冲区 | 堆追踪数据量大——至少 8MB |
| `block_client` | 阻塞目标进程直到 heapprofd 就绪 | 确保不丢失启动阶段的分配 |
| `process_cmdline` | 匹配 `/proc/pid/cmdline` | 必须是进程的 argv[0] 的精确匹配 |
| `heaps` | 追踪哪些堆 | `"libc.malloc"` = 标准的 malloc/free |

### 第三步：录制 heap profile

```bash
# 在原生 Linux 中执行（WSL2 不支持）
sudo lib/perfetto/tracebox -c heap_config.txt --txt -o heap_trace.perfetto-trace &
LD_PRELOAD=lib/perfetto/libheapprofd_glibc_preload.so ./heap_demo
wait
```

期望输出：

```
[508.981] Configured tracing session 1, #sources:1, duration:8000 ms
[509.369] approfd_producer.cc:257  Connected to the service, mode [child].
Heap test: allocating and leaking...
Allocated 100MB, sleeping 2s for heapprofd to capture...
[512.180] approfd_producer.cc:353  Shutting down child heapprofd (status 0).
[513.991] Wrote XXX bytes into heap_trace.perfetto-trace
```

> ⚠️ **WSL2 限制**：heapprofd 在 WSL2 中可以连接（`Connected to the service`），但不产生堆分配数据。WSL2 内核缺少 heapprofd 需要的 `/proc/pid/maps` 权限和 `memfd_create` 支持。在原生 Linux（或标准 KVM 虚拟机）中可正常使用。

### 第四步：SQL 分析 heap profile

```bash
TP="lib/perfetto/trace_processor_shell heap_trace.perfetto-trace"
```

**查询 1：按调用栈看分配总量**

```sql
SELECT
    f.name as function_name,
    count(*) as allocations,
    sum(a.size) as total_bytes,
    sum(a.size) / count(*) as avg_size
FROM heap_profile_allocation a
JOIN heap_profile_callsite cs ON a.callsite_id = cs.id
JOIN heap_profile_frame f ON cs.frame_id = f.id
GROUP BY f.name
ORDER BY total_bytes DESC
LIMIT 10;
```

**查询 2：找出"分配了但未释放"的泄漏**

```sql
-- heapprofd 不直接区分"泄漏"和"仍在使用"，
-- 但你可以通过"在 trace 结束前未配对 free 的 malloc"来推测
SELECT
    a.size,
    f.name as allocator
FROM heap_profile_allocation a
LEFT JOIN heap_profile_allocation f_free
    ON a.id = f_free.id AND f_free.free_ts IS NOT NULL
JOIN heap_profile_callsite cs ON a.callsite_id = cs.id
JOIN heap_profile_frame f ON cs.frame_id = f.id
WHERE f_free.id IS NULL;  -- 没有配对的 free
```

**查询 3：分配大小分布**

```sql
SELECT
    CASE
        WHEN size <= 1024 THEN 'tiny (<=1KB)'
        WHEN size <= 65536 THEN 'small (1-64KB)'
        WHEN size <= 1048576 THEN 'medium (64KB-1MB)'
        ELSE 'large (>1MB)'
    END as size_category,
    count(*) as allocations
FROM heap_profile_allocation
GROUP BY size_category
ORDER BY allocations DESC;
```

### 第五步：heapprofd vs Valgrind Massif

| 维度 | heapprofd | Valgrind Massif |
|------|-----------|----------------|
| 原理 | LD_PRELOAD 拦截 malloc | CPU 模拟 + 内存拦截 |
| 开销 | ~2-5% | ~20x 减速 |
| 是否需要重启 | Linux 桌面：需要 LD_PRELOAD | 不需要（直接 `valgrind ./prog`） |
| 追踪范围 | libc malloc/free | 所有内存分配（包括 mmap、内核分配） |
| 输出格式 | .perfetto-trace（SQL + UI） | massif.out.*（ms_print） |
| 调用栈 | ✅ | ✅ |
| 时间线 | ✅（Perfetto UI） | ❌ |
| WSL2 支持 | ❌ | ✅ |

---

## 📝 核心认知

### heapprofd 的适用场景

```
✅ 适合：
  - 分析 malloc/free 的调用栈和分配模式
  - 在 Perfetto 时间轴上关联"内存分配"和"CPU 活动"
  - Android 应用中在线开启/关闭（不需要重启 app）

❌ 不适合：
  - WSL2（内核不支持）
  - 需要追踪 mmap/munmap（用 Valgrind Massif）
  - 不重启程序的场景（Linux 桌面仍需 LD_PRELOAD）
```

### heapprofd 的采样机制

heapprofd **不是 100% 追踪**每次 malloc——它按 `sampling_interval_bytes` 采样。例如设为 4096，意味着：
- 分配 4096 字节 → 可能被采样 1 次
- 分配 1GB → 被采样 ~250,000 次
- 分配 128 字节 → 可能完全不被采样

> 这种机制和 `perf record` 的采样类似——数据是统计推断，不是全量。减小 `sampling_interval_bytes` 提高精度，但增加开销。

---

## 🧪 试试看

1. （需要原生 Linux）把 `sampling_interval_bytes` 从 4096 改成 1024——采样数增加了吗？trace 文件变大了多少？
2. 在 Perfetto UI 中打开 heap trace，搜索 `heap_profile_allocation` 表——你能看到分配的时间线吗？
3. 把 `heap_demo.c` 中的 leak 注释掉（所有分配都 free），重新录制——"未释放"的分配数量归零了吗？
4. 对比本实验（heapprofd）和 [Valgrind 模块实验 04](../../valgrind/04-massif-callgrind/) 的 Massif 输出——两种工具对同一程序的"内存分配热点"判断一致吗？

---

## 命令小结

| 命令 | 作用 |
|------|------|
| `LD_PRELOAD=lib/perfetto/libheapprofd_glibc_preload.so ./prog` | 启动 heapprofd 追踪的程序 |
| `process_cmdline: "name"` | 指定目标进程（heapprofd 只追踪匹配的进程） |
| `sampling_interval_bytes: N` | 每 N 字节采样一次（越小 = 越精确 + 越慢） |
| `heaps: "libc.malloc"` | 只追踪 libc 的 malloc/free |

---

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| trace 文件 < 1KB，heap 表全为空 | WSL2 内核不支持 heapprofd | 切换到原生 Linux |
| `Child disconnected` 但无数据 | 进程在 heapprofd 就绪前就退出了 | 加 `sleep` 或增大 `block_client_timeout_us` |
| `process_cmdline` 不匹配 | argv[0] 和 config 中写的不一致 | 用 `ps aux` 确认进程名，或用 `cat /proc/PID/cmdline` |

---

## 下一步

heapprofd 让你追踪用户态的内存分配。如果想在程序中**自定义追踪点**（类似应用级日志，但在 Perfetto 时间轴上可见）——下一步 [实验 06：应用层自定义追踪](../06-custom-tracing/)。
