#include <stdio.h>
#include <stdlib.h>

// Three functions, each with a different performance bottleneck.
// Run them under "perf stat" and compare the counter signatures.

// Bottleneck A: compute-bound — tight arithmetic, predictable branches
void compute_bound() {
    long result = 0;
    for (int i = 0; i < 50000000; i++) {
        result += (i * 7 + 13) % 9973;
    }
    printf("  [compute]  result = %ld\n", result);
}

// Bottleneck B: memory-bound — strided array access, poor cache locality
void memory_bound() {
    int size = 4000000;
    int *arr = (int *)malloc(size * sizeof(int));
    if (arr == NULL) {
        printf("  [memory]   malloc failed\n");
        return;
    }
    // Fill sequentially (cache-friendly)
    for (int i = 0; i < size; i++) {
        arr[i] = i;
    }
    // Read with large stride (cache-unfriendly — misses every cache line)
    long total = 0;
    int step = 1024;  // skip 4 KB per access = different cache line every time
    for (int i = 0; i < size; i += step) {
        total += arr[i];
    }
    printf("  [memory]   total = %ld\n", total);
    free(arr);
}

// Bottleneck C: branch-heavy — data-dependent branches, hard to predict
void branch_heavy() {
    // Generate random data so branches are unpredictable
    int n = 10000000;
    int *data = (int *)malloc(n * sizeof(int));
    if (data == NULL) {
        printf("  [branch]   malloc failed\n");
        return;
    }
    // Fill with pseudo-random numbers
    unsigned int seed = 12345;
    for (int i = 0; i < n; i++) {
        seed = seed * 1103515245 + 12345;
        data[i] = (seed >> 16) & 1;  // 0 or 1, roughly 50/50
    }
    // Branch on each element — branch predictor can't predict random input
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (data[i]) {
            count++;
        } else {
            count--;
        }
    }
    printf("  [branch]   count = %d\n", count);
    free(data);
}

int main() {
    printf("=== Three Bottleneck Patterns ===\n\n");

    printf("A: compute-bound (tight arithmetic loop)\n");
    compute_bound();

    printf("\nB: memory-bound (strided array access)\n");
    memory_bound();

    printf("\nC: branch-heavy (unpredictable branches)\n");
    branch_heavy();

    printf("\n=== Done! ===\n");
    return 0;
}
