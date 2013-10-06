/****************************************************************************
*
* Copyright 2010 --2011 Broadcom Corporation.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
*****************************************************************************/

#include <linux/sched.h>
#include <linux/cpuidle.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <linux/smp.h>
#include <linux/cpumask.h>
#include <plat/kona_pm.h>
#include <plat/pwr_mgr.h>
#include <plat/pi_mgr.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <mach/io_map.h>
#include <plat/clock.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <mach/rdb/brcm_rdb_scu.h>
#include <mach/rdb/brcm_rdb_csr.h>
#include <mach/rdb/brcm_rdb_chipreg.h>
#include <mach/rdb/brcm_rdb_root_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kps_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kpm_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_khub_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_esub_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_hsotg_ctrl.h>
#include <mach/rdb/brcm_rdb_kproc_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_gicdist.h>
#include <mach/rdb/brcm_rdb_pwrmgr.h>
#include <mach/rdb/brcm_rdb_kona_gptimer.h>
#include <linux/workqueue.h>
#include <mach/pwr_mgr.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <mach/pm.h>
#include <mach/chipregHw_inline.h>
#include <mach/sec_api.h>
#ifdef CONFIG_BCM_MODEM
#include <linux/broadcom/bcm_rpc.h>
#endif
#include <linux/percpu.h>
#include <mach/capri_dormant.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif /*CONFIG_HAS_WAKELOCK */
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */

extern unsigned read_sctlr(void);
extern void write_sctlr(unsigned value);
extern unsigned read_actlr(void);
extern void write_actlr(unsigned value);
extern void disable_clean_inv_dcache_v7_l1(void);

static DEFINE_SPINLOCK(wake_up_event_lock);
static unsigned int chip_version;

static u32 force_retention;
static u32 pm_debug = 2;
static u32 pm_en_self_refresh;
static u32 enable_test;
static u32 uart_up_delay;
static u32 one_shot;

#define UNLOCK						0x00A5A501
#define SCU_OFF_MODE				0x03030303
#define SCU_DORMANT_MODE			0x03030202
#define SCU_DORMANT2_MODE_OFF		0x03030000
#define A9_SMP_BIT					(1<<6)
#define PWRCTL_BYPASS_L2OFF			0x000000C0
#define PWRCTL_BYPASS_DORMANT		0x00000080
#define PWRCTL_BYPASS_NORMAL		0x00000000
#define PWRCTL_USE_SCU				0x00000100

#define A9_SCU_NORM				0x00
#define A9_SCU_DORM				0x02
#define A9_SCU_OFF				0x03

#define DISABLE_DORMANT_MASK \
	PWRMGR_PI_DEFAULT_POWER_STATE_ARM_CORE_DORMANT_DISABLE_MASK

#if defined(DEBUG)
#define pm_dbg printk
#else
#define pm_dbg(format...)\
	do {\
		if (pm_debug && pm_debug != 2)\
			printk(format);\
	} while (0)
#endif

static int print_clock_count(void);
static int print_sw_event_info(void);
static int arm_pll_disable(int en);
static int arm_pll_8phase_disable(int en);
static int enter_suspend_state(struct kona_idle_state *state);
static int enter_idle_state(struct kona_idle_state *state);
static int enter_dormant_state(struct kona_idle_state *state);
static int enter_retention_state(struct kona_idle_state *state);
static int capri_devtemp_cntl(struct as_one_cpu *sys, int enable);
static Boolean redo_auto = FALSE;
int suspend_debug_count;

#define PM_WAKEUP_BY_COMMON_INT_TO_AC_EVENT 1
#define PM_WAKEUP_BY_COMMON_TIMER_1_EVENT  2
#define VREQ_NONZERO_PI_MODEM_EVENT_IS_ON  4

struct capri_pm_dbg {
	u32 hubTM_stcs;
	u32 hubTM_stclo;
	u32 hubTM_stchi;
	u32 hubTM_stcm1;
	u32 pm_set[7];
	u32 pm_event;
};

struct capri_pm_dbg pre_pm_dbg;
struct capri_pm_dbg post_pm_dbg;

enum {
	SUSPEND_WFI,
	SUSPEND_RETENTION,
	SUSPEND_DEEPSLEEP,
	SUSPEND_DORMANT,
};

enum {
	CAPRI_STATE_C0,
	CAPRI_STATE_C1,
	CAPRI_STATE_C2,
	CAPRI_STATE_C3,
};

const char *sleep_prevent_clocks[] = {
	/*aon */
	"pscs_clk",
	"sim2_clk",
	"sim_clk",
	/*hub */
	"mdiomaster",
	"brom",
	"dap_switch",
	"caph_srcmixer_clk",
	"tmon_1m_clk",
	"ssp6_clk",
	"ssp6_audio_clk",
	"ssp5_clk",
	"ssp5_audio_clk",
	"ssp4_clk",
	"ssp4_audio_clk",
	"ssp3_clk",
	"ssp3_audio_clk",
	"audioh_156m_clk",
	"audioh_2p4m_clk",
	"nor_async_clk",
	/*kps */
	"spum_sec",
	"spum_open",
	"timers",
	"ssp2_clk",
	"ssp0_clk",
	"pwm_clk",
	"bsc1_clk",
	"bsc2_clk",
	"bsc3_clk",
	"ssp0_audio_clk",
	"ssp2_audio_clk",
	/*kpm */
	"sdio1_clk",
	"sdio2_clk",
	"sdio3_clk",
	"sdio4",
	"usbh_12m",
	"usbh_48m",
	"usb_ic",
	"nand_clk",
	"usb_otg_clk",
};

#ifdef CONFIG_HAS_EARLYSUSPEND
void rpc_event_suspend(struct early_suspend *h);
void rpc_event_resume(struct early_suspend *h);

static struct early_suspend rpc_early_suspend_desc = {
	.suspend = rpc_event_suspend,
	.resume = rpc_event_resume,
};
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int force_idle_wfi_in_suspend;
int force_wfi_only_in_idle;
module_param_named(force_wfi_only_in_idle, force_wfi_only_in_idle,
		   int, S_IRUGO | S_IWUSR | S_IWGRP);

int suspend_mode = SUSPEND_DORMANT;

module_param_named(suspend_mode, suspend_mode,
		   int, S_IRUGO | S_IWUSR | S_IWGRP);

int enable_clr_intr = 1;
module_param_named(enable_clr_intr, enable_clr_intr,
		   int, S_IRUGO | S_IWUSR | S_IWGRP);

int debug_ds;
module_param_named(debug_ds, debug_ds, int, S_IRUGO | S_IWUSR | S_IWGRP);

int keep_xtl_on;
module_param_named(keep_xtl_on, keep_xtl_on, int, S_IRUGO | S_IWUSR | S_IWGRP);

static int dbg_dsm_suspend;
module_param_named(dbg_dsm_suspend, dbg_dsm_suspend,
		   int, S_IRUGO | S_IWUSR | S_IWGRP);

#if defined(CONFIG_CAPRI_SYSEMI_DDR3)
extern unsigned int ddr3_phy_mode;
module_param_named(ddr3_phy_mode, ddr3_phy_mode,
	int, S_IRUGO | S_IWUSR | S_IWGRP);
#endif

static struct clk *pbsc_clk;
static struct clk *kpm_clk, *kps_clk;
/*
 * C0 is simple WFI state.. clears all the active events on A2 but not on A2
 * C1 is suspend retention state.  Keeps XTAL on but enters retention
 * C2 is only defined if dormant is defined. Keeps XTAL on but enters Dormant
 * C3 is deep sleep. Enters dormant if defined and retention if not. XTAL off.
 */

