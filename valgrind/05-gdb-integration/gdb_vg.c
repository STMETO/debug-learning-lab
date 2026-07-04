#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Valgrind + GDB Integration ──
// When Valgrind finds a problem, let GDB come in to inspect the full scene

// A function that triggers Valgrind errors but continues running
void problematic_function() {
    int *arr = (int *)malloc(3 * sizeof(int));
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    // ★ out-of-bounds write -- Valgrind will report "Invalid write"
    // With --vgdb-error=0, Valgrind pauses before each error, waiting for GDB to connect
    arr[3] = 999;

    printf("  arr[0] = %d, arr[1] = %d, arr[2] = %d\n",
           arr[0], arr[1], arr[2]);

    free(arr);
}

// A function that uses uninitialized values
void uninit_in_action() {
    int value;   // ★ uninitialized

    // Valgrind: "Conditional jump depends on uninitialised value"
    if (value > 100) {
        printf("  value > 100: %d\n", value);
    } else {
        printf("  value <= 100: %d\n", value);
    }
}

int main() {
    printf("╔══════════════════════════════════════╗\n");
    printf("║   Valgrind + GDB Integration Experiment  ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    printf("[Scenario 1] Out-of-bounds write\n");
    problematic_function();

    printf("\n[Scenario 2] Uninitialized value\n");
    uninit_in_action();

    printf("\n=== Experiment complete ===\n");
    printf("\nHint: run in another terminal:\n");
    printf("  valgrind --vgdb=yes --vgdb-error=0 ./gdb_vg\n");
    printf("  Then in a second terminal:\n");
    printf("  gdb ./gdb_vg\n");
    printf("  (gdb) target remote | vgdb\n");
    return 0;
}
