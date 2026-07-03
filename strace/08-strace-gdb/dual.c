#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// ── strace + GDB 双工具联动 ──
// strace：看"程序在做什么"（外部视角——系统调用）
// GDB：   看"程序内部是什么状态"（内部视角——变量、调用栈）

// 一个"行为异常"的函数
void process_file(const char *path) {
    printf("[strace 视角] 正在打开文件: %s\n", path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("[strace 视角] 打开失败！\n");
        // strace 能看到: openat(...) = -1 ENOENT
        // GDB 能看到: fd == -1, path == 什么值, errno == ?
        return;
    }

    // strace 能看到: read(3, ...) 的参数和返回值
    // GDB 能看到: buf 的内容、读取的字节数
    char buf[128];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    if (n > 0) {
        buf[n] = '\0';
        printf("[strace 视角] 读取了 %zd 字节\n", n);
        printf("内容: %s\n", buf);
    } else {
        printf("[strace 视角] read 返回 %zd\n", n);
        // strace 能看到: read(...) = 0（空文件）或 = -1（错误）
        // GDB 能看到: n 的精确值、errno
    }

    close(fd);
    // strace 能看到: close(3) = 0
}

// 一个循环中有异常行为的函数
void batch_process(int count) {
    printf("[strace 视角] 批量处理 %d 个请求...\n", count);

    for (int i = 0; i < count; i++) {
        // strace 能看到: 每次循环的 write 调用频率
        printf("  处理第 %d 项...\n", i);

        // 模拟间歇性慢操作（strace -T 会显示某些 write 特别慢）
        if (i == 3) {
            printf("  ★ 第 3 项有延迟...\n");
            usleep(500000);  // 0.5 秒延迟
            // strace -T: clock_nanosleep 会显示 <0.500xxx>
            // GDB: 可以 break 在这里检查 i 的值和程序状态
        }
    }
}

int main(int argc, char *argv[]) {
    printf("╔════════════════════════════════════╗\n");
    printf("║   strace + GDB 双视角调试          ║\n");
    printf("╚════════════════════════════════════╝\n\n");

    // 先做一些 strace 能看到的事
    printf("Step 1: 创建测试文件\n");
    FILE *fp = fopen("/tmp/dual_test.txt", "w");
    if (fp) {
        fprintf(fp, "Hello from strace+GDB dual tool demo!\n");
        fprintf(fp, "Line 2: This is for debugging.\n");
        fprintf(fp, "Line 3: End of file.\n");
        fclose(fp);
    }

    // 用 strace 观察文件操作，用 GDB 深入内部
    printf("\nStep 2: 处理文件（strace 视角 + GDB 视角）\n");
    process_file("/tmp/dual_test.txt");       // 成功的情况
    process_file("/tmp/nonexistent.txt");     // 失败的情况

    // 用 strace -T 看耗时，用 GDB 看循环中的变量
    printf("\nStep 3: 批量处理（strace -T 找出慢操作）\n");
    batch_process(5);

    printf("\n=== 实验结束 ===\n");
    printf("回顾一下：\n");
    printf("  strace 告诉你：打开了哪些文件、读写了多少字节、哪个调用慢\n");
    printf("  GDB 告诉你：    fd 的值、buf 的内容、变量在循环中的变化\n");
    return 0;
}
