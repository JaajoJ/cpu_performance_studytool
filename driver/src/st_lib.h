#ifndef ST_LIB_H
#define ST_LIB_H
#include <linux/module.h>
#include <linux/kernel.h>

struct stDev {
    struct device * dev;
    char core_name[8];
    int state;
    int core;
};

int st_setup(void);

int st_destroy(void);

#endif