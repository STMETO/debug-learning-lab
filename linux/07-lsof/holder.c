#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
int main(void) {
    int a = open("/etc/hosts", O_RDONLY);
    int b = open("held.log", O_CREAT | O_WRONLY | O_APPEND, 0644);
    printf("lsof_demo pid=%ld fds=%d,%d\n", (long)getpid(), a, b);
    fflush(stdout);
    for (;;) sleep(1);
}
