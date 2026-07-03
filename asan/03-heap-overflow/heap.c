#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 堆缓冲区溢出 —— 三种常见越界模式 ──

// 模式 1：向右越界（最常见：写到了分配区域的右边）
void overflow_right() {
    printf("【模式 1】向右越界（off-by-one）\n");

    int *arr = (int *)malloc(5 * sizeof(int));  // 5 个 int，索引 0~4
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }

    arr[5] = 999;   // ★ 越界！索引 5 是第 6 个元素
    printf("  arr[5] = %d （这行不会执行）\n", arr[5]);

    free(arr);
}

// 模式 2：向左越界（负索引——少见但致命）
void overflow_left() {
    printf("【模式 2】向左越界（负索引）\n");

    int *arr = (int *)malloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) {
        arr[i] = i;
    }

    arr[-1] = 777;   // ★ 越界！写到了分配区域之前
    printf("  arr[-1] = %d （这行不会执行）\n", arr[-1]);

    free(arr);
}

// 模式 3：循环中越界（循环的终止条件写错了）
void overflow_in_loop() {
    printf("【模式 3】循环越界（边界条件错误）\n");

    int *arr = (int *)malloc(5 * sizeof(int));

    // BUG: 应该是 i < 5，写成了 i <= 5
    for (int i = 0; i <= 5; i++) {
        arr[i] = i * 100;   // 当 i=5 时越界
    }

    printf("  arr[4] = %d\n", arr[4]);
    free(arr);
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== 堆缓冲区溢出实验 ===\n\n");

    switch (mode) {
        case 1: overflow_right();     break;
        case 2: overflow_left();      break;
        case 3: overflow_in_loop();   break;
        default:
            printf("用法: %s <mode>\n", argv[0]);
            printf("  mode 1: 向右越界 (arr[5])\n");
            printf("  mode 2: 向左越界 (arr[-1])\n");
            printf("  mode 3: 循环越界 (i<=5)\n");
    }

    printf("\n（如果没有 ASan 报告，bug 没被检测到——但这不意味着没有 bug）\n");
    return 0;
}
