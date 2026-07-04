# 实验 02：读懂 ASan 报告 —— 每个数字和字符的含义

> 🎯 **目标**：深入理解 ASan 报告的每一部分：多层调用栈、影子字节图、内存布局。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 01](../01-hello-asan/)（你需要看过基础的 ASan 报告）

---

## 先看示例程序

[report.c](report.c) 有三层函数调用，bug 藏得很深：

```
main() → prepare_data() → fill_buffer()
                               ↑
                          buf[5]~buf[7] 越界写入
```

- `main` 分配了 5 个 `int`（20 字节）
- `prepare_data` 传 `count=8` 给下层
- `fill_buffer` 写了 8 个 `int` → 后 3 个越界

```bash
make && ./report
```

---

## 实验开始

运行后你会看到一份完整的 ASan 报告。我们这次逐段**深度解读**。

### 段一：错误类型和访问信息

```
=================================================================
==179074==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x503000000054 ...
WRITE of size 4 at 0x503000000054 thread T0
```

| 字段 | 值 | 含义 |
|------|-----|------|
| `ERROR: ... heap-buffer-overflow` | 堆缓冲区溢出 | 堆上（`malloc` 出来的）内存被越界访问 |
| `WRITE of size 4` | 写入 4 字节 | `int` 占 4 字节，这是一次**写入**越界 |
| `at 0x503000000054` | 具体地址 | 访问的目标地址 |
| `thread T0` | 主线程 | T0 = 第一个线程 |

> **常见错误类型速查**：
>
> | 错误类型 | 含义 |
> |---------|------|
> | `heap-buffer-overflow` | 堆越界（本次） |
> | `stack-buffer-overflow` | 栈越界（局部数组） |
> | `heap-use-after-free` | 释放后使用 |
> | `double-free` | 重复释放 |
> | `global-buffer-overflow` | 全局变量越界 |
> | `stack-use-after-return` | 使用了已返回函数的局部变量 |

### 段二：调用栈 —— 最重要的部分

```
    #0 0x... in fill_buffer report.c:12
    #1 0x... in prepare_data report.c:19
    #2 0x... in main report.c:32
    #3 0x... in __libc_start_call_main ...
```

这棵"调用树"从**下往上**读（和 GDB 的 `backtrace` 一样：`#0` 是出错的瞬间，数字越大离越远）：

```
调用顺序（从上往下）：
  main()           #2    ← 程序入口
    └─ prepare_data()  #1  ← 中间层
         └─ fill_buffer()  #0  ← 越界发生在这里
```

**读调用栈查 bug 的方法**：从 `#0` 往上看，找到**第一个你写的代码**（跳过 `libc_start_call_main` 等系统帧）。

> 本报告中：
> - `#0` = `fill_buffer` 第 12 行 — **越界的精确位置**
> - `#1` = `prepare_data` 第 19 行 — 传入了 `count=8`
> - `#2` = `main` 第 32 行 — 分配了 `5 * sizeof(int)` = 20 字节
>
> 一眼就能看出：**main 分配了 20 字节，但 prepare_data 要求写 8 个 int（32 字节），fill_buffer 在第 12 行越界了。**

这套"#0 → #1 → #2 追溯链"是定位 bug 的核心路径。

### 段三：内存布局 —— "离哪块内存多远"

```
0x503000000054 is located 0 bytes to the right of 20-byte region [0x503000000040,0x503000000054)
```

翻译：

| 信息 | 含义 |
|------|------|
| `20-byte region` | 你分配了 20 字节（5 个 int × 4） |
| `[0x...040, 0x...054)` | 合法范围：从 `040` 到 `053`（`054` 不包含——区间是左闭右开） |
| `0 bytes to the right` | 越界地址 `054` 紧贴着合法区域的右边界 |

> 数学上：合法区间 `[0x040, 0x054)` = 20 字节。越界地址 `0x054` = 合法区间结束后**紧挨着**的第一个字节。

### 段四："主犯"和"从犯"——分配点回溯

```
allocated by thread T0 here:
    #0 0x... in __interceptor_malloc ...
    #1 0x... in main report.c:27
```

这告诉你：**这块内存是在哪里分配的**（和第 27 行的 `malloc` 对应）。

把"错误点"和"分配点"并排看：

```
错误点（WRITE）：        分配点（allocated）：
#0 fill_buffer:12       #0 __interceptor_malloc
#1 prepare_data:19      #1 main:27
#2 main:32
```

> 💡 **分配点-错误点对照法**：ASan 至少给出了两个关键行号——"谁分配了这块内存"和"谁越界访问了它"。多数 bug 看一下这两行，原因就清楚了。

### 段五：SUMMARY 行

```
SUMMARY: AddressSanitizer: heap-buffer-overflow report.c:12 in fill_buffer
```

> 错误类型 + 文件:行号 + 函数名。**看完报告先找 SUMMARY**——它是最精简的一句总结。

---

## 🔬 深度解读：影子字节图

这是实验 01 跳过的部分，现在是时候了。

```
Shadow bytes around the buggy address:
  0x0a067fff7ff0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  ...
=>0x0a067fff8000: fa fa 00 00 00 fa fa fa 00 00[04]fa fa fa fa fa
  ...
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  ...
```

