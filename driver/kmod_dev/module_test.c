#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cpu.h>
#include <linux/cpuidle.h>
#include "st_lib.h"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Reads a 32-bit integer from userspace and prints it");


static struct stDev * st_dev; // Device for /sys/class/st_cpu/core<X>

static struct class *st_cpu_class; // Class for /sys/class/st_cpu/

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

static int cpu_count;
static int c_state_count;

static struct device_attribute c_state_attr = {     // Device file for /sys/class/st_cpu/core<X>/set_idle_state
    .attr  = { .name = "set_idle_state", .mode = 0222 },
    .store = value_store,
};


static int __init hello_init(void)
{

    // Get initial values of the running system
    cpu_count = num_present_cpus();
    struct cpuidle_driver *drv = cpuidle_get_driver();
    c_state_count = drv->state_count;
    
    printk("CPU COUNT %d\n", cpu_count);
    printk("C_state COUNT %d\n", c_state_count);

    st_dev = kzalloc(cpu_count * sizeof(struct stDev), GFP_KERNEL); 

    // Create device files
    st_cpu_class = class_create("st_cpu"); // Class for /sys/class/misc_device/

    for (int i = 0; i < cpu_count; ++i)
    {
        //      Core
        set_core_name(st_dev[i].core_name, i);
        st_dev[i].dev = device_create(
            st_cpu_class, 
            NULL, 
            0, 
            NULL, 
            st_dev[i].core_name
        );

        //      C-state
        device_create_file(st_dev[i].dev, &c_state_attr);
    }
    printk(KERN_INFO "hello: /sys/class/st_cpu/core<X>/set_idle_state\n");
    return 0;
}

static void __exit hello_exit(void)
{

    for (int i = 0; i < cpu_count; ++i)
    {
        device_remove_file(st_dev[i].dev, &c_state_attr);
        device_destroy(st_cpu_class, st_dev[i].dev->devt);
    }
    class_destroy(st_cpu_class);
    kfree(st_dev);
    printk(KERN_INFO "hello: module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);