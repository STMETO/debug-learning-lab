#include <stdio.h>

static int transform(int input)
{
    int doubled = input * 2;
    return doubled + 1;
}

int main(void)
{
    printf("%d\n", transform(20));
    return 0;
}
