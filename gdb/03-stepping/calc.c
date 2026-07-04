#include <stdio.h>

// 一个简单的compute函数链：
// calculate() → multiply() / add_twice()

int multiply(int a, int b) {
    int result = a * b;
    return result;
}

int add(int a, int b) {
    int result = a + b;
    return result;
}

// 先乘再加：return x*2 + x*3
int calculate(int x) {
    int doubled = multiply(x, 2);   // 进入 multiply
    int tripled = multiply(x, 3);   // 再进入 multiply
    int sum = add(doubled, tripled); // 进入 add
    return sum;
}

int main() {
    int input = 4;
    int result = calculate(input);
    printf("input=%d, result=%d\n", input, result);
    return 0;
}
