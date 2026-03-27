#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cpu.h>
#include <linux/cpuidle.h>
#include "conf.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Reads a 32-bit integer from userspace and prints it");

static ssize_t value_store(struct device *dev,
                            struct device_attribute *attr,
                            const char *buf, size_t count)
{
    int value;
    int ret = kstrtoint(buf, 10, &value);
    if (ret)
        return ret;

    printk(KERN_INFO "hello: received %d\n", value);
    return count;
}

static struct device_attribute c_state_attr = {
    .attr  = { .name = "set_idle_state", .mode = 0222 },
    .store = value_store,
};

static struct device * st_core;

static struct class *st_cpu_class;

static int __init hello_init(void)
{

    // Get initial values of the running system
    int cpu_count = num_present_cpus();
    struct cpuidle_driver *drv = cpuidle_get_driver();
    int c_state_count = drv->state_count;
    
    printk("CPU COUNT %d\n", cpu_count);
    printk("C_state COUNT %d\n", c_state_count);

    // Create device files
    //  Package
    st_cpu_class = class_create("misc_device"); // Class for /sys/class/misc_device/
    //      Core
    st_core = device_create(st_cpu_class, NULL, 0, NULL, "core");
    //          C-state
    device_create_file(st_core, &c_state_attr);
    printk(KERN_INFO "hello: module loaded, device at /sys/class/misc_device/\n");
    return 0;
}

static void __exit hello_exit(void)
{
    device_remove_file(st_core, &c_state_attr);
    device_destroy(st_cpu_class, 0);
    class_destroy(st_cpu_class);
    printk(KERN_INFO "hello: module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);