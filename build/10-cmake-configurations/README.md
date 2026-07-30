# 实验 10：CMake 调试配置 —— 同一项目，明确的诊断变体

> 🎯 **目标**：构建 Debug、Release 和 ASan 版本，生成 compile_commands.json，并理解单配置/多配置生成器差异。
>
> ⏱️ 约 30 分钟
>
> 📖 **前置**：[实验 09](../09-cmake-libraries/)

## 先跑起来

~~~bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-debug
gdb -q ./build-debug/config_demo

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build-asan
./build-asan/config_demo
~~~

最后一条应该报告 heap-buffer-overflow。源码中的 <= 是故意的 bug。

## CMAKE_BUILD_TYPE 做什么？

在 Unix Makefiles 和 Ninja 这类单配置生成器中，配置时必须选一种：

~~~text
Debug            通常是 -g，低优化
Release          通常是高优化并定义 NDEBUG
RelWithDebInfo   优化 + 调试信息，排查线上问题常用
~~~

不同配置必须使用不同构建目录。不能在同一个 build 中先配置 Debug、再改成 Release，期待所有旧对象都正确转换；最稳妥的规则是“一种配置，一个构建目录”。

Visual Studio、Xcode 是多配置生成器，构建时再通过 --config Debug 选择。教程命令面向 Linux 上最常见的单配置生成器。

## 为什么 ASan 有两处选项？

~~~cmake
target_compile_options(config_demo PRIVATE -fsanitize=address ...)
target_link_options(config_demo PRIVATE -fsanitize=address)
~~~

编译阶段需要插桩，链接阶段需要 ASan runtime。只加其中一处都会得到不完整或错误的构建。

## compile_commands.json 有什么用？

启用 CMAKE_EXPORT_COMPILE_COMMANDS 后：

~~~bash
head -n 5 build-debug/compile_commands.json
~~~

它记录每个源文件的真实编译命令。clangd、clang-tidy、代码索引器以及很多 IDE 都依赖它，而不是猜测你的 include 路径和宏定义。

## 🧪 试试看

1. 把 <= 改成 <，重新构建 ASan 版本。
2. 用 cmake --build build-debug --verbose 比较 Debug 与 Release 的编译参数。
3. 配置 RelWithDebInfo，放进 GDB 看它和 Debug 的单步体验差异。

## 下一步

下一实验让构建系统继续承担测试和安装：[实验 11：CTest 与 install](../11-cmake-testing-install/)。

