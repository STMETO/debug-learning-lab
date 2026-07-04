#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// ── Decoding every format of ltrace output ──
// Rich variety of library function calls showing various return types and source markers

int main() {
    printf("=== ltrace Output Format Experiment ===\n\n");

    // ── Type 1: Return integer (normal or count) ──
    printf("[A] Functions Returning Integers\n");
    int len = strlen("measure me");
    printf("  strlen returned: %d\n", len);

    int cmp = strcmp("abc", "abd");
    printf("  strcmp(\"abc\", \"abd\") returned: %d\n", cmp);

    // ── Type 2: Return pointer ──
    printf("\n[B] Functions Returning Pointers\n");
    char *p = strdup("hello ltrace");
    printf("  strdup returned: %p\n", (void *)p);

    char *found = strchr(p, 'l');
    printf("  strchr('%s', 'l') returned: %p (points to '%s')\n", p, (void *)found, found);

    free(p);

    // ── Type 3: Return NULL (failure) ──
    printf("\n[C] Functions Returning NULL\n");
    FILE *fp = fopen("/tmp/nonexistent_dir/file.txt", "r");
    printf("  fopen(nonexistent) returned: %p\n", (void *)fp);
    // ltrace will show: fopen(...) = nil

    // ── Type 4: Math functions returning floating point ──
    printf("\n[D] Math Library Functions\n");
    double root = sqrt(64.0);
    printf("  sqrt(64) returned: %.1f\n", root);

    double power = pow(2.0, 10.0);
    printf("  pow(2, 10) returned: %.0f\n", power);

    // ── Type 5: Functions with structs/buffers ──
    printf("\n[E] Functions Modifying Buffers\n");
    char dest[32];
    memset(dest, 0, sizeof(dest));
    strcpy(dest, "copied string");
    printf("  after strcpy: %s\n", dest);

    // memcpy returns the destination pointer
    char *ret = memcpy(dest, "overwritten", 11);
    printf("  memcpy returned: %p (== dest? %s)\n", (void *)ret,
           ret == dest ? "yes" : "no");

    // Compare with memcmp
    int eq = memcmp(dest, "overwritten", 10);
    printf("  memcmp returned: %d (0==equal)\n", eq);

    // ── Type 6: Functions that can error ──
    printf("\n[F] Functions That Can Fail\n");
    // Free a non-malloc pointer — error!
    // Not actually running this to avoid crash, but you can imagine ltrace output:
    // free((void *)0x7fff...) = <void> (then libc internally detects error, calls abort)

    printf("  (Skipping actual error -- just explaining ltrace display format)\n");

    printf("\n=== Experiment End ===\n");
    return 0;
}
