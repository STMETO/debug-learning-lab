# 实验 14：CMake、Make 与 Ninja —— 三个阶段不要混淆

> 🎯 **目标**：理解配置、生成、构建和 generator，知道 CMake 不是编译器。

```text
CMakeLists.txt
      │ cmake -S . -B build
      ▼
配置：检测编译器、平台、选项和依赖
生成：写出 Makefile 或 Ninja 构建文件
      │ cmake --build build
      ▼
构建后端调用 GCC/Clang
      ▼
对象文件、库、可执行程序
```

## 先跑起来

```bash
cmake -S . -B build
cmake --build build
./build/model_demo
```

`-S .` 指定源码目录，`-B build` 指定构建目录。推荐始终分离源码与产物。

查看生成器：

```bash
grep CMAKE_GENERATOR build/CMakeCache.txt
cmake --build build --verbose
```

Verbose 输出会显示真实 GCC 命令。

## 换一个生成器

```bash
cmake -S . -B build-ninja -G Ninja
cmake --build build-ninja
```

同一份 CMakeLists 可以生成 Makefile、Ninja 或 IDE 工程。CMake 描述项目，generator 决定具体后端格式。

## 常见错误

- 在源码目录执行 `cmake .`，生成文件污染源码；
- 修改生成的 Makefile，而不是 CMakeLists；
- 把配置错误和编译错误混为一谈；
- 在已有构建目录中切换 generator。

下一步：[第一份 CMakeLists](../15-first-cmake/)。

