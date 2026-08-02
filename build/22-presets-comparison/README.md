# 实验 22：Presets、双构建系统与综合诊断

> 🎯 **目标**：固化配置，对比 Makefile/CMake，并建立构建错误定位顺序。

## Presets

```bash
cmake --list-presets
cmake --preset debug
cmake --build --preset debug
./build/debug/cmake_app
```

Preset 把 generator、构建目录和 Cache 变量纳入版本控制。`base` 隐藏并保存公共设置，其他 Preset 继承它。

## 同一源码，两种描述

```bash
make
./make_app
cmake --preset debug
cmake --build --preset debug --verbose
```

Makefile 直接描述文件规则和命令；CMake 描述 target，再生成具体后端。小型固定任务用 Makefile 很直接，多平台、多库、IDE 和依赖传播更适合 CMake。

## 故障定位顺序

| 错误 | 首先检查 |
|------|----------|
| `missing separator` | Make recipe 前的 Tab |
| `No rule to make target` | 文件名、依赖、模式规则 |
| 找不到头文件 | `-I` / target include 目录 |
| `undefined reference` | 对象、库、链接顺序 |
| `multiple definition` | 重复实现或重复链接 |
| `.so` 找不到 | RUNPATH、安装位置、加载器 |
| GDB 无行号 | `-g` 与对应构建产物 |
| CMake 使用旧路径 | `CMakeCache.txt` 与构建目录 |

## 综合挑战

1. 给程序增加一个静态库，同时修改 Makefile 和 CMake；
2. 增加 Debug、Release、ASan 三种构建；
3. 修改头文件，比较两者增量构建；
4. 用 verbose 模式对比最终 GCC 命令；
5. 故意制造 undefined reference，并分别定位。

完成本模块后，你应该能阅读和维护中小型 C/C++ 项目的两种主流构建入口。

