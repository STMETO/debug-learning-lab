# 实验 09：CMake 库项目 —— target 名称不等于产物文件名

> 🎯 **目标**：用 CMake 构建静态库、动态库和两个使用它们的程序，检查运行时库搜索路径。
>
> ⏱️ 约 25 分钟
>
> 📖 **前置**：[实验 08](../08-cmake-targets/)

## 先跑起来

~~~bash
cmake -S . -B build
cmake --build build
./build/bin/app_static
./build/bin/app_shared
ldd ./build/bin/app_shared
~~~

产物布局由 CMakeLists.txt 中的三个 OUTPUT_DIRECTORY 变量统一控制：

~~~text
build/
├── bin/app_static
├── bin/app_shared
├── lib/libmathlab.a
└── lib/libmathlab.so
~~~

## 一个源文件，两种库

~~~cmake
add_library(math_static STATIC src/mathlib.c)
add_library(math_shared SHARED src/mathlib.c)
set_target_properties(math_static PROPERTIES OUTPUT_NAME mathlab)
~~~

math_static 是 CMake 内部的 target 名，生成的文件是 libmathlab.a。把 target 名和文件名分开有利于表达意图，也避免不同平台的 .so/.dylib/.dll 后缀污染项目逻辑。

## 动态库为什么能运行？

app_shared 依赖 libmathlab.so。BUILD_RPATH 告诉动态加载器，在构建树中运行时去可执行文件相对位置的 ../lib 搜索：

~~~cmake
set_target_properties(app_shared PROPERTIES BUILD_RPATH "$ORIGIN/../lib")
~~~

先用 ldd 确认路径，再试着把 libmathlab.so 暂时改名运行 app_shared。你会得到“cannot open shared object file”。这说明链接已经成功，失败发生在程序启动阶段。

## 🧪 试试看

1. 运行 cmake --build build --target math_shared，只构建动态库。
2. 用 nm -D build/lib/libmathlab.so 查看导出符号。
3. 删除 BUILD_RPATH 并重建，使用 LD_LIBRARY_PATH=build/lib 临时运行 app_shared。

## 下一步

下一实验将配置从命令行习惯升级为可复现的 CMake 参数：[实验 10：调试配置](../10-cmake-configurations/)。

