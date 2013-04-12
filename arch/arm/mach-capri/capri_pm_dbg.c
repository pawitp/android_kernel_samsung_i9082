/******************************************************************************/
/* (c) 2011 Broadcom Corporation                                              */
/*                                                                            */
/* Unless you and Broadcom execute a separate written software license        */
/* agreement governing use of this software, this software is licensed to you */
/* under the terms of the GNU General Public License version 2, available at  */
/* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").                    */
/*                                                                            */
/******************************************************************************/

#include <linux/module.h>
#include <plat/kona_pm_dbg.h>
#include <mach/kona_timer.h>
#include <plat/pwr_mgr.h>
#include <plat/pi_mgr.h>
#include <mach/io_map.h>
#include <mach/rdb/brcm_rdb_csr.h>
#include <mach/rdb/brcm_rdb_hsotg_ctrl.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include <mach/pwr_mgr.h>
#include <mach/pi_mgr.h>
#include <linux/suspend.h>
#include <mach/pm.h>
#ifdef CONFIG_KONA_PROFILER
#include <plat/profiler.h>
#endif

#include <linux/gpio.h>


/*****************************************************************************
 *                        SLEEP STATE DEBUG INTERFACE                        *
 *****************************************************************************/

struct debug {
	int dummy;
};

#define __param_check_debug(name, p, type) \
	static inline struct type *__check_##name(void) { return (p); }

#define param_check_debug(name, p) \
	__param_check_debug(name, p, debug)

static int param_set_debug(const char *val, const struct kernel_param *kp);
static int param_get_debug(char *buffer, const struct kernel_param *kp);

static struct kernel_param_ops param_ops_debug = {
	.set = param_set_debug,
	.get = param_get_debug,
};

int wakeup_timer = 5000;
module_param_named(wakeup_timer, wakeup_timer, int,
		   S_IRUGO | S_IWUSR | S_IWGRP);

static void pm_timer_task(void);

static int pm_test_timer_restart(void *dev)
{
	pr_info("%s,wakeup_timer=%d\n", __func__, wakeup_timer);
	if (wakeup_timer) {
		/*
		   restart only when wakeup_timer large than zero
		 */
		kona_timer_set_match_start((struct kona_timer *)dev,
					   wakeup_timer * 32);
	}
	return 0;
}

void pm_timer_task(void)
{
	struct timer_ch_cfg config;
	struct kona_timer *timer_ch1;
	static int t_start;

	pr_info("%s,wakeup_timer=%d\n", __func__, wakeup_timer);

	if (wakeup_timer > 0) {

		if (t_start == 0) {

			t_start++;
			timer_ch1 = kona_timer_request("aon-timer", 1);
			config.mode = MODE_PERIODIC;
			config.arg = timer_ch1;
			config.cb = pm_test_timer_restart;
			config.reload = wakeup_timer * 32;
			kona_timer_config(timer_ch1, &config);
			kona_timer_set_match_start(timer_ch1,
						   wakeup_timer * 32);
		}
	}
}

static struct debug debug;
module_param_named(debug, debug, debug, S_IRUGO | S_IWUSR | S_IWGRP);

/* List of supported commands */
enum {
	CMD_SHOW_HELP = 'h',
};

static void cmd_show_usage(void)
{
	const char usage[] = "Usage:\n"
	    "echo 'cmd string' > /sys/module/capri_pm/parameters/debug\n"
	    "'cmd string' is constructed as follows:\n" "\n";

	pr_info("%s", usage);
}

/* Force deep sleep functionality utilized by at*mlpm command */
#define GPIO_TOUCHKEY_LED_LDO_EN 130

extern void uas_jig_force_sleep(void);
static void cmd_force_sleep(void)
{
	kona_pm_reg_pm_enter_handler(&capri_force_sleep);
	request_suspend_state(PM_SUSPEND_MEM);

	/*turn Touchkey LED off */
	gpio_direction_output(GPIO_TOUCHKEY_LED_LDO_EN, 0);
	
	uas_jig_force_sleep();
}

static int param_set_debug(const char *val, const struct kernel_param *kp)
{
	const char *p;

	if (!val)
		return -EINVAL;

	p = &val[1];

	/* First character is the command. Skip past all whitespaces
	 * after the command to reach the arguments, if any.
	 */
	while (*p == ' ' || *p == '\t')
		p++;

	switch (val[0]) {
	case 't':
	case 'T':
		pm_timer_task();
		break;
	case 'f':
	case 'F':
		cmd_force_sleep();
		break;
	case CMD_SHOW_HELP:	/* Fall-through */
	default:
		cmd_show_usage();
		break;
	}

	return 0;
}

