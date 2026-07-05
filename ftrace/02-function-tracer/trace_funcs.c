#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// A program that triggers specific kernel functions repeatedly.
// Use this with the "function" tracer + set_ftrace_filter to see
// exactly which kernel functions are called by our code.

void do_many_file_ops(int count) {
    char path[64];
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "/tmp/ftrace_func_%d.txt", i);

        // Each open/close pair triggers ext4_file_open then ext4 cleanup
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "data-%d\n", i);
            // write triggers ext4_file_write_iter
            write(fd, buf, strlen(buf));
            close(fd);
        }
    }
}

int main() {
    printf("=== Function Tracer Demo ===\n");
    printf("PID = %d\n", getpid());

    // Write a marker so we can find our trace entries
    int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);
    if (marker >= 0) {
        write(marker, ">>> FUNC DEMO START\n", 20);
    }

    // Phase 1: file operations (100 iterations)
    printf("Phase 1: 100 file ops (triggers ext4_file_* functions)...\n");
    do_many_file_ops(100);

    // Phase 2: pure syscall loop (triggers __x64_sys_getpid)
    printf("Phase 2: 10000 getpid() calls...\n");
    for (int i = 0; i < 10000; i++) {
        getpid();
    }

    if (marker >= 0) {
        write(marker, "<<< FUNC DEMO END\n", 18);
        close(marker);
    }

    printf("=== Done! ===\n");
    return 0;
}
