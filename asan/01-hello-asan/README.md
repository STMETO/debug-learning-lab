# 实验 01：Hello ASan —— 第一次看到内存 bug 被自动抓住

> 🎯 **目标**：用 `-fsanitize=address` 编译程序，运行，看到你的第一份 ASan 报告。
>
> ⏱️ 约 10 分钟

## 先看示例程序

[hello.c](hello.c) 是一个短小的程序，只有一处故意的 bug：

```c
int *arr = (int *)malloc(10 * sizeof(int));  // 分配 10 个 int（索引 0~9）

for (int i = 0; i < 10; i++) {
    arr[i] = i * 10;      // ✅ 正常：arr[0] 到 arr[9]
}

arr[10] = 999;             // ❌ 越界！索引 10 不属于这块内存
```

如果没有 ASan，这个 bug 可能会：
- 悄悄破坏 `arr` 后面的内存（也许刚好没崩溃——但数据已经坏了）
- 在别的地方莫名其妙崩溃，你根本联想不到是这里
- 换台机器、换个编译器版本，行为不同——经典的"在我机器上能跑"

---

## 实验开始

### 第一步：观察 Makefile

打开 [Makefile](Makefile)，注意和 GDB 实验的区别：

```makefile
CFLAGS = -g -Wall -Wextra -fsanitize=address -fno-omit-frame-pointer
```

| 参数 | 作用 |
|------|------|
| `-fsanitize=address` | **开启 AddressSanitizer**——这是核心 |
| `-g` | 生成调试符号，让报告显示行号（和 GDB 一样） |
| `-fno-omit-frame-pointer` | 保留栈帧指针，让调用栈更完整 |

### 第二步：编译

```bash
make
```

> 没有任何警告或错误——编译器不会发现这个 bug。只有**运行时**才会暴露。

### 第三步：运行

```bash
./hello
```

程序会输出一些正常内容，然后 ASan 终止程序并打印报告：

```
arr[0] = 0
arr[9] = 90
=================================================================
==170978==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x504000000038 ...
```

**这就是 ASan 报告**——你看到它的瞬间意味着一个内存 bug 被抓住了。

---

## 逐行解读报告

把报告拆成四段来看。**不需要一次全懂**——知道每段的作用就行。

### 段一：错误概要（第 3-6 行）

```
=================================================================
==170978==ERROR: AddressSanitizer: heap-buffer-overflow on address ...
WRITE of size 4 at 0x504000000038 thread T0
    #0 0x... in main hello.c:22
    #1 0x... in __libc_start_call_main ...
```

| 行 | 告诉你什么 |
|----|-----------|
| `ERROR: ... heap-buffer-overflow` | 错误类型：**堆缓冲区溢出** |
| `WRITE of size 4` | 你在**写入** 4 个字节时越界了（`int` = 4 字节） |
| `thread T0` | 发生在主线程 |
| `#0 ... hello.c:22` | **bug 的确切位置**：`hello.c` 第 22 行 |

> 🎯 **第 22 行 = `arr[10] = 999;`**——ASan 精确到行号。

### 段二：内存区间信息

```
0x504000000038 is located 0 bytes to the right of 40-byte region [0x...,0x...)
allocated by thread T0 here:
    #0 ... in __interceptor_malloc ...
    #1 ... in main hello.c:10
```

| 信息 | 含义 |
|------|------|
| `40-byte region` | 你分配了 40 字节（10 个 `int` × 4 字节） |
| `0 bytes to the right` | 越界发生在**紧贴着分配区域的右边界**（写完最后一个合法字节后又多写了一点点） |
| `allocated ... hello.c:10` | 这块内存是在第 10 行（`malloc` 那一行）分配的 |

> ASan 不仅告诉你**哪里越界**（段一），还告诉你**内存是在哪里分配的**（段二）。这叫"分配点-错误点"对——追查 bug 最强有力的一对信息。

### 段三：SUMMARY 行

```
SUMMARY: AddressSanitizer: heap-buffer-overflow hello.c:22 in main
```

> 这是**全文最精简的总结**——错误类型 + 文件:行号 + 函数名。以后再看到 ASan 报告，先找 SUMMARY 行。

### 段四：影子字节图（本次先跳过）

```
Shadow bytes around the buggy address:
  ...
=>0x0a087fff8000: fa fa 00 00 00 00 00[fa]fa fa fa fa fa fa fa fa
  ...
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Heap left redzone:       fa
  ...
```

> 这部分是 ASan 的"内部视角"——显示影子内存（shadow memory）的状态。我们会在[实验 02](../02-read-report/) 深入。现在只需要知道：`fa` = 红区（不可访问），`00` = 可访问，`[fa]` 框住的是你越界踩到的字节。

---

## 修复

把第 22 行的 `arr[10]` 改为 `arr[9]`（或把 `malloc` 的大小从 10 改为 11）：

```c
// 方案 1：使用合法索引
arr[9] = 999;     // arr[9] 是最后一个合法位置

// 方案 2：扩大分配
int *arr = (int *)malloc(11 * sizeof(int));  // 索引 0~10 都合法
```

改完后：

```bash
make && ./hello
```

输出：

```
arr[0] = 0
arr[9] = 90
arr[10] = 999       ← 如果用了方案 2，这行正常执行
```

**没有 ASan 报告 = bug 不存在**。

---

## 📝 关键认知

### ASan 报告 = 三要素

```
┌──────────────────────────────────────────┐
│  SUMMARY 行：    错误类型 + 位置           │
│  #0 栈帧：       错误发生在哪一行           │
│  allocated by:   这块内存在哪里分配的        │
└──────────────────────────────────────────┘
```

拿到任何 ASan 报告，**先找这三条**。

### ASan 和普通崩溃的区别

| | 没有 ASan | 有 ASan |
|------|----------|---------|
| `arr[10] = 999` | 可能不崩溃，静默破坏数据 | **立刻**终止并打印报告 |
| 定位 bug | 可能花几小时 | 报告直接指出行号 |
| 需要的操作 | 加 printf、GDB、反复排查 | `make && ./program` |

---

## 🧪 试试看

1. 把 `malloc(10 * sizeof(int))` 改成 `malloc(5 * sizeof(int))`——报告有什么不同？
2. 试试**读**越界而不是写越界：`int x = arr[10];`（不赋值，只读取）——ASan 会报吗？
3. 把 CFLAGS 里的 `-fsanitize=address` 去掉重新编译运行——会发生什么？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| ASan 报告后程序直接退出（exit code 1） | 这是 ASan 的默认行为：检测到错误 → 终止 | 正常——防止数据被进一步破坏 |
| 链接时报 `undefined reference to __asan_...` | 忘记链接 ASan 库 | 用 `gcc` 而不是 `ld`；`-fsanitize=address` 需要同时传给编译和链接 |
| ASan 和其他 sanitizer 不能同时用 | 某些 sanitizer 互斥 | 只用一个 `-fsanitize=...` 参数 |

---

## 下一步

你已经看到了第一份 ASan 报告。下一步学习 [实验 02：读懂报告](../02-read-report/)——深入理解影子字节图、调用栈的每一帧、以及 ASan 报告中的每个数字的含义。
