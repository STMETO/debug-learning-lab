#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// ── 追踪子进程实验 ──
// 用 strace -f 追踪父子进程的所有系统调用
// 用 strace -ff 把每个进程的输出写到单独文件

void child_work(int id) {
    printf("[子进程 %d] PID=%d, 开始工作...\n", id, getpid());

    // 每个子进程做不同的文件操作
    char filename[64];
    snprintf(filename, sizeof(filename), "/tmp/strace_child%d.txt", id);

    int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd >= 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Data from child process %d (PID=%d)\n", id, getpid());
        write(fd, buf, strlen(buf));
        close(fd);
    }

    usleep(50000);  // 50ms
    printf("[子进程 %d] PID=%d, 工作完成\n", id, getpid());
    exit(0);
}

int main() {
    printf("=== strace 追踪子进程实验 ===\n");
    printf("父进程 PID = %d\n\n", getpid());

    // fork 3 个子进程
    for (int i = 1; i <= 3; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            // 子进程
            child_work(i);
            // child_work 内部调用了 exit(0)
        } else if (pid > 0) {
            // 父进程
            printf("[父进程] fork 了 child #%d (PID=%d)\n", i, pid);
        } else {
            printf("[父进程] fork 失败！\n");
            return 1;
        }
    }

    // 父进程做一些自己的工作
    printf("\n[父进程] 开始自己的工作...\n");
    FILE *fp = fopen("/tmp/strace_parent.txt", "w");
    if (fp) {
        fprintf(fp, "Parent process wrote this.\n");
        fclose(fp);
    }
    usleep(100000);  // 100ms
    printf("[父进程] 工作完成\n\n");

    // 等待所有子进程结束
    printf("[父进程] 等待子进程结束...\n");
    for (int i = 0; i < 3; i++) {
        int status;
        pid_t child = wait(&status);
        printf("[父进程] 子进程 PID=%d 已结束 (exit code %d)\n",
               child, WEXITSTATUS(status));
    }

    printf("\n=== 实验结束 ===\n");
    return 0;
}
