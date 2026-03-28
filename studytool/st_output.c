#include <stdio.h>
#include <stdbool.h>
#include "st_output.h"

void st_output_arguments(const int core, const char output_mode)
{
    printf("    core: %i\n    output_mode: %c\n", core, output_mode);
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

        for (int state = 0; state < pkg->available_idle_states; state++)
        {
            printf("    C%d : %d\n",
                   state,
                   core->idle_time[state]);
        }
    }

    printf("\n===========================\n");
}

void print_json_key(char * key, const int value)
{
    printf("\"%s\":%i", key, value);
}

void st_print_package_stats_json(const PackageStats *pkg)
{
    char buf[128] = {0};
    int avg_idle[MAXIMUM_C_STATES] = {0};
    int accepted_idle_cores = 0; // if core is disabled or isolated it is not accounted in the average.

    printf("{");
    // CORE metrics
    for (int core_number = 0; core_number < pkg->all_cpus; ++core_number)
    {
        sprintf(buf, "st.core.%i.configured_max_latency", core_number);
        print_json_key(buf, pkg->coreStats[core_number].max_latency);
        int accept_core = false;
        for ( int idle_state = 0; idle_state < pkg->available_idle_states; ++idle_state )
        {
            printf(",");
            sprintf(buf, "st.core.%i.idle_state.%i", core_number, idle_state);
            print_json_key(buf, pkg->coreStats[core_number].idle_time[idle_state]);
            if( pkg->coreStats[core_number].idle_time[idle_state] )
            {
                accept_core = true;
            }
            avg_idle[idle_state] += pkg->coreStats[core_number].idle_time[idle_state];
        }
        printf(",");
        if ( accept_core )
        {
            ++accepted_idle_cores;
        }
    }

    // PACKAGES metrics
    print_json_key("st.package.online", pkg->online_cpus);
    for ( int idle_state = 0; idle_state < pkg->available_idle_states; ++idle_state )
    {
        printf(",");
        sprintf(buf, "st.package.idle_state_avg.%i", idle_state);
        if (accepted_idle_cores)
        {
            print_json_key(buf, avg_idle[idle_state] / accepted_idle_cores);
        }
    }
    printf("}");
    printf("\n");
}
