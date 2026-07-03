# 实验 10：ASan + GDB 联动 —— 在崩溃现场深入分析

> 🎯 **目标**：把 ASan 的错误检测能力和 GDB 的交互调试能力结合起来——在 ASan 发现 bug 但还没终止程序之前，用 GDB 检查内存状态。
>
> ⏱️ 约 15 分钟
>
> 📖 **前置**：[ASan 实验 01-06](../)、[GDB 实验 01-06](../../gdb/)（建议完成两个模块的基础阶段）

## 为什么需要两个工具？

```
ASan 告诉你：       "arr[10] 越界了，在 overflow.c:16"
GDB  让你看到：     "越界那一刻，arr 周围的内存是什么样的？指针指向哪里？"

ASan 是"告警系统" ──→ 告诉你出事了、在哪、什么类型
GDB 是"调查工具" ──→ 让你在事发现场慢慢检查
```

---

## 先看示例程序

[asan_gdb.c](asan_gdb.c) 有两个触发 ASan 的函数：

```bash
make
./asan_gdb 1       # 堆溢出（默认索引 5 = 越界）
./asan_gdb 2       # use-after-free
```

---

## 实验开始

### 方法一：在 GDB 中直接运行（快速联动）

```bash
gdb ./asan_gdb
```

```bash
(gdb) run
```

程序在 GDB 中运行，ASan 检测到错误：

```
==...==ERROR: AddressSanitizer: heap-buffer-overflow ...
    #0 ... in trigger_overflow asan_gdb.c:16
    ...
[Inferior 1 (process ...) exited with code 01]
```

> GDB 收到 ASan 终止程序的信号，程序结束。你可以 `backtrace` 但程序已经死了——和 core dump 分析类似。

### 方法二：在 ASan 报错前设断点（预留现场）

ASan 在检测到错误后会调用内部函数来打印报告。在你关心的函数里设断点，在 ASan 触发**之前**检查内存：

```bash
gdb ./asan_gdb
```

```bash
(gdb) break trigger_overflow
(gdb) run 1 3          # 参数：mode=1, index=3（合法访问）
```

程序停在 `trigger_overflow`，先看看合法状态下内存是什么样的：

```bash
(gdb) next              # 跳过 malloc
(gdb) next              # 跳过赋值循环
(gdb) print buf[0]
$1 = 0
(gdb) print buf[4]
$2 = 40
(gdb) print buf         # buf 指针的值（堆地址）
```

现在看 ASan 的"视角"——用 GDB 查看 malloc 返回的地址周围的内存。但影子内存在常规 GDB 命令里不可见。

```bash
(gdb) quit
```

### 方法三：让 ASan 在报错时生成 core dump（最强联动）

这是生产环境中最常用的模式：

```bash
# ASan 报错时调用 abort()，而不是 _exit()
# abort() 会生成 core dump
ASAN_OPTIONS=abort_on_error=1 ./asan_gdb 1
```

输出：

```
==...==ERROR: AddressSanitizer: heap-buffer-overflow ...
Aborted (core dumped)
```

现在用 GDB 加载 core dump：

```bash
gdb ./asan_gdb core
```

```bash
(gdb) backtrace
```

你会看到**完整的崩溃现场**——包括 ASan 的内部函数和你的代码：

```
#0  __GI_raise (sig=sig@entry=6) ...
#1  __GI_abort () ...
#2  __sanitizer::Abort () ...
#3  __asan::ReportGenericError (...) ...      ← ASan 报告错误
#4  __asan_report_store4 (...) ...            ← ASan 检测到 4 字节写入越界
#5  trigger_overflow (index=5) at asan_gdb.c:16 ← 你的代码
#6  main (argc=1, argv=...) at asan_gdb.c:41
```

```bash
(gdb) frame 5
(gdb) print index
$1 = 5
(gdb) print buf
$2 = (int *) 0x...
(gdb) print buf[4]       # 最后一个合法元素
$3 = 40
```

> 在 core dump 中，你可以：
> - 用 `frame N` 跳到你的代码所在的栈帧
> - 用 `print` 查看变量值
> - 用 `info locals` 看所有局部变量
> - 用 `x/20x buf` 查看 buf 周围的原始内存

### 完整工作流总结

```
开发阶段：
  编译时加 -fsanitize=address  →  运行测试  →  ASan 发现 bug

生产/CI 环境：
  ASAN_OPTIONS=abort_on_error=1  →  崩溃生成 core  →  GDB 分析 core

开发机：
  ASan 报告 SUMMARY → 打开 GDB → break 可疑函数 → step 到出错行 → print 检查
```

---

## 📝 ASan + GDB 联动检查清单

```
遇到 ASan 报告后：

1. 读 SUMMARY → 拿到文件名和行号
2. gdb ./program
3. break 出错文件的出错行
4. run → next/step 到越界那行之前
5. print 相关变量 → 确认值是否合理
6. 如果是释放后使用: print 指针 → 是否应该为 NULL？
7. 如果是堆溢出: print sizeof/malloc 参数 → 算错了吗？
8. 修复 → 重新编译运行 → ASan 不报 ✓
```

---

## 🧪 试试看

1. 用 `ASAN_OPTIONS=abort_on_error=1` 运行 `./asan_gdb 2`（UAF），然后用 GDB 分析 core dump——释放点在哪个函数？分配点呢？
2. 在 GDB 中，`break trigger_overflow` 后 `run`——在越界发生前，用 `x/20x buf` 查看 buf 周围 20 个字的原始内存，能看到 ASan 插入的红区吗？
3. 对比方法一（直接 run 在 GDB 里）和方法三（abort + core dump），各有什么优缺点？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| `abort_on_error=1` 没有生成 core | `ulimit -c` 是 0 | `ulimit -c unlimited`（回顾 GDB 实验 08） |
| GDB 中 `print buf` 显示已释放 | 你可能停在 ASan 终止后的帧 | 用 `frame N` 跳到越界发生前的栈帧 |
| ASan 报告的行号和 GDB 断点行号不一致 | 编译器优化改变了行号映射 | 用 `-O0` 编译 |

---

## 下一步

🟠 ASan 模块全部完成！🎉

回顾你学到的 10 个实验：

```
阶段一：编译参数 → 读报告三要素
阶段二：堆溢出 → 栈溢出 → 释放后使用 → 重复释放
阶段三：内存泄漏 → 全局溢出 → 抑制文件 → ASan+GDB
```

你现在可以：

- 用 `-fsanitize=address` 在任何 C/C++ 项目中开启 ASan
- 读懂所有五种错误类型的 ASan 报告
- 用影子字节图确认内存状态
- 用 LSAN 检测内存泄漏
- 用抑制文件在大型项目中渐进式接入 ASan
- 用 `abort_on_error` + GDB core dump 深入分析崩溃现场

返回 [模块首页](../README.md) 或 [项目主页](../../README.md)。
