#include <stdio.h>
#include <string.h>

// ── 各种data类型，供你练习 print 命令 ──

// 结构体：用来练习打印复合data类型
struct Student {
    char name[32];
    int  age;
    float score;
};

// 一个接受各种参数的函数
void examine_data(int count, double ratio, const char *label) {
    int    local_int    = count * 10;
    double local_double = ratio * 2.5;
    char   local_char   = 'Z';

    // array
    int    primes[] = {2, 3, 5, 7, 11};
    char   word[]   = "hello";

    // 结构体
    struct Student stu = {0};
    strcpy(stu.name, "Alice");
    stu.age   = 20;
    stu.score = 92.5;

    // pointer
    int  *ptr_int   = &local_int;
    int  *ptr_array = primes;  // 指向array首元素

    // ★ 重点观察点：在这里设断点，然后练习各种 print Usage
    printf("=== Checkpoint ===\n");
    printf("count=%d, ratio=%.2f, label=%s\n", count, ratio, label);
    printf("local_int=%d, local_double=%.2f, local_char=%c\n",
           local_int, local_double, local_char);
    printf("primes[0]=%d, word=%s, stu.name=%s\n",
           primes[0], word, stu.name);
    printf("ptr_int points to %d, ptr_array points to %d\n",
           *ptr_int, *ptr_array);
}

int main() {
    examine_data(5, 3.14, "debug-me");
    return 0;
}
