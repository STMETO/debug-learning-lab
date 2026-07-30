#include <stdio.h>

__attribute__((noinline))
static int suspicious_operation(int value)
{
    return value * 17;
}

int main(void)
{
    printf("%d\n", suspicious_operation(3));
    return 0;
}
