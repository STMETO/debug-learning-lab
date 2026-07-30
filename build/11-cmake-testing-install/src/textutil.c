#include <ctype.h>

#include "textutil.h"

int count_words(const char *text)
{
    int count = 0;
    int in_word = 0;

    while (*text != '\0') {
        if (isspace((unsigned char)*text)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            ++count;
        }
        ++text;
    }
    return count;
}
