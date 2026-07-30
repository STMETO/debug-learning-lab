#include <assert.h>
#include <stdio.h>

#include "parser.h"

int main(void)
{
    int value = 0;

    assert(parse_positive_int("42", &value) == 0);
    assert(value == 42);
    assert(parse_positive_int("0", &value) != 0);
    assert(parse_positive_int("-1", &value) != 0);
    assert(parse_positive_int("abc", &value) != 0);
    puts("all parser tests passed");
    return 0;
}
