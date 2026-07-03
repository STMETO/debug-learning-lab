#include <stdio.h>
#include <stdlib.h>

// ── 你的第一个 ASan 实验 ──
// 这个程序有一个 bug：对 malloc 出来的数组写了越界
// ASan 会在运行时抓到它

int main() {
    // 分配 10 个 int 的数组（索引 0~9 是合法的）
    int *arr = (int *)malloc(10 * sizeof(int));

    // 正常填充 arr[0] ~ arr[9]
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 10;
    }

    // 打印合法范围的值
    printf("arr[0] = %d\n", arr[0]);
    printf("arr[9] = %d\n", arr[9]);

    // ★ BUG: 索引 10 越界了！合法索引是 0~9
    arr[10] = 999;

    // 这行永远不会执行——ASan 在上面的越界写入时就会终止程序
    printf("arr[10] = %d\n", arr[10]);

    free(arr);
    return 0;
}
