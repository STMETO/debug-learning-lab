#include <stdio.h>

// 一个简单的加法函数 —— 等一下我们用 GDB 进入它内部看看
int add(int a, int b) {
    int result = a + b;
    return result;
}

int main() {
    int x = 10;
    int y = 20;

    // 调用 add 函数
    int sum = add(x, y);

    // 打印结果
    printf("x = %d, y = %d, sum = %d\n", x, y, sum);

    return 0;
}
