#include <stdio.h>
#include <stdlib.h>

// ── 一个简单的程序，用于观察系统调用 ──
// 用 strace 追踪它，看看 printf、fileoperation背后到底发生了什么

int main() {
    printf("=== Hello from strace lab! ===\n");

    // 1. 打印一些东西（会触发 write 系统调用）
    printf("Step 1: printing to stdout...\n");

    // 2. 打开一个file（会触发 openat 系统调用）
    FILE *fp = fopen("/tmp/strace_demo.txt", "w");
    if (fp == NULL) {
        printf("ERROR: could not open file\n");
        return 1;
    }
    printf("Step 2: opened /tmp/strace_demo.txt\n");

    // 3. writedata（会触发 write 系统调用）
    fprintf(fp, "This was written via strace demo.\n");
    fprintf(fp, "Every fprintf() becomes a write() syscall.\n");
    printf("Step 3: wrote data to file\n");

    // 4. 关闭file（会触发 close 系统调用）
    fclose(fp);
    printf("Step 4: closed file\n");

    // 5. allocate/allocation一块heap memory（会触发 brk 或 mmap 系统调用）
    int *heap_data = (int *)malloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++) {
        heap_data[i] = i * 10;
    }
    printf("Step 5: allocated and filled heap memory, heap_data[5] = %d\n", heap_data[5]);
    free(heap_data);

    printf("=== Done! ===\n");
    return 0;
}
