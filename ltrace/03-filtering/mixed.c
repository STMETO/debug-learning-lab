#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

// ── ltrace Filtering Experiment ──
// This program calls various types of library functions
// Use ltrace -e to trace only the categories you care about

void string_work() {
    printf("  [String Operations]\n");
    const char *s = "Hello, ltrace filtering demo!";
    size_t len = strlen(s);
    char *copy = strdup(s);
    char *pos  = strchr(copy, 'l');
    if (pos) printf("    first 'l' at position %ld\n", pos - copy);
    free(copy);
    printf("    string length: %zu\n", len);
}

void memory_work() {
    printf("  [Memory Operations]\n");
    char buf1[64], buf2[64];
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));
    strcpy(buf1, "memory test data");
    memcpy(buf2, buf1, strlen(buf1) + 1);
    int same = memcmp(buf1, buf2, strlen(buf1));
    printf("    memory compare result: %s\n", same == 0 ? "same" : "different");
    // Multiple small allocations
    for (int i = 0; i < 5; i++) {
        int *p = malloc(64);
        p[0] = i;
        free(p);
    }
}

void math_work() {
    printf("  [Math Operations]\n");
    double r = sqrt(144.0);
    double p = pow(2.0, 8.0);
    printf("    sqrt(144) = %.0f, pow(2,8) = %.0f\n", r, p);
}

void char_work() {
    printf("  [Character Operations]\n");
    const char *text = "AbCdEf123";
    int alpha = 0, digit = 0;
    for (int i = 0; text[i]; i++) {
        if (isalpha(text[i])) alpha++;
        if (isdigit(text[i])) digit++;
    }
    printf("    letters: %d, digits: %d\n", alpha, digit);
}

int main() {
    printf("=== ltrace Filtering Experiment ===\n\n");
    printf("Four types of operations:\n");

    string_work();
    printf("\n");
    memory_work();
    printf("\n");
    math_work();
    printf("\n");
    char_work();

    printf("\n=== Experiment End ===\n");
    return 0;
}
