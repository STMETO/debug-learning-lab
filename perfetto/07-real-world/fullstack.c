#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

// A program with performance issues at THREE different layers.
// Your job: use Perfetto's ftrace + perf data sources to find them all.
//
// Layer 1 (CPU):    compute_stats — heavy arithmetic in a tight loop
// Layer 2 (Kernel): sync_files   — fsync triggers block layer I/O
// Layer 3 (Sched):  fork_workers — many short-lived processes

void compute_stats(int n) {
    volatile long sum = 0;
    for (int i = 0; i < n; i++) {
        sum += (i * 1103515245 + 12345) % 10007;
    }
    printf("  [compute] sum = %ld\n", sum);
}

void sync_files(int count) {
    char path[64];
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "/tmp/fullstack_sync_%d.txt", i);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            write(fd, "sync me!", 8);
            fsync(fd);   // <-- forces disk flush — visible in ftrace
            close(fd);
        }
    }
}

void fork_workers(int count) {
    for (int i = 0; i < count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child: quick work, then exit
            getpid();
            _exit(0);
        } else {
            waitpid(pid, NULL, 0);
        }
    }
}

int main() {
    printf("=== Full-Stack Performance Debugging ===\n");
    int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);

    // Phase 1: CPU bottleneck
    if (marker >= 0) write(marker, "B|0|phase_compute\n", 18);
    printf("Phase 1: compute_stats (CPU hotspot)...\n");
    compute_stats(80000000);
    if (marker >= 0) write(marker, "E|0|phase_compute\n", 18);
    printf("  (Check: linux.perf should show compute_stats as hot)\n\n");

    // Phase 2: Kernel I/O bottleneck
    if (marker >= 0) write(marker, "B|0|phase_sync\n", 15);
    printf("Phase 2: sync_files (triggers fsync → block layer)...\n");
    sync_files(10);
    if (marker >= 0) write(marker, "E|0|phase_sync\n", 15);
    printf("  (Check: linux.ftrace should show ext4/block events)\n\n");

    // Phase 3: Scheduling bottleneck
    if (marker >= 0) write(marker, "B|0|phase_fork\n", 15);
    printf("Phase 3: fork_workers (scheduling pressure)...\n");
    fork_workers(30);
    if (marker >= 0) write(marker, "E|0|phase_fork\n", 15);
    printf("  (Check: sched/sched_switch should spike during this phase)\n\n");

    if (marker >= 0) close(marker);
    printf("=== Done! Open trace in Perfetto UI to see all three layers. ===\n");
    return 0;
}
