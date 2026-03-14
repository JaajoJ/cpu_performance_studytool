#include <stdio.h>
#include <stdbool.h>
#include "st_idle.h"

int st_idle(const int core, const char output_mode, const bool modify)
{
    printf("idle mode\n    core: %i\n    output_mode: %c\n    modify: %i\n", core, output_mode, modify);
    return 0;
}