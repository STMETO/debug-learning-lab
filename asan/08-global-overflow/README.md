# 实验 08：全局缓冲区溢出 —— 既不在堆也不在栈

> 🎯 **目标**：识别 `global-buffer-overflow` 错误，理解 ASan 对全局变量的"相邻变量"信息。
>
> ⏱️ 约 10 分钟
>
> 📖 **前置**：[实验 03](../03-heap-overflow/)（堆溢出）、[实验 04](../04-stack-overflow/)（栈溢出）

## 内存三段论

C 程序的变量分布在三种内存区域：

```
┌─────────────────┐
│  栈 (stack)      │ ← 局部变量、函数参数（实验 04）
├─────────────────┤
│  堆 (heap)       │ ← malloc/calloc 出来的（实验 03）
├─────────────────┤
│  全局/静态 (bss) │ ← 全局变量、static 变量（本实验）
└─────────────────┘
```

ASan 对三种溢出分别报不同的错误类型：

| 变量类型 | ASan 错误类型 |
|---------|-------------|
| 局部数组 | `stack-buffer-overflow` |
| `malloc` 出来的 | `heap-buffer-overflow` |
| 全局/静态数组 | `global-buffer-overflow` |

---

## 先看示例程序

[global.c](global.c) 定义了一些全局和静态数组：

```c
char global_buf[16] = "hello";    // 全局变量
int  global_nums[5] = {...};     // 全局变量
static char static_buf[8] = ...; // 静态变量（文件作用域）
```

```bash
make
./global 1    # 全局 char 数组越界
./global 2    # 全局 int 数组越界
./global 3    # 静态变量越界
```

---

## 实验开始

### `/global 1` —— strcpy 溢出全局数组

```
=================================================================
==...==ERROR: AddressSanitizer: global-buffer-overflow on address 0x...
WRITE of size 29 at 0x... thread T0
    #0 ... in __interceptor_memcpy
    #1 ... in overflow_global_array global.c:22
    #2 ... in main global.c:57
```

**和堆/栈溢出报告的主要区别**：内存描述段：

```
0x... is located 0 bytes to the right of global variable 'global_buf'
    defined in 'global.c:8:6' (0x...) of size 16
```

逐字段：

| 信息 | 含义 |
|------|------|
| `global variable 'global_buf'` | **变量名**——直接告诉你哪个全局变量被越界了 |
| `defined in 'global.c:8:6'` | 变量定义的**文件、行号、列号** |
| `of size 16` | 变量大小 16 字节 |

**还有！** ASan 还会告诉你邻近的全局变量：

```
0x... is located 48 bytes to the left of global variable 'global_nums'
    defined in 'global.c:9:6' (0x...) of size 20
```

> 这行是说：你越界的位置（`global_buf` 结束点）距离下一个全局变量 `global_nums`（第 9 行定义的 20 字节数组）还有 48 字节。如果越界继续扩大，会踩到 `global_nums`。

**这个功能是全局溢出报告独有的**——ASan 知道所有全局变量的布局，所以能告诉你"附近还有什么变量可能被影响"。

### 和堆溢出、栈溢出的三栏对比

| | 堆溢出 | 栈溢出 | 全局溢出 |
|------|--------|--------|---------|
| 错误类型 | `heap-buffer-overflow` | `stack-buffer-overflow` | `global-buffer-overflow` |
| 变量标识 | "20-byte region" | `'buf' (line 11)` | `global variable 'global_buf' defined in global.c:8:6` |
| 邻近信息 | 无 | 帧内对象列表 | **邻近全局变量列表** |
| 影子字节 | `fa`（堆红区） | `f1`/`f2`/`f3`（栈红区） | `f9`（全局红区） |

### `/global 2` —— 全局 int 数组越界

和模式 1 同样的 `global-buffer-overflow`，但变量换成了 `global_nums`。越界位置靠近变量 `global_nums` 定义点。

### `/global 3` —— 静态变量越界

`static char static_buf[8]` 虽然是"静态"的（作用域限制在本文件），但存储位置在全局数据段。ASan 把它和全局变量一样对待——报告 `global-buffer-overflow`。

---

## 📝 全局溢出诊断清单

```
1. 看 SUMMARY → global-buffer-overflow
2. 看 "global variable 'xxx' defined in file:line" → 哪个全局变量
3. 看 "of size N" → 变量有多大，你越了多少
4. 看邻近变量信息 → 有没有可能已经破坏了别的全局变量
5. 修复：增大数组或检查索引
```

---

## 🧪 试试看

1. `global_buf` 的大小是 16——在代码里是怎么确定的？（提示：`sizeof(global_buf)` 不是 `strlen`）
2. ASan 报告说 `48 bytes to the left of global_nums`——这个 48 是从哪里算出来的？
3. 如果两个全局变量紧挨着定义，`global_buf` 溢出会不会破坏 `global_nums` 的值？写代码验证
4. `global-buffer-overflow` 的影子字节里出现了 `f9`——回顾实验 02 的影子字节 legend，`f9` 代表什么？

## 常见错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 报告说 `global variable` 但你代码里没有这个变量名 | 编译器可能重命名或合并了变量 | 看行号，不看变量名 |
| 全局变量被越界但 ASan 没报 | 越界落到了另一个可访问的全局变量区域 | 极少发生——全局变量之间 ASan 也插了红区 |
| 静态变量越界和全局越界报告一样 | 两者放在相同的内存段 | 正常——静态就是"作用域受限的全局" |

---

## 下一步

到这里，你已经掌握了所有五种 ASan 错误类型。下一步学习 [实验 09：抑制文件](../09-suppressions/)——在大型项目中，有些已知 bug 你暂时不想看到，怎么屏蔽它们？
