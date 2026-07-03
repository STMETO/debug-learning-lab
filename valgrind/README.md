# Valgrind —— 不需要重编译的内存侦探

## 写给谁？

本模块假设你完成了 [ASan 模块](../asan/)，知道什么是堆溢出、释放后使用、内存泄漏。

> 如果你已经喜欢 ASan 的自动报错，Valgrind 会给你一个更慢但更详尽的视角——而且**不需要重新编译**。

## Valgrind 是什么？

**Valgrind** 是一个在"虚拟 CPU"上运行你的程序的工具。它模拟每一条指令，拦截每一次内存访问，记录每一个内存分配和释放。

和 ASan 的关键区别：

```
┌─────────────────────────────────────────────────┐
│  ASan：编译器插桩                                 │
│  编译时在代码中插入检查 → 运行时自动发现 bug        │
│  需要重新编译（-fsanitize=address）               │
│  快（~2x 减速）                                   │
├─────────────────────────────────────────────────┤
│  Valgrind：CPU 模拟器                             │
│  在虚拟 CPU 上运行你的程序 → 监控所有内存操作       │
│  不需要重新编译（任何二进制都能用！）               │
│  慢（~20x 减速）                                  │
└─────────────────────────────────────────────────┘
```

### 什么时候用 Valgrind 而不是 ASan？

| 场景 | 推荐工具 |
|------|---------|
| 开发阶段，能重新编译 | **ASan**（快，覆盖全） |
| 已编译的二进制，没有源码 | **Valgrind**（不需要重编译） |
| 使用了未初始化的变量 | **Valgrind**（ASan 检测不到） |
| 需要详细的泄漏分类 | **Valgrind**（definitely/indirectly/possibly） |
| CI 中快速跑测试 | **ASan**（快 10 倍） |
| 发布前的最终内存审计 | **Valgrind**（更详尽，虽然慢） |

### Valgrind 是一个工具套件

Valgrind 包含多个工具，通过 `--tool=` 选择：

| 工具 | 用途 | 类比 |
|------|------|------|
| **Memcheck** | 内存错误检测（默认） | ≈ ASan 的超集 |
| **Cachegrind** | 缓存模拟 + 性能分析 | ≈ perf stat |
| **Callgrind** | 调用图 + 性能分析 | ≈ perf record + perf report |
| **Massif** | 堆内存分析 | ≈ heaptrack |
| **Helgrind** | 线程错误检测 | ≈ ThreadSanitizer |
| **DRD** | 数据竞争检测 | ≈ ThreadSanitizer |

本模块聚焦 Valgrind **独有的、ASan 做不到的**能力，不与 ASan 模块重复。

---

## 学习路线图

| # | 实验 | 你会学到 | 时长 |
|---|------|---------|------|
| [01](01-hello-valgrind/) | Hello Valgrind | `valgrind ./program`、看懂输出格式、堆/栈溢出在 Valgrind 中怎么报（对比 ASan） | ~10min |
| [02](02-uninitialized/) | ⭐ 未初始化值 | Valgrind **独家超能力**——检测"使用未初始化的变量做判断" | ~20min |
| [03](03-leak-classification/) | 内存泄漏分类 | `--leak-check=full`、definitely/indirectly/possibly/still reachable 四种泄漏 | ~20min |
| [04](04-massif-callgrind/) | Massif & Callgrind | 堆内存剖析（Massif）、调用图性能分析（Callgrind + 火焰图） | ~30min |
| [05](05-gdb-integration/) | Valgrind + GDB | `--vgdb=yes`、在 Valgrind 发现问题时用 GDB 查看完整现场 | ~15min |

> **为什么只有 5 个实验？** 堆/栈溢出、释放后使用、重复释放等 ASan 已经覆盖得很好了。本模块聚焦 Valgrind 的**不可替代**能力：未初始化值检测、四级泄漏分类、无需重编译、Massif/Callgrind 剖析工具。

### 🏆 挑战任务

```
challenge/
├── README.md           # 挑战说明
└── mystery-binary      # 一个没有源码的二进制程序
```

Valgrind 的独特优势是**不需要源码**。挑战任务会提供一个不带调试符号的预编译二进制，你需要纯靠 Valgrind 找出它所有的内存问题。

> 挑战任务正在建设中，敬请期待。

---

## 开始之前

### 你需要什么

- Linux 系统（WSL2 / 虚拟机 / Docker 均可）
- Valgrind 已在本项目的环境中预装

```bash
# 手动安装（Ubuntu/Debian）
sudo apt install valgrind

# 验证
valgrind --version
```

### Valgrind 核心选项速查

| 选项 | 作用 |
|------|------|
| `--leak-check=full` | 程序结束后详细报告每个泄漏（**必加**） |
| `--show-leak-kinds=all` | 显示所有类型的泄漏（包括 reachable） |
| `--track-origins=yes` | 追踪未初始化值的来源（实验 02 会用） |
| `--vgdb=yes` | 启动 GDB 服务器（实验 05 会用） |
| `--tool=memcheck` | 使用 Memcheck 工具（默认，内存检测） |
| `--tool=massif` | 使用 Massif 工具（实验 04 会用） |
| `--tool=callgrind` | 使用 Callgrind 工具（实验 04 会用） |

### Valgrind vs ASan vs GDB 定位速查

```
你要找的是...

"哪个指针越界了？"                 → ASan（快）或 Valgrind（细）
"这个变量什么时候被改成这个值的？"    → GDB watch
"程序有没有用未初始化的值做判断？"    → Valgrind（独有）
"有没有内存泄漏？泄漏在哪？"         → Valgrind --leak-check=full
"第三方 .so 有没有 bug？"          → Valgrind（不需重编译）
"CI 每秒跑几百个测试——不能太慢"    → ASan
```

---

## 实验目录结构

```
03-invalid-access/
├── README.md      # 实验教程（跟着它一步步操作）
├── buggy.c        # 有 bug 的示例源码
└── Makefile
```

---

<p align="center">
  <b>准备好了吗？从 <a href="01-hello-valgrind/">实验 01：Hello Valgrind</a> 开始 👇</b>
</p>
