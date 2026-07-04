#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Four leak types -- Valgrind-level classification ──
// LSAN only splits direct/indirect, Valgrind classifies more finely

// Global pointer -- used to create "still reachable"
char *global_ptr = NULL;

// Type 1: Definitely lost -- direct leak, pointer lost
void definitely_lost() {
    printf("[Type 1] Definitely lost -- direct leak\n");
    int *p = (int *)malloc(sizeof(int) * 10);
    p[0] = 42;
    // ★ After function returns, p is destroyed, 400 bytes have no pointer referencing them
    printf("  Allocated 10 ints, pointer about to be lost...\n");
}

// Type 2: Indirectly lost -- indirect leak
struct Container {
    int *data;
    char *name;
};

void indirectly_lost() {
    printf("[Type 2] Indirectly lost -- indirect leak\n");

    struct Container *c = (struct Container *)malloc(sizeof(struct Container));
    c->data = (int *)malloc(100 * sizeof(int));
    c->name = strdup("container-data");

    printf("  Allocated Container + data + name\n");
    // ★ Only c itself is leaked (pointers to data and name are lost)
    // c leaked -> data and name become "indirectly lost"
    // But if free(c->data) and free(c->name) first, then free(c), there would be no leak
    (void)c;  // no free(c)!
}

// Type 3: Still reachable -- pointer still reachable at exit
void still_reachable() {
    printf("[Type 3] Still reachable -- pointer still reachable\n");

    global_ptr = (char *)malloc(256);
    strcpy(global_ptr, "this is reachable at exit");
    // ★ At program exit, global_ptr still points to this memory
    // Strictly not a bug (OS reclaims at exit), but may hide real leaks
    printf("  global_ptr still points to 256 bytes\n");
}

// Type 4: Possibly lost -- possible leak
void possibly_lost() {
    printf("[Type 4] Possibly lost -- possible leak\n");

    char *buf = (char *)malloc(100);
    strcpy(buf, "hello");

    // ★ Create a "possibly lost" scenario: pointer is offset
    char *interior = buf + 10;
    // If interior is the only retained pointer, Valgrind cannot determine
    // whether it's an intentional offset or the original pointer was lost

    // Here only the interior pointer is saved, original buf value is lost
    (void)interior;
    // But since we didn't save interior to a global either, both are actually lost
}

int main() {
    printf("╔══════════════════════════════════════╗\n");
    printf("║   Valgrind Four Leak Classifications  ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    definitely_lost();
    printf("\n");
    indirectly_lost();
    printf("\n");
    still_reachable();
    printf("\n");
    possibly_lost();

    printf("\n=== Program end ===\n");
    printf("(Use valgrind --leak-check=full ./leaks to view detailed report)\n");
    return 0;
}
