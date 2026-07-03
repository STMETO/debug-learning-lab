#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// ── 时间分析实验 ──
// 用 strace -tt -T 看清楚"程序的时间花在哪里"

void phase_fast() {
    printf("【阶段 A】快速操作（微秒级）\n");
    getpid();
    getppid();
    printf("  完成\n\n");
}

void phase_medium() {
    printf("【阶段 B】中等操作（毫秒级）\n");

    // 文件写入
    int fd = open("/tmp/strace_timing.txt", O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd >= 0) {
        for (int i = 0; i < 10; i++) {
            char buf[100];
            snprintf(buf, sizeof(buf), "Line %d: this is a timing test line.\n", i);
            write(fd, buf, strlen(buf));
        }
        close(fd);
    }

    // 文件读取
    fd = open("/tmp/strace_timing.txt", O_RDONLY);
    if (fd >= 0) {
        char bigbuf[2048];
        ssize_t n = read(fd, bigbuf, sizeof(bigbuf));
        (void)n;
        close(fd);
    }

    printf("  完成\n\n");
}

void phase_slow() {
    printf("【阶段 C】慢速操作（秒级）\n");

    // 故意放慢的循环——每次迭代都是一次小操作
    for (int i = 0; i < 3; i++) {
        usleep(200000);  // 0.2 秒
        printf("  slow iteration %d\n", i);
    }

    printf("  完成\n\n");
}

void phase_big_malloc() {
    printf("【阶段 D】大内存分配\n");

    // 分配 10MB 内存
    size_t size = 10 * 1024 * 1024;
    char *big = (char *)malloc(size);
    if (big) {
        memset(big, 'A', size);   // 写入 10MB
        free(big);
    }

    printf("  完成\n\n");
}

int main() {
    printf("=== strace 时间分析实验 ===\n\n");

    phase_fast();
    phase_medium();
    phase_slow();
    phase_big_malloc();

    printf("=== 实验结束 ===\n");
    return 0;
}
