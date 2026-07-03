#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── ltrace + GDB 双工具联动 ──
// ltrace: 看库函数调用的参数和返回值
// GDB:    深入函数内部看变量和执行流

int compute_score(const char *name, int base) {
    size_t len = strlen(name);

    // GDB 能看到: len 的值, name 的内容, base 的值
    // ltrace 能看到: strlen(name) 的调用和返回值
    int score = base + (int)(len * 10);

    // 模拟: 某些名字有额外的加分
    if (strstr(name, "admin")) {
        score += 100;   // ♂ GDB: score 从普通值跳变到 +100
    }

    return score;
}

void process_names() {
    const char *users[] = {
        "alice",
        "bob_admin",
        "charlie",
        "dave_admin_super",
        "eve",
        NULL
    };

    for (int i = 0; users[i] != NULL; i++) {
        // ltrace 能看到: strlen(users[i]) 每次都被调用
        // GDB 能看到: i 的值, users[i] 的内容, 返回的 score
        int s = compute_score(users[i], 50);
        printf("  用户 %-20s → 分数 %d\n", users[i], s);
    }
}

char *build_greeting(const char *name) {
    // ltrace: strlen(name), malloc(...), strcpy, strcat
    // GDB:   可以 print 每一步的中间结果
    size_t nlen = strlen(name);
    char *buf = (char *)malloc(nlen + 20);

    strcpy(buf, "Hello, ");
    strcat(buf, name);
    strcat(buf, "!");

    return buf;  // ltrace: buf = 0x...（返回指针）
                 // GDB:    print buf → "Hello, alice!"
}

int main() {
    printf("╔════════════════════════════════════╗\n");
    printf("║    ltrace + GDB 双视角调试          ║\n");
    printf("╚════════════════════════════════════╝\n\n");

    printf("【场景 1】计算分数（strlen + strstr）\n");
    process_names();

    printf("\n【场景 2】构建问候语（strlen + strcpy + strcat）\n");
    char *greeting = build_greeting("Alice");
    printf("  问候语: %s\n", greeting);
    free(greeting);

    printf("\n=== 实验结束 ===\n");
    return 0;
}
