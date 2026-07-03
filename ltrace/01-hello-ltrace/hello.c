#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 用 ltrace 看库函数调用 ──
// 每个 C 标准库函数被调用时，ltrace 都会记录下来

int main() {
    // 1. printf 是 libc 的函数 → ltrace 会显示
    printf("=== Hello from ltrace! ===\n");

    // 2. malloc 向堆管理器要内存 → ltrace 会显示参数和返回值
    int *data = (int *)malloc(10 * sizeof(int));
    printf("malloc 返回: %p\n", (void *)data);

    // 3. strlen 计算字符串长度 → ltrace 会显示
    const char *msg = "debugging with ltrace";
    size_t len = strlen(msg);
    printf("字符串 \"%s\" 的长度: %zu\n", msg, len);

    // 4. 填充和释放
    for (int i = 0; i < 10; i++) {
        data[i] = i * 10;
    }
    free(data);
    printf("free 完成\n");

    // 5. 文件操作 —— fopen/fprintf/fclose 全是库函数
    FILE *fp = fopen("/tmp/ltrace_hello.txt", "w");
    if (fp) {
        fprintf(fp, "Written via ltrace demo.\n");
        fclose(fp);
        printf("文件写入完成\n");
    }

    // 6. memcpy —— 内存复制也是库函数
    char src[] = "memory copy test";
    char dst[32];
    memcpy(dst, src, strlen(src) + 1);
    printf("memcpy 完成: %s\n", dst);

    printf("=== 实验结束 ===\n");
    return 0;
}
