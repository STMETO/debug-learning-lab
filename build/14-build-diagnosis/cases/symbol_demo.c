#include <stdio.h>

static int internal_value(int input)
{
    return input * 2;
}

int main(void)
{
    printf("%d\n", internal_value(21));
    return 0;
}
