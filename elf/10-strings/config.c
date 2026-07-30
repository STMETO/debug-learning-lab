#include <stdio.h>

static const char *endpoint = "https://api.example.test:8443/v1/health";
static const char *feature_flag = "FEATURE_EXPERIMENTAL_MODE";

int main(void)
{
    fprintf(stderr, "connecting to %s (%s)\n", endpoint, feature_flag);
    return 0;
}
