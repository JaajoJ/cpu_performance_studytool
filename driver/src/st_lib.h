#ifndef ST_LIB_H
#define ST_LIB_H
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>

struct stDev {
    struct device * dev;
    char core_name[8];
    int state;
    int enforce; // Value between 0-100 which states how much the state will be enforced. 100 -> 100% only configured state is possible.
    int core;
};

int st_setup(struct class ** st_cpu_class, struct stDev ** st_dev);

int st_destroy(struct class ** st_cpu_class, struct stDev ** st_dev);

#endif