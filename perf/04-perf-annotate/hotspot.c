#include <stdio.h>
#include <stdlib.h>

// A single hot function designed for "perf annotate" analysis.
// The loop contains different instruction patterns so you can
// see which specific operations dominate the CPU time.

// hotspot: the function we'll drill into with perf annotate
void hotspot(long *data, int n) {
    long total = 0;
    for (int i = 0; i < n; i++) {
        // Line A: arithmetic — should be cheap (few cycles)
        long val = data[i] * 7 + 13;

        // Line B: modulo — division is expensive (many cycles)
        val = val % 9973;

        // Line C: conditional — branch
        if (val > 5000) {
            total += val;
        } else {
            total -= val;
        }

        // Line D: store back (memory write)
        data[i] = val;
    }
    printf("  total = %ld\n", total);
}

int main() {
    printf("=== Hotspot for perf annotate ===\n");

    int n = 2000000;
    long *data = (long *)malloc(n * sizeof(long));
    if (data == NULL) {
        printf("malloc failed\n");
        return 1;
    }

    // Fill with initial values
    for (int i = 0; i < n; i++) {
        data[i] = i;
    }

    printf("Running hotspot loop (%d iterations)...\n", n);
    hotspot(data, n);

    free(data);
    printf("=== Done! ===\n");
    return 0;
}
