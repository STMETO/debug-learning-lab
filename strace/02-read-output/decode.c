#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

// ── 解码 strace 输出的每一个字段 ──
// 这个程序故意产生各种不同类型的系统调用返回值

int main() {
    printf("=== strace 输出格式实验 ===\n\n");

    // ── 场景 1：成功的调用 ──
    printf("【场景 1】成功的 open 和 read\n");
    // 创建一个测试文件
    FILE *fp = fopen("/tmp/strace_test.txt", "w");
    if (fp) {
        fprintf(fp, "Hello, strace!\nLine 2\nLine 3\n");
        fclose(fp);
    }

    // 用系统调用级 API 打开（更直接地反映在 strace 中）
    int fd = open("/tmp/strace_test.txt", O_RDONLY);
    printf("  open() 返回 fd = %d\n", fd);

    if (fd >= 0) {
        char buf[64] = {0};
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        printf("  read() 返回 %zd 字节\n", n);
        close(fd);
    }

    // ── 场景 2：失败的系统调用 ──
    printf("\n【场景 2】失败的系统调用 —— 不同的 errno\n");

    // 打开不存在的文件 → ENOENT
    int fd_bad = open("/tmp/does_not_exist_xyz.txt", O_RDONLY);
    printf("  open(不存在) 返回 %d, errno = %d (%s)\n",
           fd_bad, errno, strerror(errno));

    // 打开一个没有权限的路径 → EACCES
    fd_bad = open("/root/secret.txt", O_RDONLY);
    printf("  open(/root/...) 返回 %d, errno = %d (%s)\n",
           fd_bad, errno, strerror(errno));

    // ── 场景 3：返回不同值的调用 ──
    printf("\n【场景 3】不同返回值的含义\n");

    // write 返回写入的字节数
    ssize_t written = write(1, "  directly to stdout via write()\n", 34);
    printf("  write() 返回了 %zd 字节\n", written);

    // lseek 返回新的偏移位置
    fd = open("/tmp/strace_test.txt", O_RDONLY);
    if (fd >= 0) {
        off_t pos = lseek(fd, 5, SEEK_SET);
        printf("  lseek(5) 返回新位置 = %ld\n", (long)pos);
        close(fd);
    }

    // ── 场景 4：有耗时差异的调用 ──
    printf("\n【场景 4】耗时差异 —— 注意 strace 的 <耗时> 字段\n");
    printf("  快速调用（比如 getpid）...\n");
    pid_t pid = getpid();
    printf("  慢速调用（sleep 1 秒）...\n");
    sleep(1);
    printf("  PID = %d, 两个调用之间的时间差 strace 会显示\n", pid);

    printf("\n=== 实验结束 ===\n");
    return 0;
}
