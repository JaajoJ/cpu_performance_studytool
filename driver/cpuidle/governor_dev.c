// https://docs.kernel.org/driver-api/pm/cpuidle.html
// https://docs.kernel.org/admin-guide/pm/cpuidle.html
// https://docs.kernel.org/trace/kprobes.html
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpuidle.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>


static unsigned long reg_gov_addr = 0;
module_param(reg_gov_addr, ulong, 0);
MODULE_PARM_DESC(reg_gov_addr, "Address of cpuidle_register_governor");

typedef int (*reg_gov_fn)(struct cpuidle_governor *);

/*
The role of this callback is to prepare the governor for handling the (logical) CPU represented by the struct cpuidle_device object pointed to by the dev argument. 
The struct cpuidle_driver object pointed to by the drv argument represents the CPUIdle driver to be used with that CPU (among other things, 
it should contain the list of struct cpuidle_state objects representing idle states that the processor holding the given CPU can be asked to enter).

It may fail, in which case it is expected to return a negative error code, and that causes the kernel to run the architecture-specific default code for idle CPUs 
on the CPU in question instead of CPUIdle until the ->enable() governor callback is invoked for that CPU again.
*/
static int st_enable_device(struct cpuidle_driver *drv,
				  struct cpuidle_device *dev)
{
	dev->poll_limit_ns = 0;

	return 0;
}

/*
Called to select an idle state for the processor holding the (logical) CPU represented by the struct cpuidle_device object pointed to by the dev argument.

The list of idle states to take into consideration is represented by the states array of struct cpuidle_state objects held by the struct cpuidle_driver 
object pointed to by the drv argument (which represents the CPUIdle driver to be used with the CPU at hand). The value returned by this callback is 
interpreted as an index into that array (unless it is a negative error code).

The stop_tick argument is used to indicate whether or not to stop the scheduler tick before asking the processor to enter the selected idle state. 
When the bool variable pointed to by it (which is set to true before invoking this callback) is cleared to false, 
the processor will be asked to enter the selected idle state without stopping the scheduler tick on the given CPU (if the tick has been stopped on that CPU already, 
however, it will not be restarted before asking the processor to enter the idle state).

This callback is mandatory (i.e. the select callback pointer in struct cpuidle_governor must not be NULL for the registration of the governor to succeed).
*/
static int st_select(struct cpuidle_driver *drv,
			   struct cpuidle_device *dev,
			   bool *stop_tick)
{

	return 0;
}

/*
Called to allow the governor to evaluate the accuracy of the idle state selection made by the ->select() 
callback (when it was invoked last time) and possibly use the result of that to improve the accuracy of idle state selections in the future.
*/
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
    return real_register(&st_gov);
}

module_init(init_st);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jaakko-Juhani Lunden");
MODULE_DESCRIPTION("Study module for idle governor ");
