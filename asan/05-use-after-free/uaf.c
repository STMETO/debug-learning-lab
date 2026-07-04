#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Use-After-Free（Use-After-Free） ──
// free 之后还继续用那块memory —— 最危险的memory bug 之一

// Scenario 1：classic use-after-free
void classic_uaf() {
    printf("[Scenario 1] Classic write after free\n");

    int *ptr = (int *)malloc(sizeof(int) * 10);
    for (int i = 0; i < 10; i++) {
        ptr[i] = i * 10;
    }
    printf("  ptr[5] = %d (before free)\n", ptr[5]);

    free(ptr);          // ★ free了

    ptr[3] = 999;       // ★ free后还在写！
    printf("  ptr[3] = %d (this line will not execute)\n", ptr[3]);
}

// Scenario 2：read after free
void read_after_free() {
    printf("[Scenario 2] Read after free\n");

    char *str = strdup("hello world");
    printf("  Original string: %s\n", str);

    free(str);          // ★ free了

    printf("  str = %s\n", str);  // ★ read after free！
}

// Scenario 3：Dangling Pointer —— 最危险的Scenario
struct User {
    int id;
    char *name;
};

void dangling_pointer() {
    printf("[Scenario 3] Dangling pointer -- problem in struct\n");

    struct User *user = (struct User *)malloc(sizeof(struct User));
    user->id = 42;
    user->name = strdup("Alice");

    printf("  User #%d: %s\n", user->id, user->name);

    free(user->name);   // 先free了 name
    free(user);         // 又free了 user 本身

    // ★ user->name YesDangling Pointer！指向已free的memory
    printf("  Attempt to read after free: %s\n", user->name);
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== Use-After-Free Experiment ===\n\n");

    switch (mode) {
        case 1: classic_uaf();      break;
        case 2: read_after_free();  break;
        case 3: dangling_pointer(); break;
        default:
            printf("Usage: %s <mode>\n", argv[0]);
            printf("  mode 1: Write after free\n");
            printf("  mode 2: Read after free\n");
            printf("  mode 3: Dangling pointer\n");
    }

    return 0;
}
