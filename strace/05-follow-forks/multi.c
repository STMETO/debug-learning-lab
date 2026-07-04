#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// ── 追踪子进程Experiment ──
// 用 strace -f 追踪父子进程的所有系统调用
// 用 strace -ff 把每个进程的output写到单独file

void child_work(int id) {
    printf("[Child %d] PID=%d, starting work...\n", id, getpid());

    // Each child does different file operations
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
    printf("[Child %d] PID=%d, work done\n", id, getpid());
    exit(0);
}

int main() {
    printf("=== strace tracing child processes experiment ===\n");
    printf("Parent PID = %d\n\n", getpid());

    // fork 3 child processes
    for (int i = 1; i <= 3; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            // Child process
            child_work(i);
            // child_work calls exit(0)
        } else if (pid > 0) {
            // Parent process
            printf("[Parent] forked child #%d (PID=%d)\n", i, pid);
        } else {
            printf("[Parent] fork failed!\n");
            return 1;
        }
    }

    // Parent does its own work
    printf("\n[Parent] starting its own work...\n");
    FILE *fp = fopen("/tmp/strace_parent.txt", "w");
    if (fp) {
        fprintf(fp, "Parent process wrote this.\n");
        fclose(fp);
    }
    usleep(100000);  // 100ms
    printf("[Parent] work done\n\n");

    // Wait for all child processes to finish
    printf("[Parent] waiting for children to finish...\n");
    for (int i = 0; i < 3; i++) {
        int status;
        pid_t child = wait(&status);
        printf("[Parent] child PID=%d finished (exit code %d)\n",
               child, WEXITSTATUS(status));
    }

    printf("\n=== Experiment finished ===\n");
    return 0;
}