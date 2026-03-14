#ifndef ST_IDLE_H
#define ST_IDLE_H

#define MAXIMUM_C_STATES 16
#define MAXIMUM_CORES 256

typedef struct  {
    char * current_governor; // /sys/devices/system/cpu/cpuidle/current_governor_ro
    char ** available_governors; // /sys/devices/system/cpu/cpuidle/available_governors
    long int online_cpu[4]; // [0] & 0x1 is cpu 0 and the bit is indicating online or offline of that cpu: /sys/devices/system/cpu/online
    IdleCoreStats coreStats[MAXIMUM_CORES];
} PackageStats;

typedef struct  
{
    long int idle_time[MAXIMUM_C_STATES]; // [0] == POLL: /sys/devices/system/cpu/cpuX/cpuidle/state*/time
    int max_latency; // Maximum accepted latency: /sys/devices/system/cpu/cpuX/power/pm_qos_resume_latency_us
} IdleCoreStats;


int st_idle(const int core, const char output_mode, const bool modify);


#endif