static int param_get_debug(char *buffer, const struct kernel_param *kp)
{
	cmd_show_usage();
	return 0;
}

/*****************************************************************************
 *               INTERFACE TO TAKE REGISTER SNAPSHOT BEFORE SLEEP            *
 *****************************************************************************/

static u32 mm_pi_id = PI_MGR_PI_ID_MM;
static u32 hub_pi_id = PI_MGR_PI_ID_HUB_SWITCHABLE;
static u32 hub_aon_pi_id = PI_MGR_PI_ID_HUB_AON;
static u32 arm_pi_id = PI_MGR_PI_ID_ARM_CORE;
static u32 arm_subsys_pi_id = PI_MGR_PI_ID_ARM_SUB_SYSTEM;
static u32 modem_pi_id = PI_MGR_PI_ID_MODEM;

static u32 get_pi_count(void *data)
{
	u32 ret = 0;
	int id = *(int *)data;

	ret = pi_get_use_count(id);

	return ret;
}

#define AP_MIN_PWR_STATE		\
	((u32)KONA_MEMC0_NS_VA + CSR_APPS_MIN_PWR_STATE_OFFSET)
#define MODEM_MIN_PWR_STATE		\
	((u32)KONA_MEMC0_NS_VA + CSR_MODEM_MIN_PWR_STATE_OFFSET)
#define DSP_MIN_PWR_STATE		\
	((u32)KONA_MEMC0_NS_VA + CSR_DSP_MIN_PWR_STATE_OFFSET)
#define USB_OTG_P1CTL		\
	((u32)KONA_USB_HSOTG_CTRL_VA + HSOTG_CTRL_PHY_P1CTL_OFFSET)
#define HW_FREQ_CHANGE_CNTRL		\
	((u32)KONA_MEMC0_NS_VA+CSR_HW_FREQ_CHANGE_CNTRL_OFFSET)
#define DDR_PLL_PWRDN_BIT CSR_HW_FREQ_CHANGE_CNTRL_DDR_PLL_PWRDN_ENABLE_MASK

/* SNAPSHOT TABLE:
 * ---------------
 * Table of registers to be sampled before entering low power
 * state for debugging.
 */
static struct snapshot snapshot[] = {

	/*
	 * Simple register parms
	 */
	SIMPLE_PARM(AP_MIN_PWR_STATE, 0, 3),
	SIMPLE_PARM(MODEM_MIN_PWR_STATE, 0, 3),
	SIMPLE_PARM(DSP_MIN_PWR_STATE, 0, 3),
	SIMPLE_PARM(HW_FREQ_CHANGE_CNTRL, DDR_PLL_PWRDN_BIT, DDR_PLL_PWRDN_BIT),

	/*
	 * List of clocks that prevent entry to low power state
	 */
	CLK_PARM("dig_ch0_clk"),
	CLK_PARM("tpiu_clk"),
	CLK_PARM("pti_clk"),

	/*
	 * AHB register parms (needs AHB clk enabled before register read)
	 */
	AHB_REG_PARM(USB_OTG_P1CTL, 0, (1 << 30), "usb_otg_clk"),

	/*
	 * PI usage counts
	 */
	USER_DEFINED_PARM(get_pi_count, &mm_pi_id, "mm"),
	USER_DEFINED_PARM(get_pi_count, &hub_pi_id, "hub"),
	USER_DEFINED_PARM(get_pi_count, &hub_aon_pi_id, "hub_aon"),
	USER_DEFINED_PARM(get_pi_count, &arm_pi_id, "arm"),
	USER_DEFINED_PARM(get_pi_count, &arm_subsys_pi_id, "arm_subsys"),
	USER_DEFINED_PARM(get_pi_count, &modem_pi_id, "modem"),
};

/*****************************************************************************
 *                        INSTRUMENT LOW POWER STATES                        *
 *****************************************************************************/

void instrument_idle_entry(void)
{
	/* Take snapshot of registers that can potentially prevent system from
	 * entering low power state.
	 */
	snapshot_get();

	/**
	 * Take profiling counter samples
	 * before entering idle state
	 */
#ifdef CONFIG_KONA_PROFILER
	profiler_idle_entry_cb();
#endif
}

void instrument_idle_exit(void)
{
}

int __init capri_pmdbg_init(void)
{
	snapshot_table_register(snapshot, ARRAY_SIZE(snapshot));
	return 0;
}

module_init(capri_pmdbg_init);
