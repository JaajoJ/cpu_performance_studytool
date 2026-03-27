#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cpu.h>
#include <linux/cpuidle.h>
#include <linux/cdev.h> 
#include "conf.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Reads a 32-bit integer from userspace and prints it");

static ssize_t hello_write(struct file *file, const char __user *buf,
                           size_t count, loff_t *ppos)
{
    s32 value;

    if (count < sizeof(s32))
        return -EINVAL;

    if (copy_from_user(&value, buf, sizeof(s32)))
        return -EFAULT;

    printk(KERN_INFO "hello: received s32 = %d (0x%08x)\n", value, value);

    return sizeof(s32);
}

static const struct file_operations hello_fops = {
    .owner = THIS_MODULE,
    .write = hello_write,
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

    printk(KERN_INFO "hello: module loaded, device at /sys/class/misc_device/\n");
    return 0;
}

static void __exit hello_exit(void)
{
    device_destroy(st_cpu_class, 0);
    class_destroy(st_cpu_class);
    printk(KERN_INFO "hello: module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);