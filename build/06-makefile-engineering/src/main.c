#include <stdio.h>

#include "parser.h"

int main(int argc, char **argv)
{
    int value;

    if (argc != 2 || parse_positive_int(argv[1], &value) != 0) {
        fprintf(stderr, "usage: %s POSITIVE_INTEGER\n", argv[0]);
        return 2;
    }
    printf("parsed value: %d\n", value);
    return 0;
}
