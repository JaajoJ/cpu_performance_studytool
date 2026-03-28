#include <linux/module.h>
#include <linux/kernel.h>
#include "../src/st_lib.h"

static struct stDev * st_dev; // Device for /sys/class/st_cpu/core<X>

static struct class *st_cpu_class;

static int __init hello_init(void)
{
    st_setup(st_cpu_class, st_dev);
    return 0;
}

static void __exit hello_exit(void)
{
    st_destroy(st_cpu_class, st_dev);
}

module_init(hello_init);
module_exit(hello_exit);