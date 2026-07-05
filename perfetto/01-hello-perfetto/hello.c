#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// A simple program that generates enough kernel activity for Perfetto
// to capture meaningful ftrace events during a 5-second recording window.

void file_work(int count) {
    char path[64];
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "/tmp/perfetto_hello_%d.txt", i);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            write(fd, "Perfetto was here!\n", 19);
            close(fd);
        }
    }
}

int main() {
    printf("=== Hello Perfetto ===\n");
    printf("This program does file I/O while Perfetto records.\n");

    // Batch 1: light work
    printf("Batch 1: 20 file ops...\n");
    file_work(20);

    // Batch 2: more work — gives tracebox time to capture
    printf("Batch 2: 100 file ops...\n");
    file_work(100);

    // Batch 3: final burst
    printf("Batch 3: 50 file ops...\n");
    file_work(50);

    printf("=== Done! ===\n");
    return 0;
}
