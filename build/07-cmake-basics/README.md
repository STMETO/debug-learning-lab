# 实验 07：CMake 基础 —— 先配置，再构建

> 🎯 **目标**：理解 CMake 的配置和构建两步，掌握 out-of-source 构建。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 06](../06-makefile-engineering/)

## 先跑起来

~~~bash
cmake -S . -B build
cmake --build build
./build/hello_cmake
~~~

不要在源码目录直接执行 cmake .。推荐始终使用：

~~~text
cmake -S <源码目录> -B <构建目录>
~~~

这样 CMakeCache.txt、生成的 Makefile/Ninja 文件、对象文件都会留在 build 中；删除构建产物只需要删除 build。

## CMake 做了两件事

~~~text
cmake -S . -B build
    读取 CMakeLists.txt，检测编译器，生成构建后端

cmake --build build
    调用生成的后端（Make 或 Ninja）来真正编译
~~~

查看本机选择的生成器：

~~~bash
grep CMAKE_GENERATOR build/CMakeCache.txt
cmake --build build --verbose
~~~

第二条命令会显示实际 gcc 命令。CMake 没有取代编译器；它只是稳定地生成和驱动这些命令。

## 读懂 CMakeLists.txt

~~~cmake
project(cmake_basics LANGUAGES C)
add_executable(hello_cmake main.c)
target_compile_options(hello_cmake PRIVATE -Wall -Wextra -Wpedantic)
~~~

这里的核心是 target：hello_cmake 是一个真实的构建产物。现代 CMake 的选项、头文件路径和库依赖，应尽量附着到具体 target，而不是写全局 include_directories 或 add_definitions。

## 换成 Ninja

~~~bash
cmake -S . -B build-ninja -G Ninja
cmake --build build-ninja
./build-ninja/hello_cmake
~~~

同一份 CMakeLists.txt 可以生成 Makefile、Ninja 或 IDE 工程；这正是 CMake 的价值。

## 🧪 试试看

1. 删除 build 后重新配置，为什么源码不会受影响？
2. 若使用 Unix Makefiles，执行 cmake --build build --target help，看看后端可提供哪些目标；Ninja 可用 ninja -C build -t targets 查看。
3. 在 CMakeLists.txt 加入 -g 后重新构建，用 gdb 打开程序。

## 下一步

下一实验学习 [实验 08：target 与作用域](../08-cmake-targets/)。这是 CMake 最重要也最容易写错的部分。
