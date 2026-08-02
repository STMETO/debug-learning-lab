#include <stdio.h>
#include <stdlib.h>
int main(void) { int *p = malloc(sizeof(*p)); p[1] = 42; printf("%d\n", p[1]); free(p); return 0; }
