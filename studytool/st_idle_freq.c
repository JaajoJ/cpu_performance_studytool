#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "st_output.h"
#include "st_idle_freq.h"

// https://docs.kernel.org/admin-guide/pm/cpuidle.html

int read_string_addr(const char * addr, char * buffer, size_t buffer_len)
{
    int fd = open(addr, O_RDONLY);
    if (fd < 0) {
        perror("read_string_addr");
        return 1;
    }
    
    size_t bytes_read;

    while ((bytes_read = read(fd, buffer, buffer_len - 1)) > 0) {
        buffer[bytes_read] = '\0';
    }
    return 0;
}

int read_int_addr(const char *addr, long *output_int)
{
    int fd = open(addr, O_RDONLY);
    if (fd < 0) {
        perror("read_int_addr");
        return -1;
    }

    ssize_t bytes_read;
    char c;
    long value = 0;

    while ((bytes_read = read(fd, &c, 1)) > 0) {
        if (c < '0' || c > '9')
            break;

        value = value * 10 + (c - '0');
    }

    close(fd);

    *output_int = value;
    return 0;
}

int get_available_c_states()
{
    char path[128];
    int count = 0;

    for (int i = 0; i < MAXIMUM_C_STATES; i++)
    {
        sprintf(path,
            "/sys/devices/system/cpu/cpu0/cpuidle/state%d", i);

        int fd = open(path, O_RDONLY);
        if (fd < 0)
            break;

        close(fd);
        count++;
    }

    return count;
}

int st_idle_freq(const int core, const char output_mode, const bool modify)
{
    PackageStats package = st_idle_freq_get_package();

    if ( output_mode == 'h' )
    {
        st_output_arguments(core, output_mode, modify);
        st_print_package_stats(&package);
    }
    else if (output_mode == 'j')
    {
        st_print_package_stats_json(&package);
    }
    return 0;

}


PackageStats st_idle_freq_get_package()
{
    PackageStats package = {0};
    char addr_buf[128] = "";
    
    // CPU information
    package.online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    package.all_cpus = sysconf(_SC_NPROCESSORS_CONF);
    package.available_idle_states = get_available_c_states();
    st_idle_freq_get_core_idle_delta(&package);

    //printf("this %ld\n", package.all_cpus);
    for ( int core_id = 0; core_id < package.all_cpus; ++core_id )
    {
        sprintf(addr_buf, CORE_LATENCY_ADDR, core_id);
        read_int_addr(addr_buf, &package.coreStats[core_id].max_latency);
    }

    // Governor information
    read_string_addr(PACKAGE_CURRENT_GOVERNOR_ADDR, package.current_governor, MAXIMUM_GOVERNOR_NAME);
    

    return package;
}

int get_core_idle(long * c_state_idle, const int core_number, const int available_c_states)
{
    char addr_buf[128] = "";
    for (int idle_state = 0; idle_state < available_c_states; ++ idle_state )
    {
        sprintf(addr_buf, CORE_STATE_TIME_ADDR, core_number, idle_state);
        read_int_addr(addr_buf,  &c_state_idle[idle_state]);
    }
    return 0;

}

int st_idle_freq_get_core_idle_delta(PackageStats * package_stats)
{
    long c_states_time[MAXIMUM_CORES][MAXIMUM_C_STATES] = {0};
    long c_states_time_delta[MAXIMUM_C_STATES] = {0};
    long combined_time = 0;

    for (int core_number = 0; core_number < package_stats->all_cpus; ++core_number)
    {
        get_core_idle(c_states_time[core_number], core_number, package_stats->available_idle_states);
    }

    sleep(5);

    for (int core_number = 0; core_number < package_stats->all_cpus; ++core_number)
    {
        get_core_idle(c_states_time_delta, core_number, package_stats->available_idle_states);

        for (int idle_state = 0; idle_state < package_stats->available_idle_states; ++idle_state)
        {
            c_states_time[core_number][idle_state] = c_states_time_delta[idle_state] - c_states_time[core_number][idle_state];
        }
    }



    // Normalize
    
    for (int core_number = 0; core_number < package_stats->all_cpus; ++core_number)
    {
        combined_time = 0;
        for (int idle_state = 0; idle_state < package_stats->available_idle_states; ++ idle_state )
        {
            combined_time += c_states_time[core_number][idle_state];
        }
        if ( combined_time )
        {
            for (int idle_state = 0; idle_state < package_stats->available_idle_states; ++ idle_state )
            {
                package_stats->coreStats[core_number].idle_time[idle_state] = (c_states_time[core_number][idle_state] * 1000) / combined_time;
            }
        }
    }

    // calculate averages
    



    return 0;

}
