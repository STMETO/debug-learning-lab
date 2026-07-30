#include <stdio.h>

int calculate_checksum(const char *text);

int main(void)
{
    printf("checksum = %d\n", calculate_checksum("debug"));
    return 0;
}
