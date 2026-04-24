// https://docs.kernel.org/driver-api/pm/cpuidle.html
// https://docs.kernel.org/admin-guide/pm/cpuidle.html
// https://docs.kernel.org/trace/kprobes.html

// https://github.com/torvalds/linux/blob/master/include/linux/cpuidle.h

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpuidle.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/random.h>
#include <linux/kprobes.h>
#include "../src/st_lib.h"


static unsigned long reg_gov_addr = 0;
module_param(reg_gov_addr, ulong, 0);
MODULE_PARM_DESC(reg_gov_addr, "Address of cpuidle_register_governor");

typedef int (*reg_gov_fn)(struct cpuidle_governor *);

static struct stDev * st_dev; // Device for /sys/class/st_cpu/core<X>

static struct class *st_cpu_class;

static int st_enable_device(struct cpuidle_driver *drv,
				  struct cpuidle_device *dev)
{
	dev->poll_limit_ns = 0;

	return 0;
}

static int st_select(struct cpuidle_driver *drv,
			   struct cpuidle_device *dev,
			   bool *stop_tick)
{
	int ret = st_dev[dev->cpu].state;
	int enforce = st_dev[dev->cpu].enforce;
	u32 val = get_random_u32_below(101);

	*stop_tick = 1;

	if (val < enforce) {
		*stop_tick = true;
		return ret;
	}
	*stop_tick = false;
	return 0;

	return ret;
}

static void st_reflect(struct cpuidle_device *dev, int index)
{
	dev->last_state_idx = index;
}

static struct cpuidle_governor st_gov = {
	.name =			"st_governor",
	.rating =		9,
	.enable =		st_enable_device,
	.select =		st_select,
	.reflect =		st_reflect,
};

static int __init init_st(void)
{
    reg_gov_fn real_register;

    if (!reg_gov_addr) {
        pr_err("Must provide reg_gov_addr parameter\n");
        return -EINVAL;
    }

    real_register = (reg_gov_fn) reg_gov_addr;
	st_setup(&st_cpu_class, &st_dev);
    return real_register(&st_gov);
}

module_init(init_st);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Study module for idle governor ");
