#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/cpu.h>
#include <linux/cpuidle.h>
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

static struct miscdevice hello_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "hello",
    .fops  = &hello_fops,
    .mode  = 0222,
};

static int __init hello_init(void)
{
    int cpu_count = num_present_cpus();
    struct cpuidle_driver *drv = cpuidle_get_driver();
    int c_state_count = drv->state_count;

    printk("CPU COUNT %d\n", cpu_count);
    printk("C_state COUNT %d\n", c_state_count);

    // Register files
    int ret = misc_register(&hello_dev);
    if (ret) {
        printk(KERN_ERR "hello: failed to register device: %d\n", ret);
        return ret;
    }
    printk(KERN_INFO "hello: module loaded, device at /dev/hello\n");
    return 0;
}

static void __exit hello_exit(void)
{
    misc_deregister(&hello_dev);
    printk(KERN_INFO "hello: module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);