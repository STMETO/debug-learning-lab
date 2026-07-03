#include <stdio.h>

// ── 一个简单但有层次的计算程序 ──
// 用于演示 TUI 的各种布局和 .gdbinit 配置

long fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

void analyze_number(int n) {
    long fib = fibonacci(n);
    int prime = is_prime(n);

    printf("数字 %d 的分析:\n", n);
    printf("  fibonacci(%d) = %ld\n", n, fib);
    printf("  是质数吗？%s\n", prime ? "是" : "否");

    // 如果 fib 是质数，额外输出
    if (is_prime((int)fib)) {
        printf("  ★ fibonacci(%d) = %ld 也是质数！\n", n, fib);
    }
}

int main() {
    printf("=== GDB TUI 模式演示 ===\n\n");

    for (int i = 5; i <= 10; i++) {
        analyze_number(i);
        printf("\n");
    }

    return 0;
}
