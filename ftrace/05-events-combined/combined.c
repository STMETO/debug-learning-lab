#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

// A program that triggers BOTH kernel function calls AND tracepoint events.
// Use with trace-cmd to see how function tracing and event tracing combine.

int marker_fd = -1;

void mark(const char *msg) {
    if (marker_fd >= 0) write(marker_fd, msg, strlen(msg));
}

void file_ops(const char *tag, int count, int do_sync) {
    char path[64];
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "/tmp/ftrace_combined_%s_%d.txt", tag, i);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%s-%d\n", tag, i);
            write(fd, buf, strlen(buf));
            if (do_sync) fsync(fd);
            close(fd);
        }
    }
}

void fork_work() {
    pid_t pid = fork();
    if (pid == 0) {
        // Child: quick work then exit — triggers sched events
        for (int i = 0; i < 10000; i++) getpid();
        _exit(0);
    } else {
        waitpid(pid, NULL, 0);
    }
}

int main() {
    marker_fd = open("/sys/kernel/tracing/trace_marker", O_WRONLY);

    printf("=== Combined Tracing Demo ===\n");

    // Phase 1: file ops — triggers ext4 functions + syscall events
    mark("=== PHASE 1: FILE OPS ===\n");
    printf("Phase 1: 30 file ops (no fsync)...\n");
    file_ops("normal", 30, 0);

    // Phase 2: sync ops — triggers ext4 + block layer events
    mark("=== PHASE 2: SYNC OPS ===\n");
    printf("Phase 2: 3 sync file ops (with fsync)...\n");
    file_ops("sync", 3, 1);

    // Phase 3: fork — triggers sched events
    mark("=== PHASE 3: FORK ===\n");
    printf("Phase 3: fork + wait...\n");
    fork_work();

    // Phase 4: many fast syscalls
    mark("=== PHASE 4: SYSCALL FLOOD ===\n");
    printf("Phase 4: 50000 getpid() calls...\n");
    for (int i = 0; i < 50000; i++) getpid();

    if (marker_fd >= 0) close(marker_fd);
    printf("=== Done! ===\n");
    return 0;
}
