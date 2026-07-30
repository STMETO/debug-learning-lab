#include "symbols.h"

int exported_counter = 3;
static int private_counter;

int public_add(int left, int right)
{
    return left + right + private_counter;
}
