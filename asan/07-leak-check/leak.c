#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 内存泄漏检测 ──
// ASan 内置了 LSAN（LeakSanitizer），在程序退出时报告未释放的内存

// 场景 1：直接泄漏 —— 分配了但忘了 free，连指针都丢了
void direct_leak() {
    printf("【场景 1】直接泄漏 —— 指针丢失\n");

    int *data = (int *)malloc(100 * sizeof(int));
    for (int i = 0; i < 100; i++) {
        data[i] = i;
    }
    printf("  分配了 400 字节，但指针 %p 在函数返回后丢失\n", (void *)data);
    // ★ 没有 free(data) —— 函数返回后，指针 data 销毁，内存永远无法释放
}

// 场景 2：间接泄漏 —— 结构体分配了内部成员，但释放结构体时忘了释放成员
struct Record {
    int id;
    char *name;     // 指向堆上的字符串
    int *scores;    // 指向堆上的数组
};

void indirect_leak() {
    printf("【场景 2】间接泄漏 —— 释放外层忘了内层\n");

    struct Record *rec = (struct Record *)malloc(sizeof(struct Record));
    rec->id = 1;
    rec->name = strdup("Alice");           // strdup 内部调用了 malloc
    rec->scores = (int *)malloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) {
        rec->scores[i] = i * 10;
    }

    printf("  Record #%d: %s\n", rec->id, rec->name);

    free(rec);   // ★ 只释放了 Record 本身，name 和 scores 泄漏了！
    // 正确做法：
    //   free(rec->name);
    //   free(rec->scores);
    //   free(rec);
}

// 场景 3：条件泄漏 —— 只在某些分支泄漏
void conditional_leak(int mode) {
    printf("【场景 3】条件泄漏 —— mode=%d\n", mode);

    char *buffer = (char *)malloc(256);
    strcpy(buffer, "important data");
    printf("  buffer = %s\n", buffer);

    if (mode == 1) {
        printf("  正常路径：释放 buffer\n");
        free(buffer);
    } else {
        printf("  错误路径：忘了释放 buffer（模拟提前 return 或异常退出）\n");
        // ★ 在这条分支没有 free(buffer)
    }
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== 内存泄漏检测实验 ===\n\n");

    switch (mode) {
        case 1:
            direct_leak();
            break;
        case 2:
            indirect_leak();
            break;
        case 3:
            conditional_leak(2);  // mode=2 触发泄漏路径
            break;
        default:
            printf("用法: %s <mode>\n", argv[0]);
            printf("  mode 1: 直接泄漏\n");
            printf("  mode 2: 间接泄漏（结构体内部成员）\n");
            printf("  mode 3: 条件泄漏\n");
    }

    printf("\n程序即将退出——LSAN 将检查泄漏...\n");
    return 0;
}
