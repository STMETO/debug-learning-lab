#include <stdio.h>
#include <stdlib.h>

// A program with a multi-level call tree, designed to produce
// a clear and interesting FlameGraph visualization.
//
// Call tree structure (approximate CPU time in %):
//
// main (100%)
// ├── phase_compute (60%)
// │   ├── multiply_arrays (25%)
// │   │   ├── inner_product (15%)
// │   │   └── accumulate (10%)
// │   └── normalize (20%)
// │       ├── find_max (10%)
// │       └── scale_values (10%)
// ├── phase_sort (25%)
// │   ├── quick_sort (15%)          ← recursive, visible as stacked boxes
// │   └── partition (10%)
// └── phase_report (15%)
//     ├── format_output (10%)
//     └── checksum (5%)

// ----- phase_compute helpers -----

int inner_product(int *a, int *b, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

int accumulate(int *arr, int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        total += arr[i];
    }
    return total;
}

void multiply_arrays(int *a, int *b, int n) {
    int ip = inner_product(a, b, n);
    int ac = accumulate(a, n);
    // Do enough work to show up in the profile
    for (int i = 0; i < n; i++) {
        a[i] = (a[i] * b[i] + ip + ac) % 10007;
    }
}

int find_max(int *arr, int n) {
    int max = arr[0];
    for (int i = 1; i < n; i++) {
        if (arr[i] > max) max = arr[i];
    }
    return max;
}

void scale_values(int *arr, int n, int max) {
    for (int i = 0; i < n; i++) {
        arr[i] = (arr[i] * 1000) / (max + 1);
    }
}

void normalize(int *arr, int n) {
    int max = find_max(arr, n);
    // Repeat normalization several times to make it visible
    for (int r = 0; r < 200; r++) {
        scale_values(arr, n, max);
    }
}

void phase_compute(int *a, int *b, int n) {
    // Run multiple rounds to generate enough samples
    for (int r = 0; r < 50; r++) {
        multiply_arrays(a, b, n);
        normalize(a, n);
    }
}

// ----- phase_sort helpers -----
// Quick sort — recursive, will show stacked boxes in flame graph.
// Uses random pivot to guarantee balanced recursion (~log₂ n depth).

int partition(int *arr, int low, int high) {
    // Random pivot: swap a random element with arr[high] to avoid worst-case
    int pi = low + (rand() % (high - low + 1));
    int pv = arr[pi];
    arr[pi] = arr[high];
    arr[high] = pv;

    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j] < pv) {
            i++;
            int tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }
    }
    int tmp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = tmp;
    return i + 1;
}

void quick_sort(int *arr, int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

void phase_sort(int *arr, int n) {
    // Sort many rounds of a small array to keep recursion depth shallow.
    // n=50 → max recursion ~6 with random pivot, well within frame pointer limits.
    for (int r = 0; r < 4000; r++) {
        for (int i = 0; i < n; i++) {
            arr[i] = (arr[i] * 1103515245 + 12345) % 10007;
        }
        quick_sort(arr, 0, n - 1);
    }
}

// ----- phase_report helpers -----

int checksum(int *arr, int n) {
    int cs = 0;
    for (int i = 0; i < n; i++) {
        cs ^= arr[i];
    }
    return cs;
}

void format_output(int *arr, int n) {
    // Simulate formatting work
    long dummy = 0;
    for (int i = 0; i < n; i++) {
        dummy += (arr[i] * 7 + 13) % 9973;
    }
    // prevent optimization
    volatile long v = dummy;
    (void)v;
}

void phase_report(int *arr, int n) {
    for (int r = 0; r < 30; r++) {
        format_output(arr, n);
    }
    int cs = checksum(arr, n);
    printf("  [report] checksum = %d\n", cs);
}

// ----- main -----

int main() {
    printf("=== FlameGraph Demo: Multi-Level Call Tree ===\n");

    int n = 50;  // small array → ~6 levels max recursion, never hits stack depth limit
    int *a = (int *)malloc(n * sizeof(int));
    int *b = (int *)malloc(n * sizeof(int));
    if (!a || !b) {
        printf("malloc failed\n");
        return 1;
    }

    // Initialize arrays
    for (int i = 0; i < n; i++) {
        a[i] = (i * 7 + 13) % 10007;
        b[i] = (i * 3 + 17) % 5003;
    }

    printf("Phase 1: compute (60%% of CPU)...\n");
    phase_compute(a, b, n);

    printf("Phase 2: sort (25%% of CPU)...\n");
    phase_sort(a, n);

    printf("Phase 3: report (15%% of CPU)...\n");
    phase_report(a, n);

    printf("=== Done! ===\n");
    free(a);
    free(b);
    return 0;
}