static struct kona_idle_state capri_cpu_states[] = {
	{
	 .name = "C0",
	 .desc = "susp",
	 .flags = CPUIDLE_FLAG_TIME_VALID | CPUIDLE_FLAG_XTAL_ON,
	 .latency = 0,
	 .target_residency = 0,
	 .state = CAPRI_STATE_C0,
	 .enter = enter_idle_state,
	 },
	{
	 .name = "C1",
	 .desc = "susp-rtn",	/*suspend-retention (XTAL ON) */
	 .flags = CPUIDLE_FLAG_TIME_VALID | CPUIDLE_FLAG_XTAL_ON,
	 .latency = 400,
	 .target_residency = 400,
	 .state = CAPRI_STATE_C1,
	 .enter = enter_idle_state,
	 },
#ifdef CONFIG_CAPRI_DORMANT_MODE
	{
	 .name = "C2",
	 .desc = "susp-dormant",	/*suspend-retention (XTAL ON) */
	 .flags = CPUIDLE_FLAG_TIME_VALID | CPUIDLE_FLAG_XTAL_ON,
	 .latency = 600,
	 .target_residency = 600,
	 .state = CAPRI_STATE_C2,
	 .enter = enter_idle_state,
	 },
#endif
	{
	 .name = "C3",
#ifdef CONFIG_CAPRI_DORMANT_MODE
	 .desc = "ds-dormant",	/*deepsleep-dormant (XTAL OFF) */
#else
	 .desc = "ds-rtn",	/*deepsleep-retention (XTAL OFF) */
#endif
	 .flags = CPUIDLE_FLAG_TIME_VALID,
	 .latency = 1000,
	 .target_residency = 1000,
	 .state = CAPRI_STATE_C3,
	 .enter = enter_idle_state,
	 }
};

static struct as_one_cpu_ops gen_idle_ops = {
	.enable = capri_devtemp_cntl,
};

static struct as_one_cpu gen_one_cpu_ops = {
	.usg_cnt = 0,
	.ops = &gen_idle_ops,
};

/**************functions*********/

unsigned int get_chip_version(void)
{
	static int chip_version = -1;
	if (chip_version == -1)
		chip_version = (chipregHw_getChipId() & 0x0FF);
	return chip_version;
}

static inline bool vc_emi_is_enabled(void)
{
	uint32_t val;

	val = readl_relaxed(KONA_CHIPREG_VA + CHIPREG_PERIPH_MISC_REG1_OFFSET);
	if (CHIPREG_PERIPH_MISC_REG1_DIS_VC4_EMI_MASK & val)
		return (bool)false;
	else
		return (bool)true;
}

/*Entered when last CPU goes to IDLE, exited when first CPU leaves IDLE */
int __idle_allow_exit(struct as_one_cpu *sys)
{
	int ret = 0;

	/*increment usg_cnt. Return if already enabled */
	if (sys->usg_cnt++ == 0)
		ret = sys->ops->enable(sys, 0);

	return ret;
}

int __idle_allow_enter(struct as_one_cpu *sys)
{
	int ret = 0;

	/*decrement usg_cnt */
	if (sys->usg_cnt && --sys->usg_cnt == 0)
		ret = sys->ops->enable(sys, 1);

	return ret;
}

int idle_enter_exit(struct as_one_cpu *sys, int enable)
{
	int ret;
	unsigned long flgs;

	spin_lock_irqsave(&sys->lock, flgs);
	if (enable)
		ret = __idle_allow_enter(sys);
	else
		ret = __idle_allow_exit(sys);
	spin_unlock_irqrestore(&sys->lock, flgs);
	return ret;
}

static int capri_devtemp_cntl(struct as_one_cpu *sys, int enable)
{
#if defined(CONFIG_CAPRI_WA_HWJIRA_1690)
	static u32 lpddr2_temp_period;

	if (enable) {
		/*
		   Workaround for JIRA CRMEMC-919/2301(Periodic device temp.
		   polling will prevent entering deep sleep in Rhea B0)
		   Workaround  : Disable temp. polling when A9 enters LPM &
		   re-enable on exit from LPM */
		lpddr2_temp_period = readl_relaxed(KONA_MEMC0_NS_VA +
						   CSR_LPDDR2_DEV_TEMP_PERIOD_OFFSET);
		/*Disable temperature polling, 0xC3500 -> 0x350080c0
		   Disables periodic reading of the device temperature
		   the period field contains the device temperature period.
		   The timer operates in the XTAL clock domain. 0cC3500 is the
		   default value, write it back. */
		writel_relaxed(0xC3500,
			       KONA_MEMC0_NS_VA +
			       CSR_LPDDR2_DEV_TEMP_PERIOD_OFFSET);
	} else {
		/*
		   Workaround for JIRA CRMEMC-919/2301(Periodic device
		   temperature polling will prevent entering deep sleep in Capri)
		   - Disable temp. polling when A9 enters LPM & re-enable
		   on exit from LPM */
		writel_relaxed(lpddr2_temp_period, KONA_MEMC0_NS_VA +
			       CSR_LPDDR2_DEV_TEMP_PERIOD_OFFSET);
	}
#endif /*CONFIG_CAPRI_WA_HWJIRA_1690 */
	return 0;
}

void pm_scu_set_power_mode(int val)
{
	int cpu;

	/*per CPU byte access to SCU */
	cpu = get_cpu();
	writeb_relaxed(val, KONA_SCU_VA + SCU_POWER_STATUS_OFFSET + cpu);
	put_cpu();
}

int hdmi_clk_is_idle(bool enable)
{
	u32 reg_val;

	writel_relaxed(UNLOCK, KONA_PROC_CLK_VA);
	reg_val = readl_relaxed(KONA_CHIPREG_VA +
				CHIPREG_PERIPH_MISC_REG2_OFFSET);
	if (enable) {
		reg_val |=
		    CHIPREG_PERIPH_MISC_REG2_HDMI_CRYSTAL_CLK_IS_IDLE_MASK;
	} else {
		reg_val &=
		    ~CHIPREG_PERIPH_MISC_REG2_HDMI_CRYSTAL_CLK_IS_IDLE_MASK;
	}
	writel_relaxed(reg_val,
		       KONA_CHIPREG_VA + CHIPREG_PERIPH_MISC_REG2_OFFSET);
	return 0;
}

void pm_config_pll_is_idle_bits(void)
{
	u32 is_idle_mask = CHIPREG_PERIPH_MISC_REG2_SE26M_CLK_IS_IDLE_MASK |
	    CHIPREG_PERIPH_MISC_REG2_DIFF26M_CLK_IS_IDLE_MASK |
	    CHIPREG_PERIPH_MISC_REG2_XTAL_ELDO_MUX_SELECT_MASK;
	u32 reg_val;

	writel_relaxed(UNLOCK, KONA_PROC_CLK_VA);
	reg_val = readl_relaxed(KONA_CHIPREG_VA
				+ CHIPREG_PERIPH_MISC_REG2_OFFSET);
	reg_val |= is_idle_mask;
	writel_relaxed(reg_val, KONA_CHIPREG_VA
		       + CHIPREG_PERIPH_MISC_REG2_OFFSET);
}

void pm_config_pti_cntl(bool enable)
{
	u32 reg_val;

	writel_relaxed(UNLOCK, KONA_ROOT_CLK_VA);

	/*PTI/TPIU control */
	if (enable)
		writel_relaxed(0, KONA_ROOT_CLK_VA +
			       ROOT_CLK_MGR_REG_VAR8PH_DIVMODE_OFFSET);

	reg_val = readl_relaxed(KONA_ROOT_CLK_VA +
				ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_OFFSET);
	if (enable) {
		reg_val &=
		    ~ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_VAR_TPIU_VARVDD_CLK_EN_MASK;
	} else {
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_VAR_TPIU_VARVDD_CLK_EN_MASK;
	}
	writel_relaxed(reg_val |
		       ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_VAR_TPIU_VARVDD_HW_SW_GATING_SEL_MASK,
		       KONA_ROOT_CLK_VA +
		       ROOT_CLK_MGR_REG_VAR_TPIU_VARVDD_CLKGATE_OFFSET);

	reg_val = readl_relaxed(KONA_ROOT_CLK_VA +
				ROOT_CLK_MGR_REG_VAR_PTI_VARVDD_CLKGATE_OFFSET);
	if (enable)
		reg_val &=
		    ~ROOT_CLK_MGR_REG_VAR_PTI_VARVDD_CLKGATE_VAR_PTI_VARVDD_CLK_EN_MASK;
	else {
		reg_val |=
		    ROOT_CLK_MGR_REG_VAR_PTI_VARVDD_CLKGATE_VAR_PTI_VARVDD_CLK_EN_MASK;
	}
	writel_relaxed(reg_val |
		       ROOT_CLK_MGR_REG_VAR_PTI_VARVDD_CLKGATE_VAR_PTI_VARVDD_HW_SW_GATING_SEL_MASK,
		       KONA_ROOT_CLK_VA +
		       ROOT_CLK_MGR_REG_VAR_PTI_VARVDD_CLKGATE_OFFSET);
}

u32 init_deep_sleep_registers(void)
{
	if (pbsc_clk) 
		peri_clk_set_hw_gating_ctrl(pbsc_clk, CLK_GATING_AUTO);
  else
		pr_err("%s:pbsc_clk is not null\n", __func__);

	pm_config_pti_cntl(true); /*Clock init should do this */
	return 0;
}

