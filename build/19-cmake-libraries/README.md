# 实验 19：库与依赖传播 —— PUBLIC/PRIVATE/INTERFACE

> 🎯 **目标**：构建静态库和动态库，并准确选择使用要求作用域。

| 关键字 | target 自己使用 | 链接它的消费者获得 |
|--------|-----------------|--------------------|
| `PRIVATE` | 是 | 否 |
| `PUBLIC` | 是 | 是 |
| `INTERFACE` | 否 | 是 |

库编译自身需要 `include/`，消费者包含 `mathlib.h` 也需要它，因此 include 目录是 PUBLIC。

警告选项只用于检查库实现，不要求消费者继承，因此是 PRIVATE。

INTERFACE 常用于纯头文件库或只传播编译要求的 target：

```cmake
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE -Wall -Wextra)
```

```bash
cmake -S . -B build
cmake --build build
./build/app_static
./build/app_shared
```

将 include 目录改成 PRIVATE，重新使用新构建目录，观察应用为何找不到头文件。

传播的是 target 的“使用要求”，不是把编译选项复制进源码。应用通过 `target_link_libraries(app_static PRIVATE math_static)` 建立图上的边，CMake 沿这条边收集 `math_static` 的 PUBLIC 和 INTERFACE 属性。

修改 CMakeLists 后建议重新执行配置命令；CMake 通常会自动重新生成，但使用一个全新的构建目录最容易排除旧 Cache 干扰。

## 🧪 试试看

1. 用 verbose 构建确认应用获得库的 include 路径，却没有获得库的 PRIVATE 警告选项。
2. 创建一个 INTERFACE target，向两个应用统一传播警告选项。
3. 用 `ldd` 比较静态版和动态版应用。

下一步：[CMake 构建配置](../20-cmake-configurations/)。
