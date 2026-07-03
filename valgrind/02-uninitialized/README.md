# 实验 02：⭐ 未初始化值 —— Valgrind 的独家超能力

> 🎯 **目标**：用 Valgrind 检测"使用了未初始化的变量"——这是 ASan、GDB、strace 都做不到的。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 01](../01-hello-valgrind/)

## 为什么这个能力如此重要？

```
ASan 能检测：你访问了不该访问的内存（越界、释放后）
Valgrind 还能检测：你用了从没赋过值的变量

int x;          // x 的值是随机的（栈上的垃圾数据）
if (x > 0) {    // ★ 这个判断基于随机值——结果不可预测
    ...         // ASan 完全看不到这个 bug
}               // Valgrind 能看到："Conditional jump depends on uninitialised value"
```

---

## 先看示例程序

[uninit.c](uninit.c) 有四种未初始化值的场景：

| 场景 | Bug | 类型 |
|------|-----|------|
| 1 | `int x` 未初始化就用于 `if (x > 0)` | 条件分支依赖垃圾值 |
| 2 | `scores[10]` 只填了前 5 个就用来计算 | 数组部分初始化 |
| 3 | `malloc` 后直接用（没 `memset`） | 堆内存未初始化 |
| 4 | 结构体只初始化了前两个字段 | 部分初始化结构体 |

```bash
make
./uninit    # 每次运行结果可能不同！
```

---

## 实验开始

### 第一步：不加任何选项运行

```bash
valgrind ./uninit
```

Valgrind 报出了大量错误：

```
==289574== Conditional jump or move depends on uninitialised value(s)
==289574== Conditional jump or move depends on uninitialised value(s)
==289574== Use of uninitialised value of size 8
==289574== Conditional jump or move depends on uninitialised value(s)
...（共 ~15 条）
```

### 第二步：加 `--track-origins=yes` —— 追踪来源

```bash
valgrind --track-origins=yes ./uninit
```

现在每条未初始化错误都带有**来源信息**：

```
==290531== Conditional jump or move depends on uninitialised value(s)
==290531==    at 0x109217: branch_on_uninit (uninit.c:14)
==290531==    by 0x109331: main (uninit.c:55)
==290531==  Uninitialised value was created by a stack allocation
==290531==    at 0x109209: branch_on_uninit (uninit.c:9)
```

解读：

| 行 | 含义 |
|----|------|
| `Conditional jump or move depends on...` | 未初始化值被用于条件判断 |
| `at ... branch_on_uninit (uninit.c:14)` | **哪里使用了**未初始化值（if 语句） |
| `Uninitialised value was created... (uninit.c:9)` | **哪里创建了**未初始化值（`int x;` 那一行） |

> `--track-origins=yes` 给出完整的"诞生→使用"追踪链——从未初始化的栈帧到使用它的代码。

### 第三步：读两种未初始化错误

Valgrind 报告两种未初始化错误：

| 错误类型 | 含义 | 本实验中的例子 |
|---------|------|-------------|
| `Conditional jump or move depends on uninitialised value(s)` | 未初始化值被用于 `if`/`switch`/`for` 条件 | 场景 1：`if (x > 0)` |
| `Use of uninitialised value of size N` | 未初始化值被用于计算或传参 | 场景 2：`total += scores[i]` 场景 3：`strlen(buf)` |

**"Conditional jump" 比 "Use" 更危险**——它意味着程序的**控制流**受到了随机值的影响。两次运行可能走不同的分支。

### 第四步：四种场景对比——Valgrind 全抓住了

跑 `--track-origins=yes` 后，Valgrind 为每个场景给出了清晰的路径：

| 场景 | Valgrind 报告 | 来源指向 |
|------|-------------|---------|
| 1: `int x; if (x > 0)` | `Conditional jump depends on uninitialised value` | `uninit.c:9` — `int x;` |
| 2: 部分填充的数组 | `Use of uninitialised value` | `uninit.c:23` — `int scores[10];` |
| 3: malloc 后直接 strlen | `Use of uninitialised value` | `uninit.c:42` — `malloc(32)` |
| 4: 结构体部分初始化 | `Conditional jump depends on uninitialised value` | 计算 `p.x + p.y + p.z` |

### 为什么 ASan 做不到？

ASan 的工作原理是**影子内存**——在每次内存访问前检查"这块内存是不是被毒化了"。但未初始化值的内存本身是**可访问**的（它是一块合法的栈/堆内存），只是**内容没意义**。

```
ASan 问：       "这块内存可以访问吗？"  → 是 → 不报警
Valgrind 问：   "这个值有没有被写过？"  → 没有 → 报警

关键区别：Valgrind 追踪每个字节的"写历史"，ASan 只看内存边界
```

---

## 📝 命令小结

| 选项 | 作用 |
|------|------|
| `valgrind ./program` | 检测未初始化值的使用（默认开启） |
| `--track-origins=yes` | 追踪未初始化值的来源（给出"哪里创建"） |

---

## 🧪 试试看

1. 在场景 1 中加 `x = 42;`——Valgrind 还报吗？
2. 编译时加 `-O2`（开启优化）——部分未初始化错误可能消失（编译器优化掉了），这是好事还是坏事？
3. 写一个函数，`malloc` 后 `memset` 清 0 再使用——对比不 `memset` 的版本，Valgrind 的输出有什么不同？
4. `--track-origins=yes` 会让 Valgrind 慢多少？用 `time` 对比一下

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| Valgrind 报了但程序运行结果每次都对 | 垃圾值"恰好"在合理范围内 | 不修复的话某天会出 bug——换个编译器/内核/环境就暴露 |
| 编译器警告 `-Wuninitialized` 已经报了，还需要 Valgrind 吗 | 编译器只能检测**明显的**未初始化，跨函数/跨文件的检测不到 | Valgrind 是运行时全量追踪 |
| `--track-origins=yes` 让报告变长很多 | 每条错误都多了来源信息 | 用 `--log-file` 保存再慢慢看 |

---

## 下一步

未初始化值检测是 Valgrind 最独特的能力——其他工具都做不到。下一步 [实验 03：内存泄漏分类](../03-leak-classification/)——Valgrind 把泄漏分成四级：definitely lost / indirectly lost / possibly lost / still reachable，比 LSAN 细致得多。
