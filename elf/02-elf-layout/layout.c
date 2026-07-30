#include <stdio.h>

static const char *message = "sections and segments";
static int counter = 7;

int main(void)
{
    printf("%s: %d\n", message, counter);
    return 0;
}
