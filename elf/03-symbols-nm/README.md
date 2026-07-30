# 实验 03：nm —— 函数名最终在哪里？

> 🎯 **目标**：读懂常见符号类型，区分“声明了但未定义”和“仅本文件可见”。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 02](../02-elf-layout/)

## 先跑起来

~~~bash
make
nm -C main.o
nm -C symbols.o
nm -C symbols
./symbols
~~~

重点观察 main.o 中的 U public_add 和 U exported_counter：main.c 只看到了声明，因此它需要别的对象提供定义。symbols.o 则提供了这些定义。

| 标记 | 含义 | 本实验例子 |
|------|------|------------|
| U | undefined，等待链接器解析 | main.o 的 public_add |
| T | 全局 text（函数代码） | public_add |
| t | 局部 text（static 函数） | 若有 static 函数 |
| D | 已初始化全局数据 | exported_counter |
| B/b | 未初始化数据（BSS） | private_counter 常见位置 |

## 只看你关心的符号

~~~bash
nm -g symbols.o
nm -S --size-sort symbols
nm -A *.o | grep public_add
~~~

-g 只显示外部符号；-S 显示大小；-A 在多个文件前打印文件名。排查 undefined reference 时，先用 nm 确认“谁需要它，谁应该提供它”。

## static 的真正作用

把 public_add 改成 static 后重新 make。symbols.o 仍有函数代码，但链接 main.o 时无法使用它，因为该名字不再导出到对象外部。这就是 static 在 C 里的链接可见性含义。

## 🧪 试试看

1. 把 symbols.o 从链接命令中删掉，阅读链接器报错并和 nm 输出对应。
2. 给 symbols.c 加一个未初始化全局变量，观察 nm 类型。
3. 对 libc.so.6 执行 nm -D（路径可由 ldd ./symbols 获取）。

## 下一步

[实验 04：objdump 反汇编](../04-objdump-disassembly/) 从符号名继续走到实际机器指令。

