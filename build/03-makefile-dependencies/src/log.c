#include <stdio.h>

#include "config.h"
#include "log.h"

void log_startup(int port)
{
    printf("%s listening on port %d\n", APP_NAME, port);
}
