#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Massif & Callgrind Dual Tool Demo ──
// Massif: heap memory profiling -- see "how much heap memory the program uses at each phase"
// Callgrind: call graph performance analysis -- see "which function CPU time is spent on"

// A CPU-intensive function (Callgrind will show it as "hot")
long compute_fibonacci(int n) {
    if (n <= 1) return n;
    return compute_fibonacci(n - 1) + compute_fibonacci(n - 2);
}

// A memory-intensive function (Massif will show heap growth)
char *build_report(int id, const char *text) {
    size_t len = strlen(text);
    // Each call allocates new memory -- Massif will track the wave-like changes of the heap
    char *report = (char *)malloc(len + 64);
    snprintf(report, len + 64, "[Report #%d] %s", id, text);
    return report;
}

int main() {
    printf("=== Massif & Callgrind Demo ===\n\n");

    // ── Phase 1: CPU-intensive (Callgrind focus) ──
    printf("[Phase 1] CPU-intensive -- computing fibonacci(40)\n");
    long fib = compute_fibonacci(40);
    printf("  fibonacci(40) = %ld\n", fib);

    // ── Phase 2: Memory-intensive (Massif focus) ──
    printf("\n[Phase 2] Memory-intensive -- building many reports\n");
    char *reports[20];
    for (int i = 0; i < 20; i++) {
        reports[i] = build_report(i, "this is a test report string for memory profiling");
    }
    printf("  Allocated 20 reports\n");

    // ── Phase 3: Free half (Massif will see heap drop) ──
    printf("\n[Phase 3] Free half of reports\n");
    for (int i = 0; i < 10; i++) {
        free(reports[i]);
        reports[i] = NULL;
    }
    printf("  Freed first 10\n");

    // ── Phase 4: Allocate more (Massif will see heap rise again) ──
    printf("\n[Phase 4] Allocate another batch\n");
    char *big_chunk = (char *)malloc(5 * 1024 * 1024);  // 5MB
    memset(big_chunk, 'X', 5 * 1024 * 1024);
    printf("  Allocated 5MB large chunk\n\n");

    // Cleanup (intentionally leave one report unfreed as a leak demo)
    for (int i = 10; i < 19; i++) {
        free(reports[i]);
    }
    free(big_chunk);
    // ★ reports[19] is not freed -- Massif will see one block still on the heap

    printf("=== Experiment complete ===\n");
    printf("\nValgrind usage:\n");
    printf("  Massif:   valgrind --tool=massif ./workload\n");
    printf("  Callgrind: valgrind --tool=callgrind ./workload\n");
    return 0;
}
