#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── 四种泄漏类型 —— Valgrind 级分类 ──
// LSAN 只分 direct/indirect，Valgrind 分得更细

// 全局指针——用于制造 "still reachable"
char *global_ptr = NULL;

// 类型 1：Definitely lost —— 直接泄漏，指针丢失
void definitely_lost() {
    printf("【类型 1】Definitely lost —— 直接泄漏\n");
    int *p = (int *)malloc(sizeof(int) * 10);
    p[0] = 42;
    // ★ 函数返回后 p 销毁，400 字节没有任何指针指向
    printf("  分配了 10 个 int，指针即将丢失...\n");
}

// 类型 2：Indirectly lost —— 间接泄漏
struct Container {
    int *data;
    char *name;
};

void indirectly_lost() {
    printf("【类型 2】Indirectly lost —— 间接泄漏\n");

    struct Container *c = (struct Container *)malloc(sizeof(struct Container));
    c->data = (int *)malloc(100 * sizeof(int));
    c->name = strdup("container-data");

    printf("  分配了 Container + data + name\n");
    // ★ 只泄漏 c 本身（丢失了 data 和 name 的指针）
    // c 泄漏 → data 和 name 变成 "indirectly lost"
    // 但如果先 free(c->data) 和 free(c->name)，再 free(c)，就没有泄漏
    (void)c;  // 没有 free(c)！
}

// 类型 3：Still reachable —— 程序结束时还有指针指向
void still_reachable() {
    printf("【类型 3】Still reachable —— 仍有指针可达\n");

    global_ptr = (char *)malloc(256);
    strcpy(global_ptr, "this is reachable at exit");
    // ★ 程序结束时 global_ptr 还指向这块内存
    // 严格来说不是 bug（程序结束 OS 会回收），但可能掩盖真正的泄漏
    printf("  global_ptr 仍指向 256 字节\n");
}

// 类型 4：Possibly lost —— 可能泄漏
void possibly_lost() {
    printf("【类型 4】Possibly lost —— 可能泄漏\n");

    char *buf = (char *)malloc(100);
    strcpy(buf, "hello");

    // ★ 制造一个"可能泄漏"的场景：指针偏移了
    char *interior = buf + 10;
    // 如果 interior 是唯一保留的指针，Valgrind 无法确定
    // 是故意偏移还是丢失了原始指针

    // 这里只保存了 interior 指针，buf 原值丢失
    (void)interior;
    // 但因为我们也没保存 interior 到全局，实际两个都丢了
}

int main() {
    printf("╔══════════════════════════════════════╗\n");
    printf("║   Valgrind 四种泄漏分类               ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    definitely_lost();
    printf("\n");
    indirectly_lost();
    printf("\n");
    still_reachable();
    printf("\n");
    possibly_lost();

    printf("\n=== 程序结束 ===\n");
    printf("（用 valgrind --leak-check=full ./leaks 查看详细报告）\n");
    return 0;
}
