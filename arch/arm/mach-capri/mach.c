/************************************************************************************************/
/*                                                                                              */
/*  Copyright 2010  Broadcom Corporation                                                        */
/*                                                                                              */
/*     Unless you and Broadcom execute a separate written software license agreement governing  */
/*     use of this software, this software is licensed to you under the terms of the GNU        */
/*     General Public License version 2 (the GPL), available at                                 */
/*                                                                                              */
/*          http://www.broadcom.com/licenses/GPLv2.php                                          */
/*                                                                                              */
/*     with the following added to such license:                                                */
/*                                                                                              */
/*     As a special exception, the copyright holders of this software give you permission to    */
/*     link this software with independent modules, and to copy and distribute the resulting    */
/*     executable under terms of your choice, provided that you also meet, for each linked      */
/*     independent module, the terms and conditions of the license of that module.              */
/*     An independent module is a module which is not derived from this software.  The special  */
/*     exception does not apply to any modifications of the software.                           */
/*                                                                                              */
/*     Notwithstanding the above, under no circumstances may you combine this software in any   */
/*     way with any other Broadcom software provided under a license other than the GPL,        */
/*     without Broadcom's express prior written consent.                                        */
/*                                                                                              */
/************************************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cpumask.h>
#include <linux/syscalls.h>
#include <linux/mfd/bcm590xx/core.h>

#include <asm/io.h>
#include <asm/mach/map.h>
#include <asm/hardware/cache-l2x0.h>
#include <mach/rdb/brcm_rdb_scu.h>
#include <mach/rdb/brcm_rdb_kproc_clk_mgr_reg.h>

#include <mach/io_map.h>
#include <mach/clock.h>
#include <mach/gpio.h>
#include <mach/timer.h>
#include <mach/kona.h>
#include <mach/profile_timer.h>
#include <mach/pinmux.h>
#include <mach/sec_api.h>
#include <mach/chipregHw_inline.h>

static void poweroff(void)
{
#ifdef CONFIG_MFD_BCM_PMU590XX
	bcm590xx_shutdown();
#endif

	while (1) ;
}

static void restart(char mode, const char *cmd)
{
	arm_machine_restart('h', cmd);
}

#ifdef CONFIG_CACHE_L2X0

/* Default L2 settings */
static int l2off = 0;
static int l2_non_secure_access = 1;
static int l2_d_prefetch = 1;
static int l2_i_prefetch = 1;
static int l2_early_bresp = 1;
static int l2_disable_linefill = 0;
static int l2_disable_idle_delay = 1;

static int __init l2off_setup(char *str)
{
	l2off = 1;
	return 1;
}

__setup("l2off", l2off_setup);

static int __init l2_d_prefetch_setup(char *str)
{
	get_option(&str, &l2_d_prefetch);
	return 1;
}

__setup("l2_dprefetch=", l2_d_prefetch_setup);

static int __init l2_i_prefetch_setup(char *str)
{
	get_option(&str, &l2_i_prefetch);
	return 1;
}

__setup("l2_iprefetch=", l2_i_prefetch_setup);

static int __init l2_early_bresp_setup(char *str)
{
	get_option(&str, &l2_early_bresp);
	return 1;
}

__setup("l2_early_bresp=", l2_early_bresp_setup);

static int __init l2_disable_linefill_setup(char *str)
{
	l2_disable_linefill = 1;
	return 1;
}

__setup("l2_disable_linefill", l2_disable_linefill_setup);

static int __init l2_disable_idle_delay_setup(char *str)
{
	l2_disable_idle_delay = 1;
	return 1;
}

__setup("l2_disable_idle_delay", l2_disable_idle_delay_setup);

