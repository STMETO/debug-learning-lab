#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("process_demo pid=%ld\n", (long)getpid());
    fflush(stdout);
    for (;;) {
        sleep(1);
    }
}
