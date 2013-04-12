/**********************************************************************
*
* @file capri_profiler.c
* CAPRI Profiler modules
*
* Copyright 2012 Broadcom Corporation.  All rights reserved.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a
* license other than the GPL, without Broadcom's express prior written
* consent.
***********************************************************************/
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <plat/clock.h>
#include <mach/rdb/brcm_rdb_sysmap.h>
#include <mach/rdb/brcm_rdb_kpm_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kps_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_khubaon_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_root_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_khub_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kproc_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_bmdm_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_dsp_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_root_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kproc_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_khub_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_khubaon_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kpm_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_kps_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_pwrmgr.h>
#include <plat/profiler.h>
#include <plat/ccu_profiler.h>
#include <plat/pi_profiler.h>

DEFINE_CCU_PROFILER(khub) = {
	.profiler = {
	.name = "ccu_khub",.owner = THIS_MODULE,}
,.clk_dev_id = KHUB_CCU_CLK_NAME_STR,.auto_gate_sel1_offset = 0};

DEFINE_CCU_PROFILER(root) = {
	.profiler = {
	.name = "ccu_root",.owner = THIS_MODULE,}
,.clk_dev_id = ROOT_CCU_CLK_NAME_STR,.auto_gate_sel1_offset = 0};

DEFINE_CCU_PROFILER(kpm) = {
	.profiler = {
	.name = "ccu_kpm",.owner = THIS_MODULE,}
,.clk_dev_id = KPM_CCU_CLK_NAME_STR,.auto_gate_sel1_offset =
	    0,.clk_req_sel1_offset = 0,};

DEFINE_CCU_PROFILER(kps) = {
	.profiler = {
	.name = "ccu_kps",.owner = THIS_MODULE,}
,.clk_dev_id = KPS_CCU_CLK_NAME_STR,.auto_gate_sel1_offset = 0,};

DEFINE_CCU_PROFILER(kproc) = {
	.profiler = {
	.name = "ccu_kproc",.owner = THIS_MODULE,}
,.clk_dev_id = KPROC_CCU_CLK_NAME_STR,.ctrl_offset =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_CTL_OFFSET,.
	    policy_sel_offset =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_SEL_OFFSET,.
	    auto_gate_sel0_offset =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_SEL_OFFSET,.
	    clk_req_sel0_offset =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_SEL_OFFSET,.
	    counter_offset =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_CNT_OFFSET,.
	    cntr_start_mask =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_CTL_KPROC_CCU_PROF_CNT_START_MASK,.
	    cntrl_counter_mask =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_CTL_KPROC_CCU_PROF_CNT_CTRL_MASK,.
	    policy_sel_mask =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_SEL_KPROC_CCU_PROF_POLICY_SEL_MASK,.
	    auto_gate_sel0_mask =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_SEL_KPROC_CCU_PROF_AUTOGATING_SEL_MASK,.
	    clk_req_sel0_mask =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_SEL_KPROC_CCU_PROF_CLK_REQ_SEL_MASK,.
	    counter_mask =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_CNT_KPROC_CCU_PROF_CNT_MASK,.
	    cntrl_start_shift =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_CTL_KPROC_CCU_PROF_CNT_START_SHIFT,.
	    cntrl_counter_shift =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_CTL_KPROC_CCU_PROF_CNT_CTRL_SHIFT,.
	    policy_sel_shift =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_SEL_KPROC_CCU_PROF_POLICY_SEL_SHIFT,.
	    auto_gate_sel0_shift =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_SEL_KPROC_CCU_PROF_AUTOGATING_SEL_SHIFT,.
	    clk_req_sel0_shift =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_SEL_KPROC_CCU_PROF_CLK_REQ_SEL_SHIFT,.
	    counter_shift =
	    KPROC_CLK_MGR_REG_KPROC_CCU_PROF_CNT_KPROC_CCU_PROF_CNT_SHIFT,};

struct ccu_profiler *capri_ccu_profiler_tbl[] = {
	&CCU_PROFILER(khub),
	&CCU_PROFILER(root),
	&CCU_PROFILER(kpm),
	&CCU_PROFILER(kps),
	&CCU_PROFILER(kproc),
};

/**
 * ARM island PI ON Profiler
 */
DEFINE_PI_PROFILER(arm) = {
	.profiler = {
.name = "pi_arm",.owner = THIS_MODULE,},.pi_prof_addr_base =
	    HW_IO_PHYS_TO_VIRT(PWRMGR_BASE_ADDR),.pi_id =
	    PI_MGR_PI_ID_ARM_CORE,.counter_offset =
	    PWRMGR_PI_ARM_CORE_ON_COUNTER_OFFSET,.counter_en_mask =
	    PWRMGR_PI_ARM_CORE_ON_COUNTER_PI_ARM_CORE_ON_COUNTER_ENABLE_MASK,.
	    counter_mask =
	    PWRMGR_PI_ARM_CORE_ON_COUNTER_PI_ARM_CORE_ON_COUNTER_MASK,.
	    overflow_mask =
	    PWRMGR_PI_ARM_CORE_ON_COUNTER_PI_ARM_CORE_ON_COUNTER_OVERFLOW_MASK,.
	    counter_en_shift =
	    PWRMGR_PI_ARM_CORE_ON_COUNTER_PI_ARM_CORE_ON_COUNTER_ENABLE_SHIFT,.
	    counter_shift =
	    PWRMGR_PI_ARM_CORE_ON_COUNTER_PI_ARM_CORE_ON_COUNTER_SHIFT,.
	    counter_clear_offset =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_OFFSET,.counter_clear_mask =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_MASK,.
	    counter_clear_shift =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_SHIFT,};

