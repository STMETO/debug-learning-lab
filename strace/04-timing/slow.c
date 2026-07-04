#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// ── 时间分析Experiment ──
// 用 strace -tt -T 看清楚"程序的时间花在哪里"

void phase_fast() {
    printf("[Phase A] Fast operations (microseconds)\n");
    getpid();
    getppid();
    printf("  Done\n\n");
}

void phase_medium() {
    printf("[Phase B] Medium operations (milliseconds)\n");

    // File write
    int fd = open("/tmp/strace_timing.txt", O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd >= 0) {
        for (int i = 0; i < 10; i++) {
            char buf[100];
            snprintf(buf, sizeof(buf), "Line %d: this is a timing test line.\n", i);
            write(fd, buf, strlen(buf));
        }
        close(fd);
    }

    // File read
    fd = open("/tmp/strace_timing.txt", O_RDONLY);
    if (fd >= 0) {
        char bigbuf[2048];
        ssize_t n = read(fd, bigbuf, sizeof(bigbuf));
        (void)n;
        close(fd);
    }

    printf("  Done\n\n");
}

void phase_slow() {
    printf("[Phase C] Slow operations (seconds)\n");

    // Deliberately slow loop - each iteration is a small operation
    for (int i = 0; i < 3; i++) {
        usleep(200000);  // 0.2 seconds
        printf("  slow iteration %d\n", i);
    }

    printf("  Done\n\n");
}

void phase_big_malloc() {
    printf("[Phase D] Large memory allocation\n");

    // Allocate 10MB memory
    size_t size = 10 * 1024 * 1024;
    char *big = (char *)malloc(size);
    if (big) {
        memset(big, 'A', size);   // write 10MB
        free(big);
    }

    printf("  Done\n\n");
}

int main() {
    printf("=== strace timing analysis experiment ===\n\n");

    phase_fast();
    phase_medium();
    phase_slow();
    phase_big_malloc();

    printf("=== Experiment finished ===\n");
    return 0;
}