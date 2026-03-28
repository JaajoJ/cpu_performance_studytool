#ifndef ST_LIB_H
#define ST_LIB_H


#include <linux/device.h>   // struct device
#include <linux/types.h>    // dev_t

struct stDev {
    struct device * dev;
    char core_name[8];
    int state;
};

void set_core_name(char * buf, int value);


#endif