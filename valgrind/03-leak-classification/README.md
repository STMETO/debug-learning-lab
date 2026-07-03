# 实验 03：内存泄漏分类 —— 四级泄漏，比 LSAN 更细致

> 🎯 **目标**：理解 Valgrind 的四种泄漏分类——definitely lost / indirectly lost / possibly lost / still reachable——知道每种的含义和修复优先级。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 01](../01-hello-valgrind/)

## LSAN 只告诉你"漏了"，Valgrind 告诉你"怎么漏的"

```
ASan/LSAN:
  Direct leak of 400 bytes

Valgrind:
  definitely lost:    400 bytes   ← 确定泄漏，必须修
  indirectly lost:    200 bytes   ← 间接泄漏，修了确定的就好了
  possibly lost:      100 bytes   ← 可能泄漏，需要排查
  still reachable:     50 bytes   ← 程序结束时还有指针可达
```

---

## 先看示例程序

[leaks.c](leaks.c) 故意制造了四种不同类型的泄漏：

```bash
make
valgrind --leak-check=full --show-leak-kinds=all ./leaks
```

---

## 实验开始

### 四级泄漏全景

运行后 Valgrind 打印：

```
==289781== LEAK SUMMARY:
==289781==    definitely lost: 156 bytes in 3 blocks
==289781==    indirectly lost: 415 bytes in 2 blocks
==289781==      possibly lost: 100 bytes in 1 blocks
==289781==    still reachable: 256 bytes in 1 blocks
==289781==         suppressed: 0 bytes in 0 blocks
```

四级分类，逐级解读：

### 第一级：Definitely lost —— 🔴 必须修复

```
==289781== 40 bytes in 1 blocks are definitely lost in loss record 2 of 6
==289781==    at 0x4848899: malloc
==289781==    by 0x1091CD: definitely_lost (leaks.c:14)
==289781==    by 0x10934C: main (leaks.c:68)
```

**含义**：分配了内存，指向它的指针已丢失（出了作用域，被覆盖，或被释放了但结构体内部的指针没释放）。这块内存**再也无法被 `free` 了**。

**修复优先级**：🔴 最高——这是真正的 bug。

### 第二级：Indirectly lost —— 🟡 因关联而泄漏

```
==289781== 15 bytes in 1 blocks are indirectly lost
==289781==    at 0x4848899: malloc
==289781==    by 0x109239: indirectly_lost (leaks.c:31)   ← name 字段
```

**含义**：内存本身没有被直接丢弃——它被一个"本身也已泄漏"的结构体所引用。父结构体泄漏 → 子成员虽然"还有指针指向"，但那个指针在已泄漏的父结构体里。

**修复优先级**：🟡 不要直接修——先修父级的 `definitely lost`，这些间接泄漏会自动消失。

### 第三级：Possibly lost —— 🟠 需要排查

```
==289781== 100 bytes in 1 blocks are possibly lost
==289781==    at 0x4848899: malloc
==289781==    by 0x1092EB: possibly_lost (leaks.c:55)
```

**含义**：Valgrind 无法确定这块内存是否有指针指向。比如你只保留了指向分配区域**中间某个位置**的指针（`buf + 10`），Valgrind 不确定你是故意的还是丢失了原始指针。

**修复优先级**：🟠 检查代码——如果你确实在用"内部指针"访问，确认没有丢失原始指针。

### 第四级：Still reachable —— 🟢 通常可以忽略

```
==289781== 256 bytes in 1 blocks are still reachable
==289781==    at 0x4848899: malloc
==289781==    by 0x109277: still_reachable (leaks.c:44)
```

**含义**：程序退出时，还有指针指向这块内存（这里是全局变量 `global_ptr`）。程序结束前没有 `free`——但不是 bug，因为操作系统会在进程退出时回收所有内存。

**修复优先级**：🟢 最低——程序结束前自动回收。但在长时间运行的程序中（服务器），`still reachable` 累积可能变成真正的泄漏。

### 第五级：Suppressed —— ⚪ 被屏蔽

Valgrind 有一组默认的抑制规则，屏蔽了 glibc 等系统库的已知"泄漏"（其实是故意保留的缓存）。

---

## 📝 四种泄漏修复决策表

| 类型 | 含义 | 修复优先级 | 怎么修 |
|------|------|----------|--------|
| `definitely lost` | 指针丢失，内存再也无法 free | 🔴 必须修 | 找到对应的 `malloc`，确保有对应的 `free` |
| `indirectly lost` | 父结构体泄漏导致子成员也泄漏 | 🟡 先修父级 | 修复父级的 `definitely lost`，间接自动消失 |
| `possibly lost` | 可能丢失了原始指针 | 🟠 检查 | 确认内部指针的使用是否合理 |
| `still reachable` | 程序结束时还有指针指向 | 🟢 可忽略 | 长时间运行的程序需要关注 |
| `suppressed` | 被抑制规则屏蔽 | ⚪ 忽略 | 通常是系统库的已知问题 |

### 和 ASan/LSAN 的对比

| | LSAN | Valgrind |
|------|------|---------|
| 分类粒度 | Direct / Indirect | Definitely / Indirectly / Possibly / Reachable |
| "still reachable" | 不报（不算泄漏） | 可选显示 |
| 抑制文件 | 支持 | 支持（更成熟） |
| 性能 | 快（LSAN 是 ASan 的一部分） | 慢（~20x） |

---

## 🧪 试试看

1. 在 `indirectly_lost` 函数中，先 `free(c->name)` 再 `free(c->data)` 再 `free(c)`——四级泄漏各剩下多少？
2. 用 `--show-leak-kinds=definite` 只显示确定泄漏——`indirectly lost` 还会出现吗？
3. 写一个长时间运行的程序（`while(1)` 中慢慢泄漏），用 Valgrind 看 `still reachable` 会不会随时间稳定增长
4. 对比 `valgrind --leak-check=full ./leaks` 和用 ASan 编译的 `./leaks`——泄漏报告有什么不同？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| Valgrind 没说 `definitely lost` 但 LSAN 说有泄漏 | Valgrind 默认不显示 `reachable` | `--show-leak-kinds=all` |
| `indirectly lost` 修了父级还在 | 可能有多个父结构体泄漏 | 逐一检查所有 `definitely lost` |
| 报告中很多 `still reachable` 来自 glibc | glibc 的 `dlopen`/`getaddrinfo` 等有内部缓存 | 正常的——可以抑制 |

---

## 下一步

Leak classification lets you prioritize fixes. Next: [实验 04：Massif & Callgrind](../04-massif-callgrind/)——Valgrind 不止有 Memcheck！Massif 剖析堆内存使用模式，Callgrind 分析函数调用性能。
