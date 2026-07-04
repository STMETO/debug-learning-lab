#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 堆缓冲区溢出 —— 三种常见out-of-bounds模式 ──

// 模式 1：向右out-of-bounds（最常见：写到了allocate/allocation区域的右边）
void overflow_right() {
    printf("[Mode 1] Right overflow (off-by-one)\n");

    int *arr = (int *)malloc(5 * sizeof(int));  // 5 个 int，索引 0~4
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }

    arr[5] = 999;   // ★ out-of-bounds！索引 5 Yes第 6 个元素
    printf("  arr[5] = %d (this line will not execute)\n", arr[5]);

    free(arr);
}

// 模式 2：向左out-of-bounds（负索引——少见但致命）
void overflow_left() {
    printf("[Mode 2] Left overflow (negative index)\n");

    int *arr = (int *)malloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) {
        arr[i] = i;
    }

    arr[-1] = 777;   // ★ out-of-bounds！写到了allocate/allocation区域之前
    printf("  arr[-1] = %d (this line will not execute)\n", arr[-1]);

    free(arr);
}

// 模式 3：循环中out-of-bounds（循环的终止条件写错了）
void overflow_in_loop() {
    printf("[Mode 3] Loop overflow (boundary condition error)\n");

    int *arr = (int *)malloc(5 * sizeof(int));

    // BUG: 应该Yes i < 5，写成了 i <= 5
    for (int i = 0; i <= 5; i++) {
        arr[i] = i * 100;   // 当 i=5 时out-of-bounds
    }

    printf("  arr[4] = %d\n", arr[4]);
    free(arr);
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== Heap Buffer Overflow Experiment ===\n\n");

    switch (mode) {
        case 1: overflow_right();     break;
        case 2: overflow_left();      break;
        case 3: overflow_in_loop();   break;
        default:
            printf("Usage: %s <mode>\n", argv[0]);
            printf("  mode 1: Right overflow (arr[5])\n");
            printf("  mode 2: Left overflow (arr[-1])\n");
            printf("  mode 3: Loop overflow (i<=5)\n");
    }

    printf("\n(If no ASan report appears, the bug was not detected -- but that does not mean there is no bug)\n");
    return 0;
}
