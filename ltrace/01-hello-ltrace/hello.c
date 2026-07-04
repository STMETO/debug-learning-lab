#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Use ltrace to see library function calls ──
// Every C standard library function call is recorded by ltrace

int main() {
    // 1. printf is a libc function → ltrace will show it
    printf("=== Hello from ltrace! ===\n");

    // 2. malloc asks the heap manager for memory → ltrace shows args and return value
    int *data = (int *)malloc(10 * sizeof(int));
    printf("malloc returned: %p\n", (void *)data);

    // 3. strlen computes string length → ltrace will show it
    const char *msg = "debugging with ltrace";
    size_t len = strlen(msg);
    printf("String \"%s\" length: %zu\n", msg, len);

    // 4. Fill and free
    for (int i = 0; i < 10; i++) {
        data[i] = i * 10;
    }
    free(data);
    printf("free complete\n");

    // 5. File operations — fopen/fprintf/fclose are all library functions
    FILE *fp = fopen("/tmp/ltrace_hello.txt", "w");
    if (fp) {
        fprintf(fp, "Written via ltrace demo.\n");
        fclose(fp);
        printf("File write complete\n");
    }

    // 6. memcpy — memory copy is also a library function
    char src[] = "memory copy test";
    char dst[32];
    memcpy(dst, src, strlen(src) + 1);
    printf("memcpy complete: %s\n", dst);

    printf("=== Experiment End ===\n");
    return 0;
}
