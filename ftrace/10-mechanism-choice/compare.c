#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// A program that can be analyzed through multiple tracing mechanisms:
//   1. Static tracepoint  (syscalls:sys_enter_openat) — always available, low overhead
//   2. Dynamic kprobe      (perf probe ext4_file_open) — no recompile, flexible
//   3. function_graph      (ftrace ext4_file_*) — exhaustive call graph
//   4. perf sampling       (perf record -g) — statistical, low overhead
//
// This experiment helps you CHOOSE the right mechanism for each scenario.

void work_loop(const char *tag, int count, int do_sync) {
    char path[64];
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "/tmp/ftrace_compare_%s_%d.txt", tag, i);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            write(fd, tag, strlen(tag));
            if (do_sync) fsync(fd);
            close(fd);
        }
    }
}

int main() {
    printf("=== Tracing Mechanism Comparison ===\n");

    // Phase A: Normal file ops (trace with any mechanism)
    printf("Phase A: 20 normal file ops...\n");
    work_loop("normal", 20, 0);

    // Phase B: Synced file ops (creates a distinct latency spike)
    printf("Phase B: 5 synced file ops...\n");
    work_loop("sync", 5, 1);

    // Phase C: Many fast ops (high event rate — stresses sampling vs exhaustive)
    printf("Phase C: 200 fast file ops...\n");
    work_loop("fast", 200, 0);

    printf("=== Done! ===\n");
    return 0;
}
