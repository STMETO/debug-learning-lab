#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

// A program designed to generate block I/O and page fault events.
// Use block:* tracepoints and page-fault counters to analyze.

#define FILE_SIZE (50 * 1024 * 1024)  // 50 MB
#define CHUNK_SIZE 4096

// ----- Part 1: Direct I/O — generates block layer events -----

void direct_write_test(const char *path) {
    printf("  Writing 50 MB with fsync...\n");
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open for write"); return; }

    char *buf = (char *)malloc(CHUNK_SIZE);
    if (!buf) return;
    memset(buf, 'A', CHUNK_SIZE);

    int chunks = FILE_SIZE / CHUNK_SIZE;
    for (int i = 0; i < chunks; i++) {
        // Each write goes through the block layer
        ssize_t n = write(fd, buf, CHUNK_SIZE);
        if (n < 0) { perror("write"); break; }
    }

    // fsync forces dirty pages to disk — triggers block I/O completion events
    fsync(fd);
    close(fd);
    free(buf);
    printf("  Write complete.\n");
}

// ----- Part 2: mmap + page faults -----

void mmap_pagefault_test(const char *path) {
    printf("  mmap + triggering page faults...\n");
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open for read"); return; }

    struct stat st;
    fstat(fd, &st);
    size_t size = st.st_size;

    // mmap the file — no actual I/O yet (lazy mapping)
    char *map = (char *)mmap(NULL, size, PROT_READ,
                             MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) { perror("mmap"); close(fd); return; }

    // Access every 4096th byte (one per page) — triggers minor page faults
    // (data already in page cache from the write above)
    long sum = 0;
    for (size_t i = 0; i < size; i += 4096) {
        sum += map[i];
    }
    printf("  mmap sum = %ld\n", sum);

    // Drop the mapping and re-read to trigger major faults
    munmap(map, size);

    // Tell kernel to drop cached pages for this file
    printf("  Dropping page cache...\n");
    system("sync; echo 1 > /proc/sys/vm/drop_caches 2>/dev/null || true");

    // Re-mmap and access — now triggers major page faults (disk reads)
    map = (char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) { perror("mmap2"); close(fd); return; }

    sum = 0;
    for (size_t i = 0; i < size; i += 4096) {
        sum += map[i];  // Each access now reads from disk → major fault
    }
    printf("  mmap sum (after drop cache) = %ld\n", sum);

    munmap(map, size);
    close(fd);
}

// ----- Part 3: Random vs Sequential read (block latency difference) -----

void random_read_test(const char *path) {
    printf("  Random read test...\n");
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    struct stat st;
    fstat(fd, &st);
    int num_pages = st.st_size / 4096;

    char buf[4096];
    long sum = 0;

    // Sequential reads (fast — elevator scheduler can merge requests)
    for (int i = 0; i < 1000; i++) {
        pread(fd, buf, sizeof(buf), i * 4096);
        sum += buf[0];
    }
    printf("  Sequential: sum=%ld\n", sum);

    // Random reads (slow — each read may be a separate block I/O)
    sum = 0;
    unsigned int seed = 12345;
    for (int i = 0; i < 1000; i++) {
        seed = seed * 1103515245 + 12345;
        off_t offset = ((off_t)(seed >> 16) % num_pages) * 4096;
        pread(fd, buf, sizeof(buf), offset);
        sum += buf[0];
    }
    printf("  Random: sum=%ld\n", sum);

    close(fd);
}

// ----- Main -----

int main() {
    printf("=== Block I/O & Page Fault Analysis ===\n");
    const char *path = "/tmp/perf_blockio_test.dat";

    printf("\n--- Part 1: Direct Write (block layer events) ---\n");
    direct_write_test(path);

    printf("\n--- Part 2: mmap + Page Faults ---\n");
    mmap_pagefault_test(path);

    printf("\n--- Part 3: Sequential vs Random Read ---\n");
    random_read_test(path);

    printf("\n=== Done! ===\n");
    unlink(path);
    return 0;
}
