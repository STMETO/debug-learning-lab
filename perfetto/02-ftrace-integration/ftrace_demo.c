#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

// A program with diverse kernel activity for Perfetto ftrace integration.
// Each phase triggers different ftrace event categories.

void file_ops(int count) {
    char path[64];
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "/tmp/perfetto_ftrace_%d.txt", i);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            write(fd, "data", 4);
            close(fd);
        }
    }
}

void fork_burst(int count) {
    for (int i = 0; i < count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            getpid();
            _exit(0);
        } else {
            waitpid(pid, NULL, 0);
        }
    }
}

int main() {
    printf("=== Perfetto ftrace Integration Demo ===\n");

    // Phase 1: file operations — triggers ext4 and syscall events
    printf("Phase 1: 80 file ops (ext4 + syscall events)...\n");
    file_ops(80);

    // Phase 2: fork activity — triggers sched events
    printf("Phase 2: 10 fork bursts (sched events)...\n");
    fork_burst(10);

    // Phase 3: more file ops with a pause
    printf("Phase 3: 60 more file ops...\n");
    file_ops(60);

    printf("=== Done! ===\n");
    return 0;
}
