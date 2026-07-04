#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Memory Leak检测 ──
// ASan 内置了 LSAN（LeakSanitizer），在程序退出时report未free的memory

// Scenario 1：Direct Leak —— allocated但忘了 free，连pointer都丢了
void direct_leak() {
    printf("[Scenario 1] Direct leak -- pointer lost\n");

    int *data = (int *)malloc(100 * sizeof(int));
    for (int i = 0; i < 100; i++) {
        data[i] = i;
    }
    printf("  Allocated 400 bytes, but pointer %p lost after function returns\n", (void *)data);
    // ★ 没有 free(data) —— 函数return后，pointer data 销毁，memory永远无法free
}

// Scenario 2：Indirect Leak —— 结构体allocated内部成员，但free结构体时忘了free成员
struct Record {
    int id;
    char *name;     // 指向on heap的string
    int *scores;    // 指向on heap的array
};

void indirect_leak() {
    printf("[Scenario 2] Indirect leak -- freed outer struct but forgot inner members\n");

    struct Record *rec = (struct Record *)malloc(sizeof(struct Record));
    rec->id = 1;
    rec->name = strdup("Alice");           // strdup 内部调用了 malloc
    rec->scores = (int *)malloc(5 * sizeof(int));
    for (int i = 0; i < 5; i++) {
        rec->scores[i] = i * 10;
    }

    printf("  Record #%d: %s\n", rec->id, rec->name);

    free(rec);   // ★ 只free了 Record 本身，name 和 scores 泄漏了！
    // 正确做法：
    //   free(rec->name);
    //   free(rec->scores);
    //   free(rec);
}

// Scenario 3：Conditional Leak —— 只在某些分支泄漏
void conditional_leak(int mode) {
    printf("[Scenario 3] Conditional leak -- mode=%d\n", mode);

    char *buffer = (char *)malloc(256);
    strcpy(buffer, "important data");
    printf("  buffer = %s\n", buffer);

    if (mode == 1) {
        printf("  Normal path: free buffer\n");
        free(buffer);
    } else {
        printf("  Error path: forgot to free buffer (simulating early return or exceptional exit)\n");
        // ★ 在这条分支没有 free(buffer)
    }
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== Memory Leak Detection Experiment ===\n\n");

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
            printf("Usage: %s <mode>\n", argv[0]);
            printf("  mode 1: Direct leak\n");
            printf("  mode 2: Indirect leak (struct internal members)\n");
            printf("  mode 3: Conditional leak\n");
    }

    printf("\nProgram about to exit -- LSAN will check for leaks...\n");
    return 0;
}
