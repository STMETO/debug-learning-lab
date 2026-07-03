#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Valgrind + GDB 联动 ──
// Valgrind 发现问题时，让 GDB 进来查看完整现场

// 一个会触发 Valgrind 错误但能继续运行的函数
void problematic_function() {
    int *arr = (int *)malloc(3 * sizeof(int));
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    // ★ 越界写入 —— Valgrind 会报 "Invalid write"
    // 配合 --vgdb-error=0，Valgrind 会在每个错误前暂停，等 GDB 连接
    arr[3] = 999;

    printf("  arr[0] = %d, arr[1] = %d, arr[2] = %d\n",
           arr[0], arr[1], arr[2]);

    free(arr);
}

// 一个会使用未初始化值的函数
void uninit_in_action() {
    int value;   // ★ 未初始化

    // Valgrind 会报 "Conditional jump depends on uninitialised value"
    if (value > 100) {
        printf("  value > 100: %d\n", value);
    } else {
        printf("  value <= 100: %d\n", value);
    }
}

int main() {
    printf("╔══════════════════════════════════════╗\n");
    printf("║   Valgrind + GDB 联动实验             ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    printf("【场景 1】越界写入\n");
    problematic_function();

    printf("\n【场景 2】未初始化值\n");
    uninit_in_action();

    printf("\n=== 实验结束 ===\n");
    printf("\n提示：在另一个终端运行:\n");
    printf("  valgrind --vgdb=yes --vgdb-error=0 ./gdb_vg\n");
    printf("  然后在第二个终端:\n");
    printf("  gdb ./gdb_vg\n");
    printf("  (gdb) target remote | vgdb\n");
    return 0;
}
