#ifndef __CAPRI_PM_H__
#define __CAPRI_PM_H__

#include <linux/suspend.h>

#define KONA_MACH_MAX_IDLE_STATE 1
#define DEEP_SLEEP_LATENCY	300	/*latency due to xtal warm up delay */

struct as_one_cpu {
	char *name;
	u32 flags;
	u32 usg_cnt;
	struct as_one_cpu_ops *ops;
	spinlock_t lock;
};

struct as_one_cpu_ops {
	int (*init) (struct as_one_cpu * sys);
	int (*enable) (struct as_one_cpu * sys, int enable);
};

int put_CPSubsystem_to_sleep(void *data, u64 clk_idle);
void enter_wfi(void);
int capri_force_sleep(suspend_state_t state);
void capri_reduce_emi_idle_time_before_pwrdn(bool enable);
extern void request_suspend_state(suspend_state_t state);
int __idle_allow_enter(struct as_one_cpu *sys);
int __idle_allow_exit(struct as_one_cpu *sys);
int idle_enter_exit(struct as_one_cpu *sys, int enable);
#if defined(CONFIG_MACH_CAPRI_SS_CRATER)
extern void uas_jig_force_sleep(void);
#endif

#ifdef CONFIG_CAPRI_DELAYED_PM_INIT
int capri_pm_init(void);
#else
int __init capri_pm_init(void);
#endif

#endif /*__CAPRI_PM_H__*/
