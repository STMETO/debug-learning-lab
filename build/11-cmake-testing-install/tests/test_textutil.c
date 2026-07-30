#include <assert.h>
#include <stdio.h>

#include "textutil.h"

int main(void)
{
    assert(count_words("") == 0);
    assert(count_words("one") == 1);
    assert(count_words("one  two\tthree") == 3);
    puts("all textutil tests passed");
    return 0;
}
