# 实验 08：三工具全景对比 —— 什么时候用 perf？什么时候用 ftrace？什么时候用 Perfetto？

> 🎯 **目标**：建立 perf vs ftrace vs Perfetto 的综合对比矩阵；给出 5 个真实场景，选择正确的工具组合并说明理由。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：perf 模块（01-13）、ftrace 模块（01-10）、Perfetto 模块（01-07）

---

## 三工具全景矩阵

| 维度 | perf | ftrace | Perfetto |
|------|------|--------|---------|
| **数据来源** | CPU PMU 硬件计数器 | 内核函数追踪 + tracepoint | ftrace + perf + heapprofd + 应用 SDK |
| **采集方式** | 统计采样（每 N 个周期一拍） | 全量追踪（每次调用都记录） | 全量 + 采样（混合） |
| **CPU 开销** | ★☆☆☆☆ (~2%) | ★★★★☆ (~20-50%) | ★★☆☆☆ (~5-30%，取决于配置) |
| **覆盖范围** | 用户态 + 内核态 | 内核态为主 | 全栈：用户态 + 内核态 + 应用埋点 |
| **分析方式** | CLI/TUI (`perf report`) | 文本 (`cat trace` + `grep`) | SQL + Web UI (`ui.perfetto.dev`) |
| **时间线可视化** | ❌ | ❌ | ✅（核心功能） |
| **多数据源融合** | ❌ | ❌ | ✅（核心功能） |
| **100% 函数覆盖** | ❌（统计误差 ~5%） | ✅（不遗漏任何调用） | 取决于数据源 |
| **微秒级延迟** | ❌（看不到单次耗时） | ✅（function_graph） | ✅（如果启用 function_graph） |
| **硬件计数器** | ✅（IPC/cache-miss/...） | ❌ | ✅（通过 linux.perf） |
| **内存分析** | ❌ | ❌ | ⚠️（heapprofd，需原生 Linux） |
| **动态探针** | ✅（perf probe） | ❌ | ❌（但可通过 ftrace 间接使用） |
| **生产环境可用** | ✅（低开销） | ⚠️（高开销，临时使用） | ⚠️（取决于配置和数据源） |
| **学习曲线** | 中等 | 陡峭（文件接口） | 平缓（SQL + 浏览器） |

---

## 场景决策树

### 场景 1："新上线的服务 CPU 突然飙到 100%"

```
首选：perf record -g → 火焰图
原因：极低开销（~2%），能在生产环境跑，快速定位热点函数
备选：Perfetto linux.perf（如果需要同时看 ftrace 事件）
不适合：ftrace（开销太大，可能把服务器搞崩）
```

### 场景 2："本地测试时 open() 有时 1ms 有时 100ms，不知道为什么"

```
首选：ftrace function_graph + trace-cmd
原因：需要看每次调用的精确耗时，perf 采样看不到单次耗时分布
备选：Perfetto linux.ftrace（如果需要 UI 可视化）
不适合：perf record（只能看统计，"偶发 100ms"可能被平均掉）
```

### 场景 3："怀疑有内存泄漏，但程序不能重启"

```
首选：Valgrind Massif（不需要重编译）+ 分析 massif 输出
备选：heapprofd（如果程序启动时能加 LD_PRELOAD）
不适合：perf, ftrace（不追踪 malloc/free）
```

### 场景 4："用户反馈'操作偶尔卡一下'，复现概率 ~1%"

```
首选：Perfetto（ftrace 事件 + CPU 采样 + 应用埋点，全栈时间轴）
原因：1% 的偶发问题 → 需要长时间录制 → 需要 UI 在海量数据中定位异常时间点
备选：ftrace trace-cmd record（如果只关注内核路径）
不适合：perf record（统计采样可能漏掉偶发事件）
```

### 场景 5："想知道新算法比旧算法快在哪里"

```
首选：perf stat（IPC, cache-miss%, branch-miss%）
原因：计数器比值直接告诉你"为什么快"——IPC 提高？cache-miss 降低？
备选：perf record + perf annotate（钻到源码/指令级看热点变化）
不适合：ftrace（函数调用次数 ≠ CPU 效率）
```

---

## 工具组合推荐

```
简单 CPU 热点           → perf record + flamegraph           （最轻量）
精准内核延迟            → ftrace function_graph + trace-cmd  （最精确）
复杂全栈关联            → Perfetto ftrace + perf + 应用埋点   （最全面）
计数器对比              → perf stat                           （零开销）
内存泄漏                → Valgrind / heapprofd                （专有工具）
不重编译分析            → Valgrind / perf trace               （无需重编译）
动态追踪任意函数         → perf probe / ftrace trace_marker    （零侵入）
```

---

## 📝 终极总结

### 一句话选型

> **"不知道哪里慢" → perf。**
> **"知道哪个函数慢，但不知道为什么" → ftrace。**
> **"问题跨层次（应用+内核+CPU），需要关联分析" → Perfetto。**

### 你在三个模块中学到的

```
perf 模块 (13 实验)：
  perf stat → perf record → perf report → perf annotate → FlameGraph
  → "从宏观 IPC 到微观指令，完整的 CPU 性能分析链"

ftrace 模块 (10 实验)：
  tracefs → function tracer → function_graph → trace-cmd → 事件+滤波
  → "从文件接口到工具封装，内核函数调用的全量追踪"

Perfetto 模块 (8 实验)：
  tracebox → linux.ftrace → linux.perf → heapprofd → SQL → 全栈实战
  → "多数据源融合 + SQL 分析 + 浏览器 UI，系统级的统一追踪平台"
```

---

## 🎉 模块完结

```
perfetto/
├── README.md                      # 8 实验规划
├── 01-hello-perfetto/     🟢 ✅   # tracebox、config、SQL 统计、UI
├── 02-ftrace-integration/ 🟢 ✅   # linux.ftrace、事件频率、与原生 ftrace 对比
├── 03-trace-processor/    🟡 ✅   # SQL 核心表、跨事件关联、时间分桶
├── 04-cpu-profiling/      🟡 ✅   # linux.perf、采样频率、与 perf record 对比
├── 05-heapprofd/          🟡 ✅   # LD_PRELOAD、采样间隔、与 Massif 对比
├── 06-custom-tracing/     🟠 ✅   # trace_marker B/E 协议、三层次追踪
├── 07-real-world/         🟠 ✅   # 三数据源联合诊断、全栈排查方法
└── 08-compare-tools/      🟠 ✅   # 全景对比矩阵、5 场景决策树
```

> 📚 返回 [项目首页](../../README.md) 查看所有模块。
