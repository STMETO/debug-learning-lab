#include <stdio.h>
#include <stdlib.h>

// A multi-function program to demonstrate perf record/report.
// Each function does a different amount of work so you can
// see which ones are "hot" (high overhead) vs "cold" (low overhead).

// helper: called many times — small work each call, but adds up
int helper(int n) {
    int s = 0;
    for (int i = 0; i < n; i++) {
        s += i;
    }
    return s;
}

// hot_calc: the main hotspot — a tight loop doing arithmetic
void hot_calc() {
    long sum = 0;
    for (int i = 0; i < 30000000; i++) {
        sum += (i * 3 + 7) % 10007;
    }
    printf("  [hot_calc] sum = %ld\n", sum);
}

// medium_sort: bubble sort — O(n^2), moderate cost
void medium_sort(int *arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

// cold_init: called once, returns quickly — should be "cold" in perf report
void cold_init() {
    int *arr = (int *)malloc(100 * sizeof(int));
    if (arr == NULL) return;
    for (int i = 0; i < 100; i++) {
        arr[i] = 100 - i;
    }
    // Call helper 1000 times — helper will show up in the profile
    int total = 0;
    for (int i = 0; i < 1000; i++) {
        total += helper(i % 50 + 1);
    }
    printf("  [cold_init] helper total = %d\n", total);
    free(arr);
}

int main() {
    printf("=== Multi-function Workload ===\n");

    printf("Step 1: cold_init (one-time setup)\n");
    cold_init();

    printf("Step 2: hot_calc (the big hotspot)\n");
    hot_calc();

    printf("Step 3: medium_sort (moderate work)\n");
    int arr_size = 5000;
    int *arr = (int *)malloc(arr_size * sizeof(int));
    if (arr) {
        for (int i = 0; i < arr_size; i++) {
            arr[i] = arr_size - i;  // reverse order = worst case for bubble sort
        }
        medium_sort(arr, arr_size);
        printf("  [medium_sort] arr[0]=%d arr[%d]=%d\n", arr[0], arr_size-1, arr[arr_size-1]);
        free(arr);
    }

    printf("=== Done! ===\n");
    return 0;
}
