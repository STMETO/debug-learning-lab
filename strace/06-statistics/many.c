#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// ── 统计模式实验 ──
// 这个程序混合了多种系统调用
// 用 strace -c 生成一份"系统调用频率报告"

void do_file_work() {
    // 创建和写入文件
    int fd = open("/tmp/strace_stats.txt", O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd >= 0) {
        for (int i = 0; i < 20; i++) {
            char buf[64];
            int len = snprintf(buf, sizeof(buf), "Line %d: statistics test\n", i);
            write(fd, buf, len);
        }
        close(fd);
    }

    // 读取刚写入的文件
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
    // 多次小块分配和释放
    for (int i = 0; i < 10; i++) {
        int *p = (int *)malloc(100 * sizeof(int));
        p[0] = i;
        free(p);
    }

    // 一次大块分配
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
    printf("=== strace 统计模式实验 ===\n");
    printf("这个程序混合了文件、内存、时间等操作\n\n");

    do_file_work();
    printf("文件操作完成（20 次 write + 3 次 read）\n\n");

    do_memory_work();
    printf("内存操作完成（10 次小 malloc/free + 1 次 2MB）\n\n");

    do_time_work();
    printf("时间操作完成（5 次 usleep）\n\n");

    printf("=== 实验结束 ===\n");
    return 0;
}
