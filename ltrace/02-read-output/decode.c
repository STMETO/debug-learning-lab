#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// ── 解码 ltrace 输出的每一种格式 ──
// 丰富多样的库函数调用，展示各种返回值类型和来源标记

int main() {
    printf("=== ltrace 输出格式实验 ===\n\n");

    // ── 类型 1：返回整数（正常或计数） ──
    printf("【A】返回整数的函数\n");
    int len = strlen("measure me");
    printf("  strlen 返回: %d\n", len);

    int cmp = strcmp("abc", "abd");
    printf("  strcmp(\"abc\", \"abd\") 返回: %d\n", cmp);

    // ── 类型 2：返回指针 ──
    printf("\n【B】返回指针的函数\n");
    char *p = strdup("hello ltrace");
    printf("  strdup 返回: %p\n", (void *)p);

    char *found = strchr(p, 'l');
    printf("  strchr('%s', 'l') 返回: %p (指向 '%s')\n", p, (void *)found, found);

    free(p);

    // ── 类型 3：返回 NULL（失败） ──
    printf("\n【C】返回 NULL 的函数\n");
    FILE *fp = fopen("/tmp/不存在的目录/文件.txt", "r");
    printf("  fopen(不存在) 返回: %p\n", (void *)fp);
    // ltrace 会显示: fopen(...) = nil

    // ── 类型 4：返回浮点数的数学函数 ──
    printf("\n【D】数学库函数\n");
    double root = sqrt(64.0);
    printf("  sqrt(64) 返回: %.1f\n", root);

    double power = pow(2.0, 10.0);
    printf("  pow(2, 10) 返回: %.0f\n", power);

    // ── 类型 5：带结构体/缓冲区的函数 ──
    printf("\n【E】修改缓冲区的函数\n");
    char dest[32];
    memset(dest, 0, sizeof(dest));
    strcpy(dest, "copied string");
    printf("  strcpy 后: %s\n", dest);

    // memcpy 返回目标指针
    char *ret = memcpy(dest, "overwritten", 11);
    printf("  memcpy 返回: %p (== dest? %s)\n", (void *)ret,
           ret == dest ? "yes" : "no");

    // 用 memcmp 比较
    int eq = memcmp(dest, "overwritten", 10);
    printf("  memcmp 返回: %d (0==相等)\n", eq);

    // ── 类型 6：触发错误的函数 ──
    printf("\n【F】会出错的函数\n");
    // 释放一个非 malloc 的指针 —— 错误！
    // 实际不跑这行避免崩溃，但你可以想象 ltrace 的输出：
    // free((void *)0x7fff...) = <void>（然后 libc 内部检测到错误，调用 abort）

    printf("  (跳过实际错误——只说明 ltrace 的显示格式)\n");

    printf("\n=== 实验结束 ===\n");
    return 0;
}
