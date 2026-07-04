#include <stdio.h>
#include <stdlib.h>

// ── 递归版阶乘 ──
// 每一层递归都会创建一个新的栈帧
// 在深层递归处设断点，用 backtrace 观察整个调用链

unsigned long long factorial(int n) {
    // ★ 在这里设断点，每递归都会停
    //   用 backtrace 看call stack如何增长
    if (n <= 1) {
        return 1;   // 递归终点
    }
    unsigned long long result = n * factorial(n - 1);
    return result;
}

// ── 深层函数调用链 ──
// 模拟实际项目中常见的"A调B、B调C、C调D"模式

int layer_d(int x) {
    // ★ 在这里设断点，看看Yes谁调用了你
    return x * x;
}

int layer_c(int x) {
    return layer_d(x + 10) + 1;
}

int layer_b(int x) {
    return layer_c(x + 5) + 1;
}

int layer_a(int x) {
    return layer_b(x) + 1;
}

int main() {
    printf("=== Scenario 1: Recursion — observe stack growth ===\n");
    unsigned long long fac = factorial(5);
    printf("factorial(5) = %llu\n\n", fac);

    printf("=== Scenario 2: Deep call chain ===\n");
    int result = layer_a(1);
    printf("layer_a(1) = %d\n", result);

    return 0;
}
