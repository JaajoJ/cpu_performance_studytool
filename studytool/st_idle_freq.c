#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/wait.h>
#include "st_output.h"
#include "st_idle_freq.h"

// https://docs.kernel.org/admin-guide/pm/cpuidle.html


// get functions
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
    return 0;
}

// modify functions

void st_idle_freq_modify(char *path)
{
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        /* child — replace with vi */
        execlp("vi", "vi", path, NULL);
        perror("execlp");   /* only reached if execlp fails */
        _exit(1);
    }

    /* parent — wait for vi to exit */
    waitpid(pid, NULL, 0);
}

int st_set_default_config(char * config_path)
{
    int fd;
    char buf[512] = {0};
    char buf2[64] = {0};
    STConfig config = ST_CONFIG_DEFAULTS;
    PackageStats package = st_idle_freq_get_package();

    fd = open(config_path, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) 
    {
        fprintf(stderr, "Can not open config file path\n");
        return 1;
    }

    sprintf(buf, "%d # DMA_LATENCY_US\n", config.dma_latency_us);

    // write dma
    if (write(fd, buf, sizeof(buf)) == -1)
    {
        fprintf(stderr, "Unable to write to config file\n");
        close(fd);
        return 1;
    }

    // write goal c-states
    for (int i = 0; i < package.available_idle_states; ++i )
    {
        buf2[0] = '\0';
        buf[0] = '\0';
        if (i == 0)
        {
            for (int i2 = 0; i2 < package.all_cpus; ++i2)
            {
                sprintf(buf2, "%d,", i2);
                strcat(buf, buf2);
            }
        }

        sprintf(buf2, " # C-state %i target for CPUS\n", i);

        strcat(buf, buf2);

        if (write(fd, buf, strlen(buf)) == -1)
        {
            fprintf(stderr, "Unable to write to config file\n");
            close(fd);
            return 1;
        }
    }

    close(fd);
    return 0;

}

int read_line(int fd, char * buf, ssize_t max_length)
{
    char c = '\0';
    int length = 1;
    while(true)
    {
        if (length == max_length)
        {
            return 1;
        }
        read(fd, &c, 1);
        if(c == '\n' || c == EOF)
        {
            return 0;
        }
        strcat(buf, &c);
        buf[length + 1] = '\0';
        ++ length;

    }
    return 0;
}

int st_get_config(STConfig * config, char * config_path)
{
    int fd;
    char read_buf[256] = {0};
    PackageStats package = st_idle_freq_get_package();
    memset(config, 0, sizeof(STConfig));

    fd = open(config_path, O_RDONLY);
    
    if ( fd == -1 ) 
    {
        fprintf(stderr, "Can not open config file path\n");
        return 1;
    }


    // Parse DMA latency

    if (read_line(fd, read_buf, 256))
    {
        close(fd);
        return 1;
    }
    if (sscanf(read_buf, "%i", &config->dma_latency_us) != 1)
    {
        close(fd);
        return 1;
    }

    // Parse target C-states

    for (long i = 0; i < package.available_idle_states; ++i)
    {
        if (read_line(fd, read_buf, 256))
        {
            close(fd);
            return 1;
        }

        int offset = 0;
        int val, n;
        while (sscanf(&read_buf[offset], "%d%n", &val, &n) == 1) {
            offset += n;
            if (val < package.all_cpus)
            {
                config->core_target_c_state[val] = i;
            }
            else
            {
                close(fd);
                return 1;
            }

            if (read_buf[offset] == ',')
                ++offset;
            if (read_buf[offset] == '#')
                break;
        }
    }



    close(fd);
    return 0;
}

// apply functions
typedef struct
{
    int desired_cpu_latency_us;
    pthread_mutex_t stop_latency_constraint;
}  latencyThread; 

void* set_dma_latency_thread(void* arg) {
    latencyThread *threadVals =  (latencyThread *) arg;
    int desired_cpu_latency_us = threadVals->desired_cpu_latency_us;
    pthread_mutex_t *stop_thread = &threadVals->stop_latency_constraint;
    int fd;


    if (desired_cpu_latency_us < 0)
    {
        return NULL;
    }

    fd = open(PACKAGE_SUBSYSTEM_QOS_CPU_LATENCY_ADDR, O_WRONLY);
    if (fd == -1) 
    {
        fprintf(stderr, "dma_thread: DMA latency unavailable\n");
        return NULL;
    }

    printf("dma_thread: Setting the latency to %d\n", desired_cpu_latency_us);
    if (write(fd, &desired_cpu_latency_us, sizeof(desired_cpu_latency_us)) == -1)
    {
        fprintf(stderr, "dma_thread: Unable to write DMA latency\n");
        close(fd);
        return NULL;
    }

    // Wait for close signal or pthread_join
    sleep(2);
    pthread_mutex_lock(stop_thread);
    pthread_mutex_unlock(stop_thread);

    close(fd);
    printf("dma_thread: constraint released, fd closed\n");
    return NULL;
}

int st_idle_freq_apply(STConfig * config)
{

    // start dma latency constraint
    latencyThread latencyDMAThreadVals = {config->dma_latency_us, PTHREAD_MUTEX_INITIALIZER};
    pthread_mutex_lock(&latencyDMAThreadVals.stop_latency_constraint); // thread stops when lock is unlocked
    pthread_t dma_latency_thread;
    pthread_create(&dma_latency_thread, NULL, set_dma_latency_thread, &latencyDMAThreadVals); 


    // Loop until stop
    int wait = getchar();
    wait = 0;
    while( wait != '\n')
    {
        printf("Press Enter to exit...\n");
        wait = getchar();
    }


    // stop latency constraint
    pthread_mutex_unlock(&latencyDMAThreadVals.stop_latency_constraint);
    pthread_join(dma_latency_thread, NULL);

    return 0;

}