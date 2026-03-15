#ifndef ST_IDLE_H
#define ST_IDLE_H

#define MAXIMUM_C_STATES 12
#define MAXIMUM_CORES 256
#define MAXIMUM_GOVERNOR_NAME 32

// Addresses for stats
#define CORE_STATE_TIME_ADDR "/sys/devices/system/cpu/cpu%i/cpuidle/state%i/time"
#define CORE_LATENCY_ADDR "/sys/devices/system/cpu/cpu%i/power/pm_qos_resume_latency_us"

#define PACKAGE_CURRENT_GOVERNOR_ADDR "/sys/devices/system/cpu/cpuidle/current_governor_ro"
#define PACKAGE_ONLINE_CPU_ADDR "/sys/devices/system/cpu/online"

typedef struct  
{
    long idle_time[MAXIMUM_C_STATES]; // [0] == POLL: /sys/devices/system/cpu/cpuX/cpuidle/state*/time
    long max_latency; // Maximum accepted latency: /sys/devices/system/cpu/cpuX/power/pm_qos_resume_latency_us
} IdleCoreStats;

typedef struct  {
    char current_governor[MAXIMUM_GOVERNOR_NAME]; // /sys/devices/system/cpu/cpuidle/current_governor_ro
    long online_cpus;
    long all_cpus;
    int available_idle_states;
    IdleCoreStats coreStats[MAXIMUM_CORES];
} PackageStats;


PackageStats st_idle_freq_get_package();

IdleCoreStats st_idle_freq_get_core_idle(const int core_number, const int available_c_states);

int st_idle_freq(const int core, const char output_mode, const bool modify);


#endif