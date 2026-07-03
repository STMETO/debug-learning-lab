#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// ── 过滤系统调用实验 ──
// 这个程序会触发多种类型的系统调用
// 用 strace -e trace= 只追踪你关心的那一类

int main() {
    printf("=== strace 过滤实验 ===\n\n");

    // ── 文件操作 ────────────────────────
    printf("【文件操作】\n");
    FILE *fp = fopen("/tmp/strace_filter.txt", "w");
    if (fp) {
        fprintf(fp, "some data for the file\n");
        fclose(fp);
    }
    // 读取刚写入的文件
    int fd = open("/tmp/strace_filter.txt", O_RDONLY);
    if (fd >= 0) {
        char buf[64];
        read(fd, buf, sizeof(buf));
        close(fd);
    }
    printf("  文件操作完成\n\n");

    // ── 内存操作 ────────────────────────
    printf("【内存操作】\n");
    int *a = (int *)malloc(10 * sizeof(int));
    int *b = (int *)malloc(1000 * sizeof(int));
    for (int i = 0; i < 1000; i++) b[i] = i;
    free(a);
    free(b);
    printf("  内存操作完成（2 次 malloc + 2 次 free）\n\n");

    // ── 进程信息 ────────────────────────
    printf("【进程信息】\n");
    pid_t pid = getpid();
    pid_t ppid = getppid();
    printf("  PID = %d, PPID = %d\n", pid, ppid);
    printf("  进程信息获取完成\n\n");

    // ── 时间相关 ────────────────────────
    printf("【时间相关】\n");
    usleep(1000);  // 1 毫秒
    printf("  时间操作完成\n\n");

    printf("=== 实验结束 ===\n");
    return 0;
}
