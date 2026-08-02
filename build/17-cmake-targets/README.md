# 实验 17：现代 CMake 的 target 思维

> 🎯 **目标**：把源码、头文件路径、语言标准和警告绑定到具体 target。

```cmake
add_executable(target_demo)
target_sources(target_demo PRIVATE src/main.c src/calc.c)
target_include_directories(target_demo PRIVATE include)
target_compile_features(target_demo PRIVATE c_std_11)
target_compile_options(target_demo PRIVATE -Wall -Wextra)
```

每个 `target_*` 命令都先指定作用对象 `target_demo`。这样依赖来源清楚，不会让全局设置意外影响未来所有目标。

| 命令 | 描述 |
|------|------|
| `target_sources` | target 编译哪些源码 |
| `target_include_directories` | 去哪里找头文件 |
| `target_compile_features` | 需要什么语言特性/标准 |
| `target_compile_options` | 额外编译选项 |
| `target_compile_definitions` | 预处理宏 |

`PRIVATE` 目前可以理解为“只属于这个 target”。实验 19 学习库之后再解释传播语义。

配置并构建：

```bash
cmake -S . -B build
cmake --build build --verbose
./build/target_demo
```

从 verbose 输出验证 `-I` 和警告选项是否进入 GCC 命令。`target_compile_features(... c_std_11)` 表达最低语言要求；如果编译器默认标准已经满足，CMake 可能不额外添加 `-std`。CMake 命令名不区分大小写，但变量和 target 名区分；项目中应统一使用小写命令。

## 🧪 试试看

1. 删除 `target_include_directories`，确认错误发生在编译阶段。
2. 增加一个只给当前 target 使用的 `target_compile_definitions`。
3. 新建第二个可执行 target，确认第一个 target 的 PRIVATE 选项不会自动影响它。

下一步：[多目录 CMake](../18-cmake-directories/)。
