#include <stdio.h>

#include "symbols.h"

int main(void)
{
    printf("%d\n", public_add(exported_counter, 4));
    return 0;
}
