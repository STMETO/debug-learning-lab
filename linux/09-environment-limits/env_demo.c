#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
int main(void) {
    struct rlimit limit;
    getrlimit(RLIMIT_NOFILE, &limit);
    printf("APP_MODE=%s\n", getenv("APP_MODE") ? getenv("APP_MODE") : "(unset)");
    printf("RLIMIT_NOFILE=%lu\n", (unsigned long)limit.rlim_cur);
    return 0;
}
