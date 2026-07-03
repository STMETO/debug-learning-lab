#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 重复释放与非法释放 ──
// free 两次、free 栈变量、free 偏移指针

// 场景 1：经典 double-free
void classic_double_free() {
    printf("【场景 1】经典重复释放\n");

    int *ptr = (int *)malloc(sizeof(int) * 10);
    for (int i = 0; i < 10; i++) {
        ptr[i] = i;
    }
    printf("  ptr[5] = %d\n", ptr[5]);

    free(ptr);      // 第一次 free —— 正常
    printf("  第一次 free 完成\n");

    free(ptr);      // ★ 第二次 free 同一个指针！
    printf("  第二次 free 完成 （这行不会执行）\n");
}

// 场景 2：两个指针指向同一块内存
void alias_double_free() {
    printf("【场景 2】别名 double-free\n");

    char *a = strdup("important data");
    char *b = a;            // ★ b 和 a 指向同一块内存

    printf("  a = %s\n", a);
    printf("  b = %s\n", b);

    free(a);                // 第一次 free —— 正常
    printf("  free(a) 完成\n");

    free(b);                // ★ 第二次 free —— b 指向的已经释放了
    printf("  free(b) 完成 （这行不会执行）\n");
}

// 场景 3：非法释放 —— free 一个不是 malloc 出来的指针
void invalid_free() {
    printf("【场景 3】非法释放 —— free 栈上的变量\n");

    int stack_var = 42;
    int *ptr = &stack_var;

    printf("  stack_var = %d（在栈上）\n", stack_var);

    free(ptr);      // ★ 试图 free 一个栈变量的地址！
    printf("  free 完成 （这行不会执行）\n");
}

// 场景 4：free 偏移后的指针（不是 malloc 原始返回值）
void offset_free() {
    printf("【场景 4】释放偏移指针\n");

    char *original = (char *)malloc(100);
    char *middle   = original + 50;  // 指向分配区域中间

    strcpy(original, "data at the start");
    printf("  original = %s\n", original);

    free(middle);   // ★ 释放的不是 malloc 的原返回值！
    printf("  free(middle) 完成 （这行不会执行）\n");
    free(original); // 正确做法：只 free malloc 原始返回的指针
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== 重复释放 & 非法释放实验 ===\n\n");

    switch (mode) {
        case 1: classic_double_free();  break;
        case 2: alias_double_free();    break;
        case 3: invalid_free();         break;
        case 4: offset_free();          break;
        default:
            printf("用法: %s <mode>\n", argv[0]);
            printf("  mode 1: 经典 double-free\n");
            printf("  mode 2: 别名 double-free\n");
            printf("  mode 3: free 栈变量\n");
            printf("  mode 4: free 偏移指针\n");
    }

    return 0;
}
