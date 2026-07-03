#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 一个"稍复杂"的溢出场景 ──
// 用于深入分析 ASan 报告的每个部分：
//   影子字节图、调用栈层级、"X bytes to the right/left"

// 第三层：实际越界发生在这里
void fill_buffer(int *buf, int count) {
    for (int i = 0; i < count; i++) {
        buf[i] = i * 100;   // ★ 如果 count > 实际分配的大小，这里越界
    }
}

// 第二层：准备数据
void prepare_data(int *buf) {
    // 假设 buf 只有 5 个元素，但传了 count=8
    fill_buffer(buf, 8);    // 写了 8 个 int → 后 3 个越界
}

// 第一层：分配内存
int main() {
    printf("=== ASan 报告深度解读 ===\n\n");

    // 分配 5 个 int（20 字节）
    int *data = (int *)malloc(5 * sizeof(int));
    printf("分配了 5 个 int（%zu 字节）\n", 5 * sizeof(int));
    printf("合法索引: data[0] ~ data[4]\n\n");

    // 正常使用前 5 个
    prepare_data(data);

    // 打印结果——注意 prepare_data 实际上写了 8 个元素
    // 读取 data[5]~data[7] 也是一种越界读，但 ASan 已经在上面的越界写时终止了
    printf("data[0] = %d\n", data[0]);
    printf("data[4] = %d\n", data[4]);

    // ★ 以下越界读取（用于演示读越界报告）：
    // printf("data[7] = %d\n", data[7]);

    free(data);
    return 0;
}
