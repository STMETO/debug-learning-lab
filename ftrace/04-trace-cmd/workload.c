#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// A workload program for trace-cmd demonstration.
// trace-cmd record -p function_graph -l 'ext4_file_*' -F ./workload

void batch_file_ops(const char *prefix, int count, int with_fsync) {
    char path[64];
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "/tmp/tracecmd_%s_%d.txt", prefix, i);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s-data-%d\n", prefix, i);
            write(fd, buf, strlen(buf));
            if (with_fsync) {
                fsync(fd);  // Forces disk flush — will show as long duration
            }
            close(fd);
        }
    }
}

int main() {
    printf("=== trace-cmd Workload ===\n");

    // Write markers to help locate our trace entries
    int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);

    if (marker >= 0) write(marker, "=== BATCH NORMAL START ===\n", 27);
    printf("Batch 1: 50 normal file ops (no fsync)...\n");
    batch_file_ops("normal", 50, 0);

    if (marker >= 0) write(marker, "=== BATCH SYNC START ===\n", 25);
    printf("Batch 2: 10 synced file ops (with fsync — expect long durations)...\n");
    batch_file_ops("sync", 10, 1);

    if (marker >= 0) {
        write(marker, "=== WORKLOAD DONE ===\n", 22);
        close(marker);
    }

    printf("=== Done! ===\n");
    return 0;
}
