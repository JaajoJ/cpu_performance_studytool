#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cpu.h>
#include <linux/cpuidle.h>
#include "module_test.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Module for developing the study tool");

struct stDev {
    struct device * dev;
    char core_name[8];
    int state;
    int core;
};


static struct stDev * st_dev; // Device for /sys/class/st_cpu/core<X>

static struct class *st_cpu_class; // Class for /sys/class/st_cpu/

static int cpu_count;
static int c_state_count;

static ssize_t value_store(struct device *dev,
                            struct device_attribute *attr,
                            const char *buf, size_t count)
{
    int value;
    int ret = kstrtoint(buf, 10, &value);
    struct stDev *core = dev_get_drvdata(dev);

    if (ret)
        return ret;

    printk(KERN_INFO "hello: received %d\n", value);

    core->state = value % c_state_count;
    printk(KERN_INFO "hello: set new value to core %d value %d\n", core->core, core->state);
    
    return count;
}


static struct device_attribute c_state_attr = {     // Device file for /sys/class/st_cpu/core<X>/set_idle_state
    .attr  = { .name = "set_idle_state", .mode = 0222 },
    .store = value_store,
};

static void set_core_name(char * buf, int value)
{
    buf[0] = 'c';
    buf[1] = 'o';
    buf[2] = 'r';
    buf[3] = 'e';
    int offset = 4;
    if(value > 99)
    {
        buf[offset] = value / 100 + '0';
        value = value % 100;
        ++offset;
    }
    if(value > 9)
    {
        buf[offset] = value / 10 + '0';
        value = value % 10;
        ++offset;
    }

    buf[offset] = value + '0';
    ++offset;

    buf[offset] = '\0';

}
int st_setup(void)
{
        // Get initial values of the running system
    cpu_count = num_present_cpus();
    struct cpuidle_driver *drv = cpuidle_get_driver();
    if (!drv) {
        pr_err("hello: no cpuidle driver found\n");
        return -ENODEV;
    }
    
    c_state_count = drv->state_count;
    
    printk("CPU COUNT %d\n", cpu_count);
    printk("C_state COUNT %d\n", c_state_count);

    st_dev = kzalloc(cpu_count * sizeof(struct stDev), GFP_KERNEL); 
    if (!st_dev) 
        pr_err("hello: Failed kzalloc\n");

    // Create device files
    st_cpu_class = class_create("st_cpu"); // Class for /sys/class/misc_device/
    if (IS_ERR(st_cpu_class)) {
        kfree(st_dev);
        return PTR_ERR(st_cpu_class);
    }
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
        st_dev[i].core = i;

        dev_set_drvdata(st_dev[i].dev, &st_dev[i]);

        //      C-state
        device_create_file(st_dev[i].dev, &c_state_attr);
    }
    printk(KERN_INFO "hello: /sys/class/st_cpu/core<X>/set_idle_state\n");
    return 0;
}

int st_destroy(void)
{
    for (int i = 0; i < cpu_count; ++i)
    {
        device_remove_file(st_dev[i].dev, &c_state_attr);
        device_destroy(st_cpu_class, st_dev[i].dev->devt);
    }
    class_destroy(st_cpu_class);
    kfree(st_dev);
    printk(KERN_INFO "hello: module unloaded\n");
    return 0;
}
