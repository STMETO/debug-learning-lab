#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

// A program that creates multiple child processes doing different
// types of work — designed to generate interesting scheduling events
// for "perf sched" analysis.

// CPU-intensive: tight loop — should use its full time slice
void cpu_worker(int id, int duration_ms) {
    struct timeval start, now;
    gettimeofday(&start, NULL);
    volatile long x = 0;
    while (1) {
        gettimeofday(&now, NULL);
        long elapsed = (now.tv_sec - start.tv_sec) * 1000 +
                       (now.tv_usec - start.tv_usec) / 1000;
        if (elapsed >= duration_ms) break;
        for (int i = 0; i < 100000; i++) {
            x += i * 7;
        }
    }
    printf("  [cpu-worker-%d] done (%ld iterations)\n", id, x);
}

// Mixed: alternates between CPU work and sleep — gets scheduled in/out
void mixed_worker(int id, int rounds) {
    for (int i = 0; i < rounds; i++) {
        // Short CPU burst
        volatile long x = 0;
        for (int j = 0; j < 50000; j++) {
            x += j;
        }
        // Brief sleep — yields CPU
        usleep(1000);  // 1ms
    }
    printf("  [mixed-worker-%d] done\n", id);
}

int main() {
    printf("=== Scheduling Analysis Demo ===\n");
    int n_cpu = 3;
    int n_mixed = 2;

    // Spawn CPU-bound children (will compete for CPU)
    for (int i = 0; i < n_cpu; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            cpu_worker(i, 200);  // 200ms each
            _exit(0);
        }
    }

    // Spawn mixed workers (frequent sleep/wakeup)
    for (int i = 0; i < n_mixed; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            mixed_worker(i, 20);  // 20 rounds of work+sleep
            _exit(0);
        }
    }

    // Wait for all children
    for (int i = 0; i < n_cpu + n_mixed; i++) {
        wait(NULL);
    }

    printf("=== Done! ===\n");
    return 0;
}
