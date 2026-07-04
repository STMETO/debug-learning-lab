#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ── ltrace vs strace Comparison Experiment ──
// Same program: ltrace sees library functions, strace sees system calls

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
            printf("  read: %s", buf);
        }
        fclose(fp);
    }
}

void do_memory_work() {
    printf("  Allocating memory...\n");
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
    printf("║   ltrace vs strace -- Dual Perspective Compare   ║\n");
    printf("╚════════════════════════════════════╝\n\n");

    printf("[Scenario 1] File Write\n");
    write_to_file();

    printf("\n[Scenario 2] File Read\n");
    read_from_file();

    printf("\n[Scenario 3] Memory Allocation and Release\n");
    do_memory_work();

    printf("\n=== Experiment End ===\n");
    return 0;
}
