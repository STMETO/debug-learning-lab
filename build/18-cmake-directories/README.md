# 实验 18：多目录项目 —— `add_subdirectory`

> 🎯 **目标**：理解顶层、库目录和应用目录各自负责什么。

```text
CMakeLists.txt        项目入口、全局最低版本
lib/CMakeLists.txt    定义 calc 库
app/CMakeLists.txt    定义应用并链接 calc
```

顶层：

```cmake
add_subdirectory(lib)
add_subdirectory(app)
```

CMake 按顺序处理子目录。`lib` 先创建 target `calc`，`app` 随后可以引用它。

`add_subdirectory` 不是 shell 的 `cd`，也不是启动第二次 CMake；所有子目录共同构成一张 target 图。

子目录中的相对源码路径以该子目录为基准。`${CMAKE_CURRENT_SOURCE_DIR}` 表示当前正在处理的源码目录；顶层的 `${CMAKE_SOURCE_DIR}` 则始终指向整个项目根目录。可复用子项目通常优先使用前者。

```bash
cmake -S . -B build
cmake --build build
./build/app/directory_demo
```

库把头文件目录声明为 PUBLIC，因此应用链接库时自动获得 include 路径。下一实验完整解释。

## 🧪 试试看

1. 注释掉 `add_subdirectory(lib)`，观察应用链接 `calc` 时的错误。
2. 在 `app/CMakeLists.txt` 中打印 `CMAKE_CURRENT_SOURCE_DIR`。
3. 再添加一个链接 `calc` 的可执行程序，确认不需要重复写 include 路径。

下一步：[库与依赖传播](../19-cmake-libraries/)。
