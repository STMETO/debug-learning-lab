#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

// A program that exercises functions across MULTIPLE kernel subsystems
// to demonstrate advanced ftrace filtering (whitelist, blacklist, module filter).
//
// Operations by subsystem:
//   ext4:    file open/write/close/stat
//   vfs:     same operations pass through VFS layer
//   sched:   fork + usleep triggers scheduler activity
//   mm:      malloc/free large blocks triggers page allocation
//   syscalls: getpid, openat, write, close

void ext4_vfs_ops(int count) {
    char path[64];
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "/tmp/ftrace_filter_%d.txt", i);
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            struct stat st;
            fstat(fd, &st);  // triggers ext4_file_getattr
            write(fd, "hello", 5);
            close(fd);
        }
    }
}

void sched_ops() {
    // Fork — triggers sched_process_fork and scheduler functions
    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            usleep(1000);
            _exit(0);
        } else {
            waitpid(pid, NULL, 0);
        }
    }
}

void mm_ops() {
    // Allocate and free to trigger page allocator
    for (int i = 0; i < 20; i++) {
        size_t sz = (i + 1) * 1024 * 1024;  // 1-20 MB
        void *p = malloc(sz);
        if (p) {
            memset(p, 0, sz);  // touch every page
            free(p);
        }
    }
}

void syscall_flood() {
    for (int i = 0; i < 20000; i++) {
        getpid();
    }
}

int main() {
    printf("=== Filter Mastery Demo ===\n");

    int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);

    if (marker >= 0) write(marker, "=== EXT4/VFS START ===\n", 23);
    printf("Phase 1: 50 ext4 file ops...\n");
    ext4_vfs_ops(50);
    if (marker >= 0) write(marker, "=== EXT4/VFS END ===\n", 21);

    if (marker >= 0) write(marker, "=== SCHED START ===\n", 20);
    printf("Phase 2: 5 fork/wait cycles...\n");
    sched_ops();
    if (marker >= 0) write(marker, "=== SCHED END ===\n", 18);

    if (marker >= 0) write(marker, "=== MM START ===\n", 17);
    printf("Phase 3: 20 large alloc/free...\n");
    mm_ops();
    if (marker >= 0) write(marker, "=== MM END ===\n", 15);

    if (marker >= 0) write(marker, "=== SYSCALL START ===\n", 21);
    printf("Phase 4: 20000 getpid() calls...\n");
    syscall_flood();
    if (marker >= 0) write(marker, "=== SYSCALL END ===\n", 19);

    if (marker >= 0) close(marker);
    printf("=== Done! ===\n");
    return 0;
}
