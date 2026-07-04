#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// ── Statistics模式Experiment ──
// 这个程序混合了多种系统调用
// 用 strace -c 生成一份"系统调用频率report"

void do_file_work() {
    // 创建和writefile
    int fd = open("/tmp/strace_stats.txt", O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd >= 0) {
        for (int i = 0; i < 20; i++) {
            char buf[64];
            int len = snprintf(buf, sizeof(buf), "Line %d: statistics test\n", i);
            write(fd, buf, len);
        }
        close(fd);
    }

    // read刚write的file
    fd = open("/tmp/strace_stats.txt", O_RDONLY);
    if (fd >= 0) {
        char buf[256];
        for (int i = 0; i < 3; i++) {
            lseek(fd, i * 10, SEEK_SET);
            read(fd, buf, sizeof(buf));
        }
        close(fd);
    }
}

void do_memory_work() {
    // 多小块allocate/allocation和free
    for (int i = 0; i < 10; i++) {
        int *p = (int *)malloc(100 * sizeof(int));
        p[0] = i;
        free(p);
    }

    // 一大块allocate/allocation
    char *big = (char *)malloc(2 * 1024 * 1024);  // 2MB
    if (big) {
        memset(big, 0, 2 * 1024 * 1024);
        free(big);
    }
}

void do_time_work() {
    // 一些时间相关调用
    for (int i = 0; i < 5; i++) {
        usleep(10000);  // 10ms × 5 = 50ms
    }
    getpid();
    getppid();
}

int main() {
    printf("=== strace statistics mode experiment ===\n");
    printf("This program mixes file, memory, timing, and other operations\n\n");

    do_file_work();
    printf("File operations done (20 writes + 3 reads)\n\n");

    do_memory_work();
    printf("Memory operations done (10 small mallocs/frees + 1x 2MB)\n\n");

    do_time_work();
    printf("Time operations done (5 usleeps)\n\n");

    printf("=== Experiment finished ===\n");
    return 0;
}
