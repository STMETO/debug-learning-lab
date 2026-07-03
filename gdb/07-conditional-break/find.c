#include <stdio.h>

// ── 一个"大海捞针"的场景 ──
// 处理大量数据时，只有特定条件下才出 bug
// 用条件断点精准定位

// 判断是否为质数
int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

// 计算"数字的各位之和"（比如 123 → 1+2+3 = 6）
int digit_sum(int n) {
    int sum = 0;
    while (n > 0) {
        sum += n % 10;
        n /= 10;
    }
    return sum;
}

int main() {
    printf("在 1~200 中寻找满足条件的数：\n");
    printf("条件：是质数 且 各位之和也是质数\n\n");

    int count = 0;
    for (int i = 1; i <= 200; i++) {
        int prime   = is_prime(i);
        int dsum    = digit_sum(i);
        int dsum_prime = is_prime(dsum);

        if (prime && dsum_prime) {
            printf("  ✅ i=%3d  (各位之和=%d)\n", i, dsum);
            count++;
        }
    }

    printf("\n共找到 %d 个数\n", count);

    // ── 模拟一个"偶尔出错"的场景 ──
    // 假设有人报告：当 counter 等于某个特定值时程序行为不对
    // 你不可能手动 continue 几百次——用条件断点
    printf("\n=== 模拟场景：在大循环中定位特定条件 ===\n");
    int counter = 0;
    for (int j = 0; j < 1000; j++) {
        counter += j % 7;  // 每次累加 j 除以 7 的余数
        // ★ 假设你知道 counter 变成 126 时出了问题
        // 在这里设条件断点: break 50 if counter == 126
    }
    printf("最终 counter = %d\n", counter);

    return 0;
}
