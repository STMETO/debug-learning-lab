#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int config = open("/etc/hosts", O_RDONLY);
    int log = open("proc_demo.log", O_CREAT | O_WRONLY | O_APPEND, 0644);

    dprintf(log, "pid=%ld\n", (long)getpid());
    printf("proc_demo pid=%ld, fds=%d,%d\n", (long)getpid(), config, log);
    fflush(stdout);
    for (;;) sleep(1);
}
