#include <stdio.h>
#include <string.h>

// ── 全局缓冲区溢出 ──
// 全局/静态变量的越界——第三种也是最后一种缓冲区溢出类型

// 全局数组（在 .data 或 .bss 段，既不在堆也不在栈）
char global_buf[16] = "hello";
int  global_nums[5] = {10, 20, 30, 40, 50};

// 静态变量（作用域限制在当前文件，但存储位置和全局变量一样）
static char static_buf[8] = "static";

// 场景 1：全局数组越界写入
void overflow_global_array() {
    printf("【场景 1】全局数组越界\n");

    printf("  global_buf = %s\n", global_buf);
    printf("  global_buf 大小 = %zu 字节\n", sizeof(global_buf));

    // ★ 写入 30 字节到 16 字节的全局数组
    strcpy(global_buf, "this string is way too long!");
    printf("  global_buf = %s （这行不会执行）\n", global_buf);
}

// 场景 2：全局 int 数组越界
void overflow_global_ints() {
    printf("【场景 2】全局 int 数组越界\n");

    for (int i = 0; i < 5; i++) {
        printf("  global_nums[%d] = %d\n", i, global_nums[i]);
    }

    // ★ 写入 global_nums[10]（数组只有 5 个元素）
    global_nums[10] = 999;
    printf("  global_nums[10] = %d （这行不会执行）\n", global_nums[10]);
}

// 场景 3：静态变量越界
void overflow_static() {
    printf("【场景 3】静态变量越界\n");

    printf("  static_buf = %s\n", static_buf);

    // ★ 写入超出 static_buf 的容量
    strcpy(static_buf, "overflow!!!");
    printf("  static_buf = %s （这行不会执行）\n", static_buf);
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== 全局缓冲区溢出实验 ===\n\n");

    switch (mode) {
        case 1: overflow_global_array(); break;
        case 2: overflow_global_ints();  break;
        case 3: overflow_static();       break;
        default:
            printf("用法: %s <mode>\n", argv[0]);
            printf("  mode 1: 全局 char 数组越界\n");
            printf("  mode 2: 全局 int 数组越界\n");
            printf("  mode 3: 静态变量越界\n");
    }

    return 0;
}
