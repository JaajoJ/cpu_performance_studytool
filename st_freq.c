#include <stdio.h>
#include <stdbool.h>
#include "st_freq.h"

int st_freq(const int core, const char output_mode, const bool modify)
{
    printf("idle mode\n    core: %i\n    output_mode: %c\n    modify: %i\n", core, output_mode, modify);
    printf("FREQUENCY mode!\n");
    return 0;
}