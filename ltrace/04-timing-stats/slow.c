#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

// ── ltrace 时间与统计实验 ──
// 用 -t/-T 看时序，用 -c 看统计

void fast_calls() {
    printf("【阶段 A】快速调用\n");
    for (int i = 0; i < 10; i++) {
        strlen("quick test");
        isdigit('5');
    }
    printf("  10 次 strlen + 10 次 isdigit 完成\n");
}

void medium_calls() {
    printf("\n【阶段 B】中等调用\n");
    // 字符串复制 —— 中等耗时
    for (int i = 0; i < 5; i++) {
        char *s = strdup("this is a medium-length string for testing");
        if (s) free(s);
    }
    printf("  5 次 strdup+free 完成\n");
}

void slow_calls() {
    printf("\n【阶段 C】慢速调用\n");
    // 明显的延迟 —— sleep 是库函数
    for (int i = 0; i < 3; i++) {
        printf("  sleep %d...\n", i + 1);
        sleep(1);   // 1 秒 —— ltrace -T 会显示耗时
    }
    printf("  3 次 sleep 完成\n");
}

void mixed_frequencies() {
    printf("\n【阶段 D】混合频率\n");
    // strlen 被频繁调用，strdup 只调一次
    const char *text = "frequency analysis demo string";
    size_t total = 0;
    for (int i = 0; text[i]; i++) {
        total += strlen(text + i);  // ★ 每次迭代都调用 strlen！
    }
    char *copy = strdup(text);
    free(copy);
    printf("  执行了 %zu 次 strlen, 1 次 strdup\n", strlen(text));
}

int main() {
    printf("=== ltrace 时间与统计实验 ===\n\n");

    fast_calls();
    medium_calls();
    slow_calls();
    mixed_frequencies();

    printf("\n=== 实验结束 ===\n");
    return 0;
}
