# ASan（AddressSanitizer）内存错误检测

## 写给谁？

本模块假设你用完了 [GDB 模块](../gdb/)（至少完成了实验 01-04），会编译 C 程序，知道"堆"和"栈"粗略是什么。

> 没完全理解堆/栈也没关系——ASan 报错报得够清晰，你跑完第一个实验就能看懂。

## ASan 是什么？

**AddressSanitizer（ASan）** 是编译器和运行时协作的内存错误检测工具。

和 GDB 的区别：

```
┌─────────────────────────────────────────────────┐
│  GDB：你手动操作                                  │
│  设断点 → 运行 → 单步 → 查看 → "哦，这里改错了"    │
│  需要你知道该在哪儿停下来                          │
├─────────────────────────────────────────────────┤
│  ASan：全自动                                     │
│  编译时加 -fsanitize=address → 运行 → 程序崩了     │
│  → ASan 打印一份详细报告："你在第 X 行越界了"       │
│  你不需要知道 bug 在哪——ASan 告诉你               │
└─────────────────────────────────────────────────┘
```

### 它怎么做到的？

```
正常编译：          ASan 编译：
                   
源代码              源代码
  ↓                   ↓
编译器              编译器 + ASan 插桩
  ↓                   ↓
机器码              机器码 + 影子内存检查
  ↓                   ↓
运行 → 越界          运行 → 越界 → ASan 拦截 → 打印报告
  ↓                   
（悄悄破坏临近内存）   程序立即终止
```

ASan 在每个内存访问前后插入"影子内存"（shadow memory）检查。如果访问了不该访问的区域（已释放的、未分配的、越界的），ASan 立即中断程序并打印精确报告。

---

## 学习路线图

### 🟢 阶段一：初次接触（实验 01–02）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [01](01-hello-asan/) | Hello ASan | 用 `-fsanitize=address` 编译、看到第一份 ASan 报告、理解"堆越界" | ~10min |
| [02](02-read-report/) | 读懂报告 | ASan 报告的解剖结构：SUMMARY、错误类型、调用栈、影子字节图 | ~15min |

> 学完这两个实验，你能独立看懂任何 ASan 报错的核心信息。

### 🟡 阶段二：四大内存错误（实验 03–06）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [03](03-heap-overflow/) | 堆缓冲区溢出 | `malloc` 出来的区域写越界——最常见的内存 bug | ~10min |
| [04](04-stack-overflow/) | 栈缓冲区溢出 | 局部数组越界——和堆越界的报告有什么区别 | ~10min |
| [05](05-use-after-free/) | 释放后使用 | `free` 之后还用那个指针——悬垂指针（dangling pointer） | ~10min |
| [06](06-double-free/) | 重复释放 | `free` 两次——为什么第二次会崩溃 | ~10min |

> 这四个覆盖了 C/C++ 项目 90% 的内存 bug 类型。

### 🟠 阶段三：生产技能（实验 07–10）

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [07](07-leak-check/) | 内存泄漏检测 | LSAN（LeakSanitizer）、`ASAN_OPTIONS=detect_leaks=1`、解读泄漏报告 | ~15min |
| [08](08-global-overflow/) | 全局缓冲区溢出 | 全局/静态数组越界——和堆/栈溢出有什么区别 | ~10min |
| [09](09-suppressions/) | 抑制文件 | 用 `suppressions` 文件忽略已知 bug、在大型项目中渐进式接入 ASan | ~15min |
| [10](10-asan-gdb/) | ASan + GDB 联动 | 在 ASan 报错时用 GDB 断住、`break __asan::ReportGenericError`、核心转储分析 | ~15min |

> 到这里，你有能力在生产环境中使用 ASan 定位内存 bug。

### 🏆 挑战任务

```
challenge/
├── README.md           # 挑战说明
├── buggy-program.c     # 一个藏了多种内存错误的程序
└── expected.txt        # 期望找到的所有 bug 列表
```

每个模块完结后会提供综合挑战任务——不再有教程引导，给你一个真实的 bug 程序，自己找出所有内存错误。如果你完成了该模块的所有实验，应该有能力独立完成挑战。

> 挑战任务正在建设中，敬请期待。

---

## 开始之前

### 你需要什么

- GCC 4.8+ 或 Clang 3.1+（ASan 内置于现代编译器）
- 不需要额外安装任何库

```bash
# 验证你的编译器支持 ASan
echo "int main(){}" | gcc -fsanitize=address -x c - -o /tmp/test_asan 2>&1
# 没有报错 = 支持
```

### ASan vs Valgrind

| | ASan | Valgrind |
|------|------|------|
| 原理 | 编译时插桩 | 运行时模拟 CPU 执行 |
| 速度 | ~2x 减速 | ~20x 减速 |
| 检测范围 | 堆+栈+全局+释放后 | 主要是堆 |
| 使用方式 | 重新编译即可 | 不需要重新编译 |
| 适合场景 | 开发/测试阶段 | 无法重新编译的二进制 |

> 本项目推荐 ASan 优先——更快、覆盖更全、报告更易读。Valgrind 模块后面也会有。

### 关键概念速览

| 术语 | 一句话解释 |
|------|-----------|
| **影子内存（shadow memory）** | ASan 维护的一块映射表，记录"真实内存的每一字节是否可访问"。每一次内存读写先查这个表 |
| **红区（redzone）** | ASan 在每块分配的内存周围插入的"隔离带"，访问红区 = 越界 |
| **毒化（poison）** | ASan 把某块内存标记为"不可访问"（比如 `free` 之后）。再次访问 → 立即报错 |
| **调用栈（stack trace）** | 从 `main` 到出错位置的完整函数调用链——ASan 报告中最重要的部分 |

---

## 实验目录结构

每个实验目录都遵循同样的结构：

```
03-heap-overflow/
├── README.md      # 本实验的详细教程
├── overflow.c     # 有 bug 的源码
└── Makefile       # make 编译（自动加 -fsanitize=address）
```

---

## 阅读 ASan 报告的黄金法则

你会在每个实验中反复看到 ASan 报告。拿到报告后按这个顺序读：

```
1. 看 SUMMARY ──→ "AddressSanitizer: heap-buffer-overflow on address 0x..."
                 这是什么类型的错误？在哪个地址？

2. 看错误行号 ──→ "READ of size 4 at ... thread T0"
                 #0 ... in main overflow.c:8
                 哪个文件的第几行？

3. 看分配点 ────→ "0x... is located 0 bytes to the right of 40-byte region"
                 "allocated by thread T0 here:"
                 这块内存是在哪里分配的？越了多少？

4. 修 bug ────→ 改代码 → make → ./program → 没有 ASan 报错了 ✓
```

---

## 遇到问题？

- 每个实验的 README 末尾有"常见错误"小节
- ASan 运行时选项：`ASAN_OPTIONS=help=1 ./program` 列出所有可配置项
- 别忘了——ASan 报错是**好事**，它在你发布前抓到了 bug

---

<p align="center">
  <b>准备好了吗？从 <a href="01-hello-asan/">实验 01：Hello ASan</a> 开始 👇</b>
</p>
