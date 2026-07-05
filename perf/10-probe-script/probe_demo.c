#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// A program with clearly named functions — designed for "perf probe"
// dynamic tracing. Each function does a specific, traceable operation.

// ----- Probe-able functions -----

// processing data in a loop — probe here to count calls and latency
void process_item(int id, int value) {
    // Simulate variable processing time
    volatile int work = value * 7;
    for (int i = 0; i < value % 100; i++) {
        work += i;
    }
    // Prevent optimization
    (void)work;
}

// batch processing — probe at entry and exit to measure total time
void process_batch(int *data, int n) {
    for (int i = 0; i < n; i++) {
        process_item(i, data[i]);
    }
}

// memory allocation wrapper — probe to track allocation patterns
void *my_allocate(size_t size) {
    return malloc(size);
}

void my_deallocate(void *ptr) {
    free(ptr);
}

// compute operation — probe to see arguments
long compute_result(int a, int b, int c) {
    long r = (long)a * b + c;
    // Do some extra work to make it traceable
    volatile long v = r;
    for (int i = 0; i < 1000; i++) {
        v += i;
    }
    return v;
}

// ----- Main -----

int main() {
    printf("=== perf probe Demo ===\n");

    // 1. Allocate some memory
    printf("Allocating work buffer...\n");
    int n = 500;
    int *data = (int *)my_allocate(n * sizeof(int));
    if (!data) { printf("malloc failed\n"); return 1; }

    // 2. Fill with values
    for (int i = 0; i < n; i++) {
        data[i] = (i * 13 + 7) % 200;
    }

    // 3. Process the batch (this is the main trace target)
    printf("Processing batch (%d items)...\n", n);
    process_batch(data, n);

    // 4. Do some compute operations
    printf("Computing results...\n");
    long total = 0;
    for (int i = 0; i < 100; i++) {
        total += compute_result(i, i * 2, i * 3);
    }
    printf("  total = %ld\n", total);

    // 5. Clean up
    my_deallocate(data);

    printf("=== Done! ===\n");
    return 0;
}
