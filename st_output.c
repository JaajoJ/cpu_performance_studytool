#include <stdio.h>
#include <stdbool.h>
#include "st_output.h"

void st_output_arguments(const int core, const char output_mode, const bool modify)
{
    printf("    core: %i\n    output_mode: %c\n    modify: %i\n", core, output_mode, modify);
}
#include <stdio.h>

void st_print_package_stats(const PackageStats *pkg)
{
    printf("==== CPU Package Stats ====\n");

    printf("Current governor   : %s\n", pkg->current_governor);
    printf("Online CPUs        : %ld\n", pkg->online_cpus);
    printf("Total CPUs         : %ld\n", pkg->all_cpus);
    printf("Available C-states : %i\n", pkg->available_idle_states);

    printf("\nPer-core idle statistics:\n");

    for (long cpu = 0; cpu < pkg->all_cpus && cpu < MAXIMUM_CORES; cpu++)
    {
        const IdleCoreStats *core = &pkg->coreStats[cpu];

        printf("\nCPU %ld\n", cpu);
        printf("  Max latency : %ld us\n", core->max_latency);

        printf("  Idle state times:\n");

        for (int state = 0; state < MAXIMUM_C_STATES; state++)
        {
            printf("    C%d : %ld\n",
                   state,
                   core->idle_time[state]);
        }
    }

    printf("\n===========================\n");
}
