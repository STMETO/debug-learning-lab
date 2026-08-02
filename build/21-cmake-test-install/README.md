# 实验 21：CTest、install 与依赖入口

> 🎯 **目标**：把测试和可检查的安装布局纳入构建。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/stage"
find stage -type f
```

CTest 是测试调度器，不是断言框架。`add_test` 注册一个可执行命令，CI 可以统一运行 `ctest`。

`include(CTest)` 会提供 `BUILD_TESTING` 选项并启用测试。真实项目可把测试 target 放入 `if(BUILD_TESTING)`，再用 `-DBUILD_TESTING=OFF` 跳过测试构建。

安装时，库在源码树和安装树需要不同 include 路径：

```cmake
$<BUILD_INTERFACE:源码路径>
$<INSTALL_INTERFACE:include>
```

这是 generator expression，会在不同上下文生成不同值。

查找第三方依赖时优先使用 imported target：

```cmake
find_package(Threads REQUIRED)
target_link_libraries(app PRIVATE Threads::Threads)
```

Imported target 会携带正确的库、头文件和平台选项，比手写路径可靠。

`cmake --install` 只复制 `install(...)` 明确声明的内容。`--prefix "$PWD/stage"` 把实验安装到当前目录，避免写入系统路径，也便于检查最终布局。

## 🧪 试试看

1. 故意让测试断言失败，使用 `--output-on-failure` 查看输出。
2. 执行 `ctest --test-dir build -V`，观察完整测试命令。
3. 删除一条 `install` 规则，清空 `stage` 后重新安装并比较文件树。

下一步：[Presets 与综合对比](../22-presets-comparison/)。
