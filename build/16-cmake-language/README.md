# 实验 16：CMake 变量、列表、option 与 if

> 🎯 **目标**：掌握构建配置真正需要的 CMake 语言基础。

## 变量

```cmake
set(APP_NAME language_demo)
message(STATUS "APP_NAME=${APP_NAME}")
```

`${APP_NAME}` 在 CMake 配置阶段展开。未加引号的多个参数会形成列表：

```cmake
set(APP_SOURCES main.c helper.c)
add_executable(app ${APP_SOURCES})
```

CMake 的列表本质上以分号分隔。

## option 与 Cache

```cmake
option(ENABLE_WARNINGS "Enable compiler warnings" ON)
```

它创建布尔 Cache 变量，可以从命令行配置：

```bash
cmake -S . -B build -DENABLE_WARNINGS=OFF
```

Cache 保存在 `build/CMakeCache.txt`，后续配置会继续使用。配置看似“不听话”时，应检查 Cache 或使用新的构建目录。

## 条件

```cmake
if(ENABLE_WARNINGS)
    target_compile_options(language_demo PRIVATE -Wall -Wextra)
endif()
```

这里条件判断发生在配置阶段，不是程序运行阶段。

## 🧪 试试看

1. 用 ON/OFF 配置，比较 verbose 编译命令。
2. `message(STATUS "${APP_SOURCES}")` 查看列表。
3. 在同一 build 目录重新配置变量，检查 Cache。

下一步：[现代 CMake target](../17-cmake-targets/)。

