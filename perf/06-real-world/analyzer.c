#include <stdio.h>
#include <stdlib.h>

// ═══════════════════════════════════════════════════════════
// Data Analyzer — a "slow" program with 3 hidden performance bugs.
// Your mission: use perf to find and fix all of them.
// ═══════════════════════════════════════════════════════════

#define DATA_ROWS 2000
#define DATA_COLS 3000

// Bug 1: column-major access on a row-major matrix.
// The inner loop walks COLUMNS with a stride of DATA_ROWS elements,
// missing the cache on almost every access.
void compute_column_stats(int *matrix, int rows, int cols, long *col_sums) {
    for (int c = 0; c < cols; c++) {
        long sum = 0;
        for (int r = 0; r < rows; r++) {
            sum += matrix[r * cols + c];  // <-- strided access, cache-unfriendly!
        }
        col_sums[c] = sum;
    }
}

// Bug 2: repeated computation of a loop-invariant value.
// ((rows * cols) / 2 + 1) is constant for the entire function, but is
// recomputed on every single iteration of the inner loop (6,000,000 times!).
// It should be hoisted out and stored in a variable before the loop.
void normalize_data(int *matrix, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int idx = i * cols + j;
            // BUG: ((rows * cols) / 2 + 1) is computed 6 million times
            matrix[idx] = (matrix[idx] * 1000) / ((rows * cols) / 2 + 1);
        }
    }
}

// Bug 3: data-dependent branch that's unpredictable.
// Random-ish input data means the branch predictor can't guess
// whether (val % 2 == 0), causing ~50% mispredict rate.
long classify_values(int *data, int n) {
    long evens = 0, odds = 0;
    for (int i = 0; i < n; i++) {
        if (data[i] % 2 == 0) {   // <-- unpredictable when data is random-ish
            evens++;
        } else {
            odds++;
        }
    }
    return evens - odds;
}

// A helper that does valid work but is NOT a bug — it's here to
// add realistic noise to the profile.
void stats_report(long *col_sums, int cols) {
    long grand_total = 0;
    for (int i = 0; i < cols; i++) {
        grand_total += col_sums[i];
    }
    printf("  Grand total of column sums: %ld\n", grand_total);
}

int main() {
    printf("=== Data Analyzer v1.0 ===\n");

    int *matrix = (int *)malloc(DATA_ROWS * DATA_COLS * sizeof(int));
    long *col_sums = (long *)malloc(DATA_COLS * sizeof(long));
    if (!matrix || !col_sums) {
        printf("malloc failed\n");
        return 1;
    }

    // Fill with pseudo-random data
    unsigned int seed = 42;
    for (int i = 0; i < DATA_ROWS * DATA_COLS; i++) {
        seed = seed * 1103515245 + 12345;
        matrix[i] = (seed >> 16) & 0x7FFF;
    }

    printf("\nStep 1: compute column stats (cache-unfriendly)...\n");
    compute_column_stats(matrix, DATA_ROWS, DATA_COLS, col_sums);

    printf("Step 2: normalize data (repeated computation)...\n");
    normalize_data(matrix, DATA_ROWS, DATA_COLS);

    printf("Step 3: classify values (branch-unpredictable)...\n");
    long result = classify_values(matrix, DATA_ROWS * DATA_COLS);

    printf("Step 4: generate report...\n");
    stats_report(col_sums, DATA_COLS);

    printf("\n=== Analysis complete. result = %ld ===\n", result);

    free(matrix);
    free(col_sums);
    return 0;
}
