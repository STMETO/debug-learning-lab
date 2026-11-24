# Debug Learning Lab

一个面向 C/C++ 工程师和 Linux 工程师、通过动手实验学习调试、故障定位和性能分析的实践项目。

项目中的每个实验都是一个可以独立运行的小项目，包含示例源码、Makefile 和循序渐进的 README。学习重点不是记住命令，而是建立“现象 → 证据 → 定位 → 验证”的调试方法。

## 涵盖工具

| 工具 | 用途 | 状态 |
|------|------|------|
| [GDB](gdb/) | C/C++ 源码级调试器 | ✅ 12 个实验全部就绪 |
| [ASan](asan/) (AddressSanitizer) | 内存错误检测 | ✅ 10 个实验全部就绪 |
| [构建系统](build/)（Makefile / CMake） | C/C++ 构建、链接、测试与构建诊断 | ✅ 22 个实验全部就绪 |
| [ELF 与链接](elf/) | 二进制结构、符号、静态/动态链接与加载器诊断 | ✅ 12 个实验全部就绪 |
| [Linux 现场排障](linux/) | 进程、资源、procfs、日志、文件与网络现场收集 | ✅ 12 个实验全部就绪 |
| [网络排障](network/)（tcpdump） | 路由、DNS、TCP/UDP、HTTP 与抓包分析 | ✅ 12 个实验全部就绪 |
| [Git](git/) | 版本控制基础、历史追踪与回归定位 | ✅ 12 个实验全部就绪 |
| [Protobuf](protobuf/) | Schema、C++ 代码生成、序列化与兼容性诊断 | ✅ 12 个实验全部就绪 |
| [strace](strace/) | 系统调用追踪 | ✅ 8 个实验全部就绪 |
| [ltrace](ltrace/) | 库函数调用追踪 | ✅ 6 个实验全部就绪 |
| [Valgrind](valgrind/) | 内存错误与泄漏检测 | ✅ 5 个实验全部就绪 |
| [perf](perf/) | 性能分析 (CPU profiling) | ✅ 13 个实验全部就绪 |
| [ftrace](ftrace/) | 内核函数追踪 | ✅ 10 个实验全部就绪 |
| [Perfetto](perfetto/) | 系统级全栈追踪与 SQL 分析 | ✅ 9 个实验全部就绪（含 UI 指南） |
| bpftrace / eBPF | 动态内核/用户态追踪 | ⏸️ WSL2 内核无 BTF 支持，跳过 |

当前共包含 **155 个实验**：GDB 12 个、ASan 10 个、构建系统 22 个、ELF 与链接 12 个、Linux 现场排障 12 个、网络排障 12 个、Git 12 个、Protobuf 12 个、strace 8 个、ltrace 6 个、Valgrind 5 个、perf 13 个、ftrace 10 个、Perfetto 9 个。

## 项目结构

```
debug-learning-lab/
├── gdb/                  # GDB 调试实验（✅ 12 个实验）
├── asan/                 # ASan 内存检测实验（✅ 10 个实验）
├── build/                # Makefile / CMake 构建系统（✅ 22 个实验）
├── elf/                  # ELF、符号与动态链接（✅ 12 个实验）
├── linux/                # Linux 现场排障与命令行工具（✅ 12 个实验）
├── network/              # 网络排障与 tcpdump（✅ 12 个实验）
├── git/                  # Git 基础与版本定位（✅ 12 个实验）
├── protobuf/             # Protobuf C++ 与协议诊断（✅ 12 个实验）
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
sudo apt install gdb gcc build-essential cmake ninja-build binutils file

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
2. **ELF 与链接** — 看懂符号、调试信息、库依赖与动态加载过程
3. **Linux 现场排障** — 用命令行收集进程、资源、文件、日志和端口证据
4. **网络排障** — 用 `ss`、tcpdump、DNS 和 HTTP 证据定位连接问题
5. **Git** — 用历史、blame 和 bisect 定位行为从哪个提交开始改变
6. **GDB** — 从源码级调试开始，建立最直观的调试心智模型
7. **ASan** — 理解内存错误类型及其检测原理
8. **strace / ltrace** — 从系统调用和库调用层面观察程序行为
9. **Valgrind** — 不重编译的内存检测，未初始化值等独家能力
10. **perf** — 从 CPU 角度分析性能瓶颈，深入内核事件追踪
11. **ftrace** — 内核函数全量追踪，微秒级延迟分析
12. **Perfetto** — 把内核事件、CPU 采样和应用埋点放到同一条时间线上分析

构建系统模块与 GDB、ASan 的联系最紧密；如果你已经完成它们，也可以按需要交叉学习。

## 后续教程规划

优先级用于安排后续开发顺序，不代表学习者必须一次完成全部内容。

| 状态/优先级 | 教程主题 | 推荐范围 | 为什么排在这里 |
|-------------|----------|----------|----------------|
| ✅ 已实现 | [**Makefile 与 CMake**](build/) | 规则与变量、增量构建、库、Debug/Release、Sanitizer、测试、安装与 Preset | 22 个实验从基础语法递进到多文件项目、target 依赖传播和双构建系统综合实践 |
| ✅ 已实现 | [**Linux 调试必备命令**](linux/) | 进程、资源、procfs、`find`、文本管道、`lsof`、`ss`、环境与日志 | 12 个实验围绕服务现场收集证据，而非零散罗列命令 |
| ✅ 已实现 | [**网络排障与 tcpdump**](network/) | 路由、DNS、TCP/UDP、HTTP、BPF 过滤、pcap 与 Wireshark | 12 个实验以 loopback 为主，覆盖从 socket 状态到报文证据的完整链路 |
| ✅ 已实现 | [**ELF、符号与动态链接工具**](elf/) | `file`、`ldd`、`readelf`、`nm`、`objdump`、`strings`、`addr2line`、`LD_DEBUG` | 12 个实验覆盖二进制结构、符号、调试信息、静态/动态库与综合排障 |
| ✅ 已实现 | [**Git 调试工作流**](git/) | `log`、`diff`、`blame`、`bisect`、标签、恢复与 worktree | 12 个实验覆盖基础协作与版本回归定位 |
| ✅ 已实现 | [**Protobuf 调试**](protobuf/) | `.proto`、`protoc`、C++ 生成代码、序列化、兼容性与二进制检查 | 12 个实验从基础 schema 递进到协议演进和解析失败诊断 |

构建系统、ELF、Linux 现场排障与网络模块已为后续 Protobuf 调试教程准备了构建、二进制和网络基础。

## 贡献

欢迎提交新的实验、修复错误或改进文档。每个实验目录应保持独立，包含：
- `README.md` — 实验目标、步骤、思考题
- 示例源码
- Makefile（如适用）

TodoList:
    1.增加设备树插件的教程
    2.重构不合理的教程