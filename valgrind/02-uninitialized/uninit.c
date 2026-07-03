#include <stdio.h>
#include <stdlib.h>

// ── 未初始化值检测 —— Valgrind 的独家超能力 ──
// ASan 检测不到这类 bug，编译器警告也可能遗漏
// 但 Valgrind 能精确追踪"值从未初始化到被使用"的完整路径

// 场景 1：条件分支依赖于未初始化的值
void branch_on_uninit() {
    printf("【场景 1】分支依赖未初始化值\n");

    int x;  // ★ 没有初始化！

    if (x > 0) {        // Valgrind: "Conditional jump or move depends on uninitialised value(s)"
        printf("  x 是正数: %d\n", x);
    } else {
        printf("  x 是零或负数: %d\n", x);
    }
}

// 场景 2：未初始化值作为函数参数
void use_uninit_as_arg() {
    printf("【场景 2】未初始化值作为参数\n");

    int scores[10];     // ★ 没有初始化！
    int total = 0;

    // 只填充了前 5 个，忘记填充后 5 个
    for (int i = 0; i < 5; i++) {
        scores[i] = i * 10;
    }

    // ★ 后 5 个元素是垃圾值，但被用来计算了
    for (int i = 0; i < 10; i++) {
        total += scores[i];
    }
    printf("  total = %d （可能每次运行结果都不同！）\n", total);
}

// 场景 3：未初始化的堆内存
void uninit_heap() {
    printf("【场景 3】未初始化的堆内存\n");

    char *buf = (char *)malloc(32);  // malloc 不清零！
    // ★ buf 的内容是随机的——没调 memset 或直接用 calloc

    // 使用 buf 中的垃圾值做 strlen
    size_t len = strlen(buf);   // Valgrind: strlen 读取了未初始化数据
    printf("  strlen(未初始化buf) = %zu （肯定是错的）\n", len);

    free(buf);
}

// 场景 4：部分初始化的结构体
struct Point {
    int x;
    int y;
    int z;
};

void partially_init_struct() {
    printf("【场景 4】部分初始化的结构体\n");

    struct Point p;
    p.x = 10;
    p.y = 20;
    // ★ 忘记初始化 p.z！

    int sum = p.x + p.y + p.z;
    printf("  x+y+z = %d（z 是垃圾值，结果不可预测）\n", sum);
}

int main() {
    printf("╔══════════════════════════════════════╗\n");
    printf("║  ⭐ Valgrind 独家能力：未初始化值检测 ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    branch_on_uninit();
    printf("\n");
    use_uninit_as_arg();
    printf("\n");
    uninit_heap();
    printf("\n");
    partially_init_struct();

    printf("\n=== 实验结束 ===\n");
    return 0;
}
