#include <stdio.h>

// ── 一个"大海捞针"的Scenario ──
// process大量data时，只有特定条件下才出 bug
// 用条件断点精准定位

// 判断YesNo为prime
int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

// compute"数字的digit sum"（比如 123 → 1+2+3 = 6）
int digit_sum(int n) {
    int sum = 0;
    while (n > 0) {
        sum += n % 10;
        n /= 10;
    }
    return sum;
}

int main() {
    printf("Finding numbers in 1~200 that satisfy the conditions:\n");
    printf("Condition: is prime AND sum of its digits is also prime\n\n");

    int count = 0;
    for (int i = 1; i <= 200; i++) {
        int prime   = is_prime(i);
        int dsum    = digit_sum(i);
        int dsum_prime = is_prime(dsum);

        if (prime && dsum_prime) {
            printf("  ✅ i=%3d  (digit sum=%d)\n", i, dsum);
            count++;
        }
    }

    printf("\nFound %d numbers in total\n", count);

    // ── 模拟一个"偶尔出错"的Scenario ──
    // 假设有人report：当 counter 等于某个特定值时程序行为不对
    // 你不可能手动 continue 几百——用条件断点
    printf("\n=== Simulated scenario: Locate a specific condition in a large loop ===\n");
    int counter = 0;
    for (int j = 0; j < 1000; j++) {
        counter += j % 7;  // add j%7 each iteration
        // ★ assume you know counter==126 causes the bug
        // set conditional breakpoint here: break 50 if counter == 126
    }
    printf("Final counter = %d\n", counter);

    return 0;
}
