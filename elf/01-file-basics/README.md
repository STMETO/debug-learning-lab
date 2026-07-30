# 实验 01：file —— 先确认你面对的是什么二进制

> 🎯 **目标**：用 file 快速识别 ELF 类型、架构、链接方式和调试信息状态。
>
> ⏱️ 约 15 分钟

## 先跑起来

~~~bash
make
file hello
./hello
~~~

典型输出类似：

~~~text
hello: ELF 64-bit LSB pie executable, x86-64, dynamically linked, ... with debug_info, not stripped
~~~

这一行已经回答了五个关键问题：

| 字段 | 含义 |
|------|------|
| ELF 64-bit | Linux ELF 格式，64 位 |
| x86-64 | 目标 CPU 架构 |
| pie executable | 位置无关可执行文件，可被 ASLR 随机映射 |
| dynamically linked | 启动时需要动态加载 libc 等 .so |
| with debug_info, not stripped | 有调试信息，GDB 可以映射源码 |

## 比较三种产物

~~~bash
gcc -O2 -o hello_release hello.c
gcc -g -O0 -o hello_debug hello.c
cp hello_debug hello_stripped
strip hello_stripped
file hello_release hello_debug hello_stripped
~~~

release 不一定没有符号，但通常没有完整调试信息；strip 会移除可移除的符号和调试段。file 是定位时的第一眼检查，不是最终证据；后续实验会用 readelf 和 nm 深入确认。

## 📝 命令小结

| 命令 | 用途 |
|------|------|
| file 程序 | 快速判断格式、架构、链接和 stripped 状态 |
| uname -m | 查看当前机器架构 |
| ldd 程序 | 查看动态库依赖（仅对可信文件使用） |

## 🧪 试试看

1. 对 /bin/ls、一个 .o 文件、一个 .a 文件分别运行 file。
2. 用 gcc -static 构建 hello_static；file 输出和 hello 有什么不同？
3. 用 gdb 打开 hello_stripped，观察它还能显示哪些信息。

## 下一步

[实验 02：ELF 布局](../02-elf-layout/) 用 readelf 把 file 的概览拆成可验证的结构。

