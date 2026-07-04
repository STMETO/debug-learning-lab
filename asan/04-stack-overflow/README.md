# 实验 04：栈缓冲区溢出 —— 局部数组越界

> 🎯 **目标**：理解栈溢出的 ASan 报告，学会区分堆溢出和栈溢出。
>
> ⏱️ 约 10 分钟
>
> 📖 **前置**：[实验 03](../03-heap-overflow/)（你需要知道堆溢出的报告长什么样）

## 堆 vs 栈 —— 在 ASan 眼里完全不同

```
堆溢出：malloc 出来的区域 → 周围是 ASan 插入的"堆红区" (fa)
栈溢出：局部变量数组      → 周围是编译器的"栈红区" (f1/f2/f3)
```

两者的 ASan 报告用**不同的关键词**、**不同的内存布局描述**、**不同的影子字节**。

---

## 先看示例程序

[stack.c](stack.c) 有三种栈越界场景：

| 模式 | 操作 | 典型场景 |
|------|------|---------|
| 1 | `strcpy` 写入超长字符串 | 不安全的 C 字符串操作 |
| 2 | 循环中 `scores[5]` 越界 | 边界条件错误 |
| 3 | `memcpy` 拷贝过多字节 | 错误的 sizeof 计算 → 意外重叠 |

```bash
make
./stack 1    # strcpy 溢出 → stack-buffer-overflow
./stack 2    # 循环越界 → stack-buffer-overflow
./stack 3    # memcpy 重叠 → memcpy-param-overlap

---

## 实验开始

### 模式 1：strcpy 溢出 —— `./stack 1`

```
=================================================================
==...==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffe954fc57a
WRITE of size 43 at 0x7ffe954fc57a thread T0
    #0 ... in __interceptor_memcpy ...
    #1 ... in overflow_fixed stack.c:16        ← strcpy 调用行
    #2 ... in main stack.c:57
```

**关键区别一**：错误类型是 `stack-buffer-overflow` 而不是 `heap-buffer-overflow`。

**关键区别二**：内存位置描述完全不同：

```
Address 0x7ffe954fc57a is located in stack of thread T0 at offset 42 in frame
    #0 ... in overflow_fixed stack.c:8

  This frame has 1 object(s):
    [32, 42) 'buf' (line 11) <== Memory access at offset 42 overflows this variable
```

逐一解读：

| 信息 | 含义 |
|------|------|
| `in stack of thread T0` | 内存在栈上（不是堆！） |
| `at offset 42` | 越界位置距离栈帧起始 42 字节 |
| `This frame has 1 object(s)` | 这个函数里有 1 个局部变量 |
| `[32, 42) 'buf'` | `buf` 数组占 32~41 字节位置（10 字节） |
| `Memory access at offset 42` | 越界发生在偏移 42 —— **刚好是 buf 结束的下一个字节** |

> 栈溢出报告会**精确告诉你**是哪个局部变量被越界了——`buf`（第 11 行定义），大小 `[32, 42)` 即 10 字节。

### 和实验 03 堆溢出报告的三处对比

| | 堆溢出（实验 03） | 栈溢出（本实验） |
|------|-----------------|-----------------|
| 错误类型 | `heap-buffer-overflow` | `stack-buffer-overflow` |
| 内存描述 | `X bytes to the right of N-byte region` | `at offset 42 in frame ... object 'buf' [32,42)` |
| 分配信息 | `allocated by ... malloc ...` | 直接列出**帧内所有对象** |
| 影子字节红区 | `fa`（堆红区） | `f1`/`f2`/`f3`（栈红区） |

> 💡 **记忆法**：看错误类型第一个单词——`heap` 还是 `stack`。这是最快区分两类报告的方法。

### 模式 2：循环栈越界 —— `./stack 2`

```c
int scores[5] = {0};     // 栈上的 int 数组
for (int i = 0; i <= 5; i++) {
    scores[i] = i * 10 + 5;   // i=5 时访问 scores[5] → 越界
}
```

和堆溢出的循环模式（实验 03 模式 3）几乎一模一样的问题，但 ASan 报的是 `stack-buffer-overflow`：

```
This frame has 1 object(s):
  [32, 52) 'scores' (line 35) <== Memory access at offset 52 overflows this variable
```

`scores` 是 5 个 `int` = 20 字节，偏移 52 = 32 + 20，所以越界确实在 `scores` 结束后。

### 模式 3：memcpy-param-overlap —— `./stack 3`

```c
int dest[3];                        // 3 个 int = 12 字节，栈偏移 [48, 60)
int src[10] = {1,2,3,4,5,6,7,8,9,10}; // 10 个 int = 40 字节，栈偏移 [80, 120)

memcpy(dest, src, 10 * sizeof(int)); // 拷贝 40 字节到只能装 12 字节的 dest
```

实际运行报告：

```
=================================================================
==...==ERROR: AddressSanitizer: memcpy-param-overlap: memory ranges
  [0x...00,0x...28) and [0x...20,0x...48) overlap
```

**为什么不是 `stack-buffer-overflow`？** ASan 有多个检查器，`memcpy-param-overlap` 的检查优先级更高：

```
memcpy 写入范围: [48, 88)   ← 从 dest 开始写 40 字节
memcpy 读取范围: [80, 120)  ← 从 src 开始读 40 字节
                       ↑
               [80, 88) 重叠！← ASan 先检测到这个，报 memcpy-param-overlap
```

> 如果 `dest` 和 `src` 在内存中相距足够远（不重叠），ASan 会继续运行，然后在写越界处报 `stack-buffer-overflow`。

报告中的栈帧信息验证了这一点：

```
This frame has 2 object(s):
  [48, 60) 'dest'  ← dest 只有 12 字节，写 40 字节必然越界
  [80, 120) 'src'  ← src 的读范围被写入覆盖了
```

修复：只要把 `dest` 扩到足够大——`int dest[10]`——或者把 `memcpy` 的第三个参数改为 `sizeof(dest)`。

---

## 📝 栈溢出诊断清单

```
1. 看 SUMMARY → 确认是 stack-buffer-overflow
2. 看 "This frame has N object(s)" → 找到被越界的局部变量名
3. 看 "[start, end) 'var_name'" → 变量的合法范围
4. 看 "Memory access at offset ... overflows this variable"
   → 确认越界发生在哪个变量
5. 修复：扩大数组 或 修正循环边界 或 使用安全函数（strncpy 等）
```

---

## 🧪 试试看

1. `strcpy` 用什么替代更安全？（提示：`strncpy`——但注意它的一个陷阱）
2. 栈溢出的影子字节图里能找 `fa` 吗？（提示：`fa` = 堆红区，栈红区用 `f1`/`f2`/`f3`）
3. 同一个程序中既可能有堆溢出也可能有栈溢出——ASan 能同时检测吗？
4. 为什么栈溢出报告能说出变量名（`'buf'`），而堆溢出报告只说"20-byte region"？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 栈溢出报告显示 `__interceptor_memcpy` 而不是你自己的函数 | `strcpy` 内部被编译器优化为 `memcpy` | 正常——看 `#1` 而不是 `#0` 就能找到你的代码 |
| 报告说 `object 'buf'` 但名字和代码中不一致 | 编译器优化可能合并变量 | 检查行号，不看变量名 |
| 栈溢出更难复现 | 两次运行之间栈布局可能变化 | ASan 的栈红区保证每次都能检测，不会漏 |

---

## 下一步

堆和栈的越界都学会了。下一步是更棘手的 bug：[实验 05：释放后使用](../05-use-after-free/)——内存已经 `free` 了但你还拿着指针在用，ASan 怎么抓到它？
