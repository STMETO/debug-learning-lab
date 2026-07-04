#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Double Free与Invalid Free ──
// free 两、free 栈variable、free offsetpointer

// Scenario 1：classic double-free
void classic_double_free() {
    printf("[Scenario 1] Classic double-free\n");

    int *ptr = (int *)malloc(sizeof(int) * 10);
    for (int i = 0; i < 10; i++) {
        ptr[i] = i;
    }
    printf("  ptr[5] = %d\n", ptr[5]);

    free(ptr);      // 第一 free —— 正常
    printf("  First free complete\n");

    free(ptr);      // ★ 第二 free 同一个pointer！
    printf("  Second free complete (this line will not execute)\n");
}

// Scenario 2：两个pointer指向同一块memory
void alias_double_free() {
    printf("[Scenario 2] Alias double-free\n");

    char *a = strdup("important data");
    char *b = a;            // ★ b 和 a 指向同一块memory

    printf("  a = %s\n", a);
    printf("  b = %s\n", b);

    free(a);                // 第一 free —— 正常
    printf("  free(a) complete\n");

    free(b);                // ★ 第二 free —— b 指向的已经free了
    printf("  free(b) complete (this line will not execute)\n");
}

// Scenario 3：Invalid Free —— free 一个不Yes malloc 出来的pointer
void invalid_free() {
    printf("[Scenario 3] Invalid free -- freeing a stack variable\n");

    int stack_var = 42;
    int *ptr = &stack_var;

    printf("  stack_var = %d (on stack)\n", stack_var);

    free(ptr);      // ★ 试图 free 一个栈variable的地址！
    printf("  free complete (this line will not execute)\n");
}

// Scenario 4：free offset后的pointer（不Yes malloc 原始return值）
void offset_free() {
    printf("[Scenario 4] Free offset pointer\n");

    char *original = (char *)malloc(100);
    char *middle   = original + 50;  // 指向allocate/allocation区域中间

    strcpy(original, "data at the start");
    printf("  original = %s\n", original);

    free(middle);   // ★ free的不Yes malloc 的原return值！
    printf("  free(middle) complete (this line will not execute)\n");
    free(original); // 正确做法：只 free malloc 原始return的pointer
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== Double-Free & Invalid Free Experiment ===\n\n");

    switch (mode) {
        case 1: classic_double_free();  break;
        case 2: alias_double_free();    break;
        case 3: invalid_free();         break;
        case 4: offset_free();          break;
        default:
            printf("Usage: %s <mode>\n", argv[0]);
            printf("  mode 1: Classic double-free\n");
            printf("  mode 2: Alias double-free\n");
            printf("  mode 3: Free stack variable\n");
            printf("  mode 4: Free offset pointer\n");
    }

    return 0;
}
