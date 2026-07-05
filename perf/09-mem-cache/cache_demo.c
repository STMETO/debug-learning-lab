#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════
// Cache & Memory Access Pattern Demo
//
// Demonstrates two common cache-related performance issues:
//   1. False sharing — threads fighting over the same cache line
//   2. Strided access — poor spatial locality
//
// Note: "perf mem" and "perf c2c" require native Linux (not WSL2).
//       This experiment uses "perf stat" L1-dcache events instead.
// ═══════════════════════════════════════════════════════════

#define ITERATIONS 50000000

// ----- Part 1: Sequential vs Strided Access -----

// Good: sequential access — every cache line loaded is fully used
long sequential_sum(int *arr, int n) {
    long sum = 0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];  // stride = 1 element = 4 bytes → cache-friendly
    }
    return sum;
}

// Bad: strided access — only 1 element used per cache line (64 bytes)
long strided_sum(int *arr, int n, int step) {
    long sum = 0;
    for (int i = 0; i < n; i += step) {
        sum += arr[i];  // stride = step * 4 bytes → cache-unfriendly if step >= 16
    }
    return sum;
}

// ----- Part 2: False Sharing Demo -----

// Two counters in the same cache line (64 bytes).
// int = 4 bytes → they fit within a single 64-byte cache line.
// When two threads each update one counter, the cache line bounces
// between CPU cores — each write invalidates the other core's copy.
struct shared_counters {
    volatile long counter_a;  // Thread A increments this
    volatile long counter_b;  // Thread B increments this
    // Both are in the same cache line → FALSE SHARING!
};

// Fixed version: add padding to put each counter in its own cache line
struct padded_counters {
    volatile long counter_a;
    char _padding1[64 - sizeof(long)];  // pad to 64 bytes
    volatile long counter_b;
    char _padding2[64 - sizeof(long)];  // pad to 64 bytes
};

struct shared_counters shared = {0, 0};
struct padded_counters padded = {0, "", 0, ""};

void *inc_a(void *unused) {
    (void)unused;
    for (long i = 0; i < ITERATIONS; i++) {
        shared.counter_a++;
    }
    return NULL;
}

void *inc_b(void *unused) {
    (void)unused;
    for (long i = 0; i < ITERATIONS; i++) {
        shared.counter_b++;
    }
    return NULL;
}

void *inc_a_padded(void *unused) {
    (void)unused;
    for (long i = 0; i < ITERATIONS; i++) {
        padded.counter_a++;
    }
    return NULL;
}

void *inc_b_padded(void *unused) {
    (void)unused;
    for (long i = 0; i < ITERATIONS; i++) {
        padded.counter_b++;
    }
    return NULL;
}

// ----- Main -----

int main() {
    printf("=== Cache & Memory Access Demo ===\n\n");

    // Part 1: Sequential vs Strided
    printf("--- Part 1: Sequential vs Strided Access ---\n");
    int n = 8000000;
    int *arr = (int *)malloc(n * sizeof(int));
    if (!arr) { printf("malloc failed\n"); return 1; }
    for (int i = 0; i < n; i++) arr[i] = i;

    printf("Sequential sum...\n");
    long s1 = sequential_sum(arr, n);
    printf("  result = %ld\n", s1);

    printf("Strided sum (step=64 = 256 bytes = 4 cache lines)...\n");
    long s2 = strided_sum(arr, n, 64);
    printf("  result = %ld\n", s2);
    free(arr);

    // Part 2: False Sharing
    printf("\n--- Part 2: False Sharing ---\n");

    printf("Without padding (false sharing)...\n");
    pthread_t t1, t2;
    pthread_create(&t1, NULL, inc_a, NULL);
    pthread_create(&t2, NULL, inc_b, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("With padding (no false sharing)...\n");
    pthread_create(&t1, NULL, inc_a_padded, NULL);
    pthread_create(&t2, NULL, inc_b_padded, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("  shared: a=%ld b=%ld\n", shared.counter_a, shared.counter_b);
    printf("  padded: a=%ld b=%ld\n", padded.counter_a, padded.counter_b);

    printf("\n=== Done! ===\n");
    return 0;
}
