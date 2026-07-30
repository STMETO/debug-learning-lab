# ELF、符号与动态链接

## 写给谁？

这个模块接在 [构建系统](../build/) 之后。你已经知道源码会被编译、链接成程序；现在要打开这个程序，观察 ELF 文件、符号表、调试信息和动态加载器真正看见的内容。

> GDB、ASan、core dump 和“找不到动态库”不是四件孤立的事：它们都建立在 ELF、符号和链接信息之上。

## 学习路线图

| # | 实验 | 核心工具 | 你会解决的问题 |
|---|------|----------|----------------|
| [01](01-file-basics/) | 识别二进制 | file | 它是什么架构、是否动态链接、是否带符号？ |
| [02](02-elf-layout/) | ELF 布局 | readelf | section、segment、入口地址各是什么？ |
| [03](03-symbols-nm/) | 符号表 | nm | U/T/t/D/B 分别代表什么？ |
| [04](04-objdump-disassembly/) | 反汇编 | objdump | 一行 C 代码对应哪些指令？ |
| [05](05-debug-symbols/) | 调试信息 | readelf、strip | 为什么 GDB 有时只有地址？ |
| [06](06-addr2line/) | 地址符号化 | addr2line | 如何把崩溃地址还原成源码行？ |
| [07](07-static-libraries/) | 静态库 | ar、nm | 为什么静态库链接顺序会影响结果？ |
| [08](08-dynamic-libraries/) | 动态库 | ldd、readelf | DT_NEEDED、RPATH/RUNPATH 如何工作？ |
| [09](09-loader-debug/) | 加载器诊断 | LD_DEBUG | 程序启动时究竟搜索了哪些路径？ |
| [10](10-strings/) | 二进制线索 | strings | 没有源码时如何快速找配置和报错文本？ |
| [11](11-cpp-symbols/) | C++ 符号 | c++filt、nm | mangling 怎样影响链接和排障？ |
| [12](12-elf-diagnosis/) | 综合诊断 | 全部工具 | 如何从现象推到 ELF 层根因？ |

## 环境要求

~~~bash
# Ubuntu/Debian
sudo apt install binutils file gcc g++ make gdb
~~~

## ELF 的一条主线

~~~text
源代码 → .o → 静态库/.so → ELF 可执行文件 → 动态加载器启动 → GDB/崩溃分析
             ↑              ↑                    ↑
          ar, nm       readelf, ldd         addr2line, objdump
~~~

从 [实验 01：识别二进制](01-file-basics/) 开始。

