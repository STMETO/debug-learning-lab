#include <stdio.h>

__attribute__((noinline))
static int divide(int numerator, int denominator)
{
    return numerator / denominator;
}

__attribute__((noinline))
static int calculate(void)
{
    return divide(100, 5);
}

int main(void)
{
    printf("result = %d\n", calculate());
    return 0;
}
