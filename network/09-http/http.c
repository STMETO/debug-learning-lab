#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
int main(void) {
 int fd=socket(AF_INET,SOCK_STREAM,0), c,opt=1; char req[256];
 struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(18082),.sin_addr.s_addr=htonl(INADDR_LOOPBACK)};
 setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
 if(bind(fd,(struct sockaddr *)&a,sizeof(a))||listen(fd,1)) return 1;
 printf("http_demo listening on 127.0.0.1:18082\n"); fflush(stdout);
 c=accept(fd,NULL,NULL); read(c,req,sizeof(req));
 write(c,"HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nok\n",42);
 close(c); close(fd); return 0;
}
