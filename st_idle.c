#include <stdio.h>
#include <stdbool.h>
#include "st_output.h"
#include "st_idle.h"

int st_idle(const int core, const char output_mode, const bool modify)
{
    if (output_mode == 'h')
    {
        printf("Idle mode:\n");
        st_output_arguments(core, output_mode, modify);
    }
    return 0;

}