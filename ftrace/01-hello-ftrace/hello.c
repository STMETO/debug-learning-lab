#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// A minimal program to demonstrate ftrace's trace_marker.
// Run it, then read /sys/kernel/tracing/trace to see your markers.

int main() {
    printf("=== Hello ftrace ===\n");

    // Open the trace_marker — writing to this file inserts
    // a custom message into the kernel's ftrace buffer.
    int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);
    if (marker < 0) {
        perror("open trace_marker");
        printf("Hint: try running with sudo\n");
        return 1;
    }

    // Write markers before, during, and after our work
    write(marker, ">>> DEMO START\n", 15);

    // Do a few file operations — these trigger kernel functions
    // that ftrace can also capture
    int fd = open("/tmp/ftrace_hello.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "Hello from ftrace!\n", 19);
        close(fd);
    }

    // A few fast syscalls
    for (int i = 0; i < 5; i++) {
        getpid();
    }

    write(marker, "<<< DEMO END\n", 13);
    close(marker);

    printf("=== Done! Now read: sudo cat /sys/kernel/tracing/trace ===\n");
    return 0;
}
