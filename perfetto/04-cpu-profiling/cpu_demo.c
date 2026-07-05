#include <stdio.h>
#include <stdlib.h>

// A CPU-intensive program for Perfetto linux.perf profiling.
// Has two distinct hot functions so you can see them in the profile.

// Hot function 1: pure arithmetic (predictable, high IPC)
void compute_loop(long iters) {
    volatile long sum = 0;
    for (long i = 0; i < iters; i++) {
        sum += (i * 7 + 13) % 9973;
    }
    printf("  [compute] sum = %ld\n", sum);
}

// Hot function 2: division-heavy (expensive instruction)
void divide_loop(long iters) {
    volatile long result = 1;
    for (long i = 1; i < iters; i++) {
        result = (result * 1103515245 + 12345) % i;
    }
    printf("  [divide] result = %ld\n", result);
}

// Cold function — should barely show up in the profile
void cold_setup() {
    int *arr = (int *)malloc(100 * sizeof(int));
    if (arr) {
        for (int i = 0; i < 100; i++) arr[i] = i;
        free(arr);
    }
}

int main() {
    printf("=== CPU Profiling Demo ===\n");

    printf("Setup...\n");
    cold_setup();

    printf("Phase 1: compute_loop (30M iterations)...\n");
    compute_loop(30000000);

    printf("Phase 2: divide_loop (8M iterations, modulo-heavy)...\n");
    divide_loop(8000000);

    printf("Phase 3: compute_loop again (20M iterations)...\n");
    compute_loop(20000000);

    printf("=== Done! ===\n");
    return 0;
}
