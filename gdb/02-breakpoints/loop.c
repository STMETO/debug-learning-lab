#include <stdio.h>

// compute平方
int square(int n) {
    return n * n;
}

// 判断YesNo为偶数
int is_even(int n) {
    return n % 2 == 0;
}

int main() {
    int total = 0;

    for (int i = 1; i <= 5; i++) {
        int sq = square(i);

        if (is_even(sq)) {
            printf("i=%d, i²=%d (even), total=%d\n", i, sq, total);
        } else {
            printf("i=%d, i²=%d (odd), total=%d\n", i, sq, total);
        }

        total += sq;
    }

    printf("Final result: 1²+2²+3²+4²+5² = %d\n", total);
    return 0;
}
