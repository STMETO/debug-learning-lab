#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// ── 信号处理实验 ──
// 演示 GDB 如何捕获、忽略、转发信号

int sigusr1_count = 0;   // 收到 SIGUSR1 的次数
int sigint_count  = 0;   // 收到 SIGINT 的次数
int running = 1;         // 主循环控制

void handle_sigusr1(int sig) {
    (void)sig;
    sigusr1_count++;
    printf("[信号处理] 收到 SIGUSR1（第 %d 次）\n", sigusr1_count);
}

void handle_sigint(int sig) {
    (void)sig;
    sigint_count++;
    printf("[信号处理] 收到 SIGINT（第 %d 次）——但我不退出\n", sigint_count);
    if (sigint_count >= 3) {
        printf("[信号处理] 收到 3 次 SIGINT，这次真的退出了\n");
        running = 0;
    }
}

// 一个"忙等待"的工作循环
void do_work() {
    static int iteration = 0;
    iteration++;
    printf("[工作] 迭代 #%d, sigusr1=%d, sigint=%d\n",
           iteration, sigusr1_count, sigint_count);

    // 模拟一些工作
    long sum = 0;
    for (long i = 0; i < 50000000; i++) {
        sum += i % 7;
    }
    (void)sum;
}

int main() {
    printf("=== 信号处理实验 ===\n");
    printf("PID: %d\n", getpid());
    printf("\n安装信号处理函数...\n");

    // 安装自定义信号处理器
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGINT,  handle_sigint);

    printf("SIGUSR1 → 自定义处理器（计数）\n");
    printf("SIGINT  → 自定义处理器（3 次后才退出）\n");
    printf("\n提示：在另一个终端运行:\n");
    printf("  kill -USR1 %d     # 发送 SIGUSR1\n", getpid());
    printf("  kill -INT  %d     # 发送 SIGINT\n", getpid());
    printf("\n在 GDB 中你可以用 'signal SIGUSR1' 向程序发信号\n\n");

    // 主循环
    while (running) {
        do_work();
        usleep(500000);  // 0.5 秒
    }

    printf("\n程序退出。最终统计:\n");
    printf("  SIGUSR1: %d 次\n", sigusr1_count);
    printf("  SIGINT:  %d 次\n", sigint_count);

    return 0;
}
