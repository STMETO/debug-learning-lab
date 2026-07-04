#include <stdio.h>
#include <stdlib.h>

// A simple program to demonstrate perf stat counters.
// It has two distinct types of work so you can see how
// different code patterns affect CPU performance counters.

// compute_heavy: pure arithmetic in a tight loop (CPU-bound)
void compute_heavy() {
    long sum = 0;
    for (int i = 0; i < 10000000; i++) {
        sum += i * 3 + 7;
    }
    printf("  [compute] sum = %ld\n", sum);
}

// memory_heavy: walks through a large array (memory-bound)
void memory_heavy() {
    int *arr = (int *)malloc(2000000 * sizeof(int));
    if (arr == NULL) {
        printf("  [memory] malloc failed\n");
        return;
    }
    // Fill the array with data
    for (int i = 0; i < 2000000; i++) {
        arr[i] = i;
    }
    // Read back in a strided pattern (less cache-friendly)
    long total = 0;
    for (int i = 0; i < 2000000; i += 7) {
        total += arr[i];
    }
    printf("  [memory] total = %ld\n", total);
    free(arr);
}

int main() {
    printf("=== Hello from perf lab! ===\n");

    printf("Step 1: compute-heavy loop...\n");
    compute_heavy();

    printf("Step 2: memory-heavy access...\n");
    memory_heavy();

    printf("=== Done! ===\n");
    return 0;
}
