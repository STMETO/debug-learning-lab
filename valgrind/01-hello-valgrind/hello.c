#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Hello Valgrind ──
// Run with valgrind ./hello to see memory error reports
// No -fsanitize=address needed! No recompilation needed!

void heap_overflow() {
    int *arr = (int *)malloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) arr[i] = i * 10;

    arr[5] = 999;   // ★ out-of-bounds write -- Valgrind will report "Invalid write of size 4"
    printf("  arr[5] = %d (this line may not execute)\n", arr[5]);
    free(arr);
}

void memory_leak() {
    int *data = (int *)malloc(100 * sizeof(int));
    for (int i = 0; i < 100; i++) data[i] = i;
    printf("  Allocated and used 400 bytes, but forgot to free\n");
    // ★ no free(data) -- Valgrind will report "definitely lost: 400 bytes"
}

int main() {
    printf("=== Hello Valgrind ===\n\n");

    printf("[Scenario 1] Heap buffer overflow\n");
    heap_overflow();

    printf("\n[Scenario 2] Memory leak\n");
    memory_leak();

    printf("\n=== Experiment complete ===\n");
    return 0;
}