static int __init mach_l2x0_init(void)
{
	void __iomem *l2cache_base;
	uint32_t aux_val = 0;
	uint32_t aux_mask = 0xC200ffff;

	if (l2off) {
		/*  cmdline argument l2off will turn off l2 cache even if configured on */
		printk
		    ("%s: Warning: L2X0 *not* enabled due to l2off cmdline override\n",
		     __func__);
		return 0;
	}

	if (l2_disable_idle_delay) {
		/*
		 * Clearing bit 0 of the KPROC_CLK_MGR_REG_ARM_SYS_IDLE_DLY (missing from rdb)
		 * apparently also resolves our WFI/SMP/L2 problem.
		 */

		void __iomem *arm_sys_idle_delay =
		    (void __iomem *)(KONA_PROC_CLK_VA +
				     KPROC_CLK_MGR_REG_ARM_SYS_IDLE_DLY_OFFSET);
		void __iomem *kproc_wr_access =
		    (void __iomem *)(KONA_PROC_CLK_VA +
				     KPROC_CLK_MGR_REG_WR_ACCESS_OFFSET);

		writel((0xA5A5 << KPROC_CLK_MGR_REG_WR_ACCESS_PASSWORD_SHIFT)
		       | KPROC_CLK_MGR_REG_WR_ACCESS_CLKMGR_ACC_MASK,
		       kproc_wr_access);

		writel(readl(arm_sys_idle_delay) & ~1, arm_sys_idle_delay);

		pr_info("Disabling kproc_clk_mgr_reg_ARM_SYS_IDLE_DLY 0x%08x\n",
			readl(arm_sys_idle_delay));
	} else if (l2_disable_linefill) {
		/*
		 * Disabling the L2 linefill seems to allow the SCU standby to be enabled and allow
		 * both cores to do WFIs, so we add this as a runtime option. I suspect that this
		 * will have significant performance impact, so disabling SCU standby will remain
		 * the default.
		 */

		void __iomem *l2_debug_ctrl =
		    (void __iomem *)(KONA_L2C_VA + L2X0_DEBUG_CTRL);

		writel(readl(l2_debug_ctrl) | 1, l2_debug_ctrl);

		pr_info("Disabling L2 cache linefill: L2_debug_ctrl 0x%08x\n",
			readl(l2_debug_ctrl));
	} else {
		/*
		 * Clear the Standby enable bit. When this bit is set and both cores do a WFI, it
		 * causes the L2 cache to misbehave (i.e. hangs the system).
		 *
		 * With this bit clear, the WFI workaround isn't needed.
		 */

		void __iomem *scu_control_reg =
		    (void __iomem *)(KONA_SCU_VA + SCU_CONTROL_OFFSET);

		writel(readl(scu_control_reg) &
		       ~SCU_CONTROL_SCU_STANDBY_EN_MASK, scu_control_reg);

		pr_info("SCU: Disabled Standby Mode. SCU Control 0x%08x\n",
			readl(scu_control_reg));
	}

	l2cache_base = (void __iomem *)(KONA_L2C_VA);

	/*
	 * Zero bits in aux_mask will be cleared
	 * One  bits in aux_val  will be set
	 */

	aux_val |= (1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT);	/* 16-way cache */
	aux_val |= ((l2_non_secure_access ? 1 : 0) << L2X0_AUX_CTRL_NS_INT_CTRL_SHIFT);	/* Allow non-secure access */
	aux_val |= ((l2_d_prefetch ? 1 : 0) << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT);	/* Data prefetch */
	aux_val |= ((l2_i_prefetch ? 1 : 0) << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT);	/* Instruction prefetch */
	aux_val |= ((l2_early_bresp ? 1 : 0) << L2X0_AUX_CTRL_EARLY_BRESP_SHIFT);	/* Early BRESP */
	aux_val |= (2 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT);	/* 32KB */

	/*
	 * Set bit 22 in the auxiliary control register. If this bit
	 * is cleared, PL310 treats Normal Shared Non-cacheable
	 * accesses as Cacheable no-allocate.
	 */
	aux_val |= 1 << L2X0_AUX_CTRL_SHARE_OVERRIDE_SHIFT;

#ifdef CONFIG_KONA_SECURE_MONITOR_CALL
	/* TODO: Need secure call to set the aux_val prior to l2 enable */

	/* TURN ON THE L2 CACHE */
#ifdef CONFIG_MOBICORE_DRIVER
	secure_api_call(SMC_CMD_L2X0SETUP2, 0, aux_val, aux_mask, 0);
	secure_api_call(SMC_CMD_L2X0INVALL, 0, 0, 0, 0);
	secure_api_call(SMC_CMD_L2X0CTRL, 1, 0, 0, 0);
#else
	secure_api_call_init();
	secure_api_call(SSAPI_ENABLE_L2_CACHE, 0, 0, 0, 0);
#endif
#endif

	l2x0_init(l2cache_base, aux_val, aux_mask);

	return 0;
}
#endif

