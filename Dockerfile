# ============================================================
# Debug Learning Lab — Docker 镜像
# ============================================================
# 构建：
#   docker build -t debug-learning-lab .
#
# 运行（需要特权模式以使用 perf/bpftrace 等内核工具）：
#   docker run -it --cap-add=SYS_PTRACE --security-opt seccomp=unconfined debug-learning-lab
#
# 如需完整 perf/bpftrace 功能，使用 privileged 模式：
#   docker run -it --privileged -v /lib/modules:/lib/modules:ro debug-learning-lab
# ============================================================

# ── Stage 1: 基础环境 ──────────────────────────────────────
FROM ubuntu:22.04 AS base

# 避免交互式安装提示
ENV DEBIAN_FRONTEND=noninteractive

# ---- 编译工具链 ----
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    g++ \
    git \
    gdb \
    make \
    cmake \
    build-essential \
    pkg-config \
    # ---- 调试工具 ----
    strace \
    ltrace \
    valgrind \
    binutils \
    file \
    # ---- 性能分析 ----
    linux-tools-generic \
    linux-tools-common \
    # ---- 动态追踪 ----
    bpftrace \
    # ---- 网络/进程工具 ----
    procps \
    htop \
    curl \
    wget \
    lsof \
    iproute2 \
    psmisc \
    netcat-openbsd \
    iputils-ping \
    traceroute \
    tcpdump \
    # ---- 编辑器 ----
    vim \
    nano \
    less \
    # ---- 其他 ----
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# ── Stage 2: perfetto ──────────────────────────────────────
# perfetto 是独立工具，单独安装以保持层次清晰
FROM base AS with-perfetto

# 下载 perfetto trace_processor 和 traced（CLI 工具）
# 注：如需完整 SDK，请参考 https://perfetto.dev/docs/quickstart/linux-tracing
RUN mkdir -p /opt/perfetto && \
    cd /opt/perfetto && \
    curl -LO https://get.perfetto.dev/trace_processor && \
    chmod +x trace_processor && \
    curl -LO https://get.perfetto.dev/traced && \
    chmod +x traced

ENV PATH="/opt/perfetto:${PATH}"

# ── Stage 3: 实验环境 ─────────────────────────────────────
FROM with-perfetto AS lab

# 创建非 root 用户，避免在容器内以 root 运行实验
RUN useradd -m -s /bin/bash learner && \
    echo 'learner ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# 设置工作目录
WORKDIR /workspace
RUN chown learner:learner /workspace

# 复制项目文件
COPY --chown=learner:learner . /workspace/debug-learning-lab/

# 在 .bashrc 中添加便捷别名和提示
RUN echo '\n\
# ── Debug Learning Lab 环境 ──\n\
export PS1="\[\e[1;32m\]lab\[\e[0m\]:\[\e[1;34m\]\w\[\e[0m\]\$ "\n\
alias ll="ls -la"\n\
alias gdb="gdb -q"\n\
alias build="gcc -g -Wall -Wextra -o"\n\
echo "╔══════════════════════════════════════╗"\n\
echo "║   🔍 Debug Learning Lab 已就绪      ║"\n\
echo "╠══════════════════════════════════════╣"\n\
echo "║  gdb strace ltrace perf valgrind     ║"\n\
echo "║  bpftrace trace_processor            ║"\n\
echo "║                                      ║"\n\
echo "║  实验目录: ~/debug-learning-lab/     ║"\n\
echo "╚══════════════════════════════════════╝"\n\
echo\n\
' >> /home/learner/.bashrc

# 切入实验目录
RUN echo 'cd /workspace/debug-learning-lab' >> /home/learner/.bashrc

USER learner

# 默认入口
CMD ["/bin/bash"]