/**
 * ARM Subsystem PI ON profiler
 */
DEFINE_PI_PROFILER(armsubsys) = {
	.profiler = {
.name = "pi_armsub",.owner = THIS_MODULE,},.pi_prof_addr_base =
	    HW_IO_PHYS_TO_VIRT(PWRMGR_BASE_ADDR),.pi_id =
	    PI_MGR_PI_ID_ARM_SUB_SYSTEM,.counter_offset =
	    PWRMGR_PI_ARM_SUBSYSTEM_ON_COUNTER_OFFSET,.counter_en_mask =
	    PWRMGR_PI_ARM_SUBSYSTEM_ON_COUNTER_PI_ARM_SUBSYSTEM_ON_COUNTER_ENABLE_MASK,.
	    counter_mask =
	    PWRMGR_PI_ARM_SUBSYSTEM_ON_COUNTER_PI_ARM_SUBSYSTEM_ON_COUNTER_MASK,.
	    overflow_mask =
	    PWRMGR_PI_ARM_SUBSYSTEM_ON_COUNTER_PI_ARM_SUBSYSTEM_ON_COUNTER_OVERFLOW_MASK,.
	    counter_en_shift =
	    PWRMGR_PI_ARM_SUBSYSTEM_ON_COUNTER_PI_ARM_SUBSYSTEM_ON_COUNTER_ENABLE_SHIFT,.
	    counter_shift =
	    PWRMGR_PI_ARM_SUBSYSTEM_ON_COUNTER_PI_ARM_SUBSYSTEM_ON_COUNTER_SHIFT,.
	    counter_clear_offset =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_OFFSET,.counter_clear_mask =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_MASK,.
	    counter_clear_shift =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_SHIFT,};

/**
 * Modem PI ON profiler
 */
DEFINE_PI_PROFILER(modem) = {
	.profiler = {
.name = "pi_modem",.owner = THIS_MODULE,},.pi_prof_addr_base =
	    HW_IO_PHYS_TO_VIRT(PWRMGR_BASE_ADDR),.pi_id =
	    PI_MGR_PI_ID_MODEM,.counter_offset =
	    PWRMGR_PI_MODEM_ON_COUNTER_OFFSET,.counter_en_mask =
	    PWRMGR_PI_MODEM_ON_COUNTER_PI_MODEM_ON_COUNTER_ENABLE_MASK,.
	    counter_mask =
	    PWRMGR_PI_MODEM_ON_COUNTER_PI_MODEM_ON_COUNTER_MASK,.
	    overflow_mask =
	    PWRMGR_PI_MODEM_ON_COUNTER_PI_MODEM_ON_COUNTER_OVERFLOW_MASK,.
	    counter_en_shift =
	    PWRMGR_PI_MODEM_ON_COUNTER_PI_MODEM_ON_COUNTER_ENABLE_SHIFT,.
	    counter_shift =
	    PWRMGR_PI_MODEM_ON_COUNTER_PI_MODEM_ON_COUNTER_SHIFT,.
	    counter_clear_offset =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_OFFSET,.counter_clear_mask =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_MASK,.
	    counter_clear_shift =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_SHIFT,};

/**
 * MM PI ON profiler
 */
DEFINE_PI_PROFILER(mm) = {
	.profiler = {
.name = "pi_mm",.owner = THIS_MODULE,},.pi_prof_addr_base =
	    HW_IO_PHYS_TO_VIRT(PWRMGR_BASE_ADDR),.pi_id =
	    PI_MGR_PI_ID_MM,.counter_offset =
	    PWRMGR_PI_MM_ON_COUNTER_OFFSET,.counter_en_mask =
	    PWRMGR_PI_MM_ON_COUNTER_PI_MM_ON_COUNTER_ENABLE_MASK,.
	    counter_mask =
	    PWRMGR_PI_MM_ON_COUNTER_PI_MM_ON_COUNTER_MASK,.
	    overflow_mask =
	    PWRMGR_PI_MM_ON_COUNTER_PI_MM_ON_COUNTER_OVERFLOW_MASK,.
	    counter_en_shift =
	    PWRMGR_PI_MM_ON_COUNTER_PI_MM_ON_COUNTER_ENABLE_SHIFT,.
	    counter_shift =
	    PWRMGR_PI_MM_ON_COUNTER_PI_MM_ON_COUNTER_SHIFT,.
	    counter_clear_offset =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_OFFSET,.counter_clear_mask =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_MASK,.
	    counter_clear_shift =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_SHIFT,};

