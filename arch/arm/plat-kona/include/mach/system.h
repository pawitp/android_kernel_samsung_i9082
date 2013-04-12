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

#ifndef __PLAT_KONA_SYSTEM_H
#define __PLAT_KONA_SYSTEM_H

#include <linux/io.h>
#include <mach/io_map.h>
#include <mach/rdb/brcm_rdb_gicdist.h>
#include <mach/rdb/brcm_rdb_root_rst_mgr_reg.h>
#if defined(CONFIG_ARCH_CAPRI)
#include <mach/rdb/brcm_rdb_secwatchdog_adrmod.h>
#endif

#ifdef CONFIG_BCM_IDLE_PROFILER
#include <mach/profile_timer.h>
#endif

#ifdef CONFIG_BCM_KNLLOG_IRQ
#include <linux/broadcom/knllog.h>
#endif

#ifdef CONFIG_BCM_IDLE_PROFILER
DECLARE_PER_CPU(u32, idle_count);
#endif

static void arch_idle(void)
{
#ifdef CONFIG_BCM_IDLE_PROFILER
	u32 idle_enter, idle_leave;
#endif

#ifdef CONFIG_BCM_KNLLOG_IRQ
	if (gKnllogIrqSchedEnable & KNLLOG_THREAD)
		KNLLOGCALL("schedule", "0 -> 99999");
#endif

#ifdef CONFIG_BCM_IDLE_PROFILER
	idle_enter = timer_get_tick_count();
#endif
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	cpu_do_idle();

#ifdef CONFIG_BCM_IDLE_PROFILER
	idle_leave = timer_get_tick_count();
	get_cpu_var(idle_count) += (idle_leave - idle_enter);
	put_cpu_var(idle_count);
#endif

#ifdef CONFIG_BCM_KNLLOG_IRQ
	if (gKnllogIrqSchedEnable & KNLLOG_THREAD)
		KNLLOGCALL("schedule", "99999 -> 0");
#endif
}

static void arch_reset(char mode, const char *cmd)
{
	uint32_t val;

	/*
	 * Disable GIC interrupt distribution.
	 */
	__raw_writel(0, KONA_GICDIST_VA + GICDIST_ENABLE_S_OFFSET);

#ifdef CONFIG_ARCH_CAPRI
	/* Enable watchdog 2 with s very short timeout. */
	val = __raw_readl(KONA_SECWD2_VA + SECWATCHDOG_ADRMOD_SDOGCR_OFFSET);
	val &= SECWATCHDOG_ADRMOD_SDOGCR_RESERVED_MASK |
		SECWATCHDOG_ADRMOD_SDOGCR_WD_LOAD_FLAG_MASK;
	val |= SECWATCHDOG_ADRMOD_SDOGCR_EN_MASK |
		SECWATCHDOG_ADRMOD_SDOGCR_SRSTEN_MASK |
		(0x8 << SECWATCHDOG_ADRMOD_SDOGCR_CLKS_SHIFT) |
		(0x8 << SECWATCHDOG_ADRMOD_CHKLMCR_LCR_CHK_LOW_LOCK_SHIFT);
	__raw_writel(val, KONA_SECWD2_VA + SECWATCHDOG_ADRMOD_SDOGCR_OFFSET);

	while (1)
		;

	/*
	 * The following code sequence uses Kona root CCU reset. Disabled for
	 * now until it's working reliably
	 */

	/* enable reset register access */
	val = __raw_readl(KONA_ROOT_RST_VA +
			ROOT_RST_MGR_REG_WR_ACCESS_OFFSET);
	/* retain access mode */
	val &= ROOT_RST_MGR_REG_WR_ACCESS_PRIV_ACCESS_MODE_MASK;
	/* set password */
	val |= (0xA5A5 << ROOT_RST_MGR_REG_WR_ACCESS_PASSWORD_SHIFT);
	/* set access enable */
	val |= ROOT_RST_MGR_REG_WR_ACCESS_RSTMGR_ACC_MASK;
	__raw_writel(val, KONA_ROOT_RST_VA +
			ROOT_RST_MGR_REG_WR_ACCESS_OFFSET);

	/* trigger reset */
	val = __raw_readl(KONA_ROOT_RST_VA +
			ROOT_RST_MGR_REG_CHIP_SOFT_RSTN_OFFSET);
	/* retain access mode */
	val &= ROOT_RST_MGR_REG_CHIP_SOFT_RSTN_PRIV_ACCESS_MODE_MASK;
	/* mask out reset bit (zero to reset) */
	val &= ~ROOT_RST_MGR_REG_CHIP_SOFT_RSTN_CHIP_SOFT_RSTN_MASK;
	__raw_writel(val, KONA_ROOT_RST_VA +
			ROOT_RST_MGR_REG_CHIP_SOFT_RSTN_OFFSET);
#else
	/* enable reset register access */
	val = __raw_readl(KONA_ROOT_RST_VA + ROOT_RST_MGR_REG_WR_ACCESS_OFFSET);
	/* retain access mode */
	val &= ROOT_RST_MGR_REG_WR_ACCESS_PRIV_ACCESS_MODE_MASK;
	/* set password */
	val |= (0xA5A5 << ROOT_RST_MGR_REG_WR_ACCESS_PASSWORD_SHIFT);
	/* set access enable */
	val |= ROOT_RST_MGR_REG_WR_ACCESS_RSTMGR_ACC_MASK;
	__raw_writel(val, KONA_ROOT_RST_VA + ROOT_RST_MGR_REG_WR_ACCESS_OFFSET);

	/* trigger reset */
	val =
	    __raw_readl(KONA_ROOT_RST_VA +
			ROOT_RST_MGR_REG_CHIP_SOFT_RSTN_OFFSET);
	/* retain access mode */
	val &= ROOT_RST_MGR_REG_CHIP_SOFT_RSTN_PRIV_ACCESS_MODE_MASK;
	__raw_writel(val,
		     KONA_ROOT_RST_VA + ROOT_RST_MGR_REG_CHIP_SOFT_RSTN_OFFSET);
#endif

	while (1)
		;
}

#endif /*__PLAT_KONA_SYSTEM_H */