static int enable_sleep_prevention_clock(int enable)
{
	int i = 0;
	struct clk *clk;
	int no_of_clocks = 0;
	no_of_clocks = ARRAY_SIZE(sleep_prevent_clocks);

	for (i = 0; i < no_of_clocks; i++) {
		clk = clk_get(NULL, sleep_prevent_clocks[i]);
		if (IS_ERR_OR_NULL(clk)) {
			pr_info("NULL CLK - %s\n", sleep_prevent_clocks[i]);
		} else {
			if (enable)
				clk_enable(clk);
			else {
				if (clk->use_cnt > 0) {
					pr_info("%s: %s use_cnt  -  %d\n",
						__func__, clk->name,
						clk->use_cnt);
				}
				do {
					clk_disable(clk);
				} while (clk->use_cnt > 0);
			}
		}
	}
	if (!enable) {
		print_clock_count();
		print_sw_event_info();
	}
	return 0;
}

static int pm_enable_scu_standby(int enable)
{
	u32 reg_val = 0;
	reg_val = readl_relaxed(KONA_SCU_VA + SCU_CONTROL_OFFSET);
	if (enable)
		reg_val |= SCU_CONTROL_SCU_STANDBY_EN_MASK;
	else
		reg_val &= ~SCU_CONTROL_SCU_STANDBY_EN_MASK;
	writel_relaxed(reg_val, KONA_SCU_VA + SCU_CONTROL_OFFSET);
	return 0;
}

static int pm_enable_mdm_self_refresh(bool enable)
{
#if !defined(CONFIG_CAPRI_SYSEMI_DDR3)
	/* ***** Warning: Should we Ignore it when it's DDR3 ? */
	u32 reg_val;

	reg_val = readl_relaxed(KONA_MEMC0_NS_VA
				+ CSR_MODEM_MIN_PWR_STATE_OFFSET);
	if (enable == true)
		reg_val &= ~CSR_MODEM_MIN_PWR_STATE_MODEM_MIN_PWR_STATE_MASK;
	else
		reg_val |= CSR_MODEM_MIN_PWR_STATE_MODEM_MIN_PWR_STATE_MASK;
	writel_relaxed(reg_val, KONA_MEMC0_NS_VA
		       + CSR_MODEM_MIN_PWR_STATE_OFFSET);

	reg_val = readl_relaxed(KONA_MEMC0_NS_VA
				+ CSR_DSP_MIN_PWR_STATE_OFFSET);
	if (enable == true)
		reg_val &= ~CSR_DSP_MIN_PWR_STATE_DSP_MIN_PWR_STATE_MASK;
	else
		reg_val |= CSR_DSP_MIN_PWR_STATE_DSP_MIN_PWR_STATE_MASK;
	writel_relaxed(reg_val, KONA_MEMC0_NS_VA
		       + CSR_DSP_MIN_PWR_STATE_OFFSET);
#endif
	return 0;
}

static int pm_enable_vc4_self_refresh(bool enable)
{
	u32 reg_val;

	if (vc_emi_is_enabled() == false)
		return 0;

	/*MEMC1 is only enabled for BIG HW */
	reg_val = readl_relaxed(KONA_MEMC1_NS_VA
				+ CSR_APPS_MIN_PWR_STATE_OFFSET);
	if (enable == true)
		reg_val &= ~CSR_APPS_MIN_PWR_STATE_APPS_MIN_PWR_STATE_MASK;
	else
		reg_val |= CSR_APPS_MIN_PWR_STATE_APPS_MIN_PWR_STATE_MASK;
	writel_relaxed(0, KONA_MEMC1_NS_VA + CSR_APPS_MIN_PWR_STATE_OFFSET);

	reg_val = readl_relaxed(KONA_MEMC1_NS_VA
				+ CSR_MODEM_MIN_PWR_STATE_OFFSET);
	if (enable == true)
		reg_val &= ~CSR_MODEM_MIN_PWR_STATE_MODEM_MIN_PWR_STATE_MASK;
	else
		reg_val |= CSR_MODEM_MIN_PWR_STATE_MODEM_MIN_PWR_STATE_MASK;
	writel_relaxed(reg_val, KONA_MEMC1_NS_VA
		       + CSR_MODEM_MIN_PWR_STATE_OFFSET);

	reg_val = readl_relaxed(KONA_MEMC1_NS_VA
				+ CSR_DSP_MIN_PWR_STATE_OFFSET);
	if (enable == true)
		reg_val &= ~CSR_DSP_MIN_PWR_STATE_DSP_MIN_PWR_STATE_MASK;
	else
		reg_val |= CSR_DSP_MIN_PWR_STATE_DSP_MIN_PWR_STATE_MASK;
	writel_relaxed(reg_val, KONA_MEMC1_NS_VA
		       + CSR_DSP_MIN_PWR_STATE_OFFSET);

	return 0;
}

static int pm_enable_self_refresh(bool enable)
{
#if !defined(CONFIG_CAPRI_SYSEMI_DDR3)
	/* ***** Warning: DDR3 doesn't uses these registers */
	u32 reg_val;
	if (enable == true) {
		writel_relaxed(0, KONA_MEMC0_NS_VA
			       + CSR_APPS_MIN_PWR_STATE_OFFSET);
		reg_val = readl_relaxed(KONA_MEMC0_NS_VA
					+ CSR_HW_FREQ_CHANGE_CNTRL_OFFSET);
		reg_val |= CSR_HW_FREQ_CHANGE_CNTRL_DDR_PLL_PWRDN_ENABLE_MASK;
		writel_relaxed(reg_val, KONA_MEMC0_NS_VA
			       + CSR_HW_FREQ_CHANGE_CNTRL_OFFSET);
	} else {
		writel_relaxed(1, KONA_MEMC0_NS_VA
			       + CSR_APPS_MIN_PWR_STATE_OFFSET);
		reg_val = readl_relaxed(KONA_MEMC0_NS_VA
					+ CSR_HW_FREQ_CHANGE_CNTRL_OFFSET);
		reg_val &= ~CSR_HW_FREQ_CHANGE_CNTRL_DDR_PLL_PWRDN_ENABLE_MASK;
		writel_relaxed(reg_val, KONA_MEMC0_NS_VA
			       + CSR_HW_FREQ_CHANGE_CNTRL_OFFSET);
	}
#endif
	return 0;
}

static void pm_sys_emi_low_power(void)
{
#if !defined(CONFIG_CAPRI_SYSEMI_DDR3)
	/* ***** Warning: DDR3 doesn't uses these registers */
	u32 reg_val;

	reg_val = readl_relaxed(KONA_MEMC0_NS_VA
				+ CSR_HW_FREQ_CHANGE_CNTRL_OFFSET);
	reg_val |= CSR_HW_FREQ_CHANGE_CNTRL_HW_FREQ_CHANGE_EN_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_HW_AUTO_PWR_TRANSITION_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_DDR_PLL_PWRDN_ENABLE_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_XTAL_LPWR_RX_CNTRL_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_SYS_PLL_078MHZ_LPWR_RX_CNTRL_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_SYS_PLL_104MHZ_LPWR_RX_CNTRL_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_SYS_PLL_156MHZ_LPWR_RX_CNTRL_MASK;
	reg_val &=
	    ~CSR_HW_FREQ_CHANGE_CNTRL_DISABLE_DLL_CALIB_ON_CLK_CHANGE_MASK;
	writel_relaxed(reg_val, KONA_MEMC0_NS_VA
		       + CSR_HW_FREQ_CHANGE_CNTRL_OFFSET);
#endif
}

