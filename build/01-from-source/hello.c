#include <stdio.h>

#define SCALE 3

static int multiply(int value)
{
    return value * SCALE;
}

int main(void)
{
    int input = 7;
    int result = multiply(input);

    printf("%d * %d = %d\n", input, SCALE, result);
    return 0;
}
