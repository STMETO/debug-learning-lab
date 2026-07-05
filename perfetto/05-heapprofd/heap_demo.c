#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// A program with deliberate allocation patterns for heapprofd analysis.
// Includes: leak, large allocations, and repeated alloc/free cycles.

void leak_some(int count) {
    printf("  Leaking %d x 1MB allocations...\n", count);
    for (int i = 0; i < count; i++) {
        char *p = malloc(1024 * 1024);
        if (p) memset(p, 'L', 1024 * 1024);
        // deliberately not freed — will show as "leaked" in heapprofd
    }
}

void large_allocs() {
    printf("  Allocating 5 x 10MB blocks...\n");
    void *ptrs[5];
    for (int i = 0; i < 5; i++) {
        ptrs[i] = malloc(10 * 1024 * 1024);
        if (ptrs[i]) memset(ptrs[i], 0, 10 * 1024 * 1024);
    }
    for (int i = 0; i < 5; i++) free(ptrs[i]);
}

void rapid_alloc_free(int rounds) {
    printf("  %d rapid alloc/free cycles...\n", rounds);
    for (int i = 0; i < rounds; i++) {
        for (int sz = 64; sz <= 65536; sz *= 2) {
            void *p = malloc(sz);
            if (p) { memset(p, i & 0xFF, sz); free(p); }
        }
    }
}

int main() {
    printf("=== Heap Profiling Demo ===\n");

    printf("Phase 1: deliberate leak...\n");
    leak_some(10);  // 10MB leaked

    printf("Phase 2: large allocations...\n");
    large_allocs();

    printf("Phase 3: rapid alloc/free...\n");
    rapid_alloc_free(100);

    printf("Phase 4: more leaks...\n");
    leak_some(5);  // 5MB more leaked (15MB total leaked)

    printf("Sleeping 2s for heapprofd to capture...\n");
    sleep(2);

    printf("=== Done! (15MB deliberately not freed) ===\n");
    return 0;
}
