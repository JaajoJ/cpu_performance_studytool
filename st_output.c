#include <stdio.h>
#include <stdbool.h>
#include "st_output.h"

void st_output_arguments(const int core, const char output_mode, const bool modify)
{
    printf("    core: %i\n    output_mode: %c\n    modify: %i\n", core, output_mode, modify);
}