static void pm_vc_emi_low_power(void)
{
	u32 reg_val;

	if (vc_emi_is_enabled() == false)
		return;

	reg_val = readl_relaxed(KONA_MEMC1_NS_VA
				+ CSR_HW_FREQ_CHANGE_CNTRL_OFFSET);
	reg_val |= CSR_HW_FREQ_CHANGE_CNTRL_HW_FREQ_CHANGE_EN_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_HW_AUTO_PWR_TRANSITION_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_DDR_PLL_PWRDN_ENABLE_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_XTAL_LPWR_RX_CNTRL_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_SYS_PLL_078MHZ_LPWR_RX_CNTRL_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_SYS_PLL_104MHZ_LPWR_RX_CNTRL_MASK |
	    CSR_HW_FREQ_CHANGE_CNTRL_SYS_PLL_156MHZ_LPWR_RX_CNTRL_MASK;

/* only turn on auto Vref when it can be supported */
#ifdef CONFIG_CAPRI_LPDDR2_AUTO_VREF
	reg_val &=
	    ~CSR_HW_FREQ_CHANGE_CNTRL_DISABLE_DLL_CALIB_ON_CLK_CHANGE_MASK;
#else
	reg_val |=
	    CSR_HW_FREQ_CHANGE_CNTRL_DISABLE_DLL_CALIB_ON_CLK_CHANGE_MASK;
#endif
	writel_relaxed(reg_val, KONA_MEMC1_NS_VA
		       + CSR_HW_FREQ_CHANGE_CNTRL_OFFSET);
}

static int pm_config_deep_sleep(void)
{
	arm_pll_disable(true);
	arm_pll_8phase_disable(true);

	pwr_mgr_arm_core_dormant_enable(false /*disallow dormant */ );
	pm_enable_scu_standby(true);
	pm_config_pll_is_idle_bits();
	hdmi_clk_is_idle(true);

	/*Use SCU in A9's for retention state status */
	if (chip_version >= CAPRI_A1) {
		writel_relaxed(PWRCTL_USE_SCU,
			       KONA_CHIPREG_VA +
			       CHIPREG_PERIPH_MISC_REG3_OFFSET);
	}

	/*Configure memory for Low Power */
	pm_sys_emi_low_power();
	pm_vc_emi_low_power();

	pm_enable_self_refresh(true);
	pm_enable_vc4_self_refresh(true);
	pm_enable_mdm_self_refresh(true);
	return 0;
}

static int print_clock_count(void)
{
	struct clk *clk;
	clk = clk_get(NULL, ROOT_CCU_CLK_NAME_STR);
	if (IS_ERR_OR_NULL(clk)) {
		pr_err("Inavlid clock name: %s\n", __func__);
		BUG_ON(1);
		return -EINVAL;
	}
	pm_dbg("%s:  %s clock count %d\n", __func__, clk->name, clk->use_cnt);
	clk = clk_get(NULL, KHUB_CCU_CLK_NAME_STR);
	if (IS_ERR_OR_NULL(clk)) {
		pr_err("Inavlid clock name: %s\n", __func__);
		BUG_ON(1);
		return -EINVAL;
	}
	pm_dbg("%s:  %s clock count %d\n", __func__, clk->name, clk->use_cnt);

	clk = clk_get(NULL, KHUBAON_CCU_CLK_NAME_STR);
	if (IS_ERR_OR_NULL(clk)) {
		pr_err("Inavlid clock name: %s\n", __func__);
		BUG_ON(1);
		return -EINVAL;
	}
	pm_dbg("%s:  %s clock count %d\n", __func__, clk->name, clk->use_cnt);

	clk = clk_get(NULL, KPM_CCU_CLK_NAME_STR);
	if (IS_ERR_OR_NULL(clk)) {
		pr_err("Inavlid clock name: %s\n", __func__);
		BUG_ON(1);
		return -EINVAL;
	}
	pm_dbg("%s:  %s clock count %d\n", __func__, clk->name, clk->use_cnt);

	clk = clk_get(NULL, KPS_CCU_CLK_NAME_STR);
	if (IS_ERR_OR_NULL(clk)) {
		pr_err("Inavlid clock name: %s\n", __func__);
		BUG_ON(1);
		return -EINVAL;
	}
	pm_dbg("%s:  %s clock count %d\n", __func__, clk->name, clk->use_cnt);

	return 0;
}

static void clear_wakeup_events(void)
{
	unsigned long flgs;
	spin_lock_irqsave(&wake_up_event_lock, flgs);
	/*only clearing SW1 means that 0 needs to manually be cleared */
	pwr_mgr_event_clear_events(SOFTWARE_1_EVENT, SOFTWARE_1_EVENT);
	pwr_mgr_event_clear_events(COMMON_INT_TO_AC_EVENT,
				   COMMON_INT_TO_AC_EVENT);
	pwr_mgr_event_clear_events(COMMON_TIMER_1_EVENT, COMMON_TIMER_1_EVENT);
	pwr_mgr_event_clear_events(COMMON_TIMER_2_EVENT, COMMON_TIMER_2_EVENT);
	pwr_mgr_event_clear_events(UBRX_EVENT, UBRX_EVENT);
	pwr_mgr_event_clear_events(UB2RX_EVENT, UB2RX_EVENT);
	pwr_mgr_event_clear_events(SIMDET_EVENT, SIMDET_EVENT);
	pwr_mgr_event_clear_events(SIM2DET_EVENT, SIM2DET_EVENT);
	pwr_mgr_event_clear_events(PMU_INT_A_EVENT, PMU_INT_A_EVENT);
	pwr_mgr_event_clear_events(ULPI1_EVENT, ULPI1_EVENT);
	pwr_mgr_event_clear_events(ULPI2_EVENT, ULPI2_EVENT);
	pwr_mgr_event_clear_events(KEY_CI_0_EVENT, KEY_CI_0_EVENT);
	spin_unlock_irqrestore(&wake_up_event_lock, flgs);
}

/*
For timebeing, COMMON_INT_TO_AC_EVENT related functions are added here
We may have to move these fucntions to somewhere else later
*/
static void clear_wakeup_interrupts(void)
{
	pm_dbg("%s\n", __func__);

/* clear interrupts for COMMON_INT_TO_AC_EVENT*/
	if (enable_clr_intr) {
		writel_relaxed(0xffffffff, KONA_CHIPREG_VA +
			       CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR0_OFFSET);
		writel_relaxed(0xffffffff, KONA_CHIPREG_VA +
			       CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR1_OFFSET);
		writel_relaxed(0xffffffff, KONA_CHIPREG_VA +
			       CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR2_OFFSET);
		writel_relaxed(0xffffffff, KONA_CHIPREG_VA +
			       CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR3_OFFSET);
		writel_relaxed(0xffffffff, KONA_CHIPREG_VA +
			       CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR4_OFFSET);
		writel_relaxed(0xffffffff, KONA_CHIPREG_VA +
			       CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR5_OFFSET);
		writel_relaxed(0xffffffff, KONA_CHIPREG_VA +
			       CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR6_OFFSET);
	}
}

static void config_wakeup_interrupts(void)
{
	unsigned long flgs;

	pm_dbg("%s\n", __func__);

	spin_lock_irqsave(&wake_up_event_lock, flgs);

	/*all enabled interrupts can trigger COMMON_INT_TO_AC_EVENT */

	writel_relaxed(readl_relaxed(KONA_GICDIST_VA
				     + GICDIST_ENABLE_SET1_OFFSET),
		       KONA_CHIPREG_VA +
		       CHIPREG_INTERRUPT_EVENT_4_PM_SET0_OFFSET);
	writel_relaxed(readl_relaxed
		       (KONA_GICDIST_VA + GICDIST_ENABLE_SET2_OFFSET),
		       KONA_CHIPREG_VA +
		       CHIPREG_INTERRUPT_EVENT_4_PM_SET1_OFFSET);
	writel_relaxed(readl_relaxed
		       (KONA_GICDIST_VA + GICDIST_ENABLE_SET3_OFFSET),
		       KONA_CHIPREG_VA +
		       CHIPREG_INTERRUPT_EVENT_4_PM_SET2_OFFSET);
	writel_relaxed(readl_relaxed
		       (KONA_GICDIST_VA + GICDIST_ENABLE_SET4_OFFSET),
		       KONA_CHIPREG_VA +
		       CHIPREG_INTERRUPT_EVENT_4_PM_SET3_OFFSET);
	writel_relaxed(readl_relaxed
		       (KONA_GICDIST_VA + GICDIST_ENABLE_SET5_OFFSET),
		       KONA_CHIPREG_VA +
		       CHIPREG_INTERRUPT_EVENT_4_PM_SET4_OFFSET);
	writel_relaxed(readl_relaxed
		       (KONA_GICDIST_VA + GICDIST_ENABLE_SET6_OFFSET),
		       KONA_CHIPREG_VA +
		       CHIPREG_INTERRUPT_EVENT_4_PM_SET5_OFFSET);
	writel_relaxed(readl_relaxed
		       (KONA_GICDIST_VA + GICDIST_ENABLE_SET7_OFFSET),
		       KONA_CHIPREG_VA +
		       CHIPREG_INTERRUPT_EVENT_4_PM_SET6_OFFSET);

	spin_unlock_irqrestore(&wake_up_event_lock, flgs);
}

