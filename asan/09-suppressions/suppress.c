#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 抑制文件实验 ──
// 在大型项目中，你可能有一些已知但暂时无法修复的 ASan 警告
// 抑制文件让你"屏蔽"已知问题，聚焦新 bug

// 场景 1：一个已知的堆溢出
void known_overflow() {
    int *arr = (int *)malloc(3 * sizeof(int));
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 999;   // ★ 已知 bug，暂时不修（假设有历史原因）
    free(arr);
}

// 场景 2：一个新的 use-after-free
void new_uaf_bug() {
    int *ptr = (int *)malloc(sizeof(int) * 5);
    ptr[0] = 42;
    free(ptr);
    ptr[1] = 100;   // ★ 新 bug，需要被 ASan 报告
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== 抑制文件实验 ===\n\n");

    switch (mode) {
        case 1:
            printf("运行已知堆溢出...\n");
            known_overflow();
            break;
        case 2:
            printf("运行新的 UAF bug...\n");
            new_uaf_bug();
            break;
        case 3:
            printf("运行两个 bug...\n");
            printf("  (不带抑制文件时两个都会报)\n");
            known_overflow();
            new_uaf_bug();
            break;
        default:
            printf("用法: %s <mode>\n", argv[0]);
            printf("  mode 1: 已知堆溢出\n");
            printf("  mode 2: 新的 UAF\n");
            printf("  mode 3: 两个都运行\n");
            printf("\n抑制文件用法:\n");
            printf("  ASAN_OPTIONS=suppressions=known_bugs.supp %s 3\n", argv[0]);
    }

    return 0;
}
