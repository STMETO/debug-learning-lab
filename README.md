# Debug Learning Lab

一个面向 C/C++ 工程师和 Linux 工程师、通过动手实验学习调试、故障定位和性能分析的实践项目。

项目中的每个实验都是一个可以独立运行的小项目，包含示例源码、Makefile 和循序渐进的 README。学习重点不是记住命令，而是建立“现象 → 证据 → 定位 → 验证”的调试方法。

## 涵盖工具

| 工具 | 用途 | 状态 |
|------|------|------|
| [GDB](gdb/) | C/C++ 源码级调试器 | ✅ 12 个实验全部就绪 |
| [ASan](asan/) (AddressSanitizer) | 内存错误检测 | ✅ 10 个实验全部就绪 |
| [构建系统](build/)（Makefile / CMake） | C/C++ 构建、链接、测试与构建诊断 | ✅ 14 个实验全部就绪 |
| [strace](strace/) | 系统调用追踪 | ✅ 8 个实验全部就绪 |
| [ltrace](ltrace/) | 库函数调用追踪 | ✅ 6 个实验全部就绪 |
| [Valgrind](valgrind/) | 内存错误与泄漏检测 | ✅ 5 个实验全部就绪 |
| [perf](perf/) | 性能分析 (CPU profiling) | ✅ 13 个实验全部就绪 |
| [ftrace](ftrace/) | 内核函数追踪 | ✅ 10 个实验全部就绪 |
| [Perfetto](perfetto/) | 系统级全栈追踪与 SQL 分析 | ✅ 9 个实验全部就绪（含 UI 指南） |
| bpftrace / eBPF | 动态内核/用户态追踪 | ⏸️ WSL2 内核无 BTF 支持，跳过 |

当前共包含 **87 个实验**：GDB 12 个、ASan 10 个、构建系统 14 个、strace 8 个、ltrace 6 个、Valgrind 5 个、perf 13 个、ftrace 10 个、Perfetto 9 个。

## 项目结构

```
debug-learning-lab/
├── gdb/                  # GDB 调试实验（✅ 12 个实验）
├── asan/                 # ASan 内存检测实验（✅ 10 个实验）
├── build/                # Makefile / CMake 构建系统（✅ 14 个实验）
├── strace/               # strace 系统调用追踪（✅ 8 个实验）
├── ltrace/               # ltrace 库函数调用追踪（✅ 6 个实验）
├── valgrind/             # Valgrind 内存错误检测（✅ 5 个实验）
├── perf/                 # perf 性能分析（✅ 13 个实验）
├── ftrace/               # ftrace 内核函数追踪（✅ 10 个实验）
├── perfetto/             # Perfetto 全栈追踪（✅ 9 个实验）
├── lib/                  # 外部依赖或辅助资源（按模块说明使用）
└── ...
```

## 快速开始

### 环境要求

- Linux 系统（WSL2 / 虚拟机 / 原生均可）
- GCC 或 Clang（需带调试符号 `-g`）
- GDB（建议 10.0+）
- CMake 3.21+ 与 GNU Make（学习构建系统模块时需要；Ninja 可选）

```bash
# Ubuntu/Debian
sudo apt install gdb gcc build-essential cmake ninja-build

# 验证安装
gdb --version
```

### 开始第一个实验

```bash
cd gdb/01-hello-gdb/
make
gdb ./hello
```

然后跟着 [实验教程](gdb/01-hello-gdb/README.md) 一步步操作。

### 使用 Docker（无需本地安装工具）

项目提供了开箱即用的 Docker 镜像，预装所有调试工具，免去环境配置的麻烦。

```bash
# 构建镜像
docker build -t debug-learning-lab .

# 启动容器（基础模式，支持 gdb/strace/ltrace/valgrind）
docker run -it --cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
    -v $(pwd):/workspace/debug-learning-lab \
    debug-learning-lab

# 启动容器（全功能模式，额外支持 perf/bpftrace）
docker run -it --privileged \
    -v /lib/modules:/lib/modules:ro \
    -v $(pwd):/workspace/debug-learning-lab \
    debug-learning-lab
```

| 运行模式 | 需要的参数 | 可用工具 |
|----------|-----------|----------|
| 基础模式 | `--cap-add=SYS_PTRACE --security-opt seccomp=unconfined` | gdb, strace, ltrace, valgrind |
| 全功能模式 | `--privileged -v /lib/modules:/lib/modules:ro` | 以上全部 + perf, bpftrace |

> ⚠️ **perf 模块注意**：perf 需要 WSL2 原生 shell 环境，Docker 容器内 `perf_event_open()` 系统调用受限。详见 [perf 模块 README](perf/README.md)。

容器启动后会自动进入实验目录，所有工具可直接使用。

## 学习路线

1. **构建系统** — 先理解编译、链接、调试符号与 Debug/Release 构建
2. **GDB** — 从源码级调试开始，建立最直观的调试心智模型
3. **ASan** — 理解内存错误类型及其检测原理
4. **strace / ltrace** — 从系统调用和库调用层面观察程序行为
5. **Valgrind** — 不重编译的内存检测，未初始化值等独家能力
6. **perf** — 从 CPU 角度分析性能瓶颈，深入内核事件追踪
7. **ftrace** — 内核函数全量追踪，微秒级延迟分析
8. **Perfetto** — 把内核事件、CPU 采样和应用埋点放到同一条时间线上分析

构建系统模块与 GDB、ASan 的联系最紧密；如果你已经完成它们，也可以按需要交叉学习。

## 后续教程规划

优先级用于安排后续开发顺序，不代表学习者必须一次完成全部内容。

| 状态/优先级 | 教程主题 | 推荐范围 | 为什么排在这里 |
|-------------|----------|----------|----------------|
| ✅ 已实现 | [**Makefile 与 CMake**](build/) | 编译、链接、Debug/Release、Sanitizer、测试、安装、Preset 与构建诊断 | 14 个实验已覆盖从单文件到库、工程化配置和真实构建错误排查 |
| P0 | **Linux 调试必备命令** | `find`、`grep`、`awk`、`sed`、管道重定向、权限、进程和环境变量 | 这些命令是运行实验、收集现场信息和组合调试工具的基础能力 |
| P1 | **ELF、符号与动态链接工具** | `file`、`ldd`、`readelf`、`nm`、`objdump`、`strings`、`addr2line` | 与 GDB、core dump、ASan、动态库问题直接衔接，能解释二进制“里面有什么” |
| P1 | **Git 调试工作流** | `log`、`diff`、`blame`、`bisect`、标签和回退 | 解决“什么时候引入了 bug”和“哪个提交改变了行为”，把单次定位扩展到版本历史 |
| P2 | **Linux 网络排障** | `ss`、`ip`、`lsof`、`tcpdump`、过滤表达式、抓包与程序日志对照 | 适合在掌握 strace 后继续学习连接、协议和线上网络问题定位 |
| P2 | **Protobuf 调试** | `.proto`、`protoc`、C++ 生成代码、字段兼容性、二进制消息抓取与解析 | 依赖构建系统和基础二进制知识，适合放在工程化和网络排障之后 |

构建系统模块已经为后续 `readelf`、`nm`、`ldd` 与 Protobuf 教程准备了构建基础。Linux 常见命令建议作为下一项 P0 内容，先服务于后续实验，不建议一开始写成覆盖所有命令的大而全手册。

## 贡献

欢迎提交新的实验、修复错误或改进文档。每个实验目录应保持独立，包含：
- `README.md` — 实验目标、步骤、思考题
- 示例源码
- Makefile（如适用）
