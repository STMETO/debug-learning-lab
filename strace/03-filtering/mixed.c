#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// ── 过滤系统调用Experiment ──
// 这个程序会触发多种类型的系统调用
// 用 strace -e trace= 只追踪你关心的那一类

int main() {
    printf("=== strace filtering experiment ===\n\n");

    // ── fileoperation ────────────────────────
    printf("[File Operations]\n");
    FILE *fp = fopen("/tmp/strace_filter.txt", "w");
    if (fp) {
        fprintf(fp, "some data for the file\n");
        fclose(fp);
    }
    // read刚write的file
    int fd = open("/tmp/strace_filter.txt", O_RDONLY);
    if (fd >= 0) {
        char buf[64];
        read(fd, buf, sizeof(buf));
        close(fd);
    }
    printf("  File operations done\n\n");

    // ── memoryoperation ────────────────────────
    printf("[Memory Operations]\n");
    int *a = (int *)malloc(10 * sizeof(int));
    int *b = (int *)malloc(1000 * sizeof(int));
    for (int i = 0; i < 1000; i++) b[i] = i;
    free(a);
    free(b);
    printf("  Memory operations done (2 mallocs + 2 frees)\n\n");

    // ── 进程信息 ────────────────────────
    printf("[Process Info]\n");
    pid_t pid = getpid();
    pid_t ppid = getppid();
    printf("  PID = %d, PPID = %d\n", pid, ppid);
    printf("  Process info retrieved\n\n");

    // ── 时间相关 ────────────────────────
    printf("[Time-related]\n");
    usleep(1000);  // 1 毫秒
    printf("  Time operations done\n\n");

    printf("=== Experiment finished ===\n");
    return 0;
}
