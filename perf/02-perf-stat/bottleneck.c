#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Three functions, each with a different performance bottleneck.
// Run them under "perf stat" and compare the counter signatures.
//
// Usage:
//   ./bottleneck        → run all three functions
//   ./bottleneck 1      → only compute_bound
//   ./bottleneck 2      → only memory_bound
//   ./bottleneck 3      → only branch_heavy

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

int main(int argc, char *argv[]) {
    int choice = 0;  // 0 = all, 1 = compute, 2 = memory, 3 = branch

    if (argc > 1) {
        choice = atoi(argv[1]);
        if (choice < 0 || choice > 3) {
            printf("Usage: %s [1|2|3]\n", argv[0]);
            printf("  1 = compute-bound  (tight arithmetic loop)\n");
            printf("  2 = memory-bound   (strided array access)\n");
            printf("  3 = branch-heavy   (unpredictable branches)\n");
            printf("  (no arg)           run all three\n");
            return 1;
        }
    }

    if (choice == 0) {
        printf("=== Three Bottleneck Patterns ===\n\n");
        printf("A: compute-bound (tight arithmetic loop)\n");
        compute_bound();
        printf("\nB: memory-bound (strided array access)\n");
        memory_bound();
        printf("\nC: branch-heavy (unpredictable branches)\n");
        branch_heavy();
    } else if (choice == 1) {
        printf("=== Bottleneck: Compute-Bound ===\n");
        compute_bound();
    } else if (choice == 2) {
        printf("=== Bottleneck: Memory-Bound ===\n");
        memory_bound();
    } else if (choice == 3) {
        printf("=== Bottleneck: Branch-Heavy ===\n");
        branch_heavy();
    }

    printf("\n=== Done! ===\n");
    return 0;
}
