#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── ASan + GDB 联动实验 ──
// 在 ASan 检测到错误但还没终止程序之前，用 GDB 抓住现场

// 一个会触发堆溢出的函数
void trigger_overflow(int index) {
    int *buf = (int *)malloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) {
        buf[i] = i * 10;
    }

    printf("准备访问 buf[%d]...\n", index);
    buf[index] = 999;    // ★ 如果 index >= 5，这里越界
    printf("写入成功（这行可能不会执行）\n");

    free(buf);
}

// 一个会触发 use-after-free 的函数
void trigger_uaf() {
    char *str = strdup("debug me");
    printf("字符串: %s\n", str);
    free(str);
    printf("释放后访问: %s\n", str);  // ★ UAF
}

int main(int argc, char *argv[]) {
    int mode = 1;
    int index = 5;   // 默认越界

    if (argc > 1) mode = atoi(argv[1]);
    if (argc > 2) index = atoi(argv[2]);

    printf("=== ASan + GDB 联动实验 ===\n\n");

    switch (mode) {
        case 1:
            trigger_overflow(index);
            break;
        case 2:
            trigger_uaf();
            break;
        default:
            printf("用法: %s <mode> [index]\n", argv[0]);
            printf("  mode 1: 堆溢出\n");
            printf("  mode 2: use-after-free\n");
    }

    return 0;
}
