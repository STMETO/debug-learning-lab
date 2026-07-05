#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// A program that does many kernel memory allocations (via malloc/free
// and direct syscalls), plus a deliberate kernel-memory-like workload.
//
// Use kmem:kmalloc and kmem:kfree tracepoints to track kernel allocations
// triggered by this program's system calls.

#define N_ALLOCS 5000
#define LEAK_INTERVAL 20  // leak every N-th allocation

// ----- Part 1: Many small allocations (triggers kmem:kmalloc via brk/mmap) -----

void many_small_allocs() {
    printf("  %d small allocations (1-256 bytes)...\n", N_ALLOCS);
    void **ptrs = (void **)malloc(N_ALLOCS * sizeof(void *));
    if (!ptrs) return;

    for (int i = 0; i < N_ALLOCS; i++) {
        size_t size = (i % 256) + 1;
        ptrs[i] = malloc(size);
        if (ptrs[i]) {
            memset(ptrs[i], i & 0xFF, size);
        }
    }

    // Free most, but not all — simulated "leak pattern"
    int leaked = 0;
    for (int i = 0; i < N_ALLOCS; i++) {
        if (i % LEAK_INTERVAL == 0) {
            leaked++;  // deliberately not freed
        } else {
            free(ptrs[i]);
        }
    }
    printf("  Freed %d, leaked %d allocations\n", N_ALLOCS - leaked, leaked);
    free(ptrs);
}

// ----- Part 2: Large allocations (triggers mmap in kernel) -----

void large_allocs() {
    printf("  Large allocations (1-4 MB)...\n");
    void *ptrs[20];

    for (int i = 0; i < 20; i++) {
        size_t size = (i + 1) * 1024 * 1024;  // 1MB, 2MB, ..., 20MB
        ptrs[i] = malloc(size);
        if (ptrs[i]) {
            // Touch every page to force actual allocation
            memset(ptrs[i], 0, size);
        }
    }

    for (int i = 0; i < 20; i++) {
        free(ptrs[i]);
    }
    printf("  Large allocations done.\n");
}

// ----- Part 3: Fork + alloc (triggers kernel slab for task_struct etc.) -----

void fork_and_alloc() {
    printf("  Forking to trigger kernel object allocations...\n");
    for (int i = 0; i < 10; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child: allocate some memory and exit quickly
            void *p = malloc(4096);
            if (p) memset(p, 0, 4096);
            free(p);
            _exit(0);
        } else if (pid > 0) {
            // Parent: wait briefly
            waitpid(pid, NULL, 0);
        }
    }
    printf("  Fork/alloc cycles done.\n");
}

// ----- Part 4: Simulated kernel-like workload (syscall-heavy) -----

void syscall_heavy_workload() {
    printf("  Syscall-heavy workload...\n");
    // Repeated getpid() + small malloc/free — each triggers kernel activity
    for (int i = 0; i < 100000; i++) {
        getpid();  // fast syscall, but still enters kernel
        void *p = malloc(64);
        if (p) {
            memset(p, i & 0xFF, 64);
            free(p);
        }
    }
    printf("  Syscall-heavy done.\n");
}

// ----- Main -----

int main() {
    printf("=== Kernel Memory & Leak Analysis ===\n\n");

    printf("--- Part 1: Many Small Allocations ---\n");
    many_small_allocs();

    printf("\n--- Part 2: Large Allocations (mmap) ---\n");
    large_allocs();

    printf("\n--- Part 3: Fork + Kernel Objects ---\n");
    fork_and_alloc();

    printf("\n--- Part 4: Syscall-Heavy Workload ---\n");
    syscall_heavy_workload();

    printf("\n=== Done! ===\n");
    return 0;
}
