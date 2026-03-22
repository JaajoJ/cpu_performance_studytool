# Notes


## OCP 

https://www.redhat.com/en/blog/loading-kernel-modules-to-openshift-nodes-using-coreos-layering

## CPUIDLE 

// BACKGROUND //

// https://docs.kernel.org/driver-api/pm/cpuidle.html
// https://docs.kernel.org/admin-guide/pm/cpuidle.html

struct cpuidle_driver -> https://github.com/torvalds/linux/blob/master/include/linux/cpuidle.h#L93

struct cpuidle_driver {
	const char		*name;
	struct module 		*owner;
    unsigned int            bctimer:1;
	struct cpuidle_state	states[CPUIDLE_STATE_MAX];
	int			state_count;
	int			safe_state_index;
    struct cpumask		*cpumask;
	const char		*governor;
};


struct_cpuidle_device -> https://github.com/torvalds/linux/blob/master/include/linux/cpuidle.h#L93

struct cpuidle_device {
	unsigned int		registered:1;
	unsigned int		enabled:1;
	unsigned int		poll_time_limit:1;
	unsigned int		cpu;
	ktime_t			next_hrtimer;

	int			last_state_idx;
	u64			last_residency_ns;
	u64			poll_limit_ns;
	u64			forced_idle_latency_limit_ns;
	struct cpuidle_state_usage	states_usage[CPUIDLE_STATE_MAX];
	struct cpuidle_state_kobj *kobjs[CPUIDLE_STATE_MAX];
	struct cpuidle_driver_kobj *kobj_driver;
	struct cpuidle_device_kobj *kobj_dev;
	struct list_head 	device_list;

#ifdef CONFIG_ARCH_NEEDS_CPU_IDLE_COUPLED
	cpumask_t		coupled_cpus;
	struct cpuidle_coupled	*coupled;
#endif
};


struct cpuidle_state {
	char		name[CPUIDLE_NAME_LEN];
	char		desc[CPUIDLE_DESC_LEN];

	s64		exit_latency_ns;
	s64		target_residency_ns;
	unsigned int	flags;
	unsigned int	exit_latency; 
	int		power_usage; 
	unsigned int	target_residency; 

	int (*enter)	(struct cpuidle_device *dev,
			struct cpuidle_driver *drv,
			int index);

	void (*enter_dead) (struct cpuidle_device *dev, int index);
	int (*enter_s2idle)(struct cpuidle_device *dev,
			    struct cpuidle_driver *drv,
			    int index);
};
