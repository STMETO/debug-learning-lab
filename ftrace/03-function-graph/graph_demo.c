#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// A program with nested kernel call paths — designed for function_graph.
// Each file operation triggers a deep chain: our code → VFS → ext4 → block layer.
// function_graph shows this nesting with microsecond durations.

// Fast path: open, write a few bytes, close — normal sub-ms operation
void fast_file_op(int id) {
    char path[64];
    snprintf(path, sizeof(path), "/tmp/ftrace_graph_fast_%d.txt", id);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "fast", 4);
        close(fd);
    }
}

// Slow path: open, write, fsync — forces data to disk, much slower
void slow_file_op(int id) {
    char path[64];
    snprintf(path, sizeof(path), "/tmp/ftrace_graph_slow_%d.txt", id);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "slow data that needs to be synced", 33);
        fsync(fd);   // <-- this is the slow part! Triggers block layer flush
        close(fd);
    }
}

int main() {
    printf("=== Function Graph Demo ===\n");

    int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);

    // Phase 1: Fast operations (normal latency)
    if (marker >= 0) write(marker, "=== PHASE 1: FAST OPS ===\n", 27);
    printf("Phase 1: 10 fast file ops (open/write/close, no fsync)...\n");
    for (int i = 0; i < 10; i++) {
        fast_file_op(i);
    }

    // Phase 2: Slow operations (fsync forces disk flush)
    if (marker >= 0) write(marker, "=== PHASE 2: SLOW OPS (with fsync) ===\n", 40);
    printf("Phase 2: 3 slow file ops (with fsync — watch the duration!)...\n");
    for (int i = 0; i < 3; i++) {
        slow_file_op(i);
    }

    // Phase 3: Mix of fast ops to show contrast
    if (marker >= 0) write(marker, "=== PHASE 3: FAST OPS AGAIN ===\n", 33);
    printf("Phase 3: 10 more fast ops for contrast...\n");
    for (int i = 10; i < 20; i++) {
        fast_file_op(i);
    }

    if (marker >= 0) close(marker);

    printf("=== Done! Now read: sudo cat /sys/kernel/tracing/trace ===\n");
    return 0;
}
