#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// ── Signal Handling Experiment ──
// 演示 GDB 如何捕获、忽略、转发信号

int sigusr1_count = 0;   // 收到 SIGUSR1 的数
int sigint_count  = 0;   // 收到 SIGINT 的数
int running = 1;         // 主循环控制

void handle_sigusr1(int sig) {
    (void)sig;
    sigusr1_count++;
    printf("[Signal handler] Received SIGUSR1 (time %d)\n", sigusr1_count);
}

void handle_sigint(int sig) {
    (void)sig;
    sigint_count++;
    printf("[Signal handler] Received SIGINT (time %d) -- but I won't quit\n", sigint_count);
    if (sigint_count >= 3) {
        printf("[Signal handler] Received SIGINT 3 times, now really quitting\n");
        running = 0;
    }
}

// 一个"忙等待"的Work循环
void do_work() {
    static int iteration = 0;
    iteration++;
    printf("[Work] Iteration #%d, sigusr1=%d, sigint=%d\n",
           iteration, sigusr1_count, sigint_count);

    // 模拟一些Work
    long sum = 0;
    for (long i = 0; i < 50000000; i++) {
        sum += i % 7;
    }
    (void)sum;
}

int main() {
    printf("=== Signal Handling Experiment ===\n");
    printf("PID: %d\n", getpid());
    printf("\nInstalling signal handlers...\n");

    // 安装自定义Signal Handler器
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGINT,  handle_sigint);

    printf("SIGUSR1 -> custom handler (counting)\n");
    printf("SIGINT  -> custom handler (quits after 3 times)\n");
    printf("\nTip: run in another terminal:\n");
    printf("  kill -USR1 %d     # send SIGUSR1\n", getpid());
    printf("  kill -INT  %d     # send SIGINT\n", getpid());
    printf("\nIn GDB you can use 'signal SIGUSR1' to send a signal to the program\n\n");

    // 主循环
    while (running) {
        do_work();
        usleep(500000);  // 0.5 秒
    }

    printf("\nProgram exiting. Final stats:\n");
    printf("  SIGUSR1: %d times\n", sigusr1_count);
    printf("  SIGINT:  %d times\n", sigint_count);

    return 0;
}
