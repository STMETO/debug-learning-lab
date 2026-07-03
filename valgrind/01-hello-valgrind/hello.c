#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Hello Valgrind ──
// 用 valgrind ./hello 就能看到内存错误报告
// 不需要 -fsanitize=address！不需要重新编译！

void heap_overflow() {
    int *arr = (int *)malloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) arr[i] = i * 10;

    arr[5] = 999;   // ★ 越界写入——Valgrind 会报 "Invalid write of size 4"
    printf("  arr[5] = %d （这行可能不会执行）\n", arr[5]);
    free(arr);
}

void memory_leak() {
    int *data = (int *)malloc(100 * sizeof(int));
    for (int i = 0; i < 100; i++) data[i] = i;
    printf("  分配并使用了 400 字节，但忘了 free\n");
    // ★ 没有 free(data) —— Valgrind 会报 "definitely lost: 400 bytes"
}

int main() {
    printf("=== Hello Valgrind ===\n\n");

    printf("【场景 1】堆缓冲区溢出\n");
    heap_overflow();

    printf("\n【场景 2】内存泄漏\n");
    memory_leak();

    printf("\n=== 实验结束 ===\n");
    return 0;
}
