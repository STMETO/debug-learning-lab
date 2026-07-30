# 实验 01：从源码到程序 —— 构建系统到底在管理什么？

> 🎯 **目标**：亲手执行预处理、编译、汇编、链接四个阶段，理解 .o 文件和 -g 的意义。
>
> ⏱️ 约 20 分钟

## 先跑起来

~~~bash
make
./hello
~~~

输出：

~~~text
7 * 3 = 21
~~~

现在看 [Makefile](Makefile)。它没有直接用一条 gcc 命令，而是把一个 C 文件拆成四个阶段：

~~~text
hello.c → hello.i → hello.s → hello.o → hello
 源代码     预处理      汇编      目标文件   可执行文件
~~~

## 第一步：观察四个产物

~~~bash
make clean
make stages
ls -lh hello*
file hello.o hello
~~~

| 产物 | 含义 | 能直接运行吗？ |
|------|------|----------------|
| hello.i | 展开头文件和宏后的 C 源码 | 不能 |
| hello.s | 汇编代码 | 不能 |
| hello.o | 可重定位目标文件，尚未链接 libc | 不能 |
| hello | 已链接的 ELF 可执行文件 | 可以 |

分别手动执行也完全等价：

~~~bash
gcc -E -o hello.i hello.c
gcc -g -S -o hello.s hello.i
gcc -g -c -o hello.o hello.s
gcc -g -o hello hello.o
~~~

> 注意：真实项目通常不需要保留 .i 和 .s；这里把它们保存下来，是为了看清构建过程。

## 第二步：为什么 .o 不能运行？

~~~bash
./hello.o
~~~

它不会像程序一样工作。hello.o 只有本文件的机器码，其中的 printf 仍是“未解析的外部符号”。链接阶段把它与 C 运行时和 libc 组合起来，才得到可执行文件。

~~~bash
nm hello.o | grep printf
nm hello | grep ' main$'
~~~

输出中的 U printf 表示 undefined：这个符号需要由其他对象提供。后续实验会专门处理这类链接错误。

## 第三步：-g 是给谁的？

~~~bash
gcc -O2 -o hello_release hello.c
gcc -g -O0 -o hello_debug hello.c
gdb -q ./hello_debug
~~~

在 GDB 中输入：

~~~text
(gdb) break multiply
(gdb) run
(gdb) print value
~~~

-g 让 ELF 里带有源码行、函数和变量的映射。它不等于关闭优化；通常调试构建会同时使用 -g 和 -O0，便于单步和查看局部变量。

## 📝 核心认知

~~~text
编译（compile）解决：每个 .c 文件如何变成 .o
链接（link）解决：所有 .o 和库如何组合成一个程序
构建系统解决：哪些文件变化后需要重新编译，命令如何稳定地重复执行
~~~

## 🧪 试试看

1. 修改 SCALE 为 4，再运行 make。哪些文件会重新生成？
2. 用 readelf -S hello_debug | grep debug 看看调试信息段是否存在。
3. 把 Makefile 中 hello: hello.o 的依赖删掉，make 还知道先编译 .o 吗？

## 下一步

下一实验进入 [实验 02：Makefile 基础](../02-makefile-basics/)。你会让 make 自动判断“该不该重编译”。
