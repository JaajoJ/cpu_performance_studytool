#ifndef ST_LIB_H
#define ST_LIB_H
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>

struct stDev {
    struct device * dev;
    char core_name[8];
    int state;
    int core;
};

int st_setup(struct class * st_cpu_class, struct stDev * st_dev);

int st_destroy(struct class * st_cpu_class, struct stDev * st_dev);

#endif