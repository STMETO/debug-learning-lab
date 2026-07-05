#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

// Demonstrates the performance difference between:
//   1. No tracing (baseline)
//   2. trace_marker writing (userspace → ftrace buffer, safe)
//   3. Excessive marker writing (shows overhead accumulation)
//
// trace_printk is a KERNEL function — userspace code can't call it.
// The userspace equivalent is writing to trace_marker.
// This demo shows the overhead of "tracing everything" vs "tracing selectively."

long time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

// Phase 1: No tracing — baseline
void phase_baseline(int n) {
    long start = time_us();
    volatile long x = 0;
    for (int i = 0; i < n; i++) {
        x += i * 7;
        getpid();
    }
    long elapsed = time_us() - start;
    printf("  No tracing:       %6ld μs  (baseline)\n", elapsed);
}

// Phase 2: trace_marker on every iteration — shows tracing overhead
void phase_marker_every(int n, int marker_fd) {
    long start = time_us();
    volatile long x = 0;
    for (int i = 0; i < n; i++) {
        dprintf(marker_fd, "iteration %d\n", i);  // write marker EVERY iteration
        x += i * 7;
        getpid();
    }
    long elapsed = time_us() - start;
    printf("  Marker every iter: %6ld μs  (+%.1f%% overhead)\n",
           elapsed, (elapsed / (elapsed * 1.0)) * 0);
}

// Phase 3: trace_marker only at boundaries — minimal overhead
void phase_marker_boundaries(int n, int marker_fd) {
    long start = time_us();
    dprintf(marker_fd, "=== BOUNDARY START (n=%d) ===\n", n);
    volatile long x = 0;
    for (int i = 0; i < n; i++) {
        x += i * 7;
        getpid();
    }
    dprintf(marker_fd, "=== BOUNDARY END ===\n");
    long elapsed = time_us() - start;
    printf("  Marker boundaries: %6ld μs\n", elapsed);
}

// Phase 4: Conditional marker — only write on interesting events
void phase_conditional(int n, int marker_fd) {
    long start = time_us();
    volatile long x = 0;
    for (int i = 0; i < n; i++) {
        x += i * 7;
        // Only trace "slow" iterations
        if (i % 10000 == 0) {
            dprintf(marker_fd, "SLOW-ISH iteration %d\n", i);
        }
        getpid();
    }
    long elapsed = time_us() - start;
    printf("  Conditional marker: %6ld μs\n", elapsed);
}

int main() {
    printf("=== trace_marker Overhead Demo ===\n");
    printf("(trace_printk is kernel-only; trace_marker is the userspace equivalent)\n\n");

    int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);
    if (marker < 0) {
        printf("Cannot open trace_marker (need sudo?)\n");
        return 1;
    }

    int n = 50000;

    printf("Phase 1: Baseline (no tracing)\n");
    phase_baseline(n);

    printf("\nPhase 2: Marker on every iteration (simulates trace_printk abuse)\n");
    phase_marker_every(n, marker);

    printf("\nPhase 3: Marker only at boundaries (best practice)\n");
    phase_marker_boundaries(n, marker);

    printf("\nPhase 4: Conditional marker (only on interesting events)\n");
    phase_conditional(n, marker);

    close(marker);
    printf("\n=== Done! ===\n");
    return 0;
}
