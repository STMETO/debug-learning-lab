#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 释放后使用（Use-After-Free） ──
// free 之后还继续用那块内存 —— 最危险的内存 bug 之一

// 场景 1：经典 use-after-free
void classic_uaf() {
    printf("【场景 1】经典释放后写入\n");

    int *ptr = (int *)malloc(sizeof(int) * 10);
    for (int i = 0; i < 10; i++) {
        ptr[i] = i * 10;
    }
    printf("  ptr[5] = %d (free 之前)\n", ptr[5]);

    free(ptr);          // ★ 释放了

    ptr[3] = 999;       // ★ 释放后还在写！
    printf("  ptr[3] = %d （这行不会执行）\n", ptr[3]);
}

// 场景 2：释放后读取
void read_after_free() {
    printf("【场景 2】释放后读取\n");

    char *str = strdup("hello world");
    printf("  原始字符串: %s\n", str);

    free(str);          // ★ 释放了

    printf("  str = %s\n", str);  // ★ 释放后读取！
}

// 场景 3：悬垂指针 —— 最危险的场景
struct User {
    int id;
    char *name;
};

void dangling_pointer() {
    printf("【场景 3】悬垂指针 —— 结构体中的问题\n");

    struct User *user = (struct User *)malloc(sizeof(struct User));
    user->id = 42;
    user->name = strdup("Alice");

    printf("  用户 #%d: %s\n", user->id, user->name);

    free(user->name);   // 先释放了 name
    free(user);         // 又释放了 user 本身

    // ★ user->name 是悬垂指针！指向已释放的内存
    printf("  释放后尝试读取: %s\n", user->name);
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== Use-After-Free 实验 ===\n\n");

    switch (mode) {
        case 1: classic_uaf();      break;
        case 2: read_after_free();  break;
        case 3: dangling_pointer(); break;
        default:
            printf("用法: %s <mode>\n", argv[0]);
            printf("  mode 1: 释放后写入\n");
            printf("  mode 2: 释放后读取\n");
            printf("  mode 3: 悬垂指针\n");
    }

    return 0;
}
