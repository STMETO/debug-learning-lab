# 实验 13：同一项目，Makefile 与 CMake —— 比较的是建模方式

> 🎯 **目标**：用两种构建系统生成完全相同的程序，知道该根据什么做技术选型。
>
> ⏱️ 约 20 分钟
>
> 📖 **前置**：[实验 12](../12-cmake-presets/)

## 先跑起来

Makefile 版本：

~~~bash
make
make run
~~~

CMake 版本：

~~~bash
cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cmake
./build/cmake/compare_demo
~~~

两个程序都应输出：

~~~text
mean = 25.0
~~~

## 同一张依赖图，两种表达

~~~text
main.c + stats.c + stats.h
              │
       compare_demo
~~~

| 维度 | Makefile | CMake |
|------|----------|-------|
| 直接描述 | 具体命令和文件规则 | target 及其使用要求 |
| 构建后端 | GNU Make | 可生成 Make、Ninja、IDE 工程 |
| 头文件依赖 | -MMD 自动生成 .d | 后端自动处理 |
| 配置表达 | 变量、条件、目标 | cache 变量、target、Preset |
| 小工具/脚本 | 非常直接 | 可能显得有额外仪式 |
| 跨平台/多库/IDE | 维护成本较高 | 更容易统一 |

## 不要做“语法数量”比较

Makefile 不是低级版 CMake，CMake 也不是更长的 Makefile。它们解决问题的层次不同：

- 需要精确控制少量 shell 命令、构建目标固定：Makefile 简洁清晰。
- 有多个库、可选依赖、跨平台、IDE 或 CI 配置：CMake 的 target 模型更容易扩展。
- CMake 生成 Unix Makefiles 时，最终仍是 make 在执行编译命令；你可以用 cmake --build --verbose 看到它。

## 观察增量构建

~~~bash
touch include/stats.h
make
cmake --build build/cmake --verbose
~~~

两套构建都会重新编译受头文件影响的源文件。Makefile 靠 .d 文件，CMake 把依赖扫描交给生成的构建后端。

## 🧪 试试看

1. 用 make -n 和 cmake --build build/cmake --verbose 对比真实 gcc 命令。
2. 给 stats.c 新增一个源文件，分别修改两套配置。
3. 把 CMake 生成器改为 Ninja，观察项目描述是否需要改变。

## 下一步

最后一个实验不再提供“正确构建”，而是给你一组真实错误：[实验 14：构建错误诊断](../14-build-diagnosis/)。

