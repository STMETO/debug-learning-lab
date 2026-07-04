#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 栈缓冲区溢出 —— localarrayout-of-bounds ──
// 和Heap Overflow的区别：report中的shadowbytesYes f1/f2/f3 而不Yes fa

// Scenario 1：固定arrayout-of-bounds
void overflow_fixed() {
    printf("[Scenario 1] Fixed-length array overflow\n");

    char buf[10];     // 10 bytes的on stack缓冲区
    strcpy(buf, "hello");
    printf("  buf = %s\n", buf);

    // ★ write 20 个bytes到 10 bytes的缓冲区
    strcpy(buf, "this string is way too long for the buffer");
    printf("  buf = %s (this line will not execute)\n", buf);
}

// Scenario 2：循环中的栈out-of-bounds（最隐蔽的 bug）
void overflow_loop() {
    printf("[Scenario 2] Loop write overflow\n");

    int scores[5] = {0};   // 5 个 int 的栈array

    // 假装要compute什么
    printf("  Computing some scores...\n");

    // BUG: 应该Yes i < 5
    for (int i = 0; i <= 5; i++) {
        scores[i] = i * 10 + 5;   // i=5 时out-of-bounds
    }

    printf("  scores[4] = %d\n", scores[4]);
}

// Scenario 3：memcpy 溢出
void overflow_memcpy() {
    printf("[Scenario 3] memcpy stack overflow\n");

    int dest[3];                   // 3 个 int = 12 bytes
    int src[10] = {1,2,3,4,5,6,7,8,9,10};

    // BUG: 拷贝 10 个 int 到只能装 3 个的array
    memcpy(dest, src, 10 * sizeof(int));

    printf("  dest[0] = %d\n", dest[0]);
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== Stack Buffer Overflow Experiment ===\n\n");

    switch (mode) {
        case 1: overflow_fixed();   break;
        case 2: overflow_loop();    break;
        case 3: overflow_memcpy();  break;
        default:
            printf("Usage: %s <mode>\n", argv[0]);
            printf("  mode 1: strcpy overflow\n");
            printf("  mode 2: Loop overflow\n");
            printf("  mode 3: memcpy overflow\n");
    }

    return 0;
}
