#ifndef ST_IDLE_H
#define ST_IDLE_H

#define MAXIMUM_C_STATES 12
#define MAXIMUM_CORES 256
#define MAXIMUM_GOVERNOR_NAME 32

// CONFIGS
#define ST_CONFIG_DEFAULT_PATH "/opt/st_config.cfg"
#define ST_CONFIG_DEFAULTS {200,-1,{0},{0},{0},st_get_package()}
#define ST_FREQ_STEP 200000

// Addresses for stats
#define CORE_STATE_TIME_ADDR "/sys/devices/system/cpu/cpu%i/cpuidle/state%i/time"
#define CORE_STATE_ABOVE_ADDR "/sys/devices/system/cpu/cpu%i/cpuidle/state%i/above"
#define CORE_STATE_BELOW_ADDR "/sys/devices/system/cpu/cpu%i/cpuidle/state%i/below"
#define CORE_LATENCY_ADDR "/sys/devices/system/cpu/cpu%i/power/pm_qos_resume_latency_us"
#define CORE_FREQ_ADDR "/sys/devices/system/cpu/cpu%i/cpufreq/scaling_cur_freq"
#define CORE_SCALING_MIN_FREQUENCY "/sys/devices/system/cpu/cpu%i/cpufreq/scaling_min_freq"
#define CORE_SCALING_MAX_FREQUENCY "/sys/devices/system/cpu/cpu%i/cpufreq/scaling_max_freq"
#define CORE_MAX_FREQUENCY "/sys/devices/system/cpu/cpu%i/cpufreq/cpuinfo_max_freq"
#define CORE_MIN_FREQUENCY "/sys/devices/system/cpu/cpu%i/cpufreq/cpuinfo_min_freq"

#define PACKAGE_CURRENT_GOVERNOR_ADDR "/sys/devices/system/cpu/cpuidle/current_governor_ro"
#define PACKAGE_CURRENT_GOVERNOR_W_ADDR "/sys/devices/system/cpu/cpuidle/current_governor"
#define PACKAGE_AVAILABLE_GOVERNORS_ADDR "/sys/devices/system/cpu/cpuidle/available_governors"
#define PACKAGE_ONLINE_CPU_ADDR "/sys/devices/system/cpu/online"
#define PACKAGE_INTEL_PSTATE_DISABLE "/sys/devices/system/cpu/intel_pstate/status"
#define PACKAGE_MAX_FREQUENCY "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"
#define PACKAGE_MIN_FREQUENCY "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq"
#define PACKAGE_UNCORE_CURRENT_FREQUENCY "/sys/devices/system/cpu/intel_uncore_frequency/package_00_die_00/current_freq_khz"
#define PACKAGE_UNCORE_MIN_FREQUENCY "/sys/devices/system/cpu/intel_uncore_frequency/package_00_die_00/min_freq_khz"
#define PACKAGE_UNCORE_MAX_FREQUENCY "/sys/devices/system/cpu/intel_uncore_frequency/package_00_die_00/max_freq_khz"
#define PACKAGE_UNCORE_GET_MIN_FREQUENCY "/sys/devices/system/cpu/intel_uncore_frequency/package_00_die_00/initial_min_freq_khz"
#define PACKAGE_UNCORE_GET_MAX_FREQUENCY "/sys/devices/system/cpu/intel_uncore_frequency/package_00_die_00/initial_max_freq_khz"

#define PACKAGE_SUBSYSTEM_QOS_CPU_LATENCY_ADDR "/dev/cpu_dma_latency"

// st_module bindings
#define ST_MODULE_C_STATE_ADDR "/sys/class/st_cpu/core%i/set_idle_state"
#define ST_MODULE_C_STATE_ENFORCE_ADDR "/sys/class/st_cpu/core%i/set_idle_state_enforce"
#define ST_MODULE_GOVERNOR_NAME "st_governor"

typedef struct  
{
    int idle_time[MAXIMUM_C_STATES]; // [0] == POLL: /sys/devices/system/cpu/cpuX/cpuidle/state*/time
    int below[MAXIMUM_C_STATES]; 
    int above[MAXIMUM_C_STATES]; 
    int current_frequency;
    long max_latency; // Maximum accepted latency: /sys/devices/system/cpu/cpuX/power/pm_qos_resume_latency_us
} IdleCoreStats;

typedef struct  {
    char current_governor[MAXIMUM_GOVERNOR_NAME]; // /sys/devices/system/cpu/cpuidle/current_governor_ro
    char available_governors[MAXIMUM_GOVERNOR_NAME * 8]; // MULTISTRING FORMAT: "governor1\0governor2\0" /sys/devices/system/cpu/cpuidle/available_governors
    int available_governor_count;
    long online_cpus;
    long all_cpus;
    int available_idle_states;
    long max_frequency;
    long min_frequency;
    long max_uncore_frequency;
    long min_uncore_frequency;
    long current_uncore_frequency;
    IdleCoreStats coreStats[MAXIMUM_CORES];
} PackageStats;

typedef struct 
{
    int dma_latency_us;
    int uncore_frequency;
    int core_target_c_state[MAXIMUM_CORES];
    int core_target_c_state_enforce[MAXIMUM_C_STATES];
    int core_target_frequency[MAXIMUM_CORES];
    PackageStats package;
} STConfig;


int st_get_config(STConfig * config, char * config_path);

int st_set_default_config(char * config_path);

void st_modify(char *path);

PackageStats st_get_package();

int st_get_core_frequency(PackageStats * package_stats);

int st_get_core_idle_delta(PackageStats * package_stats);

int st_collect(const int core, const char output_mode);

int st_apply(STConfig * config);

int st_set_c_state(int core, int state, const char * address_format);

#endif
