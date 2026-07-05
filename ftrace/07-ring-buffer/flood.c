#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// A program that generates a flood of kernel events to demonstrate
// ring buffer overflow. Run first with default buffer, then with
// increased buffer_size_kb to see the difference.

#define BURST_COUNT 500

void burst_file_ops(int burst_id) {
    char path[64];
    for (int i = 0; i < BURST_COUNT; i++) {
        snprintf(path, sizeof(path), "/tmp/ftrace_flood_%d_%d.txt", burst_id, i);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            write(fd, "data", 4);
            close(fd);
        }
    }
}

int main() {
    printf("=== Ring Buffer Flood Test ===\n");

    int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);

    // Burst 1: with default buffer (likely some overflow)
    if (marker >= 0) write(marker, "=== BURST 1 START (expect overflow) ===\n", 42);
    printf("Burst 1: %d file ops (default buffer — watch for overflow)...\n", BURST_COUNT);
    burst_file_ops(1);
    if (marker >= 0) write(marker, "=== BURST 1 END ===\n", 21);

    // Burst 2: same operation to compare
    if (marker >= 0) write(marker, "=== BURST 2 START ===\n", 22);
    printf("Burst 2: %d more file ops...\n", BURST_COUNT);
    burst_file_ops(2);
    if (marker >= 0) write(marker, "=== BURST 2 END ===\n", 21);

    // Burst 3: rapid syscall flood (generates lots of small events)
    if (marker >= 0) write(marker, "=== BURST 3 START (syscall flood) ===\n", 40);
    printf("Burst 3: 100000 rapid getpid() calls...\n");
    for (int i = 0; i < 100000; i++) {
        getpid();
    }
    if (marker >= 0) write(marker, "=== BURST 3 END ===\n", 21);

    if (marker >= 0) close(marker);
    printf("=== Done! Check: sudo cat /sys/kernel/tracing/trace | head -5 ===\n");
    return 0;
}