static void capri_pm_dbg_store(struct capri_pm_dbg *pm_dbg_ptr)
{
	pm_dbg_ptr->hubTM_stcs = readl_relaxed(KONA_TMR_HUB_VA +
					KONA_GPTIMER_STCS_OFFSET);
	pm_dbg_ptr->hubTM_stclo = readl_relaxed(KONA_TMR_HUB_VA +
					KONA_GPTIMER_STCLO_OFFSET);
	pm_dbg_ptr->hubTM_stchi = readl_relaxed(KONA_TMR_HUB_VA +
					KONA_GPTIMER_STCHI_OFFSET);
	pm_dbg_ptr->hubTM_stcm1 = readl_relaxed(KONA_TMR_HUB_VA +
					KONA_GPTIMER_STCM1_OFFSET);

	pm_dbg_ptr->pm_set[0] = readl_relaxed(KONA_CHIPREG_VA +
		       CHIPREG_INTERRUPT_EVENT_4_PM_SET0_OFFSET);
	pm_dbg_ptr->pm_set[1] = readl_relaxed(KONA_CHIPREG_VA +
			   CHIPREG_INTERRUPT_EVENT_4_PM_SET1_OFFSET);
	pm_dbg_ptr->pm_set[2] = readl_relaxed(KONA_CHIPREG_VA +
			   CHIPREG_INTERRUPT_EVENT_4_PM_SET2_OFFSET);
	pm_dbg_ptr->pm_set[3] = readl_relaxed(KONA_CHIPREG_VA +
			   CHIPREG_INTERRUPT_EVENT_4_PM_SET3_OFFSET);
	pm_dbg_ptr->pm_set[4] = readl_relaxed(KONA_CHIPREG_VA +
			   CHIPREG_INTERRUPT_EVENT_4_PM_SET4_OFFSET);
	pm_dbg_ptr->pm_set[5] = readl_relaxed(KONA_CHIPREG_VA +
			   CHIPREG_INTERRUPT_EVENT_4_PM_SET5_OFFSET);
	pm_dbg_ptr->pm_set[6] = readl_relaxed(KONA_CHIPREG_VA +
			   CHIPREG_INTERRUPT_EVENT_4_PM_SET6_OFFSET);

	pm_dbg_ptr->pm_event = 0;

	if (pwr_mgr_is_event_active(COMMON_INT_TO_AC_EVENT))
		pm_dbg_ptr->pm_event |= PM_WAKEUP_BY_COMMON_INT_TO_AC_EVENT;

	if (pwr_mgr_is_event_active(COMMON_TIMER_1_EVENT))
		pm_dbg_ptr->pm_event |= PM_WAKEUP_BY_COMMON_TIMER_1_EVENT;

	if (pwr_mgr_is_event_active(VREQ_NONZERO_PI_MODEM_EVENT))
		pm_dbg_ptr->pm_event |= VREQ_NONZERO_PI_MODEM_EVENT_IS_ON;

}

int print_sw_event_info()
{
	u32 reg_val = 0;

	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_ARM_CORE_POLICY_OFFSET
				+ SOFTWARE_0_EVENT * 4);
	pm_dbg("SW0 policy for Modem and ARM core : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_MM_POLICY_OFFSET
				+ SOFTWARE_0_EVENT * 4);
	pm_dbg("SW0 policy for MM : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_HUB_POLICY_OFFSET
				+ SOFTWARE_0_EVENT * 4);
	pm_dbg("SW0 policy for AON and HUB : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_ARM_SUBSYSTEM_POLICY_OFFSET
				+ SOFTWARE_0_EVENT * 4);
	pm_dbg("SW0 policy for ARM Sub system : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_ARM_CORE_POLICY_OFFSET
				+ SOFTWARE_1_EVENT * 4);
	pm_dbg("SW1 policy for Modem and ARM core : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_MM_POLICY_OFFSET +
				SOFTWARE_1_EVENT * 4);
	pm_dbg("SW1 policy for MM : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_HUB_POLICY_OFFSET +
				SOFTWARE_1_EVENT * 4);
	pm_dbg("SW1 policy for AON and HUB : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_ARM_SUBSYSTEM_POLICY_OFFSET
				+ SOFTWARE_1_EVENT * 4);
	pm_dbg("SW1 policy for ARM Sub system : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_ARM_CORE_POLICY_OFFSET
				+ SOFTWARE_2_EVENT * 4);
	pm_dbg("SW2 policy for Modem and ARM core : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_MM_POLICY_OFFSET +
				SOFTWARE_2_EVENT * 4);
	pm_dbg("SW2 policy for MM : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_HUB_POLICY_OFFSET +
				SOFTWARE_2_EVENT * 4);
	pm_dbg("SW2 policy for AON and HUB : %08x\n", reg_val);
	reg_val = readl_relaxed(KONA_PWRMGR_VA
				+ PWRMGR_LCDTE_VI_ARM_SUBSYSTEM_POLICY_OFFSET
				+ SOFTWARE_2_EVENT * 4);
	pm_dbg("SW2 policy for ARM Sub system : %08x\n", reg_val);
	return 0;
}

static int arm_pll_disable(int en)
{
/*	clk_set_pll_pwr_on_idle(ROOT_CCU_PLL0A, (bool)en); */
/*	clk_set_pll_pwr_on_idle(ROOT_CCU_PLL1A, (bool)en); */
	clk_set_crystal_pwr_on_idle((bool)en);
	return 0;
}

static int arm_pll_8phase_disable(int en)
{
	u32 reg_val;

	writel_relaxed(UNLOCK, KONA_ROOT_CLK_VA);

	if (en) {
		reg_val = readl_relaxed(KONA_ROOT_CLK_VA
					+ ROOT_CLK_MGR_REG_PLL0CTRL0_OFFSET);
		reg_val &= ~ROOT_CLK_MGR_REG_PLL0CTRL0_PLL0_8PHASE_EN_MASK;
		writel_relaxed(reg_val, KONA_ROOT_CLK_VA
			       + ROOT_CLK_MGR_REG_PLL0CTRL0_OFFSET);
		reg_val = readl_relaxed(KONA_ROOT_CLK_VA
					+ ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET);
		reg_val &= ~ROOT_CLK_MGR_REG_PLL1CTRL0_PLL1_8PHASE_EN_MASK;
		writel_relaxed(reg_val, KONA_ROOT_CLK_VA
			       + ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET);
	} else {
		reg_val = readl_relaxed(KONA_ROOT_CLK_VA
					+ ROOT_CLK_MGR_REG_PLL0CTRL0_OFFSET);
		reg_val |= ROOT_CLK_MGR_REG_PLL0CTRL0_PLL0_8PHASE_EN_MASK;
		writel_relaxed(reg_val, KONA_ROOT_CLK_VA
			       + ROOT_CLK_MGR_REG_PLL0CTRL0_OFFSET);
		reg_val = readl_relaxed(KONA_ROOT_CLK_VA
					+ ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET);
		reg_val |= ROOT_CLK_MGR_REG_PLL1CTRL0_PLL1_8PHASE_EN_MASK;
		writel_relaxed(reg_val, KONA_ROOT_CLK_VA
			       + ROOT_CLK_MGR_REG_PLL1CTRL0_OFFSET);
	}
	return 0;
}

static void capri_wfi(void)
{
	u32 timer_lsw = 0;

	enter_wfi();
	if (chip_version == CAPRI_A0) {
		/* wait for Hub Clock to tick
		 *(This is a HW BUG Workaround for JIRA HWCAPRI-1092))  */
		timer_lsw = readl_relaxed(KONA_TMR_HUB_VA
					  + KONA_GPTIMER_STCLO_OFFSET);
		while (timer_lsw == readl_relaxed(KONA_TMR_HUB_VA
						  +
						  KONA_GPTIMER_STCLO_OFFSET)) ;
	}
}

