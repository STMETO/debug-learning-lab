# 实验 08：CMake target 与作用域 —— 依赖会自己携带使用说明

> 🎯 **目标**：掌握 add_library、target_link_libraries 以及 PUBLIC/PRIVATE/INTERFACE 的含义。
>
> ⏱️ 约 30 分钟
>
> 📖 **前置**：[实验 07](../07-cmake-basics/)

## 先跑起来

~~~bash
cmake -S . -B build
cmake --build build
./build/target_demo
~~~

这里有两个 target：

~~~text
format（静态库） ─────→ target_demo（可执行文件）
     带着 include/ 和 C11 要求
~~~

main.c 能够 include format.h，不是因为项目全局设置了 -Iinclude，而是因为 target_demo 链接 format 时自动继承了 format 的 PUBLIC 使用要求。

## PUBLIC、PRIVATE、INTERFACE

~~~cmake
target_include_directories(format PUBLIC include)
target_compile_options(format PRIVATE -Wall -Wextra)
target_link_libraries(target_demo PRIVATE format)
~~~

| 关键字 | format 自己编译时使用 | 使用 format 的 target 也得到 |
|--------|-----------------------|-------------------------------|
| PRIVATE | 是 | 否 |
| PUBLIC | 是 | 是 |
| INTERFACE | 否 | 是 |

因此：

- 库的 public 头文件目录必须是 PUBLIC；
- 只想检查库自身实现的警告选项通常是 PRIVATE；
- 纯头文件库可以用 INTERFACE library，它本身没有 .c 可以编译。

## 看见真实编译命令

~~~bash
cmake --build build --verbose
~~~

main.c 的编译命令中也会带上 -I.../include。试着把 PUBLIC 改成 PRIVATE，清理后重新构建：

~~~bash
rm -rf build
cmake -S . -B build
cmake --build build
~~~

预期会报 format.h 找不到。这个错误说明 include 路径没有作为库 API 的一部分传播。

## 常见反模式

~~~cmake
include_directories(include)     # 全局状态，依赖来源不清楚
add_definitions(-DDEBUG)         # 全局状态，影响未来所有 target
~~~

它们不是永远不能用，但在新项目里，优先写 target_include_directories 和 target_compile_definitions。这样从 CMakeLists.txt 就能看出每个 target 的真实需求。

## 🧪 试试看

1. 新建一个 header-only target，给它添加 INTERFACE include 目录。
2. 把 c_std_11 从 PUBLIC 改成 PRIVATE，思考库的头文件若使用 C11 特性会发生什么。
3. 在 format.c 中引入一个未使用变量，观察 PRIVATE 警告选项只影响谁。

## 下一步

下一实验把 target 扩展到静态库和动态库：[实验 09：CMake 库项目](../09-cmake-libraries/)。

