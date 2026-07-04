#include <stdio.h>

// ── 一个简单但有层的compute程序 ──
// 用于演示 TUI 的各种布局和 .gdbinit config

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

    printf("Analysis of number %d:\n", n);
    printf("  fibonacci(%d) = %ld\n", n, fib);
    printf("  Is it prime? %s\n", prime ? "Yes" : "No");

    // 如果 fib Yesprime，额外output
    if (is_prime((int)fib)) {
        printf("  ★ fibonacci(%d) = %ld is also prime!\n", n, fib);
    }
}

int main() {
    printf("=== GDB TUI Mode Demo ===\n\n");

    for (int i = 5; i <= 10; i++) {
        analyze_number(i);
        printf("\n");
    }

    return 0;
}