#ifdef CONFIG_CAPRI_RETENTION_MODE
static int capri_retention_state(struct kona_idle_state *state)
{
	/* Code for basic retention of all A9 CCUs */
#define NUM_OF_CPUS 2
	int cpu;
	u32 sctlr[NUM_OF_CPUS];
	u32 actlr[NUM_OF_CPUS];

	/*per CPU access to clear/inv D$ */
	cpu = get_cpu();
	sctlr[cpu] = read_sctlr();
	put_cpu();
	disable_clean_inv_dcache_v7_l1();
	cpu = get_cpu();
	actlr[cpu] = read_actlr();
	put_cpu();
	write_actlr(read_actlr() & ~A9_SMP_BIT);
	 /**/ pm_scu_set_power_mode(A9_SCU_DORM);

	capri_wfi();

	pm_scu_set_power_mode(A9_SCU_NORM);

	/*perCpu config D$ */
	cpu = get_cpu();
	write_actlr(actlr[cpu]);
	write_sctlr(sctlr[cpu]);
	put_cpu();
	 /**/ return 0;
}
#endif

#ifdef CONFIG_CAPRI_DORMANT_MODE
void print_my_dbg_info(int core)
{
	/*extern const char *_capri__event2str[];
	 * removed due to checkpatch. */
	u32 i, int_id;
	u32 read_id;

	if (core == 1) {
		int_id = readl_relaxed(KONA_GICCPU_VA + 0x18);
		if ((int_id & 0x3ff) == 0x3ff) {
			/* acknowledge the null */
			int_id = readl_relaxed(KONA_GICCPU_VA + 0xC);
			pr_info("null core1 %d\n", int_id);
		}
	} else {
		int_id = readl_relaxed(KONA_GICCPU_VA + 0x18);
		if ((int_id & 0x3ff) == 0x3ff) {
			/* acknowledge the null */
			readl_relaxed(KONA_GICCPU_VA + 0xC);
			int_id = readl_relaxed(KONA_GICCPU_VA + 0x18);
			read_id = readl_relaxed(KONA_CHIPREG_VA
						+ CHIPREG_BOOT_2ND_ADDR_OFFSET);
			pr_info("null core0 %d %d\n", read_id, int_id);
		} else {
			pr_info("core0 %d\n", int_id);
		}
		for (i = 0; i < 0x94; i++) {
			if (i == SOFTWARE_0_EVENT ||
			    i == VREQ_NONZERO_PI_MODEM_EVENT ||
			    i == COMMON_TIMER_2_EVENT || i == SOFTWARE_2_EVENT)
				continue;

			/*if (pwr_mgr_is_event_active(i))
			 *      pr_info("%s\n",
			 *              _capri__event2str[i]);
			 */
		}
	}
}

void set_hub_autogate(bool enable)
{
	uint32_t val;

	if (!enable) {
		writel(0xA5A501, KONA_HUB_CLK_VA);
		val = readl_relaxed(KONA_HUB_CLK_VA +
				    KHUB_CLK_MGR_REG_HUB_CLKGATE_OFFSET);
		writel(0xA5A500, KONA_HUB_CLK_VA);

		if ((val &
		     KHUB_CLK_MGR_REG_HUB_CLKGATE_HUB_HW_SW_GATING_SEL_MASK) ==
		    0) {
			/* disable autogate */
			clk_disable_autogate(clk_get(NULL, "hub_clk"));
			pr_info("disable HUB clock autogate (0x%x)\n", val);
			redo_auto = TRUE;
		}
	} else {
		if (redo_auto) {
			/* re-enable autogate if originally set */
			pr_info("re-enable HUB clock autogate\n");
			clk_enable_autogate(clk_get(NULL, "hub_clk"));
			redo_auto = FALSE;
		}
	}
}

struct pi *arm_sub_pi;

static int capri_suspend_dormant(bool enter_dormant)
{
	int tmp_val;
	suspend_debug_count = 2;

	if (one_shot == 0) {
		one_shot = 1;
		init_deep_sleep_registers();
		arm_pll_disable(true);
	}
	suspend_debug_count = 3;
	clear_wakeup_interrupts();
	clear_wakeup_events();
	config_wakeup_interrupts();
	/*set A9's to retention state status */

	if (force_retention)
		enable_sleep_prevention_clock(0);

	if (dbg_dsm_suspend) {
		capri_clock_print_act_clks();
		capri_pi_mgr_print_act_pis();
	}
	if (!arm_sub_pi)
		arm_sub_pi = pi_mgr_get(PI_MGR_PI_ID_ARM_SUB_SYSTEM);

	if (kps_clk->use_cnt == 0 && kpm_clk->use_cnt == 0) {
		tmp_val = readl_relaxed(KONA_PWRMGR_VA +
			PWRMGR_SOFTWARE_0_VI_ARM_SUBSYSTEM_POLICY_OFFSET);
		pr_info("%s:SW0_EVENT(arm_sub)=%d\n",
			__func__, tmp_val);
		if ((tmp_val & 0x07) == 0x5)
			writel_relaxed(tmp_val&0xfb, KONA_PWRMGR_VA +
				PWRMGR_SOFTWARE_0_VI_ARM_SUBSYSTEM_POLICY_OFFSET
		);
		pr_info("arm_subUseCnt = %d\n",
			arm_sub_pi->usg_cnt);
	} else {
		pr_info("arm_subUseCnt = %d, kpsClkCnt = %d, kpmClkCnt = %d\n",
			arm_sub_pi->usg_cnt, kps_clk->use_cnt,
			kpm_clk->use_cnt);
	}

	capri_pm_dbg_store(&pre_pm_dbg);
	suspend_debug_count = 4;
	dormant_enter(CAPRI_DORMANT_CLUSTER_DOWN, CAPRI_DORMANT_SUSPEND_PATH);
	suspend_debug_count = 5;
	capri_pm_dbg_store(&post_pm_dbg);
	/*log wake up events*/
	if (dbg_dsm_suspend) {
		#define MAX_LOG_EVENTID 5
		int i;
		int eventId[MAX_LOG_EVENTID];
		pwr_mgr_get_wakeup_events(eventId, MAX_LOG_EVENTID);
		for (i = 0; i < MAX_LOG_EVENTID; i++)
			pr_info("wakeup ID:%d\n", eventId[i]);
	}

	if (kps_clk->use_cnt == 0 && kpm_clk->use_cnt == 0) {
			if ((tmp_val & 0x07) == 0x5)
				writel_relaxed(tmp_val, KONA_PWRMGR_VA +
				PWRMGR_SOFTWARE_0_VI_ARM_SUBSYSTEM_POLICY_OFFSET
		);
	}
	/*enable SW2 Active bit */
	pwr_mgr_event_set(SOFTWARE_2_EVENT, 1);

	clear_wakeup_interrupts();
	/*process and clear event for wake up */
	pwr_mgr_process_events(LCDTE_EVENT, BRIDGE_TO_MODEM_EVENT, false);
	pwr_mgr_process_events(USBOTG_EVENT, ACI_EVENT, false);
	pwr_mgr_process_events(VPM_WAKEUP_EVENT, ULPI2_EVENT, false);
	suspend_debug_count = 6;
	return 0;
}
#endif

static int capri_suspend_deepsleep(void)
{
	if (one_shot == 0) {
		one_shot = 1;
		init_deep_sleep_registers();
		arm_pll_disable(true);
	}

	clear_wakeup_interrupts();
	clear_wakeup_events();
	config_wakeup_interrupts();
	/*set A9's to retention state status */

	if (debug_ds & 1)

		writel_relaxed(SCU_OFF_MODE,
			       KONA_SCU_VA + SCU_POWER_STATUS_OFFSET);
	else
		writel_relaxed(SCU_DORMANT_MODE,
			       KONA_SCU_VA + SCU_POWER_STATUS_OFFSET);

	if (chip_version == CAPRI_A1)
		writel_relaxed(PWRCTL_BYPASS_DORMANT,
			       KONA_CHIPREG_VA +
			       CHIPREG_PERIPH_MISC_REG3_OFFSET);

	if (force_retention)
		enable_sleep_prevention_clock(0);

	if (dbg_dsm_suspend) {
		capri_clock_print_act_clks();
		capri_pi_mgr_print_act_pis();
	}

	{
#ifdef CONFIG_CAPRI_WA_HWJIRA_1584
		/* JIRA HWCAPRI-1584 */
		udelay(100);
#endif
	}
	capri_wfi();

	/*enable SW2 Active bit */
	pwr_mgr_event_set(SOFTWARE_2_EVENT, 1);

	if (chip_version == CAPRI_A1)
		writel_relaxed(PWRCTL_BYPASS_NORMAL,
			       KONA_CHIPREG_VA +
			       CHIPREG_PERIPH_MISC_REG3_OFFSET);

	writel_relaxed(SCU_DORMANT2_MODE_OFF, KONA_SCU_VA
		       + SCU_POWER_STATUS_OFFSET);

	clear_wakeup_interrupts();
	/*process and clear event for wake up */
	pwr_mgr_process_events(LCDTE_EVENT, BRIDGE_TO_MODEM_EVENT, false);
	pwr_mgr_process_events(USBOTG_EVENT, ACI_EVENT, false);
	pwr_mgr_process_events(VPM_WAKEUP_EVENT, ULPI2_EVENT, false);
	return 0;
}

