# 实验 15：第一份 CMakeLists.txt —— 逐行解释三个命令

> 🎯 **目标**：理解 CMake 的命令调用语法和最小项目结构。

```cmake
cmake_minimum_required(VERSION 3.21)
project(hello_cmake VERSION 1.0 LANGUAGES C)
add_executable(hello main.c)
```

## CMake 命令语法

```cmake
command(argument1 argument2 ...)
```

命令名大小写不敏感，但项目通常统一小写；参数用空格或分号形成列表。

### cmake_minimum_required

声明项目依赖的最低 CMake 版本，并选择相应策略行为。它应位于顶层文件开头。

### project

- `hello_cmake`：项目名称；
- `VERSION 1.0`：项目版本；
- `LANGUAGES C`：启用 C 编译器检测。

它会产生 `PROJECT_NAME`、`PROJECT_VERSION` 等变量。

### add_executable

创建名为 `hello` 的 executable target，并把 `main.c` 作为源码。第一个参数是 CMake target 名，也是默认产物名。

```bash
cmake -S . -B build
cmake --build build
./build/hello
```

## 🧪 试试看

1. 把 target 名改为 `app`。
2. 删除 `LANGUAGES C`，观察默认启用哪些语言。
3. 写错源文件名，判断错误发生在配置还是构建阶段。

下一步：[CMake 变量与条件](../16-cmake-language/)。

