#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

// A program that does 500 file operations where most are fast,
// but every 50th operation does fsync — causing intermittent latency spikes.
//
// The user sees: "it's usually instant, but sometimes pauses for a moment."
// Your job: use ftrace to find WHY.

#define TOTAL_OPS 500
#define SYNC_INTERVAL 50   // every 50th op does fsync

long time_us(struct timeval *start) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - start->tv_sec) * 1000000L +
           (now.tv_usec - start->tv_usec);
}

int main() {
    printf("=== Intermittent Latency Test ===\n");
    printf("Total ops: %d, fsync every %d ops\n", TOTAL_OPS, SYNC_INTERVAL);

    int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);

    struct timeval start;
    gettimeofday(&start, NULL);

    for (int i = 0; i < TOTAL_OPS; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/tmp/ftrace_latency_%d.txt", i % 10);

        int do_sync = (i % SYNC_INTERVAL == 0);

        struct timeval op_start;
        gettimeofday(&op_start, NULL);

        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), "iteration-%d\n", i);
            write(fd, buf, strlen(buf));
            if (do_sync) {
                if (marker >= 0) {
                    char m[64];
                    snprintf(m, sizeof(m), ">>> FSYNC at iteration %d\n", i);
                    write(marker, m, strlen(m));
                }
                fsync(fd);
            }
            close(fd);
        }

        long elapsed = time_us(&op_start);

        if (do_sync) {
            printf("  op %3d [SYNC]  took %6ld μs  <<<\n", i, elapsed);
        } else if (elapsed > 500) {
            printf("  op %3d [SLOW]  took %6ld μs  <-- unexpected!\n", i, elapsed);
        }
    }

    long total = time_us(&start);
    printf("\n=== Total: %ld μs (%.2f ms) ===\n", total, total / 1000.0);

    if (marker >= 0) close(marker);
    return 0;
}
