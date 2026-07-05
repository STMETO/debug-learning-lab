#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

// A program that performs various system calls — designed to be
// traced with both "perf trace" and "strace" so you can compare them.

void file_operations() {
    // Open, write, read, close — classic file I/O syscalls
    int fd = open("/tmp/perf_trace_demo.txt",
                  O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open"); return; }

    const char *msg = "Hello from perf trace demo!\n";
    write(fd, msg, strlen(msg));

    close(fd);

    // Re-open and read back
    fd = open("/tmp/perf_trace_demo.txt", O_RDONLY);
    if (fd < 0) { perror("open for read"); return; }
    char buf[128];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("  [file] read back: %s", buf);
    }
    close(fd);
}

void memory_operations() {
    // malloc/free — these may or may not trigger syscalls (brk/mmap)
    // depending on malloc's internal caching
    for (int i = 0; i < 100; i++) {
        char *p = (char *)malloc(1024);
        if (p) {
            p[0] = 'A' + (i % 26);
            free(p);
        }
    }
    printf("  [memory] 100 alloc/free cycles done\n");
}

void network_operations() {
    // socketpair — triggers socket-related syscalls
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
        perror("socketpair");
        return;
    }
    const char *msg = "ping";
    write(sv[0], msg, 4);
    char buf[16];
    read(sv[1], buf, sizeof(buf));
    close(sv[0]);
    close(sv[1]);
    printf("  [network] socketpair ping-pong done\n");
}

void process_operations() {
    // getpid, getuid — fast syscalls that don't block
    pid_t pid = getpid();
    uid_t uid = getuid();
    printf("  [process] pid=%d uid=%d\n", pid, uid);
}

int main() {
    printf("=== perf trace Demo ===\n");

    printf("Step 1: file operations...\n");
    file_operations();

    printf("Step 2: memory operations...\n");
    memory_operations();

    printf("Step 3: network operations...\n");
    network_operations();

    printf("Step 4: process info...\n");
    process_operations();

    printf("=== Done! ===\n");
    return 0;
}
