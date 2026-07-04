#include <stdio.h>
#include <stdlib.h>

// ── Uninitialized value detection -- Valgrind's exclusive superpower ──
// ASan cannot detect this type of bug, compiler warnings may also miss it
// But Valgrind can precisely trace the complete path of "value from uninitialized to use"

// Scenario 1: conditional branch depends on uninitialized value
void branch_on_uninit() {
    printf("[Scenario 1] Branch depends on uninitialized value\n");

    int x;  // ★ not initialized!

    if (x > 0) {        // Valgrind: "Conditional jump or move depends on uninitialised value(s)"
        printf("  x is positive: %d\n", x);
    } else {
        printf("  x is zero or negative: %d\n", x);
    }
}

// Scenario 2: uninitialized value used as function argument
void use_uninit_as_arg() {
    printf("[Scenario 2] Uninitialized value as argument\n");

    int scores[10];     // ★ not initialized!
    int total = 0;

    // Only filled first 5, forgot to fill the last 5
    for (int i = 0; i < 5; i++) {
        scores[i] = i * 10;
    }

    // ★ Last 5 elements are garbage values, but are used in calculation
    for (int i = 0; i < 10; i++) {
        total += scores[i];
    }
    printf("  total = %d (result may differ each run!)\n", total);
}

// Scenario 3: uninitialized heap memory
void uninit_heap() {
    printf("[Scenario 3] Uninitialized heap memory\n");

    char *buf = (char *)malloc(32);  // malloc does not zero!
    // ★ buf content is random -- no memset or calloc used

    // Using garbage values in buf for strlen
    size_t len = strlen(buf);   // Valgrind: strlen reads uninitialized data
    printf("  strlen(uninitialized buf) = %zu (definitely wrong)\n", len);

    free(buf);
}

// Scenario 4: partially initialized struct
struct Point {
    int x;
    int y;
    int z;
};

void partially_init_struct() {
    printf("[Scenario 4] Partially initialized struct\n");

    struct Point p;
    p.x = 10;
    p.y = 20;
    // ★ forgot to initialize p.z!

    int sum = p.x + p.y + p.z;
    printf("  x+y+z = %d (z is garbage, result unpredictable)\n", sum);
}

int main() {
    printf("╔══════════════════════════════════════╗\n");
    printf("║  ⭐ Valgrind Exclusive: Uninitialized Value Detection ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    branch_on_uninit();
    printf("\n");
    use_uninit_as_arg();
    printf("\n");
    uninit_heap();
    printf("\n");
    partially_init_struct();

    printf("\n=== Experiment complete ===\n");
    return 0;
}
