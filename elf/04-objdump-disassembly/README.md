# 实验 04：objdump —— 从 C 代码走到机器指令

> 🎯 **目标**：把函数符号、地址、汇编和源代码对应起来，并观察优化造成的变化。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 03](../03-symbols-nm/)

## 先跑起来

~~~bash
make debug release
objdump -d -Mintel debug | less
objdump -S -Mintel debug | less
~~~

-d 反汇编可执行代码；-S 在有调试信息时混合显示源代码。搜索 sum_to：

~~~bash
objdump -d -Mintel --disassemble=sum_to debug
objdump -S -Mintel --disassemble=sum_to debug
~~~

## Debug 和 Release 的差异

~~~bash
objdump -d -Mintel --disassemble=sum_to debug
objdump -d -Mintel --disassemble=sum_to release
~~~

sum_to 被标记为 noinline，避免 -O2 直接把调用折叠掉。你仍会看到 release 的循环更紧凑、寄存器使用不同。优化器不保证“一行 C 对应一段固定汇编”，这也是 Release 下 GDB 单步跳行、变量 optimized out 的根源。

## 常用组合

| 命令 | 用途 |
|------|------|
| objdump -d 程序 | 全部反汇编 |
| objdump -S 程序 | 源码和汇编混排 |
| objdump -t 程序 | 符号表（nm 的另一种视图） |
| objdump -p 程序 | program header 和动态信息摘要 |

## 🧪 试试看

1. 删除 noinline，观察 release 中是否还能找到 sum_to。
2. 用 nm debug | grep sum_to 获得地址，再用 objdump 验证。
3. 用 gdb 的 disassemble /m sum_to 与 objdump -S 对比。

## 下一步

[实验 05：调试符号](../05-debug-symbols/) 解释 objdump -S 和 GDB 的源码信息从哪里来。

