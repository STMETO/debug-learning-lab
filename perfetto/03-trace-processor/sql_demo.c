#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

// Generates diverse kernel events for SQL analysis practice.
// Each phase creates a distinct event pattern you can query with SQL.

void file_ops(const char *tag, int count, int do_sleep) {
    char path[64];
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "/tmp/perfetto_sql_%s_%d.txt", tag, i);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) { write(fd, tag, strlen(tag)); close(fd); }
    }
    if (do_sleep) usleep(300000);  // 300ms gap to separate phases in timeline
}

void fork_work(int count) {
    for (int i = 0; i < count; i++) {
        pid_t pid = fork();
        if (pid == 0) { getpid(); _exit(0); }
        else { waitpid(pid, NULL, 0); }
    }
}

int main() {
    printf("=== SQL Practice: Multi-Phase Workload ===\n");

    printf("Phase A: light file ops (20 files)...\n");
    file_ops("light", 20, 1);

    printf("Phase B: fork burst (8 forks)...\n");
    fork_work(8);
    usleep(300000);

    printf("Phase C: heavy file ops (100 files)...\n");
    file_ops("heavy", 100, 0);

    printf("=== Done! ===\n");
    return 0;
}
