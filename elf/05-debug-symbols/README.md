# 实验 05：调试符号 —— “有函数名”不等于“能调试”

> 🎯 **目标**：区分 ELF 符号表与 DWARF 调试信息，观察 strip 对 GDB 的影响。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 04](../04-objdump-disassembly/)

## 先跑起来

~~~bash
make debug release stripped
file debug release stripped
readelf -SW debug | grep debug
readelf -SW stripped | grep debug
~~~

debug 包含 .debug_info、.debug_line 等 DWARF section；stripped 由 debug 复制后执行 strip --strip-debug，因此程序机器码仍在，但源码行和局部变量映射被移除。

## 用 GDB 比较

~~~bash
gdb -q ./debug
~~~

~~~text
(gdb) break transform
(gdb) run
(gdb) list
(gdb) print doubled
~~~

对 stripped 重复操作。你可能仍能按函数名设断点（取决于未被移除的动态/普通符号），但通常不能 list 源码，也看不到 doubled。调试信息才是“地址 → 文件、行、变量”的地图。

## 生产实践：分离调试信息

发布二进制不一定要完全失去可诊断性。常见流程是：

~~~bash
objcopy --only-keep-debug debug debug.debug
strip --strip-debug debug
objcopy --add-gnu-debuglink=debug.debug debug
~~~

把 debug.debug 与二进制、构建 ID 一起归档，线上 core dump 才能可靠还原。这里先理解概念，不要在唯一副本上直接 strip。

## 🧪 试试看

1. 比较 ls -lh debug stripped 的大小。
2. 对 release 加 -g -O2，观察它有调试段但局部变量体验仍可能受优化影响。
3. 用 readelf -n debug 查看 build ID。

## 下一步

[实验 06：addr2line](../06-addr2line/) 把调试信息用于最常见的“地址还原源码行”。

