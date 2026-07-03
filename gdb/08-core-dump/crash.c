#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ── 一个会崩溃的程序 ──
// 用于生成 core dump，学习事后分析

struct Config {
    char  name[32];
    int   value;
    char *description;   // 指针——稍后会出问题的关键
};

// 计算描述信息的长度
int description_length(const char *desc) {
    return strlen(desc);   // ★ 如果 desc 是 NULL，strlen 会段错误
}

// 一个看似正常的函数
void print_config(struct Config *cfg) {
    printf("名称: %s\n", cfg->name);
    printf("数值: %d\n", cfg->value);
    int len = description_length(cfg->description);  // ★ NULL 传入 strlen → 崩溃
    printf("描述 (%d 字符): %s\n", len, cfg->description);
}

// malloc 失败时返回 NULL，我们没有检查
void setup_broken_config(struct Config *cfg, int mode) {
    strcpy(cfg->name, "test");

    if (mode == 0) {
        cfg->value = 42;
        // BUG: 忘记给 description 分配内存！
        cfg->description = NULL;
    } else if (mode == 1) {
        cfg->value = 100;
        cfg->description = strdup("正常描述");  // strdup 分配了内存
    } else {
        cfg->value = 999;
        // BUG: 分配了内存但没检查 malloc 返回值
        cfg->description = (char *)malloc(100);
        // 如果 malloc 失败返回 NULL, strcpy 也会崩溃
        if (cfg->description) {
            strcpy(cfg->description, "大内存描述信息...");
        }
    }
}

int main(int argc, char *argv[]) {
    int mode = 0;
    if (argc > 1) {
        mode = atoi(argv[1]);
    }

    printf("=== Core Dump 实验 ===\n");
    printf("运行模式: %d\n\n", mode);

    struct Config cfg;
    setup_broken_config(&cfg, mode);

    printf("配置信息:\n");
    print_config(&cfg);   // mode=0 时这里会崩溃！

    // 正常流程下释放内存
    if (cfg.description) {
        free(cfg.description);
    }

    return 0;
}
