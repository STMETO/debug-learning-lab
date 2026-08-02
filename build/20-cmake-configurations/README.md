# 实验 20：Debug、Release 与 Sanitizer

> 🎯 **目标**：理解单配置生成器的 `CMAKE_BUILD_TYPE` 和 target 链接选项。

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build-asan
./build-asan/config_demo
```

Unix Makefiles/Ninja 通常是单配置生成器：一种配置对应一个构建目录。Visual Studio/Xcode 常在构建时用 `--config Debug` 选择。

`CMAKE_BUILD_TYPE` 是配置阶段写入 Cache 的输入。同一构建目录再次配置时会沿用旧值，因此示例为 Debug、Release、ASan 使用不同目录，避免“这次到底用了什么选项”的歧义。

ASan 既要编译插桩，也要链接 runtime：

```cmake
target_compile_options(config_demo PRIVATE -fsanitize=address)
target_link_options(config_demo PRIVATE -fsanitize=address)
```

生成编译数据库：

```bash
cmake -S . -B build-debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

`compile_commands.json` 供 clangd、clang-tidy 和 IDE 使用。

`buggy.c` 故意包含堆越界。ASan 版本运行后报告错误并返回非零状态是预期结果。普通 Debug/Release 版本没有检测报告，并不代表程序正确。

## 🧪 试试看

1. 分别 verbose 构建三个目录，比较编译和链接选项。
2. 执行 `cmake -S . -B build-asan -DENABLE_ASAN=OFF`，观察 Cache 值如何被更新。
3. 打开 `CMakeCache.txt` 查找 `CMAKE_BUILD_TYPE` 和 `ENABLE_ASAN`。

下一步：[CTest 与安装](../21-cmake-test-install/)。
