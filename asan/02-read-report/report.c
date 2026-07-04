#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 一个"稍复杂"的溢出Scenario ──
// 用于深入分析 ASan report的每个部分：
//   shadowbyte map、call stack层级、"X bytes to the right/left"

// 第三层：实际out-of-bounds发生在这里
void fill_buffer(int *buf, int count) {
    for (int i = 0; i < count; i++) {
        buf[i] = i * 100;   // ★ 如果 count > 实际allocate/allocation的大小，这里out-of-bounds
    }
}

// 第二层：准备data
void prepare_data(int *buf) {
    // 假设 buf 只有 5 个元素，但传了 count=8
    fill_buffer(buf, 8);    // 写了 8 个 int → 后 3 个out-of-bounds
}

// 第一层：allocate/allocationmemory
int main() {
    printf("=== ASan Report In-Depth Analysis ===\n\n");

    // allocate/allocation 5 个 int（20 bytes）
    int *data = (int *)malloc(5 * sizeof(int));
    printf("Allocated 5 ints (%zu bytes)\n", 5 * sizeof(int));
    printf("Valid indices: data[0] ~ data[4]\n\n");

    // 正常使用前 5 个
    prepare_data(data);

    // 打印result——注意 prepare_data 实际上写了 8 个元素
    // read data[5]~data[7] 也Yes一种out-of-bounds读，但 ASan 已经在上面的out-of-bounds写时终止了
    printf("data[0] = %d\n", data[0]);
    printf("data[4] = %d\n", data[4]);

    // ★ 以下out-of-boundsread（用于演示读out-of-boundsreport）：
    // printf("data[7] = %d\n", data[7]);

    free(data);
    return 0;
}
