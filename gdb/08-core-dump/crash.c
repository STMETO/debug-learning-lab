#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ── 一个会崩溃的程序 ──
// 用于生成 core dump，学习事后分析

struct Config {
    char  name[32];
    int   value;
    char *description;   // pointer——稍后会出问题的关键
};

// compute描述信息的length
int description_length(const char *desc) {
    return strlen(desc);   // ★ 如果 desc Yes NULL，strlen 会段error
}

// 一个看似正常的函数
void print_config(struct Config *cfg) {
    printf("Name: %s\n", cfg->name);
    printf("Value: %d\n", cfg->value);
    int len = description_length(cfg->description);  // ★ NULL 传入 strlen → 崩溃
    printf("Description (%d chars): %s\n", len, cfg->description);
}

// malloc 失败时return NULL，我们没有check
void setup_broken_config(struct Config *cfg, int mode) {
    strcpy(cfg->name, "test");

    if (mode == 0) {
        cfg->value = 42;
        // BUG: forgot to给 description allocate/allocationmemory！
        cfg->description = NULL;
    } else if (mode == 1) {
        cfg->value = 100;
        cfg->description = strdup("正常描述");  // strdup allocatedmemory
    } else {
        cfg->value = 999;
        // BUG: allocatedmemory但没check malloc return值
        cfg->description = (char *)malloc(100);
        // 如果 malloc 失败return NULL, strcpy 也会崩溃
        if (cfg->description) {
            strcpy(cfg->description, "大memory描述信息...");
        }
    }
}

int main(int argc, char *argv[]) {
    int mode = 0;
    if (argc > 1) {
        mode = atoi(argv[1]);
    }

    printf("=== Core Dump Experiment ===\n");
    printf("Run mode: %d\n\n", mode);

    struct Config cfg;
    setup_broken_config(&cfg, mode);

    printf("Config info:\n");
    print_config(&cfg);   // mode=0 时这里会崩溃！

    // 正常流程下freememory
    if (cfg.description) {
        free(cfg.description);
    }

    return 0;
}