/**
 * HUB Switchable PI ON profiler
 */
DEFINE_PI_PROFILER(hub) = {
	.profiler = {
.name = "pi_hub",.owner = THIS_MODULE,},.pi_prof_addr_base =
	    HW_IO_PHYS_TO_VIRT(PWRMGR_BASE_ADDR),.pi_id =
	    PI_MGR_PI_ID_HUB_SWITCHABLE,.counter_offset =
	    PWRMGR_PI_HUB_SWITCHABLE_ON_COUNTER_OFFSET,.
	    counter_en_mask =
	    PWRMGR_PI_HUB_SWITCHABLE_ON_COUNTER_PI_HUB_SWITCHABLE_ON_COUNTER_ENABLE_MASK,.
	    counter_mask =
	    PWRMGR_PI_HUB_SWITCHABLE_ON_COUNTER_PI_HUB_SWITCHABLE_ON_COUNTER_MASK,.
	    overflow_mask =
	    PWRMGR_PI_HUB_SWITCHABLE_ON_COUNTER_PI_HUB_SWITCHABLE_ON_COUNTER_OVERFLOW_MASK,.
	    counter_en_shift =
	    PWRMGR_PI_HUB_SWITCHABLE_ON_COUNTER_PI_HUB_SWITCHABLE_ON_COUNTER_ENABLE_SHIFT,.
	    counter_shift =
	    PWRMGR_PI_HUB_SWITCHABLE_ON_COUNTER_PI_HUB_SWITCHABLE_ON_COUNTER_SHIFT,.
	    counter_clear_offset =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_OFFSET,.counter_clear_mask =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_MASK,.
	    counter_clear_shift =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_SHIFT,};

/**
 * HUB AON PI ON Profiler
 */
DEFINE_PI_PROFILER(aon) = {
	.profiler = {
.name = "pi_aon",.owner = THIS_MODULE,},.pi_prof_addr_base =
	    HW_IO_PHYS_TO_VIRT(PWRMGR_BASE_ADDR),.pi_id =
	    PI_MGR_PI_ID_HUB_AON,.counter_offset =
	    PWRMGR_PI_HUB_AON_ON_COUNTER_OFFSET,.counter_en_mask =
	    PWRMGR_PI_HUB_AON_ON_COUNTER_PI_HUB_AON_ON_COUNTER_ENABLE_MASK,.
	    counter_mask =
	    PWRMGR_PI_HUB_AON_ON_COUNTER_PI_HUB_AON_ON_COUNTER_MASK,.
	    overflow_mask =
	    PWRMGR_PI_HUB_AON_ON_COUNTER_PI_HUB_AON_ON_COUNTER_OVERFLOW_MASK,.
	    counter_en_shift =
	    PWRMGR_PI_HUB_AON_ON_COUNTER_PI_HUB_AON_ON_COUNTER_ENABLE_SHIFT,.
	    counter_shift =
	    PWRMGR_PI_HUB_AON_ON_COUNTER_PI_HUB_AON_ON_COUNTER_SHIFT,.
	    counter_clear_offset =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_OFFSET,.counter_clear_mask =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_MASK,.
	    counter_clear_shift =
	    PWRMGR_PC_PIN_OVERRIDE_CONTROL_CLEAR_PI_COUNTERS_SHIFT,};

static struct pi_profiler *capri_pi_profiler_tbl[] = {
	&PI_PROFILER(arm),
	&PI_PROFILER(armsubsys),
	&PI_PROFILER(modem),
	&PI_PROFILER(mm),
	&PI_PROFILER(hub),
	&PI_PROFILER(aon),
};

static struct platform_device capri_profiler_device = {
	.name = "kona_profiler",
	.id = -1,
	.dev = {
		.platform_data = NULL,
		},
};

static int __init capri_profiler_init(void)
{
	pr_info("%s\n", __func__);
	return platform_device_register(&capri_profiler_device);
}

device_initcall(capri_profiler_init);
static int __init capri_profiler_register(void)
{
	int idx;
	int ret = 0;
	/**
	 * Register all CCU Profilers
	 */
	pr_info("%s\n", __func__);
	for (idx = 0; idx < ARRAY_SIZE(capri_ccu_profiler_tbl); idx++) {
		ret = ccu_profiler_register(capri_ccu_profiler_tbl[idx]);
		if (ret) {
			pr_info("%s:ccu profiler failed\n", __func__);
			return ret;
		}
	}
	/**
	 * Register all PI ON profilers
	 */
	for (idx = 0; idx < ARRAY_SIZE(capri_pi_profiler_tbl); idx++) {
		ret = pi_profiler_register(capri_pi_profiler_tbl[idx]);
		if (ret) {
			pr_info("%s:ccu profiler failed\n", __func__);
			return ret;
		}
	}
	return 0;
}

late_initcall(capri_profiler_register);
