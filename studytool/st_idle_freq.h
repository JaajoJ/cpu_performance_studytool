#ifndef ST_IDLE_H
#define ST_IDLE_H

#define MAXIMUM_C_STATES 12
#define MAXIMUM_CORES 256
#define MAXIMUM_GOVERNOR_NAME 32

// CONFIGS
#define ST_CONFIG_DEFAULT_PATH "/opt/st_config.cfg"
#define ST_CONFIG_DEFAULTS {200,{0},st_get_package()}

// Addresses for stats
#define CORE_STATE_TIME_ADDR "/sys/devices/system/cpu/cpu%i/cpuidle/state%i/time"
#define CORE_LATENCY_ADDR "/sys/devices/system/cpu/cpu%i/power/pm_qos_resume_latency_us"

#define PACKAGE_CURRENT_GOVERNOR_ADDR "/sys/devices/system/cpu/cpuidle/current_governor_ro"
#define PACKAGE_ONLINE_CPU_ADDR "/sys/devices/system/cpu/online"

#define PACKAGE_SUBSYSTEM_QOS_CPU_LATENCY_ADDR "/dev/cpu_dma_latency"

// st_module bindings
#define ST_MODULE_C_STATE_ADDR "/sys/class/st_cpu/core%i/set_idle_state"

typedef struct  
{
    int idle_time[MAXIMUM_C_STATES]; // [0] == POLL: /sys/devices/system/cpu/cpuX/cpuidle/state*/time
    long max_latency; // Maximum accepted latency: /sys/devices/system/cpu/cpuX/power/pm_qos_resume_latency_us
} IdleCoreStats;

typedef struct  {
    char current_governor[MAXIMUM_GOVERNOR_NAME]; // /sys/devices/system/cpu/cpuidle/current_governor_ro
    long online_cpus;
    long all_cpus;
    int available_idle_states;
    IdleCoreStats coreStats[MAXIMUM_CORES];
} PackageStats;

typedef struct 
{
    int dma_latency_us;
    int core_target_c_state[MAXIMUM_CORES];
    PackageStats package;
} STConfig;


int st_get_config(STConfig * config, char * config_path);

int st_set_default_config(char * config_path);

void st_modify(char *path);

PackageStats st_get_package();

int st_get_core_idle_delta(PackageStats * package_stats);

int st_collect(const int core, const char output_mode, const bool modify);

int st_apply(STConfig * config);

int st_set_c_state(int core, int state);

#endif