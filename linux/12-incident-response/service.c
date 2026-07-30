#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
int main(void) {
    const char *path = getenv("APP_CONFIG");
    int config, fd, opt = 1;
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(18081), .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
    if (path == NULL || (config = open(path, O_RDONLY)) < 0) { perror("APP_CONFIG"); return 2; }
    /* 保持配置文件描述符打开，便于用 lsof 观察服务现场。 */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); return 3; }
    if (listen(fd, 8) < 0) return 4;
    printf("incident_service pid=%ld port=18081\n", (long)getpid());
    fflush(stdout);
    for (;;) sleep(1);
}
