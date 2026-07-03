# 实验 07：内存泄漏检测 —— 你借了不还

> 🎯 **目标**：用 LSAN（LeakSanitizer）检测内存泄漏，读懂泄漏报告——直接泄漏 vs 间接泄漏。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[实验 03–06](../)——你需要会区分堆/栈/释放类错误

## 泄漏不是"崩溃"

前六个实验的 bug 都会让 ASan 立即终止程序。但内存泄漏不一样：

```
越界/释放类 bug  → 程序立即崩溃 → ASan 报告（程序终止前）
内存泄漏         → 程序"正常"结束 → LSAN 报告（程序退出后）
```

LSAN（LeakSanitizer）是 ASan 的一部分，在程序 `return` 之后扫描堆，报告未释放的内存。

---

## 先看示例程序

[leak.c](leak.c) 有三种泄漏模式：

| 模式 | 操作 | 泄漏类型 |
|------|------|---------|
| 1 | `malloc` 后不 `free`，指针丢失 | 直接泄漏 |
| 2 | `free` 结构体但忘记释放内部成员 | 间接泄漏 |
| 3 | 只在某个条件分支泄漏 | 条件泄漏 |

```bash
make
./leak 1    # 直接泄漏
./leak 2    # 间接泄漏（结构体成员）
./leak 3    # 条件泄漏
```

---

## 实验开始

### 模式 1：直接泄漏 —— `./leak 1`

程序正常结束，但在退出前 LSAN 打印：

```
=================================================================
==...==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 400 byte(s) in 1 object(s) allocated from:
    #0 ... in __interceptor_malloc
    #1 ... in direct_leak leak.c:12          ← 分配点
    #2 ... in main leak.c:72

SUMMARY: AddressSanitizer: 400 byte(s) leaked in 1 allocation(s).
```

解读：

| 信息 | 含义 |
|------|------|
| `Direct leak` | **直接泄漏**——分配的指针丢了，没有任何变量还指在这块内存上 |
| `400 byte(s)` | 泄漏了 400 字节（100 个 int × 4） |
| `1 object(s)` | 1 个 `malloc` 调用产生的泄漏 |
| `leak.c:12` | `malloc` 的调用位置 |

> `Direct leak` 意味着这块内存**再也**无法被 `free` 了——指向它的指针已经随着函数返回而销毁。

### 模式 2：间接泄漏 —— `./leak 2`

```
Direct leak of 20 byte(s) in 1 object(s) allocated from:
    #1 ... in indirect_leak leak.c:33     ← scores 数组
Direct leak of 6 byte(s) in 1 object(s) allocated from:
    #1 ... in indirect_leak leak.c:32     ← name 字符串
SUMMARY: ... 26 byte(s) leaked in 2 allocation(s).
```

两份泄漏都是 `Direct leak`——代码先 `free(rec)` 释放了外层结构体，内层的 `name` 和 `scores` 的指针随之消失，它们指向的内存变成直接泄漏。

> 如果代码中还有指针残留但从未被使用（但指针仍存在），LSAN 会报 `Indirect leak`。

### 模式 3：条件泄漏 —— `./leak 3`

只在 `mode != 1` 时泄漏。这种 bug 在测试中很难暴露——正常路径 `free` 了，错误路径忘了。LSAN 不管路径——它只看**程序结束时还剩多少未释放的内存**。

### LSAN 三种泄漏类型

| 类型 | 含义 | 能修复吗 |
|------|------|---------|
| `Direct leak` | 指针已消失，内存永远无法释放 | ✅ 必须修复 |
| `Indirect leak` | 指针还存在于某个结构体中，但结构体本身泄漏了 | 先修复父级的直接泄漏 |
| `Suppressed` | 被抑制文件屏蔽的泄漏 | 视情况而定 |

---

## 📝 LSAN 选项

```bash
# 只检测泄漏，不检测其他错误（速度快）
ASAN_OPTIONS=detect_leaks=1:detect_odr_violation=0 ./leak 1

# 程序退出时不检查泄漏（有时第三方库有已知泄漏）
ASAN_OPTIONS=detect_leaks=0 ./leak 1

# 第一个错误后继续检测
ASAN_OPTIONS=halt_on_error=0 ./leak 1
```

---

## 🧪 试试看

1. 修复模式 2：在 `free(rec)` 之前先 `free(rec->name)` 和 `free(rec->scores)`——LSAN 还报吗？
2. 在模式 1 中 `free(data)`——为什么 IDE/编译器不警告，但 LSAN 能抓到？这说明了什么？
3. 试试 `ASAN_OPTIONS=detect_leaks=0 ./leak 1`——和默认有什么不同？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 程序退出后没有任何泄漏报告 | `detect_leaks` 默认为 1 但依赖 ASan 运行时 | 确保用 `-fsanitize=address` 链接 |
| 报告说 `Direct leak` 但代码里有 `free` | `free` 可能在某个未执行的分支 | 检查条件分支覆盖 |
| LSAN 报告第三方库的泄漏 | 第三方库可能有已知问题 | 用抑制文件屏蔽（见实验 09） |

---

## 下一步

现在你已经覆盖了堆、栈、释放后、重复释放、泄漏五种错误。还有最后一种：[实验 08：全局缓冲区溢出](../08-global-overflow/)——全局/静态变量的越界访问，ASan 报告里有什么独特信息？
