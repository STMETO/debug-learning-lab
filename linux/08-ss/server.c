#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
int main(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(18080), .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) || listen(fd, 8)) return 1;
    printf("ss_demo pid=%ld listening on 127.0.0.1:18080\n", (long)getpid());
    fflush(stdout);
    for (;;) sleep(1);
}
