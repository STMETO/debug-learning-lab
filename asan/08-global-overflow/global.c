#include <stdio.h>
#include <string.h>

// ── global缓冲区溢出 ──
// global/staticvariable的out-of-bounds——第三种也Yes最后一种缓冲区溢出类型

// globalarray（在 .data 或 .bss 段，既不在堆也不在栈）
char global_buf[16] = "hello";
int  global_nums[5] = {10, 20, 30, 40, 50};

// staticvariable（作用域限制在当前file，但存储位置和global variable一样）
static char static_buf[8] = "static";

// Scenario 1：globalarrayout-of-boundswrite
void overflow_global_array() {
    printf("[Scenario 1] Global array overflow\n");

    printf("  global_buf = %s\n", global_buf);
    printf("  global_buf size = %zu bytes\n", sizeof(global_buf));

    // ★ write 30 bytes到 16 bytes的globalarray
    strcpy(global_buf, "this string is way too long!");
    printf("  global_buf = %s (this line will not execute)\n", global_buf);
}

// Scenario 2：global int arrayout-of-bounds
void overflow_global_ints() {
    printf("[Scenario 2] Global int array overflow\n");

    for (int i = 0; i < 5; i++) {
        printf("  global_nums[%d] = %d\n", i, global_nums[i]);
    }

    // ★ write global_nums[10]（array只有 5 个元素）
    global_nums[10] = 999;
    printf("  global_nums[10] = %d (this line will not execute)\n", global_nums[10]);
}

// Scenario 3：staticvariableout-of-bounds
void overflow_static() {
    printf("[Scenario 3] Static variable overflow\n");

    printf("  static_buf = %s\n", static_buf);

    // ★ write超出 static_buf 的容量
    strcpy(static_buf, "overflow!!!");
    printf("  static_buf = %s (this line will not execute)\n", static_buf);
}

int main(int argc, char *argv[]) {
    int mode = 1;
    if (argc > 1) mode = atoi(argv[1]);

    printf("=== Global Buffer Overflow Experiment ===\n\n");

    switch (mode) {
        case 1: overflow_global_array(); break;
        case 2: overflow_global_ints();  break;
        case 3: overflow_static();       break;
        default:
            printf("Usage: %s <mode>\n", argv[0]);
            printf("  mode 1: Global char array overflow\n");
            printf("  mode 2: Global int array overflow\n");
            printf("  mode 3: Static variable overflow\n");
    }

    return 0;
}
