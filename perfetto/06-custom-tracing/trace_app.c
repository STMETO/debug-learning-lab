#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

// Demonstrates Perfetto-compatible application-level tracing via trace_marker.
//
// In production Android/Linux apps, you'd use the Perfetto C++ SDK:
//   TRACE_EVENT("category", "my_function");
//
// On Linux desktop without the SDK, trace_marker achieves the same thing:
//   write(trace_marker_fd, "B|pid|my_function\n", ...);
//
// The "B|pid|name" format is Perfetto's text tracing protocol —
// Perfetto/tracebox recognizes it and converts it to proper slice events.

static int trace_fd = -1;
static int my_pid;

// Initialize tracing — open trace_marker
void trace_init() {
    trace_fd = open("/sys/kernel/tracing/trace_marker", O_WRONLY);
    my_pid = getpid();
    if (trace_fd >= 0) {
        dprintf(trace_fd, "# Initialized custom tracing for pid=%d\n", my_pid);
    }
}

// Mark the BEGINNING of a traced operation
void trace_begin(const char *name) {
    if (trace_fd < 0) return;
    // Perfetto text format: B|<pid>|<name>
    dprintf(trace_fd, "B|%d|%s\n", my_pid, name);
}

// Mark the END of a traced operation
void trace_end(const char *name) {
    if (trace_fd < 0) return;
    // Perfetto text format: E|<pid>|<name>
    dprintf(trace_fd, "E|%d|%s\n", my_pid, name);
}

// Trace an instantaneous event (counter, log message, etc.)
void trace_instant(const char *name, const char *value) {
    if (trace_fd < 0) return;
    dprintf(trace_fd, "I|%d|%s=%s\n", my_pid, name, value);
}

void trace_close() {
    if (trace_fd >= 0) close(trace_fd);
}

// ----- Application logic with tracing -----

void process_batch(int batch_id, int items) {
    char name[64];
    snprintf(name, sizeof(name), "process_batch_%d", batch_id);
    trace_begin(name);

    volatile long sum = 0;
    for (int i = 0; i < items; i++) {
        sum += (i * 7 + 13) % 9973;
    }

    trace_instant("batch_result", "ok");
    trace_end(name);
    printf("  Batch %d: sum=%ld (%d items)\n", batch_id, sum, items);
}

void load_config() {
    trace_begin("load_config");
    // Simulate config loading work
    int fd = open("/tmp/perfetto_app_config.ini", O_RDONLY);
    if (fd >= 0) {
        char buf[64];
        read(fd, buf, sizeof(buf));
        close(fd);
    }
    usleep(50000);  // 50ms simulated I/O
    trace_end("load_config");
    printf("  Config loaded\n");
}

void save_results() {
    trace_begin("save_results");
    int fd = open("/tmp/perfetto_app_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        dprintf(fd, "results: ok\n");
        close(fd);
    }
    usleep(30000);  // 30ms simulated I/O
    trace_end("save_results");
    printf("  Results saved\n");
}

int main() {
    printf("=== Custom App Tracing Demo ===\n");

    trace_init();
    if (trace_fd < 0) {
        printf("Warning: trace_marker not available (run with sudo)\n");
    }

    printf("Phase 1: load config...\n");
    load_config();

    printf("Phase 2: process batches...\n");
    for (int b = 0; b < 5; b++) {
        process_batch(b, 500000);
    }

    printf("Phase 3: save results...\n");
    save_results();

    trace_close();
    printf("=== Done! ===\n");
    return 0;
}
