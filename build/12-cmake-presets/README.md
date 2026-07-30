# 实验 12：CMake Presets —— 把“如何构建”也纳入版本控制

> 🎯 **目标**：用 CMakePresets.json 固化 Debug、Release、ASan 配置，并认识工具链文件的边界。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 11](../11-cmake-testing-install/)

## 先跑起来

~~~bash
cmake --list-presets
cmake --preset debug
cmake --build --preset debug
./build/debug/preset_demo

cmake --preset asan
cmake --build --preset asan
./build/asan/preset_demo
~~~

Preset 把原本容易散落在 shell 历史中的参数：

~~~bash
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug ...
~~~

收敛成一个可读、可审查、可提交的名字：

~~~bash
cmake --preset debug
~~~

## 读懂继承关系

~~~text
base
 ├── debug
 │    └── asan
 └── release
~~~

base 统一指定 Ninja、构建目录格式和 compile_commands.json；debug 和 release 只表达配置差异；asan 继承 debug，再打开 ENABLE_ASAN。这样不会复制三遍公共配置。

## 为什么 Presets 对调试很重要？

“请用 -O1 -g 并打开 ASan 复现”如果只存在聊天记录里，很难重现。Presets 让构建配置成为项目 API：开发者、CI、IDE 都可调用同一个名字。

## 工具链文件是什么？

[cmake/arm-none-eabi.cmake](cmake/arm-none-eabi.cmake) 是交叉编译的最小示例。它回答“用哪个编译器、目标系统是什么”，而不是“本项目有哪些源文件”。真正使用前必须先安装对应工具链：

~~~bash
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
~~~

交叉编译出的 ARM 程序不能直接在 x86 Linux 上运行；需要开发板、模拟器或远程调试器。因此本实验只阅读工具链文件，不要求你执行该命令。

## 🧪 试试看

1. 给 base 新增一项警告配置，观察三个 Preset 是否都继承。
2. 删除 build/asan 后，用 cmake --preset asan 重新生成。
3. 打开 build/debug/compile_commands.json，确认它属于具体 Preset。

## 下一步

下一实验用完全相同的源码分别由 Makefile 和 CMake 构建：[实验 13：双构建系统对比](../13-make-cmake-compare/)。

