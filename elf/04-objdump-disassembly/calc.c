#include <stdio.h>

__attribute__((noinline))
static int sum_to(int limit)
{
    int total = 0;

    for (int i = 1; i <= limit; ++i) {
        total += i;
    }
    return total;
}

int main(void)
{
    printf("sum = %d\n", sum_to(10));
    return 0;
}