static int capri_suspend_retention(void)
{
	/* Code for basic retention of all A9 CCUs */

	/*disable PLL */
	arm_pll_disable(true);

	/*enable AUTOGATING BSC */
	clear_wakeup_interrupts();
	clear_wakeup_events();
	config_wakeup_interrupts();
	/*set A9's to retention state status */

	writel_relaxed(SCU_DORMANT_MODE, KONA_SCU_VA + SCU_POWER_STATUS_OFFSET);

	if (force_retention)
		enable_sleep_prevention_clock(0);

	if (dbg_dsm_suspend) {
		capri_clock_print_act_clks();
		capri_pi_mgr_print_act_pis();
	}

	capri_wfi();

	/*enable SW2 Active bit */
	pwr_mgr_event_set(SOFTWARE_2_EVENT, 1);

	writel_relaxed(SCU_DORMANT2_MODE_OFF, KONA_SCU_VA
		       + SCU_POWER_STATUS_OFFSET);

	/*enable PLL */
	arm_pll_disable(true);
	clear_wakeup_interrupts();
	/*process and clear event for wake up */
	pwr_mgr_process_events(LCDTE_EVENT, BRIDGE_TO_MODEM_EVENT, false);
	pwr_mgr_process_events(USBOTG_EVENT, ACI_EVENT, false);
	pwr_mgr_process_events(VPM_WAKEUP_EVENT, ULPI2_EVENT, false);
	return 0;
}

int enter_idle_state(struct kona_idle_state *state)
{
	unsigned long flgs;

	BUG_ON(!state);

	if (state->state == CAPRI_STATE_C0 || force_idle_wfi_in_suspend) {
		/* We could use the ener_suspend_state directly
		 * if the capri_cpu_states but just so that we
		 * would have a common function for adding any
		 * logging for state counts etc.
		 */
		return enter_suspend_state(state);
	}

	/*disable PLL */
	arm_pll_disable(true);

	/*Turn off XTAL only for deep sleep state */
	if ((state->flags & CPUIDLE_FLAG_XTAL_ON) || keep_xtl_on)
		clk_set_crystal_pwr_on_idle(false);

	clear_wakeup_interrupts();
	clear_wakeup_events();
	config_wakeup_interrupts();

	/*Enter when last CPU enters WFI */
	idle_enter_exit(&gen_one_cpu_ops, 1);

	if (force_wfi_only_in_idle) {
		enter_suspend_state(state);
	} else {
		switch (state->state) {
		case CAPRI_STATE_C1:
			enter_retention_state(state);
			break;
		case CAPRI_STATE_C2:
		case CAPRI_STATE_C3:
			enter_dormant_state(state);
			break;
		default:
			enter_suspend_state(state);
			break;
		}
	}

	/*Exit when first CPU leaves WFI */
	idle_enter_exit(&gen_one_cpu_ops, 0);

	spin_lock_irqsave(&wake_up_event_lock, flgs);
	/*enable SW2 Active bit */
	pwr_mgr_event_set(SOFTWARE_2_EVENT, 1);
	clear_wakeup_interrupts();
	/*process and clear event for wake up */
	pwr_mgr_process_events(LCDTE_EVENT, BRIDGE_TO_MODEM_EVENT, false);
	pwr_mgr_process_events(USBOTG_EVENT, ACI_EVENT, false);
	pwr_mgr_process_events(VPM_WAKEUP_EVENT, ULPI2_EVENT, false);
	spin_unlock_irqrestore(&wake_up_event_lock, flgs);

	if ((state->flags & CPUIDLE_FLAG_XTAL_ON) || keep_xtl_on)
		clk_set_crystal_pwr_on_idle(true);
	return -1;
}

static int enter_suspend_state(struct kona_idle_state *state)
{
	capri_wfi();
	return -1;
}

static int enter_dormant_state(struct kona_idle_state *state)
{
#ifdef CONFIG_CAPRI_DORMANT_MODE
	/* Start entering dormant in idle only starting A2 or later */
	if (chip_version > CAPRI_A1)
		dormant_enter(CAPRI_DORMANT_CORE_DOWN, CAPRI_DORMANT_IDLE_PATH);
	else
		enter_retention_state(state);
#else
	/* Function would be called for C3 state irrespective of
	 * weather dormant is defined or not.  If dormant is not deifned
	 * fall back to retention.
	 */
	enter_retention_state(state);
#endif
	return 0;
}

static int enter_retention_state(struct kona_idle_state *state)
{
	/*eventually updated to support retention IDLE entry/exit */
#ifdef CONFIG_CAPRI_RETENTION_MODE
	capri_retention_state(state);
#else
	capri_wfi();
#endif
	return 0;
}

int kona_mach_pm_prepare(void)
{
	force_idle_wfi_in_suspend = 1;
	pwr_mgr_notice_suspend(1);
	/* disable HUB autogating before suspend */
	set_hub_autogate(false);
	suspend_debug_count = 9;
	return 0;
}

void kona_mach_pm_finish(void)
{
	force_idle_wfi_in_suspend = 0;
	pwr_mgr_notice_suspend(0);
	/* re-enable HUB autogating after suspend */
	set_hub_autogate(true);
	suspend_debug_count = 8;
}

int kona_mach_pm_enter(suspend_state_t state)
{
	int ret = 0;

	/*Enter when CPU0 enters WFI */
	capri_devtemp_cntl(NULL, 1);

	switch (state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:

#ifdef CONFIG_HAS_WAKELOCK
		/*Don't enter WFI if any wake lock is active
		   Added to take care of wake locks that gets activiated
		   just before interrupts are dsiabled during suspend */
		if (has_wake_lock(WAKE_LOCK_SUSPEND) ||
		    has_wake_lock(WAKE_LOCK_IDLE)) {
			pr_info("%s:wake lock active, skip WFI\n", __func__);
			break;
		}
#endif /*CONFIG_HAS_WAKELOCK */

		/* suspend */
		pr_info("%s:Enter mode=%d\n", __func__, suspend_mode);
		switch (suspend_mode) {
		case SUSPEND_RETENTION:
			ret = capri_suspend_retention();
			break;
		case SUSPEND_DEEPSLEEP:
			ret = capri_suspend_deepsleep();
			break;
		case SUSPEND_DORMANT:
		suspend_debug_count = 1;
#ifdef CONFIG_CAPRI_DORMANT_MODE
			ret = capri_suspend_dormant(true);
#else
			ret = capri_suspend_deepsleep();
#endif
			break;
		case SUSPEND_WFI:
		default:
			capri_wfi();
			break;
		}
		break;
	default:
		pr_info("%s:Exit(error)\n", __func__);
		ret = -EINVAL;
	}
	/*Exit when CPU0 leaves WFI */
	capri_devtemp_cntl(NULL, 0);
	suspend_debug_count = 7;
	pr_info("%s:Exit\n", __func__);
	return ret;
}

int kona_mach_get_idle_states(struct kona_idle_state **idle_states)
{
	pr_info("CAPRI => kona_mach_get_idle_states\n");
	*idle_states = capri_cpu_states;
	return ARRAY_SIZE(capri_cpu_states);
}

#ifdef CONFIG_CAPRI_DELAYED_PM_INIT
int capri_pm_init(void)
#else
int __init capri_pm_init(void)
#endif
{
	chip_version = get_chip_version();
	pm_config_deep_sleep();
	spin_lock_init(&gen_one_cpu_ops.lock);
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&rpc_early_suspend_desc);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	if (!pbsc_clk) {
		pbsc_clk = clk_get(NULL, PMU_BSC_PERI_CLK_NAME_STR);
		if (IS_ERR_OR_NULL(pbsc_clk)) {
			pr_err("pbsc_clk Inavlid clock name: %s\n", __func__);
			BUG_ON(1);
			return -EINVAL;
		}
	}
	kpm_clk = clk_get(NULL, KPM_CCU_CLK_NAME_STR);
	if (IS_ERR_OR_NULL(kpm_clk)) {
		pr_err("Inavlid clock name: %s\n", __func__);
		BUG_ON(1);
		return -EINVAL;
	}
	kps_clk = clk_get(NULL, KPS_CCU_CLK_NAME_STR);
	if (IS_ERR_OR_NULL(kps_clk)) {
		pr_err("Inavlid clock name: %s\n", __func__);
		BUG_ON(1);
		return -EINVAL;
	}
	return kona_pm_init();
}

