# 实验 05：释放后使用（Use-After-Free） —— 幻影指针

> 🎯 **目标**：理解 use-after-free 的 ASan 报告三段落结构：出错点 → 释放点 → 分配点。
>
> ⏱️ 约 10 分钟
>
> 📖 **前置**：[实验 03](../03-heap-overflow/)（堆溢出）、[实验 04](../04-stack-overflow/)（栈溢出）

## 这是什么 bug？

```c
int *ptr = malloc(40);
free(ptr);          // 内存归还给堆管理器
ptr[3] = 999;       // ★ 还在用！ptr 现在指向一块"已不属于你"的内存
```

`free` 之后还保留指针，然后继续用它——这就是 **use-after-free（UAF）**。它是安全漏洞的第一大来源（CWE-416）。

---

## 先看示例程序

[uaf.c](uaf.c) 有三种释放后使用的场景：

| 模式 | 操作 | 典型场景 |
|------|------|---------|
| 1 | `free` 后写入 | 释放了缓冲区后又往里写数据 |
| 2 | `free` 后读取 | 释放了字符串后又 `printf` 它 |
| 3 | 结构体中的悬垂指针 | 释放了成员又释放整体，用反了顺序 |

```bash
make
./uaf 1    # 释放后写入
./uaf 2    # 释放后读取
./uaf 3    # 悬垂指针
```

---

## 实验开始

### 模式 1：释放后写入 —— `./uaf 1`

这是 use-after-free 报告的完整结构：

```
=================================================================
==...==ERROR: AddressSanitizer: heap-use-after-free on address 0x50400000001c
WRITE of size 4 at 0x50400000001c thread T0
    #0 ... in classic_uaf uaf.c:20          ← ① 出错：释放后在哪里用了
    #1 ... in main uaf.c:65
```

和之前两种报告不一样——use-after-free 报告多了一个**关键段落**：

```
freed by thread T0 here:                     ← ② 释放：谁 free 了这块内存
    #0 ... in __interceptor_free
    #1 ... in classic_uaf uaf.c:18          ← free(ptr) 在这里
    #2 ... in main uaf.c:65

previously allocated by thread T0 here:       ← ③ 分配：这块内存最初在哪 malloc
    #0 ... in __interceptor_malloc
    #1 ... in classic_uaf uaf.c:12          ← malloc 在这里
    #2 ... in main uaf.c:65
```

> 🎯 **Use-after-free 报告的"三段式"结构**：
>
> ```
> ① 错误点 （ERROR: heap-use-after-free）
>    ↓
> ② 释放点 （freed by thread T0 here:）
>    ↓
> ③ 分配点 （previously allocated by thread T0 here:）
> ```
>
> 这是唯一一种会让你看到**三份**调用栈的 ASan 报告类型。

追溯 bug：

```
③ 分配点: uaf.c:12   malloc(40)           ← "这块内存诞生于此"
    ↓ （正常使用）
② 释放点: uaf.c:18   free(ptr)            ← "内存在此被回收"
    ↓ （指针还保留着！）
① 出错点: uaf.c:20   ptr[3] = 999         ← "回收后又用了！"
```

### 模式 2：释放后读取 —— `./uaf 2`

报告从 `WRITE` 变成了 `READ`：

```
READ of size 1 at 0x... thread T0
    #0 ... in read_after_free uaf.c:30     ← printf 读取了已释放的字符串
    ...
freed by thread T0 here:
    #1 ... in read_after_free uaf.c:28     ← free(str) 在这里
    ...
```

> Use-after-free **不管是读还是写**都会触发 ASan。都是严重 bug——读到的数据可能是垃圾或已被复用。

### 模式 3：悬垂指针 —— `./uaf 3`

```c
struct User *user = malloc(sizeof(struct User));
user->name = strdup("Alice");

free(user->name);   // 先释放 name
free(user);         // 再释放 user（这两个顺序反了！）

printf("%s\n", user->name);  // ★ user->name 已经释放了
```

报告会指出释放顺序的问题：

```
freed by thread T0 here:
    #1 ... user->name was freed first      ← 最先被释放的
```

---

## ASan 如何检测 use-after-free？

ASan 在 `free` 之后不立即把内存还给操作系统，而是：

```
free 之前：  [ 你的数据 ]           影子字节 = 00（可访问）
free 之后：  [ 你的数据 ] 隔离区     影子字节 = fd（已释放，不可访问）
             ↑                    ↑
            数据还在！             但访问它会触发 ASan 报错
```

影子字节从 `00` → `fd`（freed）。你再访问 → ASan 看到 `fd` → 报 `heap-use-after-free`。

> 这就是你在影子字节 legend 中看到的 `Freed heap region: fd` 的应用。

---

## 📝 Use-after-free 诊断清单

```
1. 看 SUMMARY → 确认是 heap-use-after-free
2. 看 ERROR 段 → 找到出错位置和调用栈
3. 看 freed by 段 → 谁 free 的？free 和 use 的时间顺序对吗？
4. 看 allocated by 段 → 最初在哪分配的？（补充完整时间线）
5. 修复：在 free 之后将指针设为 NULL，或调整释放顺序
```

---

## 🧪 试试看

1. 在 `free(ptr)` 后加上 `ptr = NULL;`——ASan 还报错吗？为什么这不一定是最好的修复？
2. 模式 2 中如果字符串被别的 `malloc` 复用了，`printf` 会打印出什么？
3. 对比 uaf 报告的影子字节和堆溢出报告的影子字节——`fd` 和 `fa` 分别出现在哪里？
4. 栈上分配的变量会被 use-after-free 检测到吗？（提示：`stack-use-after-return` 是另一个错误类型）

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `freed by` 段显示 `??` | 释放发生在另一个翻译单元，调试符号不可用 | 确保所有 `.c` 都用 `-g` 编译 |
| `heap-use-after-free` 说 `READ` 但你的代码只有写 | 某行的副作用可能触发读取 | 看完整调用栈定位 |
| 释放了但 ASan 没报 | 那块内存恰好还没被 ASan 标记为 `fd` | 正常情况下不会——ASan 在 `free` 时立即毒化 |

---

## 下一步

你已经会了堆溢出、栈溢出、释放后使用。只剩最后一种常见的释放类 bug：[实验 06：重复释放与非法释放](../06-double-free/)——`free` 两次同一个指针会怎样？
