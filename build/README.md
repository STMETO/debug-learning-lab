# 构建系统：Makefile 与 CMake

## 写给谁？

这个模块面向已经会编译 C/C++ 程序、并希望理解“程序为什么能被调试”的学习者。它和 GDB、ASan、Valgrind 模块紧密相连：调试器能否看到源码和变量，Sanitizer 能否正确运行，往往首先取决于构建配置。

> 构建系统不是把一条 gcc 命令藏起来；它是在维护一张“源码、头文件、库、编译选项、产物”之间的依赖图。

## 你会获得什么

完成本模块后，你应当能够：

- 看懂从 .c 到 .o、再到可执行文件和动态库的过程；
- 用 Makefile 管理自动依赖、库、Debug/Release/ASan 构建；
- 用现代 CMake 的 target 模型组织中小型 C/C++ 项目；
- 用 CTest、install、Presets 建立可复现的构建流程；
- 遇到 undefined reference、缺失符号、找不到动态库时，知道从哪一层排查。

## 学习路线图

### 🟢 阶段一：先理解构建发生了什么（01–03）

| # | 实验 | 你会学到 | 时长 |
|---|------|----------|------|
| [01](01-from-source/) | 从源码到程序 | 预处理、编译、汇编、链接和调试符号 | ~20min |
| [02](02-makefile-basics/) | Makefile 基础 | target、依赖、变量、自动变量与增量构建 | ~20min |
| [03](03-makefile-dependencies/) | 自动头文件依赖 | 模式规则、.d 文件、-MMD -MP 与并行构建 | ~25min |

### 🟡 阶段二：Makefile 工程实践（04–06）

| # | 实验 | 你会学到 | 时长 |
|---|------|----------|------|
| [04](04-makefile-libraries/) | 静态库与动态库 | ar、-l、rpath 与运行时加载 | ~30min |
| [05](05-makefile-debug/) | 可调试构建 | Debug/Release/ASan、优化和符号 | ~25min |
| [06](06-makefile-engineering/) | 小型工程组织 | build 目录、test、help 和非递归 Make | ~30min |

### 🟠 阶段三：CMake 的 target 思维（07–12）

| # | 实验 | 你会学到 | 时长 |
|---|------|----------|------|
| [07](07-cmake-basics/) | CMake 入门 | 配置阶段、生成阶段、out-of-source 构建 | ~20min |
| [08](08-cmake-targets/) | target 与作用域 | PUBLIC/PRIVATE/INTERFACE 与依赖传播 | ~30min |
| [09](09-cmake-libraries/) | CMake 库项目 | 静态库、动态库、链接与 rpath | ~25min |
| [10](10-cmake-configurations/) | 调试配置 | build type、Sanitizer、compile_commands.json | ~30min |
| [11](11-cmake-testing-install/) | 测试与安装 | CTest、install、可消费的库 | ~30min |
| [12](12-cmake-presets/) | Presets 与工具链 | 可复现配置、Ninja、工具链文件 | ~25min |

### 🔴 阶段四：把知识用于排障（13–14）

| # | 实验 | 你会学到 | 时长 |
|---|------|----------|------|
| [13](13-make-cmake-compare/) | 同项目双构建 | Makefile 与 CMake 的能力边界和取舍 | ~20min |
| [14](14-build-diagnosis/) | 构建错误诊断 | undefined reference、重复定义、符号缺失、动态库问题 | ~35min |

## 环境要求

- GCC 或 Clang，建议 GCC 9+；
- GNU Make 4+；
- CMake 3.21+；实验 12 另外推荐 Ninja；
- 可选：GDB、readelf、nm、ldd（后续二进制工具教程会专门讲解）。

~~~bash
# Ubuntu/Debian
sudo apt install build-essential cmake ninja-build gdb

gcc --version
make --version
cmake --version
~~~

## 两个工具的关系

~~~text
源文件 + 头文件 + 库 + 编译选项
                 │
        ┌────────┴────────┐
        │                 │
    Makefile          CMakeLists.txt
        │                 │
     make            cmake -S -B
        │                 │
        └───── 具体构建后端 ─────┘
                 │
        Make / Ninja / IDE 工程
                 │
            可执行文件、库、测试
~~~

Make 是“执行规则”的构建工具；CMake 是“描述项目”的生成器。CMake 可以生成 Makefile，也可以生成 Ninja 或 IDE 工程。两者并非替代关系，但项目通常只维护其中一种作为主要入口。

## 开始

从 [实验 01：从源码到程序](01-from-source/) 开始。先亲手执行一次 gcc 的四个阶段，后面的 Makefile 和 CMake 才不会变成需要死记的语法。