/* Force sleep functionality utilized by at*mlpm command */
int capri_force_sleep(suspend_state_t state)
{
	int i;

	local_irq_disable();
	local_fiq_disable();

	force_retention = 1;

	/* continually attempt deep sleep */
	while (1) {
		for (i = 0; i < PWR_MGR_NUM_EVENTS; i++) {
			int test = 0;

			test |= (i == SOFTWARE_0_EVENT) ? 1 : 0;
			test |= (i == SOFTWARE_2_EVENT) ? 1 : 0;
			test |= (i == VREQ_NONZERO_PI_MODEM_EVENT) ? 1 : 0;

			if (test == 0)
				pwr_mgr_event_trg_enable(i, 0);
		}

		/* enter idle state */
		kona_mach_pm_enter(PM_SUSPEND_MEM);

	}
}

/* Reduce EMI idle time before power down */
void capri_reduce_emi_idle_time_before_pwrdn(bool enable)
{
#define REDUCED_EMI_IDLE_TIME_BEFORE_PWRDN  \
	(4 << CSR_DDR_SW_POWER_DOWN_CONTROL_IDLE_TIME_SHIFT)

	u32 reg_val;
	static bool enabled;
	static u32 orig_sys_emi_pwrdn_idle_time;
	static u32 orig_vc4_emi_pwrdn_idle_time;

	if ((enable == true) && (enabled == false)) {
		/* Switch to reduced idle time (save original) */
		reg_val = readl(KONA_MEMC0_NS_VA +
				CSR_DDR_SW_POWER_DOWN_CONTROL_OFFSET);
		orig_sys_emi_pwrdn_idle_time = reg_val &
		    CSR_DDR_SW_POWER_DOWN_CONTROL_IDLE_TIME_MASK;
		reg_val &= ~CSR_DDR_SW_POWER_DOWN_CONTROL_IDLE_TIME_MASK;
		reg_val |= REDUCED_EMI_IDLE_TIME_BEFORE_PWRDN;
		writel(reg_val, KONA_MEMC0_NS_VA +
		       CSR_DDR_SW_POWER_DOWN_CONTROL_OFFSET);

		if (vc_emi_is_enabled()) {
			reg_val = readl(KONA_MEMC1_NS_VA +
					CSR_DDR_SW_POWER_DOWN_CONTROL_OFFSET);
			orig_vc4_emi_pwrdn_idle_time = reg_val &
			    CSR_DDR_SW_POWER_DOWN_CONTROL_IDLE_TIME_MASK;
			reg_val &=
			    ~CSR_DDR_SW_POWER_DOWN_CONTROL_IDLE_TIME_MASK;
			reg_val |= REDUCED_EMI_IDLE_TIME_BEFORE_PWRDN;
			writel(reg_val, KONA_MEMC1_NS_VA +
			       CSR_DDR_SW_POWER_DOWN_CONTROL_OFFSET);
		}

		enabled = true;
	} else {
		/* Restore original power down idle time */
		reg_val = readl(KONA_MEMC0_NS_VA +
				CSR_DDR_SW_POWER_DOWN_CONTROL_OFFSET);
		if ((reg_val & CSR_DDR_SW_POWER_DOWN_CONTROL_IDLE_TIME_MASK) !=
		    REDUCED_EMI_IDLE_TIME_BEFORE_PWRDN) {
			pr_warn("%s: EMI idle power down time was modified\n",
				__func__);
			WARN_ON(1);
		} else {
			reg_val &=
			    ~CSR_DDR_SW_POWER_DOWN_CONTROL_IDLE_TIME_MASK;
			reg_val |= orig_sys_emi_pwrdn_idle_time;
			writel(reg_val, KONA_MEMC0_NS_VA +
			       CSR_DDR_SW_POWER_DOWN_CONTROL_OFFSET);
		}

		if (vc_emi_is_enabled()) {
			reg_val = readl(KONA_MEMC1_NS_VA +
					CSR_DDR_SW_POWER_DOWN_CONTROL_OFFSET);
			if ((reg_val &
			     CSR_DDR_SW_POWER_DOWN_CONTROL_IDLE_TIME_MASK) !=
			    REDUCED_EMI_IDLE_TIME_BEFORE_PWRDN) {
				pr_warn("%s: EMI idle power down time was "
					"modified\n", __func__);
				WARN_ON(1);
			} else {
				reg_val &=
				    ~CSR_DDR_SW_POWER_DOWN_CONTROL_IDLE_TIME_MASK;
				reg_val |= orig_vc4_emi_pwrdn_idle_time;
				writel(reg_val, KONA_MEMC1_NS_VA +
				       CSR_DDR_SW_POWER_DOWN_CONTROL_OFFSET);
			}
		}

		enabled = false;
	}
}

#ifdef CONFIG_DEBUG_FS

static struct clk *uartb_clk;
static int clk_active = 1;
struct delayed_work uartb_wq;

static void uartb_wq_handler(struct work_struct *work)
{
	if (force_retention) {
		if (!uartb_clk)
			uartb_clk = clk_get(NULL, "uartb_clk");
		clk_disable(uartb_clk);
		clk_active = 0;
	} else {		/*releases the WFI to be active again... */
		uart_up_delay = 0;
	}
}

void uartb_pwr_mgr_event_cb(u32 event_id, void *param)
{
	if (force_retention) {
		if (!clk_active) {
			if (!uartb_clk)
				uartb_clk = clk_get(NULL, "uartb_clk");
			clk_enable(uartb_clk);
			clk_active = 1;
		}
		cancel_delayed_work_sync(&uartb_wq);
		schedule_delayed_work(&uartb_wq, msecs_to_jiffies(3000));
	} else {		/* After the UART interrupt is detected,
				 * the system will be out of
				 * retention for 3 seconds.*/
		uart_up_delay = 1;
		cancel_delayed_work_sync(&uartb_wq);
		schedule_delayed_work(&uartb_wq, msecs_to_jiffies(3000));
	}
}

DEFINE_SIMPLE_ATTRIBUTE(set_cp_idle_fops,
			NULL, put_CPSubsystem_to_sleep, "%llu\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
void rpc_event_suspend(struct early_suspend *h)
{
#ifdef CONFIG_BCM_MODEM
	BcmRpc_SetApSleep(1);
#endif
}

void rpc_event_resume(struct early_suspend *h)
{
#ifdef CONFIG_BCM_MODEM
	BcmRpc_SetApSleep(0);
#endif
}
#endif

static struct dentry *dent_capri_pm_root_dir;
int __init capri_pm_debug_init(void)
{
	INIT_DELAYED_WORK(&uartb_wq, uartb_wq_handler);

	pwr_mgr_register_event_handler(UBRX_EVENT,
				       uartb_pwr_mgr_event_cb, NULL);

	/* create root clock dir /clock */
	dent_capri_pm_root_dir = debugfs_create_dir("capri_pm", 0);
	if (!dent_capri_pm_root_dir)
		return -ENOMEM;
	if (!debugfs_create_u32("pm_debug", S_IRUGO | S_IWUSR,
				dent_capri_pm_root_dir, (int *)&pm_debug))
		return -ENOMEM;

	if (!debugfs_create_u32("pm_en_self_refresh", S_IRUGO | S_IWUSR,
				dent_capri_pm_root_dir,
				(int *)&pm_en_self_refresh))
		return -ENOMEM;

	if (!debugfs_create_u32("force_retention", S_IRUGO | S_IWUSR,
				dent_capri_pm_root_dir,
				(int *)&force_retention))
		return -ENOMEM;

	if (!debugfs_create_u32("enable_test", S_IRUGO | S_IWUSR,
				dent_capri_pm_root_dir, (int *)&enable_test))
		return -ENOMEM;

	if (!debugfs_create_file("cp_idle",
				 S_IWUSR | S_IRUSR, dent_capri_pm_root_dir,
				 NULL, &set_cp_idle_fops))
		return -ENOMEM;

	return 0;
}

late_initcall(capri_pm_debug_init);

#endif

#ifndef CONFIG_CAPRI_DELAYED_PM_INIT
device_initcall(capri_pm_init);
#endif
