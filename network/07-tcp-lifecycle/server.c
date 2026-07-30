#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
int main(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0), client, opt = 1;
    char buffer[32];
    struct sockaddr_in addr = {.sin_family=AF_INET,.sin_port=htons(18080),.sin_addr.s_addr=htonl(INADDR_LOOPBACK)};
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(fd,(struct sockaddr *)&addr,sizeof(addr)) || listen(fd,1)) return 1;
    printf("tcp_demo listening on 127.0.0.1:18080\n"); fflush(stdout);
    client = accept(fd, NULL, NULL);
    read(client, buffer, sizeof(buffer));
    write(client, "ok\n", 3);
    close(client); close(fd); return 0;
}
