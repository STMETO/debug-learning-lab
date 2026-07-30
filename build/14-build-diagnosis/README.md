# 实验 14：构建错误诊断 —— 先判断失败在哪个阶段

> 🎯 **目标**：用编译器、链接器、动态加载器和符号工具定位常见构建错误。
>
> ⏱️ 约 35 分钟
>
> 📖 **前置**：[实验 13](../13-make-cmake-compare/)

## 排查总路线

~~~text
找不到头文件？         编译阶段（compiler）
undefined reference？  链接阶段（linker）
multiple definition？  链接阶段（linker）
程序启动找不到 .so？   运行阶段（dynamic loader）
GDB 没有行号/变量？     产物与调试符号（debug info）
~~~

先确认错误属于哪一层，比盲目添加 -I、-L、-l 或重新 make clean 更重要。

## 案例一：undefined reference

~~~bash
make undefined-reference
~~~

预期失败，错误包含：

~~~text
undefined reference to 'calculate_checksum'
~~~

头文件中的声明让编译器相信函数存在，但最终没有任何 .o 或库提供定义。排查顺序：

1. 函数实现文件是否真的参与了链接？
2. 若实现位于库中，是否加了正确的 -L 路径和 -l 名称？
3. 静态库的 -l 是否写在使用它的对象文件之后？
4. C 与 C++ 混合时，是否需要 extern "C"？

## 案例二：multiple definition

~~~bash
make multiple-definition
~~~

duplicate_a.c 和 duplicate_b.c 都定义了同一个外部符号。链接器无法选择哪一个实现。常见根因是把函数实现写进 .h、把同一个 .c 编译/链接了两次，或错误地复制了全局变量定义。

## 案例三：找不到头文件

~~~bash
make missing-header
~~~

这个错误发生在生成 .o 之前。修复前先问：头文件真的存在吗？它属于本项目、第三方依赖还是系统依赖？只有确认路径后才加 -I；不要用大量 -I 掩盖目录结构问题。

## 案例四：程序能编译，启动却失败

~~~bash
make shared-no-rpath
ldd build/bin/no_rpath
make run-no-rpath
~~~

最后一条预期失败，因为动态加载器找不到 build/lib/libgreeting.so。再试：

~~~bash
make shared-rpath
make run-rpath
~~~

两者都在链接时使用 -lgreeting，但 with_rpath 额外记录了运行时搜索路径。临时验证也可以：

~~~bash
LD_LIBRARY_PATH=build/lib ./build/bin/no_rpath
~~~

## 案例五：GDB 为什么只显示地址？

~~~bash
make no-debug with-debug
readelf -S build/bin/no_debug | grep debug
readelf -S build/bin/with_debug | grep debug
gdb -q ./build/bin/with_debug
~~~

with_debug 带有 .debug_* 段，GDB 才能把地址映射回源码。no_debug 仍然可能保留导出函数名，但通常没有行号、局部变量和可靠单步能力。

## 📝 命令小结

| 命令 | 先回答什么问题 |
|------|----------------|
| make -n | Makefile 到底会执行什么命令？ |
| cmake --build --verbose | CMake 生成的真实命令是什么？ |
| nm / nm -D | 某个符号是否存在于对象/动态库？ |
| ldd | 程序启动时会加载哪些动态库？ |
| readelf -S | ELF 是否包含调试信息等 section？ |
| file | 产物是不是预期架构、预期类型？ |

## 🧪 挑战

1. 给 undefined_main.c 补上 calculate_checksum 的实现，并修改 Makefile 使目标通过。
2. 将 duplicate_symbol 改成一个文件定义、另一个文件只声明，重新构建。
3. 给 no_rpath 找到一种不使用 LD_LIBRARY_PATH 的正确修复方式。
4. 用 GDB 分别打开 no_debug 和 with_debug，在 internal_value 设断点，记录区别。

## 完成模块

你现在已经能从“源文件变成程序”的整条链路定位问题。下一步推荐进入项目中的 GDB、ASan、Valgrind 模块，把正确构建出的诊断能力真正用于 bug 排查。

