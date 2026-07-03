#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

// ── ltrace 过滤实验 ──
// 这个程序调用了多种库函数
// 用 ltrace -e 只追踪你关心的那几类

void string_work() {
    printf("  [字符串操作]\n");
    const char *s = "Hello, ltrace filtering demo!";
    size_t len = strlen(s);
    char *copy = strdup(s);
    char *pos  = strchr(copy, 'l');
    if (pos) printf("    第一个 'l' 在位置 %ld\n", pos - copy);
    free(copy);
    printf("    字符串长度: %zu\n", len);
}

void memory_work() {
    printf("  [内存操作]\n");
    char buf1[64], buf2[64];
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));
    strcpy(buf1, "memory test data");
    memcpy(buf2, buf1, strlen(buf1) + 1);
    int same = memcmp(buf1, buf2, strlen(buf1));
    printf("    内存比较结果: %s\n", same == 0 ? "相同" : "不同");
    // 多次小分配
    for (int i = 0; i < 5; i++) {
        int *p = malloc(64);
        p[0] = i;
        free(p);
    }
}

void math_work() {
    printf("  [数学操作]\n");
    double r = sqrt(144.0);
    double p = pow(2.0, 8.0);
    printf("    sqrt(144) = %.0f, pow(2,8) = %.0f\n", r, p);
}

void char_work() {
    printf("  [字符操作]\n");
    const char *text = "AbCdEf123";
    int alpha = 0, digit = 0;
    for (int i = 0; text[i]; i++) {
        if (isalpha(text[i])) alpha++;
        if (isdigit(text[i])) digit++;
    }
    printf("    字母: %d, 数字: %d\n", alpha, digit);
}

int main() {
    printf("=== ltrace 过滤实验 ===\n\n");
    printf("四类操作:\n");

    string_work();
    printf("\n");
    memory_work();
    printf("\n");
    math_work();
    printf("\n");
    char_work();

    printf("\n=== 实验结束 ===\n");
    return 0;
}
