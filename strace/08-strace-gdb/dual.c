#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// ── strace + GDB 双工具联动 ──
// strace：看"程序在做什么"（外部视角——系统调用）
// GDB：   看"程序内部Yes什么状态"（内部视角——variable、call stack）

// 一个"行为异常"的函数
void process_file(const char *path) {
    printf("[strace view] opening file: %s\n", path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("[strace view] open failed!\n");
        // strace 能看到: openat(...) = -1 ENOENT
        // GDB 能看到: fd == -1, path == 什么值, errno == ?
        return;
    }

    // strace 能看到: read(3, ...) 的参数和return值
    // GDB 能看到: buf 的内容、read的bytes数
    char buf[128];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    if (n > 0) {
        buf[n] = '\0';
        printf("[strace view] read %zd bytes\n", n);
        printf("Content: %s\n", buf);
    } else {
        printf("[strace view] read returned %zd\n", n);
        // strace 能看到: read(...) = 0（空file）或 = -1（error）
        // GDB 能看到: n 的精确值、errno
    }

    close(fd);
    // strace 能看到: close(3) = 0
}

// 一个循环中有异常行为的函数
void batch_process(int count) {
    printf("[strace view] batch processing %d requests...\n", count);

    for (int i = 0; i < count; i++) {
        // strace 能看到: 每循环的 write 调用频率
        printf("  processing item %d...\n", i);

        // 模拟间歇性慢operation（strace -T 会显示某些 write 特别慢）
        if (i == 3) {
            printf("  * item 3 has delay...\n");
            usleep(500000);  // 0.5 秒延迟
            // strace -T: clock_nanosleep 会显示 <0.500xxx>
            // GDB: 可以 break 在这里check i 的值和程序状态
        }
    }
}

int main(int argc, char *argv[]) {
    printf("╔════════════════════════════════════╗\n");
    printf("║   strace + GDB dual-view debugging ║\n");
    printf("╚════════════════════════════════════╝\n\n");

    // 先做一些 strace 能看到的事
    printf("Step 1: create test file\n");
    FILE *fp = fopen("/tmp/dual_test.txt", "w");
    if (fp) {
        fprintf(fp, "Hello from strace+GDB dual tool demo!\n");
        fprintf(fp, "Line 2: This is for debugging.\n");
        fprintf(fp, "Line 3: End of file.\n");
        fclose(fp);
    }

    // 用 strace 观察fileoperation，用 GDB 深入内部
    printf("\nStep 2: process files (strace view + GDB view)\n");
    process_file("/tmp/dual_test.txt");       // 成功的情况
    process_file("/tmp/nonexistent.txt");     // 失败的情况

    // 用 strace -T 看耗时，用 GDB 看循环中的variable
    printf("\nStep 3: batch process (strace -T to find slow operations)\n");
    batch_process(5);

    printf("\n=== Experiment finished ===\n");
    printf("Review:\n");
    printf("  strace shows: which files were opened, how many bytes were read/written, which calls are slow\n");
    printf("  GDB shows:      fd values, buf contents, variable changes across loop iterations\n");
    return 0;
}
