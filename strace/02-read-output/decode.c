#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

// ── 解码 strace output的每一个字段 ──
// 这个程序故意产生各种不同类型的系统调用return值

int main() {
    printf("=== strace output format experiment ===\n\n");

    // ── Scenario 1：成功的调用 ──
    printf("[Scenario 1] Successful open and read\n");
    // 创建一个testfile
    FILE *fp = fopen("/tmp/strace_test.txt", "w");
    if (fp) {
        fprintf(fp, "Hello, strace!\nLine 2\nLine 3\n");
        fclose(fp);
    }

    // 用系统调用级 API 打开（更直接地反映在 strace 中）
    int fd = open("/tmp/strace_test.txt", O_RDONLY);
    printf("  open() returned fd = %d\n", fd);

    if (fd >= 0) {
        char buf[64] = {0};
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        printf("  read() returned %zd bytes\n", n);
        close(fd);
    }

    // ── Scenario 2：失败的系统调用 ──
    printf("\n[Scenario 2] Failed system calls - different errno\n");

    // 打开不存在的file → ENOENT
    int fd_bad = open("/tmp/does_not_exist_xyz.txt", O_RDONLY);
    printf("  open(non-existent) returned %d, errno = %d (%s)\n",
           fd_bad, errno, strerror(errno));

    // 打开一个没有权限的路径 → EACCES
    fd_bad = open("/root/secret.txt", O_RDONLY);
    printf("  open(/root/...) returned %d, errno = %d (%s)\n",
           fd_bad, errno, strerror(errno));

    // ── Scenario 3：不同return值的含义 ──
    printf("\n[Scenario 3] Meaning of different return values\n");

    // write returnwrite的bytes数
    ssize_t written = write(1, "  directly to stdout via write()\n", 34);
    printf("  write() returned %zd bytes\n", written);

    // lseek return新的offset位置
    fd = open("/tmp/strace_test.txt", O_RDONLY);
    if (fd >= 0) {
        off_t pos = lseek(fd, 5, SEEK_SET);
        printf("  lseek(5) returned new position = %ld\n", (long)pos);
        close(fd);
    }

    // ── Scenario 4：有耗时差异的调用 ──
    printf("\n[Scenario 4] Time difference - note strace's <time> field\n");
    printf("  Fast call (e.g., getpid)...\n");
    pid_t pid = getpid();
    printf("  Slow call (sleep 1 second)...\n");
    sleep(1);
    printf("  PID = %d, strace will show the time difference between the two calls\n", pid);

    printf("\n=== Experiment finished ===\n");
    return 0;
}