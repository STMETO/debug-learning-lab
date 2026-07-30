#include <stdio.h>

#include "textutil.h"

int main(void)
{
    printf("words: %d\n", count_words("CMake makes builds reproducible"));
    return 0;
}