#ifdef CONFIG_KONA_ATAG_DT
/* Capri has 6 banks of GPIO pins */
uint32_t dt_pinmux_gpio_mask[6] = { 0, 0, 0, 0, 0, 0 };

uint32_t dt_gpio[192];
#endif

static int __init mach_init(void)
{
	pm_power_off = poweroff;
	arm_pm_restart = restart;

#ifdef CONFIG_CACHE_L2X0
	mach_l2x0_init();
#endif

#ifdef CONFIG_HAVE_CLK
	//mach_clock_init();
#endif

//#ifndef CONFIG_MACH_CAPRI_FPGA
	pinmux_init();
//#endif

#ifdef CONFIG_KONA_ATAG_DT
	printk(KERN_INFO
	       "pinmux_gpio_mask: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
	       dt_pinmux_gpio_mask[0], dt_pinmux_gpio_mask[1],
	       dt_pinmux_gpio_mask[2], dt_pinmux_gpio_mask[3],
	       dt_pinmux_gpio_mask[4], dt_pinmux_gpio_mask[5]);
#endif

	/* There are 6 banks of GPIO pins */
	kona_gpio_init(6);
	return 0;
}

early_initcall(mach_init);

static void __init timer_init(void)
{
	struct gp_timer_setup gpt_setup;

#ifdef CONFIG_HAVE_ARM_TWD
	extern void __iomem *twd_base;
	twd_base = __io(KONA_PTIM_VA);
#endif

	/*
	 * IMPORTANT:
	 * If we have to use slave-timer as system timer, two modifications are required
	 * 1) modify the name of timer as, gpt_setup.name = "slave-timer";
	 * 2) By default when the clock manager comes up it disables most of
	 *    the clock. So if we switch to slave-timer we should prevent the
	 *    clock manager from doing this. So, modify plat-kona/include/mach/clock.h
	 *
	 * By default aon-timer as system timer the following is the config
	 * #define BCM2165x_CLK_TIMERS_FLAGS     (TYPE_PERI_CLK | SW_GATE | DISABLE_ON_INIT)
	 * #define BCM2165x_CLK_HUB_TIMER_FLAGS  (TYPE_PERI_CLK | SW_GATE)
	 *
	 * change it as follows to use slave timer as system timer
	 *
	 * #define BCM2165x_CLK_TIMERS_FLAGS     (TYPE_PERI_CLK | SW_GATE)
	 * #define BCM2165x_CLK_HUB_TIMER_FLAGS  (TYPE_PERI_CLK | SW_GATE | DISABLE_ON_INIT)
	 */
	gpt_setup.name = "aon-timer";
	gpt_setup.ch_num = 0;
	gpt_setup.rate = CLOCK_TICK_RATE;

	/* Call the init function of timer module */
	gp_timer_init(&gpt_setup);
	profile_timer_init(IOMEM(KONA_PROFTMR_VA));
}

struct sys_timer kona_timer = {
	.init = timer_init,
};

EXPORT_SYMBOL(sys_open);
EXPORT_SYMBOL(sys_read);
