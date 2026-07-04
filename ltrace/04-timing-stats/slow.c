#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

// ── ltrace Timing and Statistics Experiment ──
// Use -t/-T to see timing, use -c to see statistics

void fast_calls() {
    printf("[Phase A] Fast Calls\n");
    for (int i = 0; i < 10; i++) {
        strlen("quick test");
        isdigit('5');
    }
    printf("  10 strlen + 10 isdigit calls complete\n");
}

void medium_calls() {
    printf("\n[Phase B] Medium Calls\n");
    // String copy — medium duration
    for (int i = 0; i < 5; i++) {
        char *s = strdup("this is a medium-length string for testing");
        if (s) free(s);
    }
    printf("  5 strdup+free calls complete\n");
}

void slow_calls() {
    printf("\n[Phase C] Slow Calls\n");
    // Obvious delay — sleep is a library function
    for (int i = 0; i < 3; i++) {
        printf("  sleep %d...\n", i + 1);
        sleep(1);   // 1 second — ltrace -T will show elapsed time
    }
    printf("  3 sleep calls complete\n");
}

void mixed_frequencies() {
    printf("\n[Phase D] Mixed Frequency\n");
    // strlen is called frequently, strdup only once
    const char *text = "frequency analysis demo string";
    size_t total = 0;
    for (int i = 0; text[i]; i++) {
        total += strlen(text + i);  // ★ strlen called on every iteration!
    }
    char *copy = strdup(text);
    free(copy);
    printf("  Executed %zu strlen calls, 1 strdup call\n", strlen(text));
}

int main() {
    printf("=== ltrace Timing and Statistics Experiment ===\n\n");

    fast_calls();
    medium_calls();
    slow_calls();
    mixed_frequencies();

    printf("\n=== Experiment End ===\n");
    return 0;
}