### 影子内存是如何工作的？

ASan 把程序的**每 8 个字节**映射为 1 个**影子字节**（shadow byte）：

```
真实内存：    [0][1][2][3][4][5][6][7] [8][9]...
               ↓                      ↓
影子内存：    1 个 byte 描述这 8 个     1 个 byte...
```

影子字节的值 = 这 8 个字节中**有**几个可以访问：

| 影子字节值 | 含义 |
|-----------|------|
| `00` | 8 个字节**全部**可访问 |
| `01` ~ `07` | **部分**可访问（数字 = 可访问的字节数）|
| `fa` | 堆红区——不可访问 |
| `fd` | 已释放的堆内存——不可访问 |
| `f1`/`f2`/`f3` | 栈红区——不可访问 |

### 解读本报告的影子字节

```
=>0x0a067fff8000: fa fa 00 00 00 fa fa fa 00 00[04]fa fa fa fa fa
                              ← 左红区 → ← 我们的 → ← 右红区 → ← 别的 →
```

逐字节翻译：

| 影子字节 | 值 | 代表什么 |
|----------|-----|---------|
| `fa fa` | 左红区 | 我们分配区域**之前**的隔离带（不可访问） |
| `00 00` | 完全可访问 | 我们的数据——前 16 字节（`data[0]`~`data[3]`） |
| `00` | 完全可访问 | 又 8 字节 — 等等，我们只分配了 20 字节... |

仔细观察 `[04]`：

> `04` 被 `[ ]` 括起来——这就是**越界踩到的那个影子字节**。`04` 表示这 8 字节中只有前 4 个是可访问的（即 `data[4]`，共 4 字节），后 4 个字节（`data[5]` 占用的空间）是不可访问的。

所以实际布局是：

```
malloc(20 字节) → 实际在影子内存中占 2.5 个影子字节
 fa fa  │  00  │  00  │  04  │  fa fa fa
 红区        16字节     4字节 ←越界踩到这里
        └── 20 字节合法区域 ──┘
```

> `fill_buffer` 尝试写 `buf[5]`（即第 21~24 字节），踩进了 `04` 影子字节的"不可访问"后半部分。

### 常用影子字节速记

你不需要记住整个 legend。90% 的情况下只需要认这四个：

| 影子字节 | 场景 |
|---------|------|
| `00` | 安全，正常访问 |
| `fa` | 堆红区，越界踩到这里 → 报 `heap-buffer-overflow` |
| `fd` | 已 `free`，再用 → 报 `heap-use-after-free` |
| `f1`/`f2`/`f3` | 栈红区，局部数组越界 → 报 `stack-buffer-overflow` |

---

## 🛠️ 控制 ASan 输出：ASAN_OPTIONS

你可以通过环境变量定制 ASan 的行为：

```bash
# 检测到第一个错误后不终止（继续检测后续错误）
ASAN_OPTIONS=halt_on_error=0 ./report

# 报告后生成 core dump（配合 GDB 使用，实验 10 会深入）
ASAN_OPTIONS=abort_on_error=1 ./report

# 只检测泄漏，不检测越界（加速运行）
ASAN_OPTIONS=detect_leaks=1:detect_stack_use_after_return=0 ./report

# 查看所有可用选项
ASAN_OPTIONS=help=1 ./report
```

> 多个选项用 `:` 分隔：`ASAN_OPTIONS=option1=val1:option2=val2`

---

## 📝 读报告的黄金流程

```
1. SUMMARY 行        → 错误类型 + 位置（10 秒定位）
2. #0 栈帧           → 精确到行号（30 秒找到出错代码）
3. "X bytes to..."   → 越了多少（理解严重程度）
4. "allocated by"    → 在哪里分配的（找根因）
5. 影子字节图         → 确认红区类型（可选，疑难杂症时用）
```

---

## 🧪 试试看

1. 把 `fill_buffer(buf, 8)` 改为 `fill_buffer(buf, 5)`——ASan 还报错吗？
2. 在 `main` 里加 `printf("data[7] = %d\n", data[7]);`（越界**读**），报告的 `WRITE` 会变成什么？
3. 用 `ASAN_OPTIONS=halt_on_error=0` 运行，报告有什么不同？
4. 对比本报告的调用栈和实验 01 的调用栈——哪个更长？为什么？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 调用栈中看不到自己的函数名 | 编译时没加 `-g` 或 strip 了符号 | 确保 `-g` 在 CFLAGS 中 |
| 影子字节图看不清楚 | 报告太长，终端滚动过去了 | 把 stderr 重定向：`./report 2>asan.log` |
| `X bytes to the left` vs `to the right` | left = 负偏移（下溢），right = 正偏移（上溢） | left 常见于 `arr[-1]`，right 常见于 `arr[n]` |
| 报告说 `stack-buffer-overflow` 但代码里没有 `malloc` | 局部数组越界——栈溢出和堆溢出是不同的类型 | 看"段一"的错误类型，检查局部数组声明 |

---

## 下一步

现在你能完全看懂任何 ASan 报告了。下一步开始逐个击破四大内存错误——从最常见的开始：[实验 03：堆缓冲区溢出](../03-heap-overflow/)。
