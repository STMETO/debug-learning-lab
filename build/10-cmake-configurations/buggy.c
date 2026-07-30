#include <stdio.h>
#include <stdlib.h>

static int sum(const int *numbers, size_t count)
{
    int result = 0;

    for (size_t i = 0; i <= count; ++i) {
        result += numbers[i];
    }
    return result;
}

int main(void)
{
    int *numbers = malloc(2 * sizeof(*numbers));
    if (numbers == NULL) {
        return 1;
    }

    numbers[0] = 10;
    numbers[1] = 20;
    printf("sum = %d\n", sum(numbers, 2));
    free(numbers);
    return 0;
}
