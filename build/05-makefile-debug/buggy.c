#include <stdio.h>
#include <stdlib.h>

static int average(const int *values, size_t count)
{
    int total = 0;

    for (size_t i = 0; i <= count; ++i) {
        total += values[i];
    }
    return total / (int)count;
}

int main(void)
{
    int *values = malloc(3 * sizeof(*values));
    if (values == NULL) {
        return 1;
    }

    values[0] = 10;
    values[1] = 20;
    values[2] = 30;
    printf("average = %d\n", average(values, 3));
    free(values);
    return 0;
}
