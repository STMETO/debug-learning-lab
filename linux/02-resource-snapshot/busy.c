#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    volatile unsigned long long value = 0;
    char *memory = malloc(32 * 1024 * 1024);

    if (memory != NULL) {
        memory[0] = 1;
    }
    printf("resource_demo pid=%ld\n", (long)getpid());
    fflush(stdout);
    for (;;) {
        value = value * 33 + 7;
    }
}
