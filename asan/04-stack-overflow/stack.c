#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 栈缓冲区溢出 —— 局部数组越界 ──
// 和堆溢出的区别：报告中的影子字节是 f1/f2/f3 而不是 fa

// 场景 1：固定数组越界
void overflow_fixed() {
    printf("【场景 1】固定长度数组越界\n");

    char buf[10];     // 10 字节的栈上缓冲区
    strcpy(buf, "hello");
    printf("  buf = %s\n", buf);

    // ★ 写入 20 个字节到 10 字节的缓冲区
    strcpy(buf, "this string is way too long for the buffer");
    printf("  buf = %s （这行不会执行）\n", buf);
}

// 场景 2：循环中的栈越界（最隐蔽的 bug）
void overflow_loop() {
    printf("【场景 2】循环写越界\n");

    int scores[5] = {0};   // 5 个 int 的栈数组

    // 假装要计算什么
    printf("  计算一些分数...\n");

    // BUG: 应该是 i < 5
    for (int i = 0; i <= 5; i++) {
        scores[i] = i * 10 + 5;   // i=5 时越界
    }

    printf("  scores[4] = %d\n", scores[4]);
}

// 场景 3：memcpy 溢出
void overflow_memcpy() {
    printf("【场景 3】memcpy 栈溢出\n");

    int dest[3];                   // 3 个 int = 12 字节
    int src[10] = {1,2,3,4,5,6,7,8,9,10};

    // BUG: 拷贝 10 个 int 到只能装 3 个的数组
    memcpy(dest, src, 10 * sizeof(int));

    printf("  dest[0] = %d\n", dest[0]);
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== 栈缓冲区溢出实验 ===\n\n");

    switch (mode) {
        case 1: overflow_fixed();   break;
        case 2: overflow_loop();    break;
        case 3: overflow_memcpy();  break;
        default:
            printf("用法: %s <mode>\n", argv[0]);
            printf("  mode 1: strcpy 溢出\n");
            printf("  mode 2: 循环越界\n");
            printf("  mode 3: memcpy 溢出\n");
    }

    return 0;
}
