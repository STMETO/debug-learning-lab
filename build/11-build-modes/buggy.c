#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *values = malloc(2 * sizeof(*values));
    values[0] = 10; values[1] = 20;
    printf("%d\n", values[2]);
    free(values); return 0;
}
