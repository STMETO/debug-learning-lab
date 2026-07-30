#include <stdio.h>
#include <unistd.h>
int main(void) {
    for (int i = 1; i <= 5; ++i) {
        FILE *fp = fopen("status.txt", "w");
        fprintf(fp, "iteration=%d\n", i);
        fclose(fp);
        sleep(1);
    }
    return 0;
}
