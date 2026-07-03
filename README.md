# Debug Learning Lab

一个通过动手实验学习 Linux 调试工具的实践项目。

## 涵盖工具

| 工具 | 用途 | 状态 |
|------|------|------|
| [GDB](gdb/) | C/C++ 源码级调试器 | ✅ 12 个实验全部就绪 |
| [ASan](asan/) (AddressSanitizer) | 内存错误检测 | ✅ 10 个实验全部就绪 |
| [strace](strace/) | 系统调用追踪 | ✅ 8 个实验全部就绪 |
| [ltrace](ltrace/) | 库函数调用追踪 | 📋 6 个实验已规划 |
| perf | 性能分析 (CPU profiling) | ⏸️ 需原生 Linux（WSL2 暂不支持） |
| perfetto / trace-cmd | 内核与用户态追踪 | 📋 计划中 |
| Valgrind | 内存泄漏与错误检测 | 📋 计划中 |
| bpftrace / eBPF | 动态内核/用户态追踪 | 📋 计划中 |
| git | 代码仓库管理工具 | 📋 计划中 |

## 项目结构

```
debug-learning-lab/
├── gdb/                  # GDB 调试实验（✅ 12 个实验）
├── asan/                 # ASan 内存检测实验（✅ 10 个实验）
├── strace/               # strace 系统调用追踪（📋 8 个实验）
├── perf/                 # perf 性能分析（规划中）
└── ...
```

## 快速开始

### 环境要求

- Linux 系统（WSL2 / 虚拟机 / 原生均可）
- GCC 或 Clang（需带调试符号 `-g`）
- GDB（建议 10.0+）

```bash
# Ubuntu/Debian
sudo apt install gdb gcc build-essential

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

容器启动后会自动进入实验目录，所有工具可直接使用。

## 学习路线

1. **GDB** — 从源码级调试开始，建立最直观的调试心智模型
2. **Sanitizers** — 理解内存错误类型及其检测原理
3. **strace / ltrace** — 从系统调用层面观察程序行为
4. **perf** — 从 CPU 角度分析性能瓶颈
5. **perfetto / eBPF** — 深入内核，追踪复杂系统中的问题

## 贡献

欢迎提交新的实验、修复错误或改进文档。每个实验目录应保持独立，包含：
- `README.md` — 实验目标、步骤、思考题
- 示例源码
- Makefile（如适用）
