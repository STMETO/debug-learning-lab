#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── SuppressionfileExperiment ──
// 在大型项目中，你可能有一些known但暂时无法修复的 ASan 警告
// Suppressionfile让你"屏蔽"known问题，聚焦新 bug

// Scenario 1：一个known的Heap Overflow
void known_overflow() {
    int *arr = (int *)malloc(3 * sizeof(int));
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 999;   // ★ known bug，暂时不修（假设有历史原因）
    free(arr);
}

// Scenario 2：一个新的 use-after-free
void new_uaf_bug() {
    int *ptr = (int *)malloc(sizeof(int) * 5);
    ptr[0] = 42;
    free(ptr);
    ptr[1] = 100;   // ★ 新 bug，需要被 ASan report
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== Suppression File Experiment ===\n\n");

    switch (mode) {
        case 1:
            printf("Running known heap overflow...\n");
            known_overflow();
            break;
        case 2:
            printf("Running new UAF bug...\n");
            new_uaf_bug();
            break;
        case 3:
            printf("Running both bugs...\n");
            printf("  (Without suppression file, both will be reported)\n");
            known_overflow();
            new_uaf_bug();
            break;
        default:
            printf("Usage: %s <mode>\n", argv[0]);
            printf("  mode 1: Known heap overflow\n");
            printf("  mode 2: New UAF\n");
            printf("  mode 3: Run both\n");
            printf("\nSuppression file usage:\n");
            printf("  ASAN_OPTIONS=suppressions=known_bugs.supp %s 3\n", argv[0]);
    }

    return 0;
}
