#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/syscall.h>
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

int st_collect(const int core, const char output_mode, const bool modify)
{
    PackageStats package = st_get_package();
    st_get_core_idle_delta(&package);

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



PackageStats st_get_package()
{
    PackageStats package = {0};
    char addr_buf[128] = "";
    
    // CPU information
    package.online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    package.all_cpus = sysconf(_SC_NPROCESSORS_CONF);
    package.available_idle_states = get_available_c_states();

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

int st_get_core_idle_delta(PackageStats * package_stats)
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

void st_modify(char *path)
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
    PackageStats package = st_get_package();

    fd = open(config_path, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) 
    {
        fprintf(stderr, "Can not open config file path\n");
        return 1;
    }

    sprintf(buf, "%d # DMA_LATENCY_US\n", config.dma_latency_us);

    // write dma
    if (write(fd, buf, strlen(buf)) == -1)
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
                sprintf(buf2, "%d", i2);
                strcat(buf, buf2);
                if (i2 + 1 != package.all_cpus)
                {
                    strcat(buf, ",");
                }
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
    char c[2] = {0};
    
    int length = 1;
    while(true)
    {
        if (length == max_length)
        {
            return 1;
        }
        read(fd, c, 1);
        if(c[0] == '\n' || c[0] == EOF)
        {
            return 0;
        }
        strcat(buf, c);
        buf[length + 1] = '\0';
        ++ length;

    }
    return 0;
}

int st_get_config(STConfig * config, char * config_path)
{
    int fd;
    char read_buf[256] = {0};
    PackageStats package = st_get_package();
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
        fprintf(stderr, "Line length too big for DMA latency.\n");
        close(fd);
        return 1;
    }
    if (sscanf(read_buf, "%i", &config->dma_latency_us) != 1)
    {
        fprintf(stderr, "Invalid format for DMA latency.\n");
        close(fd);
        return 1;
    }

    // Parse target C-states

    for (long i = 0; i < package.available_idle_states; ++i)
    {
        memset(read_buf, 0, sizeof(read_buf));
        if (read_line(fd, read_buf, 256))
        {
            fprintf(stderr, "Line length too big for C-state %ld.\n", i);
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
                printf("Configured CPU %i C-state target %ld.\n", val, i);
            }
            else
            {
                fprintf(stderr, "Invalid CPU found: %i\n", val);
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
        fprintf(stderr, "dma_thread: Latency value %i us\n", desired_cpu_latency_us);
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

int st_set_c_state(int core, int state)
{
    int fd;
    char module_path[64] = {0};

    snprintf(module_path, sizeof(module_path), ST_MODULE_C_STATE_ADDR, core);
    fd = open(module_path, O_WRONLY);
    if (fd == -1) 
    {
        fprintf(stderr, "Unable to write core %i to state %i \n", core, state);
        return -1;
    }
    char int_string[32];
    snprintf(int_string, sizeof(int_string), "%d", 42);

    if (write(fd, int_string, strlen(int_string)) == -1)
    {
        fprintf(stderr, "Unable to write core %i to state %i \n", core, state);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

bool is_module_loaded(const char *module_name)
{
    FILE *f = fopen("/proc/modules", "r");
    if (!f) 
    {
        perror("fopen /proc/modules");
        return false;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) 
    {
        char name[64];
        sscanf(line, "%63s", name);
        if (strcmp(name, module_name) == 0) 
        {
            fclose(f);
            return true;
        }
    }
    fclose(f);
    return false;
}

int st_check_kmod()
{
    return is_module_loaded("st_module");
}

int st_apply(STConfig * config)
{
    int ret = 0;

    // initialize values
    latencyThread latencyDMAThreadVals = {config->dma_latency_us, PTHREAD_MUTEX_INITIALIZER};;
    pthread_t dma_latency_thread;

    // Check settings
    bool enable_latency_constraint = 0;
    bool enable_governor = 0;
    bool enable_c_sates = 0;
    
    if (config->dma_latency_us > -1)
        enable_latency_constraint = 1;
    if (st_check_kmod())
        enable_c_sates = 1;

    // DMA
    if ( enable_latency_constraint )
    { 
        pthread_mutex_lock(&latencyDMAThreadVals.stop_latency_constraint); // thread stops when lock is unlocked
        pthread_create(&dma_latency_thread, NULL, set_dma_latency_thread, &latencyDMAThreadVals); 
    }


    // SETUP GOVERNOR
    if ( enable_governor )
    {
        printf("Enabling governor\n");
        
    }

    // Setup C-States

    if ( enable_c_sates )
    {
        printf("Enabling c-states\n");
        
        for (int i = 0; i < config->package.all_cpus; ++i)
        {
            ret = st_set_c_state(i, config->core_target_c_state[i]);
            if ( ret )
            {
                fprintf(stderr, "Failed to set C-state for cpu %i state %i", i, config->core_target_c_state[i]);
            }
        }
    }


    // Loop until stop
    int wait ; //  = getchar();
    wait = 0;
    while( wait != '\n')
    {
        printf("Press Enter to exit...\n");
        wait = getchar();
    }


    // stop latency constraint
    if (enable_latency_constraint)
    {
        pthread_mutex_unlock(&latencyDMAThreadVals.stop_latency_constraint);
        pthread_join(dma_latency_thread, NULL);
    }
    return 0;

}