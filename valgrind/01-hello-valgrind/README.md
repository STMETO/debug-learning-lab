# 实验 01：Hello Valgrind —— 不重编译，直接跑

> 🎯 **目标**：第一次用 Valgrind，看到它如何在不重新编译的情况下检测内存错误和泄漏。
>
> ⏱️ 约 10 分钟

## Valgrind 和 ASan 的第一眼区别

```
ASan:  gcc -fsanitize=address → ./a.out   → ASan 报告（程序自动终止）
Valgrind: gcc（正常编译）   → valgrind ./a.out → Valgrind 报告（程序继续运行）

ASan 需要重新编译。Valgrind 不需要。
ASan 在第一个错误处终止。Valgrind 会继续运行，报告所有错误。
```

---

## 先看示例程序

[hello.c](hello.c) 有两个故意的 bug：

- **堆溢出**：`arr[5] = 999`（只分配了 5 个元素）
- **内存泄漏**：`malloc(400)` 后忘了 `free`

```bash
make && ./hello    # 注意：不用 -fsanitize=address！
```

程序"正常"运行完毕——因为堆溢出恰好没踩到关键数据。

---

## 实验开始

### 第一步：直接在 Valgrind 中运行

```bash
valgrind ./hello
```

Valgrind 的 stderr 输出（关键部分）：

```
==289463== Invalid write of size 4
==289463==    at 0x109201: heap_overflow (hello.c:13)
==289463==    by 0x109233: main (hello.c:24)
==289463==  Address 0x4a8c094 is 0 bytes after a block of size 20 alloc'd
==289463==    at 0x1091BE: heap_overflow (hello.c:10)
==289463==    by 0x109233: main (hello.c:24)
```

> 格式和 ASan 非常像——错误类型（`Invalid write`）+ 调用栈 + 内存位置（`0 bytes after a block of size 20`）。

### 第二步：和 ASan 输出对比

同一个 bug，两种工具的报告：

| | ASan | Valgrind |
|------|------|---------|
| 关键词 | `heap-buffer-overflow` | `Invalid write of size N` |
| 位置描述 | `0 bytes to the right of 40-byte region` | `0 bytes after a block of size 20 alloc'd` |
| 编译要求 | `-fsanitize=address` | 不需要 |
| 性能 | ~2x 减速 | ~20x 减速 |
| 程序是否终止 | 是（默认） | 否（继续运行，报告更多错误） |

### 第三步：泄漏检测——默认就跑

Valgrind 在程序结束时自动打印泄漏摘要：

```
==289463== HEAP SUMMARY:
==289463==     in use at exit: 400 bytes in 1 blocks
==289463==   total heap usage: 3 allocs, 2 frees, 4,516 bytes allocated
==289463==
==289463== 400 bytes in 1 blocks are definitely lost in loss record 1 of 1
==289463==    at 0x4848899: malloc (in /usr/libexec/valgrind/...)
==289463==    by 0x1091CD: memory_leak (hello.c:19)
==289463==    by 0x109244: main (hello.c:27)
==289463==
==289463== LEAK SUMMARY:
==289463==    definitely lost: 400 bytes in 1 blocks
==289463==    indirectly lost: 0 bytes in 0 blocks
==289463==      possibly lost: 0 bytes in 0 blocks
==289463==    still reachable: 0 bytes in 0 blocks
==289463==         suppressed: 0 bytes in 0 blocks
```

关键信息：

| 行 | 含义 |
|----|------|
| `HEAP SUMMARY: in use at exit: 400 bytes` | 程序退出时还有 400 字节没释放 |
| `definitely lost: 400 bytes` | 确定泄漏（指针丢失，无法再 free） |
| `indirectly lost: 0 bytes` | 间接泄漏（实验 03 会讲） |
| `total heap usage: 3 allocs, 2 frees` | 3 次 malloc，2 次 free——少了 1 次 |

### 第四步：常用选项

```bash
# 详细泄漏报告（显示每个泄漏的调用栈）
valgrind --leak-check=full ./hello

# 同时显示 reachable 类型的泄漏
valgrind --leak-check=full --show-leak-kinds=all ./hello

# 将报告输出到文件
valgrind --log-file=vg.log ./hello
```

---

## 📝 命令小结

| 命令 | 作用 |
|------|------|
| `valgrind ./program` | 用 Memcheck 检测内存错误和泄漏（默认） |
| `valgrind --leak-check=full ./program` | 详细泄漏报告（推荐） |
| `valgrind --log-file=文件 ./program` | 将报告写入文件 |

### Valgrind vs ASan 快速决策

```
你有源码、可以重新编译 → ASan（10 倍更快）
没有源码、不能重编译   → Valgrind（唯一选择）
需要找未初始化值       → Valgrind（ASan 做不到）
CI 需要速度            → ASan
发布前最终审计         → Valgrind（更详尽）
```

---

## 🧪 试试看

1. 用 `valgrind ls /tmp`——`ls` 有内存错误吗？有泄漏吗？
2. 对比 `valgrind ./hello` 和 `ASAN_OPTIONS=... ./hello_asan`（重新用 `-fsanitize=address` 编译）——哪个输出更快出结果？哪个信息更全？
3. 在 `heap_overflow` 中把 `arr[5]=999` 改成 `arr[50000]=999`——Valgrind 还能报吗？报告有什么不同？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| Valgrind 报告中没有行号 | 编译时没加 `-g` | 加 `-g` 重新编译 |
| Valgrind 太慢 | 程序本身运行 1 秒，Valgrind 可能要 20 秒 | 正常的——Valgrind 是 CPU 模拟器 |
| `still reachable` 很多但不是我的代码 | glibc/第三方库的全局缓存 | 正常——关注 `definitely lost` 即可 |

---

## 下一步

Valgrind 能检测堆溢出和泄漏——和 ASan 一样。但有一件事**只有 Valgrind 能做到**：检测未初始化值的使用。下一步 [实验 02：未初始化值](../02-uninitialized/)——这是 Valgrind 的独家超能力。
