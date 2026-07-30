#include <stdio.h>

#include "stats.h"

int main(void)
{
    const int values[] = {10, 20, 30, 40};

    printf("mean = %.1f\n", mean(values, 4));
    return 0;
}
