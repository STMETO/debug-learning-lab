#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ── ltrace vs strace 对比实验 ──
// 同一个程序，ltrace 看到库函数，strace 看到系统调用

void write_to_file() {
    FILE *fp = fopen("/tmp/compare_demo.txt", "w");
    if (fp) {
        fprintf(fp, "Line 1: data from compare demo\n");
        fprintf(fp, "Line 2: more data\n");
        fclose(fp);
    }
}

void read_from_file() {
    FILE *fp = fopen("/tmp/compare_demo.txt", "r");
    if (fp) {
        char buf[128];
        while (fgets(buf, sizeof(buf), fp)) {
            printf("  读取: %s", buf);
        }
        fclose(fp);
    }
}

void do_memory_work() {
    printf("  分配内存...\n");
    char *a = (char *)malloc(1024);
    char *b = (char *)malloc(4096);
    strcpy(a, "hello memory");
    strcpy(b, "world memory");
    printf("  a = %s, b = %s\n", a, b);
    free(a);
    free(b);
}

int main() {
    printf("╔════════════════════════════════════╗\n");
    printf("║   ltrace vs strace —— 双视角对比   ║\n");
    printf("╚════════════════════════════════════╝\n\n");

    printf("【场景 1】文件写入\n");
    write_to_file();

    printf("\n【场景 2】文件读取\n");
    read_from_file();

    printf("\n【场景 3】内存分配和释放\n");
    do_memory_work();

    printf("\n=== 实验结束 ===\n");
    return 0;
}
