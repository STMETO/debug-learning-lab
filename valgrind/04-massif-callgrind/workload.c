#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Massif & Callgrind 双工具演示 ──
// Massif: 堆内存剖析 —— 看"程序在哪个阶段用了多少堆内存"
// Callgrind: 调用图性能分析 —— 看"CPU 时间花在哪个函数上"

// 一个 CPU 密集型的函数（Callgrind 会显示它很"热"）
long compute_fibonacci(int n) {
    if (n <= 1) return n;
    return compute_fibonacci(n - 1) + compute_fibonacci(n - 2);
}

// 一个内存密集型的函数（Massif 会显示堆增长）
char *build_report(int id, const char *text) {
    size_t len = strlen(text);
    // 每次调用分配新内存——Massif 会追踪堆的波浪形变化
    char *report = (char *)malloc(len + 64);
    snprintf(report, len + 64, "[Report #%d] %s", id, text);
    return report;
}

int main() {
    printf("=== Massif & Callgrind 演示 ===\n\n");

    // ── 阶段 1：CPU 密集型（Callgrind 关注） ──
    printf("【阶段 1】CPU 密集型 —— 计算 fibonacci(40)\n");
    long fib = compute_fibonacci(40);
    printf("  fibonacci(40) = %ld\n", fib);

    // ── 阶段 2：内存密集型（Massif 关注） ──
    printf("\n【阶段 2】内存密集型 —— 构建大量报告\n");
    char *reports[20];
    for (int i = 0; i < 20; i++) {
        reports[i] = build_report(i, "this is a test report string for memory profiling");
    }
    printf("  分配了 20 个报告\n");

    // ── 阶段 3：释放一半（Massif 会看到堆下降） ──
    printf("\n【阶段 3】释放一半报告\n");
    for (int i = 0; i < 10; i++) {
        free(reports[i]);
        reports[i] = NULL;
    }
    printf("  释放了前 10 个\n");

    // ── 阶段 4：再分配一些（Massif 会看到堆回升） ──
    printf("\n【阶段 4】再分配一批\n");
    char *big_chunk = (char *)malloc(5 * 1024 * 1024);  // 5MB
    memset(big_chunk, 'X', 5 * 1024 * 1024);
    printf("  分配了 5MB 大块\n\n");

    // 清理（故意留一个报告不释放，作为泄漏演示）
    for (int i = 10; i < 19; i++) {
        free(reports[i]);
    }
    free(big_chunk);
    // ★ reports[19] 没有释放 —— Massif 会看到还有一块留在堆上

    printf("=== 实验结束 ===\n");
    printf("\nValgrind 运行方式:\n");
    printf("  Massif:   valgrind --tool=massif ./workload\n");
    printf("  Callgrind: valgrind --tool=callgrind ./workload\n");
    return 0;
}
