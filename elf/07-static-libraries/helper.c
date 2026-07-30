#include "helper.h"

int checksum(const char *text)
{
    int result = 0;

    while (*text != '\0') {
        result += (unsigned char)*text++;
    }
    return result;
}
