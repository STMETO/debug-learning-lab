#include <stdio.h>

int main(void)
{
#ifdef ENABLE_ASAN
    puts("preset demo: AddressSanitizer enabled");
#else
    puts("preset demo: standard build");
#endif
    return 0;
}
