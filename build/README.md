# Makefile 与 CMake：从语法到工程实践

本模块只讲 Makefile 和 CMake。它不重复教授 Linux 命令或 C/C++ 语法，但会从两种构建系统各自最基础的语法开始。

## 前置知识

开始前需要：

- 会使用基本 Linux 命令和 shell；
- 能读懂简单 C/C++ 程序；
- 知道源文件、头文件、目标文件、静态库、动态库和可执行文件的区别；
- 理解预处理、编译、汇编、链接的大致含义；
- 用过 `gcc -c main.c -o main.o` 和 `gcc main.o -o app`；
- 大致知道 `-I`、`-L`、`-l`、`-g`、`-O0` 的用途。

## 学习路线

### Makefile 基础（01–06）

| # | 实验 | 新知识 |
|---|------|--------|
| [01](01-first-makefile/) | 第一份 Makefile | `make`、默认目标 |
| [02](02-rule-syntax/) | 规则语法 | target、prerequisite、recipe、Tab |
| [03](03-incremental-build/) | 增量构建 | 时间戳、依赖、`-B` |
| [04](04-variables/) | 变量 | `= := ?= +=`、命令行覆盖 |
| [05](05-automatic-variables/) | 自动变量 | `$@`、`$<`、`$^`、`$?` |
| [06](06-phony-targets/) | 操作目标 | `.PHONY`、all、run、clean、help |

### Makefile 工程实践（07–13）

| # | 实验 | 新知识 |
|---|------|--------|
| [07](07-multifile/) | 多文件项目 | 分别编译、统一链接 |
| [08](08-pattern-rules/) | 模式规则 | `%`、wildcard、patsubst |
| [09](09-auto-dependencies/) | 头文件依赖 | `-MMD -MP`、`.d`、`-include` |
| [10](10-directories-parallel/) | 目录与并行 | build 目录、`|`、`-j`、`-n` |
| [11](11-build-modes/) | 构建模式 | Debug、Release、ASan |
| [12](12-libraries/) | 库 | `.a`、`.so`、链接顺序、rpath |
| [13](13-make-engineering/) | 工程化 | 测试、公共规则、反模式 |

### CMake（14–22）

| # | 实验 | 新知识 |
|---|------|--------|
| [14](14-cmake-model/) | CMake 模型 | 配置、生成、构建、generator |
| [15](15-first-cmake/) | 第一份 CMakeLists | project、add_executable |
| [16](16-cmake-language/) | CMake 语法 | 变量、列表、option、if、Cache |
| [17](17-cmake-targets/) | target 思维 | sources、include、options |
| [18](18-cmake-directories/) | 多目录 | add_subdirectory |
| [19](19-cmake-libraries/) | 库与传播 | STATIC/SHARED、PUBLIC/PRIVATE/INTERFACE |
| [20](20-cmake-configurations/) | 构建配置 | Debug、Release、ASan |
| [21](21-cmake-test-install/) | 测试与安装 | CTest、install、find_package |
| [22](22-presets-comparison/) | Presets 与综合对比 | Preset、双构建、故障诊断 |

建议按顺序学习。01–06 每节只引入少量 Makefile 语法，后面的实验会反复使用它们。
