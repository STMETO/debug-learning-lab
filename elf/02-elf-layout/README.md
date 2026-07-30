# 实验 02：readelf —— 读懂 ELF 的目录和装载视图

> 🎯 **目标**：区分 section 与 segment，找到入口地址、调试段、符号表和动态段。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 01](../01-file-basics/)

## 先跑起来

~~~bash
make
readelf -h layout
readelf -S layout
readelf -l layout
readelf -d layout
~~~

## 四个最常用视图

| 命令 | 关注点 |
|------|--------|
| readelf -h | ELF class、架构、文件类型、入口地址 |
| readelf -S | section：给链接器、调试器和分析工具看的逻辑分区 |
| readelf -l | program header/segment：给内核加载器映射内存的视图 |
| readelf -d | dynamic section：动态库依赖和运行时搜索路径 |

## section 和 segment 不是同一个概念

~~~text
section：.text .rodata .data .bss .debug_info ...
         面向链接、符号、调试

segment：LOAD、DYNAMIC、INTERP ...
         面向内核把文件映射进进程地址空间
~~~

用下面两条命令定位关键 section：

~~~bash
readelf -SW layout | grep -E '\.text|\.rodata|\.data|\.bss|\.debug'
readelf -lW layout
~~~

layout.c 中的函数机器码通常在 .text，字符串在 .rodata，counter 在 .data；未初始化全局变量则会在 .bss。-g 生成的调试信息位于 .debug_*，不会被程序运行时加载。

## PIE 为什么入口地址看起来很小？

现代发行版默认常构建 PIE。readelf -h 的 Type 会是 DYN（Position-Independent Executable），运行时实际地址会被 ASLR 加上随机基址。不要把运行时 GDB 地址直接交给 addr2line；实验 06 会处理这个区别。

## 🧪 试试看

1. 改用 gcc -no-pie 构建，再比较 readelf -h 的 Type。
2. 用 readelf -S layout | grep debug 确认调试段存在。
3. 运行 readelf -d layout | grep NEEDED，找出 libc。

## 下一步

[实验 03：nm 与符号表](../03-symbols-nm/) 将解释“函数和变量的名字”在 ELF 中如何存在。

