#include <stdio.h>
#include "string_utils.h"

int str_length(const char *s) {
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

int word_count(const char *s) {
    int count = 0;
    int in_word = 0;

    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == ' ') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            count++;
        }
    }
    return count;
}

void reverse_print(const char *s) {
    int len = str_length(s);
    for (int i = len - 1; i >= 0; i--) {
        putchar(s[i]);
    }
}
