#include <stdio.h>

// math_utils.h 里的声明
#include "math_utils.h"
#include "string_utils.h"

int main() {
    int a = 15, b = 6;

    // 使用 math_utils 的函数
    printf("=== 数学工具测试 ===\n");
    printf("gcd(%d, %d) = %d\n", a, b, gcd(a, b));
    printf("factorial(%d) = %lld\n", 5, factorial(5));
    printf("power(%d, %d) = %lld\n", 2, 10, power(2, 10));

    // 使用 string_utils 的函数
    printf("\n=== 字符串工具测试 ===\n");
    printf("\"%s\" 的长度 = %d\n", "Hello World", str_length("Hello World"));
    printf("\"%s\" 的单词数 = %d\n", "Hello World", word_count("Hello World"));
    printf("reverse(\"%s\") = ", "debug");
    reverse_print("debug");
    printf("\n");

    return 0;
}
