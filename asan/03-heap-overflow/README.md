# 实验 03：堆缓冲区溢出 —— 最常见的内存 bug

> 🎯 **目标**：深入理解堆溢出，包括向右越界、向左越界、循环越界三种模式。
>
> ⏱️ 约 10 分钟
>
> 📖 **前置**：[实验 02](../02-read-report/)（你需要会读 ASan 报告的调用栈和内存布局）

## 什么是"堆溢出"？

你在堆上用 `malloc` 分配了 N 字节，但访问了第 N+1 个字节（或第 -1 个）。

```
malloc(20) → [合法的 20 字节] | 越界 ← ASan 红区
             arr[0] ... arr[4]   arr[5]
```

这是 C/C++ 项目中最常见的内存 bug，没有之一。

---

## 先看示例程序

[heap.c](heap.c) 有三种堆越界模式：

| 模式 | 操作 | 典型场景 |
|------|------|---------|
| 1 | `arr[5]` 写越界 | off-by-one 错误 |
| 2 | `arr[-1]` 写越界 | 负索引 |
| 3 | `for (i=0; i<=5; i++)` | 循环边界写错 |

```bash
make
./heap 1    # 向右越界
./heap 2    # 向左越界
./heap 3    # 循环越界
```

---

## 实验开始

### 模式 1：向右越界（off-by-one）—— `./heap 1`

```
=================================================================
==...==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x503000000054
WRITE of size 4 at 0x503000000054 thread T0
    #0 0x... in overflow_right heap.c:16
    #1 0x... in main heap.c:59
```

```
0x503000000054 is located 0 bytes to the right of 20-byte region [0x040,0x054)
allocated by thread T0 here:
    #0 ... in __interceptor_malloc
    #1 ... in overflow_right heap.c:11     ← malloc 在这里
```

解读：

| 字段 | 含义 |
|------|------|
| `heap-buffer-overflow` | 堆溢出——错误发生在 `malloc` 出来的内存上 |
| `overflow_right heap.c:16` | 越界代码：`arr[5] = 999;` |
| `20-byte region` | `5 * sizeof(int)` = 20 字节 |
| `0 bytes to the right` | 碰巧紧贴着右边界——off-by-one |
| `allocated ... heap.c:11` | `malloc` 调用位置 |

> `0 bytes to the right` 的意思是**刚好在合法区域最后一个字节之后的下一个字节**——即 `arr[5]` 的位置（arr[4] 占用第 17-20 字节，arr[5] 起始于第 21 字节 = 刚好在右边界）。

### 模式 2：向左越界 —— `./heap 2`

报告的关键差异在于这一行：

```
... is located 4 bytes to the left of 20-byte region ...
```

对比：

| | 模式 1 | 模式 2 |
|------|--------|--------|
| 代码 | `arr[5]` | `arr[-1]` |
| 位置描述 | `0 bytes to the right` | `4 bytes to the left` |
| 原因 | 写到了分配区域**之后** | 写到了分配区域**之前** |

> `4 bytes to the left` = arr[-1] 是一个 `int`（4 字节），位于合法区域开头的左边。

### 模式 3：循环越界 —— `./heap 3`

```c
for (int i = 0; i <= 5; i++) {   // BUG: 应该是 i < 5
    arr[i] = i * 100;              // i=5 时 arr[5] 越界
}
```

报告没什么新内容——和模式 1 一样是 `heap-buffer-overflow`。但这种 bug 尤其危险，因为：
- 肉眼扫代码很难发现 `<=` vs `<` 的差别
- 如果数组恰好分配在更大的连续内存块中，可能不立即崩溃
- 当你终于崩溃时，现场早已面目全非

> ASan 不管循环写了几次——只要有一次越界，它立刻停止。

---

## 堆溢出 vs 栈溢出（预告）

我们会在实验 04 详细对比。先记一个关键区别：

| | 堆溢出 | 栈溢出 |
|------|--------|--------|
| 报告类型 | `heap-buffer-overflow` | `stack-buffer-overflow` |
| 影子字节红区 | `fa`（堆红区） | `f1`/`f2`/`f3`（栈红区） |
| 报告中的分配信息 | `allocated by ... malloc` | `in stack of thread ... frame ...` |

---

## 📝 堆溢出诊断清单

拿到 `heap-buffer-overflow` 报告后：

```
1. 看 SUMMARY → 确认是 heap-buffer-overflow
2. 看 #0 栈帧 → 找到越界的那一行
3. 看 "X bytes to the right/left" → 确认越界方向和距离
4. 看 "allocated by" → 找到 malloc 的位置 → 对比分配大小和访问索引
5. 修复：扩大分配 或 修正索引 或 修正循环边界
```

---

## 🧪 试试看

1. 模式 1 中把 `malloc(5 * sizeof(int))` 改为 `malloc(6 * sizeof(int))`，ASan 还报吗？
2. 模式 2 的 `..to the left..` 为什么说 4 bytes 而不是 1 byte？
3. 模式 3 中 `i <= 5` 改成 `i < 6`，结果一样吗？ASan 怎么认为？
4. 同时用 `./heap 1` 和 `./heap 2`——两份 SUMMARY 行的函数名一样吗？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 报告说 `0 bytes to the right` 但代码里索引是对的 | 分配大小算错了 | 检查 `malloc(N * sizeof(T))`——单位是否正确 |
| 向左越界没有报错 | 越界落到另一块可访问内存 | 运气问题——ASan 有概率漏检，但比没 ASan 好得多 |
| 影子字节中出现 `04` 而不是 `fa` | 越界刚好踩到部分可访问的阴影字节 | 详见实验 02 的影子字节解读 |

---

## 下一步

堆上的 bug 会了，下一步处理栈上的：[实验 04：栈缓冲区溢出](../04-stack-overflow/)——局部数组越界，ASan 报告有什么不同？
