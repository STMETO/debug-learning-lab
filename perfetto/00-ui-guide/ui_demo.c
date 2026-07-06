#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
int main() {
    printf("=== UI Demo Workload ===\n");
    for (int b = 0; b < 3; b++) {
        printf("Batch %d: 10 file ops + fork...\n", b);
        for (int i = 0; i < 10; i++) {
            char p[64]; snprintf(p,64,"/tmp/ui_%d.txt",i);
            int fd = open(p,O_WRONLY|O_CREAT|O_TRUNC,0644);
            if(fd>=0){ write(fd,"x",1); close(fd); }
        }
        pid_t pid = fork();
        if(pid==0){ for(volatile int j=0;j<5000000;j++); _exit(0); }
        else waitpid(pid,NULL,0);
        usleep(500000);
    }
    printf("Done.\n"); return 0;
}
