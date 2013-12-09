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

#include <linux/version.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <asm/mach/arch.h>
#include <mach/io_map.h>
#include <linux/io.h>

#include<mach/clock.h>
#include<plat/pi_mgr.h>
#include<mach/pi_mgr.h>
#include<mach/pwr_mgr.h>
#include<plat/pwr_mgr.h>
#include <mach/rdb/brcm_rdb_bmdm_pwrmgr.h>
#include <plat/kona_cpufreq_drv.h>
#include <plat/kona_reset_reason.h>
#ifdef CONFIG_DEBUG_FS
#include <mach/rdb/brcm_rdb_chipreg.h>
#include <mach/rdb/brcm_rdb_padctrlreg.h>
#include <mach/chip_pinmux.h>
#include <mach/pinmux.h>
#endif

#include <linux/i2c.h>
#include <linux/i2c-kona.h>
#ifdef CONFIG_MFD_BCM_PWRMGR_SW_SEQUENCER
#include <linux/delay.h>
#include <mach/rdb/brcm_rdb_sysmap.h>
#include <mach/rdb/brcm_rdb_khubaon_clk_mgr_reg.h>
#include <../drivers/i2c/busses/i2c-bsc.h>
#include <plat/clock.h>
#endif
#ifdef CONFIG_KONA_AVS
#include <plat/kona_avs.h>
#endif
#include <mach/pm.h>

#define VLT_LUT_SIZE	16
static int delayed_init_complete;

#ifdef CONFIG_CAPRI_DELAYED_PM_INIT
struct pi_mgr_qos_node delay_arm_lpm;

struct pm_late_init {
	int dummy;
};

#define __param_check_pm_late_init(name, p, type) \
	static inline struct type *__check_##name(void) { return (p); }

#define param_check_pm_late_init(name, p) \
	__param_check_pm_late_init(name, p, pm_late_init)

static int param_set_pm_late_init(const char *val,
				  const struct kernel_param *kp);
static struct kernel_param_ops param_ops_pm_late_init = {
	.set = param_set_pm_late_init,
};

static struct pm_late_init pm_late_init;
module_param_named(pm_late_init, pm_late_init, pm_late_init, S_IWUSR | S_IWGRP);
#endif

#ifdef CONFIG_DEBUG_FS
const char *_capri__event2str[] = {
	__stringify(LCDTE_EVENT),
	__stringify(SSP3_FS_EVENT),
	__stringify(SSP3_RXD_EVENT),
	__stringify(SSP3_CLK_EVENT),
	__stringify(SSP2_FS_EVENT),
	__stringify(SSP2_RXD_EVENT),
	__stringify(SSP2_CLK_EVENT),
	__stringify(SSP4_FS_EVENT),
	__stringify(SSP4_RXD_EVENT),
	__stringify(SSP4_CLK_EVENT),
	__stringify(SSP0_FS_EVENT),
	__stringify(SSP0_RXD_EVENT),
	__stringify(SSP0_CLK_EVENT),
	__stringify(DIGCLKREQ_EVENT),
	__stringify(ANA_SYS_REQ_EVENT),
	__stringify(HSIC2_UTMI_LINE_STATE0_EVENT),
	__stringify(UBRX_EVENT),	/*0X10 */
	__stringify(UBCTSN_EVENT),
	__stringify(UB2RX_EVENT),
	__stringify(UB2CTSN_EVENT),
	__stringify(UB3RX_EVENT),
	__stringify(UB3CTSN_EVENT),
	__stringify(UB4RX_EVENT),
	__stringify(UB4CTSN_EVENT),
	__stringify(SIMDET_EVENT),
	__stringify(SIM2DET_EVENT),
	__stringify(GPIO_91_EVENT),
	__stringify(GPIO_175_EVENT),
	__stringify(SDIO4_DATA_3_EVENT),
	__stringify(SDIO4_DATA_1_EVENT),
	__stringify(SDDAT3_EVENT),
	__stringify(SDDAT1_EVENT),
	__stringify(SSP2_RXD_1_EVENT),	/*0X20 */
	__stringify(HUB2MM_WAKEUP_EVENT),
	__stringify(SWCLKTCK_EVENT),
	__stringify(SWDIOTMS_EVENT),
	__stringify(KEY_CI_0_EVENT),
	__stringify(KEY_CI_1_EVENT),
	__stringify(KEY_CI_2_EVENT),
	__stringify(KEY_CI_3_EVENT),
	__stringify(KEY_CI_4_EVENT),
	__stringify(KEY_CI_5_EVENT),
	__stringify(KEY_CI_6_EVENT),
	__stringify(KEY_CI_7_EVENT),
	__stringify(CAWAKE_EVENT),
	__stringify(CAREADY_EVENT),
	__stringify(CAFLAG_EVENT),
	__stringify(BATRM_EVENT),
	__stringify(USBDP_EVENT),	/*0X30 */
	__stringify(USBDN_EVENT),
	__stringify(USBH2_PHY_RESUME_EVENT),
	__stringify(PMU_INT_A_EVENT),
	__stringify(GPIO_8_A_EVENT),
	__stringify(GPIO_9_A_EVENT),
	__stringify(GPIO_10_A_EVENT),
	__stringify(GPIO_11_A_EVENT),
	__stringify(GPIO_12_A_EVENT),
	__stringify(GPIO_13_A_EVENT),
	__stringify(GPIO_54_A_EVENT),
	__stringify(GPIO_64_A_EVENT),
	__stringify(GPIO_97_A_EVENT),
	__stringify(GPIO_102_A_EVENT),
	__stringify(GPIO_114_A_EVENT),
	__stringify(GPIO_56_A_EVENT),
	__stringify(GPIO_140_A_EVENT),	/*0X40 */
	__stringify(GPIO96_A_EVENT),
	__stringify(GPIO_142_A_EVENT),
	__stringify(GPIO_1_A_EVENT),
	__stringify(GPIO_2_A_EVENT),
	__stringify(GPIO_92_A_EVENT),
	__stringify(GPIO_3_A_EVENT),
	__stringify(GPIO_4_A_EVENT),
	__stringify(GPIO_5_A_EVENT),
	__stringify(GPIO_6_A_EVENT),
	__stringify(GPIO_7_A_EVENT),
	__stringify(SSP5_CLK_A_EVENT),
	__stringify(SSP5_RXD_A_EVENT),
	__stringify(PHY_RESUME_O_AON_1_A_EVENT),
	__stringify(MODEMBUS_ACTIVE_A_EVENT),
	__stringify(SSP5_FS_A_EVENT),
	__stringify(SSP6_CLK_A_EVENT),	/*0X50 */
	__stringify(SSP6_RXD_A_EVENT),
	__stringify(ARM_POWER_ON_KONA_FABRIC_A_EVENT),
	__stringify(SSP6_FS_A_EVENT),
	__stringify(USBH2_PHY_RESUME_A_EVENT),
	__stringify(PMU_INT_B_EVENT),
	__stringify(GPIO_8_B_EVENT),
	__stringify(GPIO_9_B_EVENT),
	__stringify(GPIO_10_B_EVENT),
	__stringify(GPIO_11_B_EVENT),
	__stringify(GPIO_12_B_EVENT),
	__stringify(GPIO_13_B_EVENT),
	__stringify(GPIO_54_B_EVENT),
	__stringify(GPIO_64_B_EVENT),
	__stringify(GPIO_97_B_EVENT),
	__stringify(GPIO_102_B_EVENT),
	__stringify(GPIO_114_B_EVENT),	/*0X60 */
	__stringify(GPIO_56_B_EVENT),
	__stringify(GPIO_140_B_EVENT),
	__stringify(GPIO96_B_EVENT),
	__stringify(GPIO_142_B_EVENT),
	__stringify(GPIO_1_A_B_EVENT),
	__stringify(GPIO_2_A_B_EVENT),
	__stringify(GPIO_92_B_EVENT),
	__stringify(GPIO_3_B_EVENT),
	__stringify(GPIO_4_B_EVENT),
	__stringify(GPIO_5_B_EVENT),
	__stringify(GPIO_6_B_EVENT),
	__stringify(GPIO_7_B_EVENT),
	__stringify(SSP5_CLK_B_EVENT),
	__stringify(SSP5_RXD_B_EVENT),
	__stringify(PHY_RESUME_O_AON_1_B_EVENT),
	__stringify(MODEMBUS_ACTIVE_B_EVENT),	/*0X70 */
	__stringify(SSP5_FS_B_EVENT),
	__stringify(SSP6_CLK_B_EVENT),
	__stringify(SSP6_RXD_B_EVENT),
	__stringify(ARM_POWER_ON_KONA_FABRIC_B_EVENT),
	__stringify(SSP6_FS_B_EVENT),
	__stringify(USBH2_PHY_RESUME_B_EVENT),
	__stringify(COMMON_TIMER_0_EVENT),
	__stringify(COMMON_TIMER_1_EVENT),
	__stringify(COMMON_TIMER_2_EVENT),
	__stringify(COMMON_TIMER_3_EVENT),
	__stringify(COMMON_TIMER_4_EVENT),
	__stringify(HSIC2_UTMI_LINE_STATE1_EVENT),
	__stringify(COMMON_INT_TO_AC_EVENT),
	__stringify(COMMON_INT_TO_MM_EVENT),
	__stringify(TZCFG_INT_TO_AC_EVENT),
	__stringify(DMA_REQUEST_EVENT),	/*0X80 */
	__stringify(MODEM1_EVENT),
	__stringify(MODEM2_EVENT),
	__stringify(MODEM_UART_EVENT),
	__stringify(BRIDGE_TO_AC_EVENT),
	__stringify(BRIDGE_TO_MODEM_EVENT),
	__stringify(VREQ_NONZERO_PI_MODEM_EVENT),
	__stringify(USBOTG_EVENT),
	__stringify(GPIO_0_EVENT),
	__stringify(GPIO_14_EVENT),
	__stringify(ACI_EVENT),
	__stringify(SOFTWARE_0_EVENT),
	__stringify(SOFTWARE_1_EVENT),
	__stringify(SOFTWARE_2_EVENT),
	__stringify(VPM_WAKEUP_EVENT),
	__stringify(ESW_WAKEUP_EVENT),
	__stringify(GPIO_115_EVENT),	/*0X90 */
	__stringify(USB0PFT_EVENT),
	__stringify(USB1PFT_CDN_EVENT),
	__stringify(ULPI1_EVENT),
	__stringify(ULPI2_EVENT),	/*0X94 */
};

#endif

struct pm_special_event_range capri_special_event_list[] = {
	{PMU_INT_A_EVENT, USBH2_PHY_RESUME_A_EVENT},
	{PMU_INT_B_EVENT, USBH2_PHY_RESUME_B_EVENT},
	{KEY_CI_0_EVENT, KEY_CI_7_EVENT}
};

struct pwr_mgr_info capri_pwr_mgr_info = {
	.num_pi = PI_MGR_PI_ID_MAX,
	.base_addr = (u32)KONA_PWRMGR_VA,

#if defined(CONFIG_KONA_PWRMGR_REV2)
	.flags = PM_PMU_I2C | I2C_SIMULATE_BURST_MODE,
	.pwrmgr_intr = BCM_INT_ID_PWR_MGR,
#else
	.flags = PM_PMU_I2C,
#endif
	.special_event_list = capri_special_event_list,
	.num_special_event_range = ARRAY_SIZE(capri_special_event_list),
};

struct capri_event_table {
	u32 event_id;
	u32 trig_type;
	u32 policy_modem;
	u32 policy_arm_core;
	u32 policy_arm_sub;
	u32 policy_hub_aon;
	u32 policy_hub_switchable;
	u32 policy_mm;
	u32 policy_mm_sub;
	u32 policy_mm_sub2;
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	u32 policy_esub;
	u32 policy_esub_sub;
#endif
};

static const struct capri_event_table event_table[] = {
	{
	 .event_id = SOFTWARE_0_EVENT,
	 .trig_type = PM_TRIG_BOTH_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 1,
	 .policy_arm_sub = 5,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 5,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 5,
	 .policy_esub_sub = 5,
#endif

	 },
	{
	 .event_id = SOFTWARE_1_EVENT,
	 .trig_type = PM_TRIG_BOTH_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 1,
	 .policy_arm_sub = 1,
	 .policy_hub_aon = 1,
	 .policy_hub_switchable = 1,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 1,
	 .policy_esub_sub = 1,
#endif

	 },
	{
	 .event_id = SOFTWARE_2_EVENT,
	 .trig_type = PM_TRIG_BOTH_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 5,
	 .policy_arm_sub = 4,
	 .policy_hub_aon = 4,
	 .policy_hub_switchable = 4,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 4,
	 .policy_esub_sub = 4,
#endif

	 },
	{
	 .event_id = VREQ_NONZERO_PI_MODEM_EVENT,
	 .trig_type = PM_TRIG_POS_EDGE,
	 .policy_modem = 5,
	 .policy_arm_core = 1,
	 .policy_arm_sub = 1,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 1,
	 .policy_esub_sub = 1,
#endif
	 },
	{
	 .event_id = COMMON_INT_TO_AC_EVENT,
	 .trig_type = PM_TRIG_POS_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 5,
	 .policy_arm_sub = 5,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 5,
	 .policy_esub_sub = 5,
#endif
	 },
	{
	 .event_id = COMMON_TIMER_1_EVENT,
	 .trig_type = PM_TRIG_POS_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 5,
	 .policy_arm_sub = 5,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 5,
	 .policy_esub_sub = 5,
#endif
	 },
	{
	 .event_id = COMMON_TIMER_2_EVENT,
	 .trig_type = PM_TRIG_POS_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 5,
	 .policy_arm_sub = 5,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 5,
	 .policy_esub_sub = 5,
#endif
	 },
	{
	 .event_id = KEY_CI_0_EVENT,
	 .trig_type = PM_TRIG_POS_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 5,
	 .policy_arm_sub = 5,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 1,
	 .policy_esub_sub = 1,
#endif
	 },
	{
	 .event_id = UBRX_EVENT,
	 .trig_type = PM_TRIG_NEG_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 5,
	 .policy_arm_sub = 5,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 5,
	 .policy_esub_sub = 5,
#endif
	 },
	{
	 .event_id = UB2RX_EVENT,
	 .trig_type = PM_TRIG_NEG_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 5,
	 .policy_arm_sub = 5,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 5,
	 .policy_esub_sub = 5,
#endif
	 },
	{
	 .event_id = SIMDET_EVENT,
	 .trig_type = PM_TRIG_BOTH_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 5,
	 .policy_arm_sub = 5,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 5,
	 .policy_esub_sub = 5,
#endif
	 },
	{
	 .event_id = SIM2DET_EVENT,
	 .trig_type = PM_TRIG_BOTH_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 5,
	 .policy_arm_sub = 5,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 5,
	 .policy_esub_sub = 5,
#endif
	 },
	{
	 .event_id = PMU_INT_A_EVENT,
	 .trig_type = PM_TRIG_BOTH_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 5,
	 .policy_arm_sub = 5,
	 .policy_hub_aon = 5,
	 .policy_hub_switchable = 5,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 5,
	 .policy_esub_sub = 5,
#endif
	 },
	{
	 .event_id = ULPI1_EVENT,
	 .trig_type = PM_TRIG_POS_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 1,
	 .policy_arm_sub = 1,
	 .policy_hub_aon = 1,
	 .policy_hub_switchable = 1,
	 .policy_mm = 1,
	 .policy_mm_sub = 4,
	 .policy_mm_sub2 = 1,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 1,
	 .policy_esub_sub = 1,
#endif
	 },
	{
	 .event_id = ULPI2_EVENT,
	 .trig_type = PM_TRIG_POS_EDGE,
	 .policy_modem = 1,
	 .policy_arm_core = 1,
	 .policy_arm_sub = 1,
	 .policy_hub_aon = 1,
	 .policy_hub_switchable = 1,
	 .policy_mm = 1,
	 .policy_mm_sub = 1,
	 .policy_mm_sub2 = 4,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	 .policy_esub = 1,
	 .policy_esub_sub = 1,
#endif
	 },
};

#ifdef CONFIG_KONA_PMU_BSC_HS_MODE
#ifdef CONFIG_KONA_PWRMGR_REV2
#define RESTART_DELAY 		1
#define WRITE_DELAY	  		3
#define VLT_CHANGE_DELAY	0x20
#else
#define RESTART_DELAY 		6
#define WRITE_DELAY	  		6
#define VLT_CHANGE_DELAY	0x28
#endif /*CONFIG_KONA_PWRMGR_REV2 */
#else
#define RESTART_DELAY 		0x10
#define WRITE_DELAY	  		0x80
#define VLT_CHANGE_DELAY	0x80
#endif /*CONFIG_KONA_PMU_BSC_HS_MODE */

#ifdef CONFIG_CAPRI_PWRMGR_USE_DUMMY_SEQ
static struct i2c_cmd i2c_cmd[] = {
	{REG_ADDR, 0},		/*0 - NOP */
	{REG_ADDR, 0},		/*1 - */
	{WAIT_TIMER, 0x02},	/*2 - */
	{END, 0},		/*3 - */
	{REG_ADDR, 0},		/*4 - */
	{REG_ADDR, 0},		/*5 - */
	{REG_ADDR, 0},		/*6 - */
	{REG_ADDR, 0},		/*7 - */
	{REG_ADDR, 0},		/*8 - */
	{REG_ADDR, 0},		/*9 - */
	{REG_ADDR, 0},		/*10 - */
	{REG_ADDR, 0},		/*11 - */
	{REG_ADDR, 0},		/*12 - */
	{REG_ADDR, 0},		/*13 - */
	{REG_ADDR, 0},		/*14 - */
	{REG_ADDR, 0},		/*15 - */
	{REG_ADDR, 0},		/*16 - */
	{REG_ADDR, 0},		/*17 - - */
	{REG_ADDR, 0},		/*18 - - */
	{REG_ADDR, 0},		/*19 - */
	{REG_ADDR, 0},		/*20 - */
	{REG_ADDR, 0},		/*21 - */
	{REG_ADDR, 0},		/*22 - */
	{REG_ADDR, 0},		/*23 - */
	{REG_ADDR, 0},		/*24 - */
	{REG_ADDR, 0},		/*25 - */
	{REG_ADDR, 0},		/*26 - */
	{REG_ADDR, 0},		/*27 - */
	{REG_ADDR, 0},		/*28 - */
	{REG_ADDR, 0},		/*29 - */
	{REG_ADDR, 0},		/*30 - */
	{REG_ADDR, 0},		/*31 - */
	{REG_ADDR, 0},		/*32 - */
	{REG_ADDR, 0},		/*33 - */
	{REG_ADDR, 0},		/*34 - */
	{REG_ADDR, 0},		/*35 - */
	{REG_ADDR, 0},		/*36 - */
	{REG_ADDR, 0},		/*37 - */
	{REG_ADDR, 0},		/*38 - */
	{REG_ADDR, 0},		/*39 - */
	{REG_ADDR, 0},		/*40 - */
	{REG_ADDR, 0},		/*41 - */
	{REG_ADDR, 0},		/*42 - */
	{REG_ADDR, 0},		/*43 - */
	{REG_ADDR, 0},		/*44 - */
	{REG_ADDR, 0},		/*45 - */
	{REG_ADDR, 0},		/*46 - */
	{REG_ADDR, 0},		/*47 - */
	{REG_ADDR, 0},		/*48 - */
	{REG_ADDR, 0},		/*49 - */
	{REG_ADDR, 0},		/*50 - */
	{REG_ADDR, 0},		/*51 - */
	{REG_ADDR, 0},		/*52 - */
	{REG_ADDR, 0},		/*53 - */
	{REG_ADDR, 0},		/*54 - */
	{REG_ADDR, 0},		/*55 - */
	{REG_ADDR, 0},		/*56 - */
	{REG_ADDR, 0},		/*57 - */
	{REG_ADDR, 0},		/*58 - */
	{REG_ADDR, 0},		/*59 - */
	{REG_ADDR, 0},		/*60 - */
	{REG_ADDR, 0},		/*61 - */
	{REG_ADDR, 0},		/*62 - */
	{END, 0},		/*63 - */

};
#else
static struct i2c_cmd i2c_cmd[] = {
	{REG_ADDR, 0x5c},
	{REG_DATA, 0x00},
	{REG_ADDR, 0x20},
	{REG_DATA, 0x0B},
	{WAIT_TIMER, 0x01},
	{REG_DATA, 0x01},
	{WAIT_TIMER, 0x01},
	{JUMP_VOLTAGE, 0x00},
	{I2C_DATA, 0x10},
	{I2C_DATA, 0xc9},
	{I2C_VAR, 0x00},
	{SET_PC_PINS, 0x11},
	{REG_ADDR, 0x31},
	{JUMP, 0x31},
	{I2C_DATA, 0x10},
	{I2C_DATA, 0xc0},
	{I2C_VAR, 0x00},
	{REG_ADDR, 0x31},
	{JUMP, 0x31},
	{I2C_DATA, 0x10},
	{I2C_DATA, 0xd2},
	{I2C_VAR, 0x00},
	{REG_ADDR, 0x31},
	{JUMP, 0x31},
	{I2C_DATA, 0x10},
	{I2C_DATA, 0x00},
	{WAIT_TIMER, 0x06},
	{REG_ADDR, 0x20},
	{REG_DATA, 0x0B},
	{WAIT_TIMER, 0x01},
	{REG_DATA, 0x01},
	{WAIT_TIMER, 0x01},
	{I2C_DATA, 0x11},
	{REG_ADDR, 0x20},
	{WAIT_TIMER, 0x06},
	{REG_DATA, 0x0F},
	{WAIT_TIMER, 0x06},
	{REG_DATA, 0x01},
	{WAIT_TIMER, 0x10},
	{REG_ADDR, 0x2d},
	{JUMP, 0x2d},
	{I2C_DATA, 0x10},
	{I2C_DATA, 0x00},
	{I2C_DATA, 0x00},
	{WAIT_TIMER, 0x0A},
	{SET_READ_DATA, 0x48},
	{REG_DATA, 0x01},
	{SET_PC_PINS, 0xe0},
	{END, 0x00},
	{WAIT_TIMER, 0x50},
	{REG_ADDR, 0x5c},
	{REG_DATA, 0x04},
	{END, 0x00},
	{SET_READ_DATA, 0x28},
	{END, 0x00},
	{SET_PC_PINS, 0x51},
	{WAIT_TIMER, 0xf0},
	{WAIT_TIMER, 0xf0},
	{END, 0x00},
	{SET_PC_PINS, 0x10},
	{REG_ADDR, 0x32},
	{JUMP, 0x32},
	{SET_PC_PINS, 0x44},
	{END, 0x00},
};

#endif

/*Default voltage lookup table
Need to move this to board-file
*/
#ifdef CONFIG_CAPRI_DISABLE_LP_VOLT_CHANGE
static u8 pwrmgr_default_volt_lut[] = {
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2a,
	0x2E
};
#else
static u8 pwrmgr_default_volt_lut[] = {
	0xC0,
	0x44,
	0x64,
	0x48,
	0x48,
	0x50,
	0x52,
	0x56,
	0x64,
	0x5c,
	0x60,
	0x64,
	0x66,
	0x68,
	0x64,
	0x73
};
#endif
#ifdef CONFIG_CAPRI_PWRMGR_USE_DUMMY_SEQ
static struct v0x_spec_i2c_cmd_ptr v_ptr[V_SET_MAX] = {
	{
	 .other_ptr = 0,
	 .set2_val = 0,		/*Retention voltage inx */
	 .set2_ptr = 0,
	 .set1_val = 0,		/*wakeup from retention voltage inx */
	 .set1_ptr = 0,
	 .zerov_ptr = 0,	/*Not used for capri */
	 },
	{
	 .other_ptr = 0,
	 .set2_val = 0,		/*Retention voltage inx */
	 .set2_ptr = 0,
	 .set1_val = 0,		/*wakeup from retention voltage inx */
	 .set1_ptr = 0,
	 .zerov_ptr = 0,	/*Not used for capri */
	 },
	{
	 .other_ptr = 0,
	 .set2_val = 0,		/*Retention voltage inx */
	 .set2_ptr = 0,
	 .set1_val = 0,		/*wakeup from retention voltage inx */
	 .set1_ptr = 0,
	 .zerov_ptr = 0,	/*Not used for capri */
	 },

};
#else
static struct v0x_spec_i2c_cmd_ptr v_ptr[V_SET_MAX] = {
	{
	 .other_ptr = 8,
	 .set2_val = 2,		/*Retention voltage inx */
	 .set2_ptr = 11,
	 .set1_val = 1,		/*wakeup from retention voltage inx */
	 .set1_ptr = 59,
	 .zerov_ptr = 59,	/*Clear PC1 used for capri */
	 },
	{
	 .other_ptr = 14,
	 .set2_val = 2,		/*Retention voltage inx */
	 .set2_ptr = 55,
	 .set1_val = 1,		/*wakeup from retention voltage inx */
	 .set1_ptr = 14,
	 .zerov_ptr = 62,	/* Clear PC3 for capri */
	 },
	{
	 .other_ptr = 19,
	 .set2_val = 6,		/*Retention voltage inx */
	 .set2_ptr = 19,
	 .set1_val = 5,		/*wakeup from retention voltage inx */
	 .set1_ptr = 19,
	 .zerov_ptr = 19,	/* VSR used for capri */
	 },

};
#endif
struct mm_ctrl capri_mm_ctrl = {
	.mm_ctrl_reg = (u32)KONA_CHIPREG_VA + CHIPREG_PERIPH_MISC_REG2_OFFSET,
	.mm_crystal_clk_is_idle_mask =
	    CHIPREG_PERIPH_MISC_REG2_MM_CRYSTAL_CLK_IS_IDLE_MASK,
};

#if defined(CONFIG_KONA_PWRMGR_REV2)
#define SW_SEQ_I2C_RD_OFF 24
#define SW_SEQ_I2C_RD_SLV_ID_OFF1 24
#define SW_SEQ_I2C_RD_REG_ADDR_OFF 25
#define SW_SEQ_I2C_RD_SLV_ID_OFF2 32
#define SW_SEQ_I2C_RD_FIFO_OFF 53
#define SW_SEQ_I2C_WR_OFF  41
#define SW_SEQ_I2C_WR_SLV_ID_OFF 41
#define SW_SEQ_I2C_WR_REG_ADDR_OFF 42
#define SW_SEQ_I2C_WR_VAL_ADDR_OFF 43
#ifdef CONFIG_CAPRI_WA_HWJIRA_1590
#define SW_SEQ_I2C_PC_TOGGLE_OFF 47
#endif
#define SW_SEQ_TIMEOUT 100
#endif

int __init capri_pwr_mgr_init()
{

	int i;
	struct pi *pi;
	struct pm_policy_cfg cfg;
	cfg.ac = 1;
	cfg.atl = 0;

	writel(0x0000003C, KONA_PWRMGR_VA + PWRMGR_VI_TO_VO0_MAP_OFFSET);
	writel(0x00000001, KONA_PWRMGR_VA + PWRMGR_VI_TO_VO1_MAP_OFFSET);
	writel(0x00000002, KONA_PWRMGR_VA + PWRMGR_VI_TO_VO2_MAP_OFFSET);
	writel(0x0000422f, KONA_PWRMGR_VA +
	       PWRMGR_PC_PIN_OVERRIDE_CONTROL_OFFSET);

#ifdef CONFIG_MFD_BCM_PWRMGR_SW_SEQUENCER
	/*configure i2c to HS mode */
	{
		int sts = 0;
		int clkVal, regVal, reg_val;
		/*select PMU_BSC clock to 26Mhz */
		reg_val = CLK_WR_ACCESS_PASSWORD << CLK_WR_PASSWORD_SHIFT;
		writel(reg_val | CLK_WR_ACCESS_EN,
		       KONA_AON_CLK_VA + KHUBAON_CLK_MGR_REG_WR_ACCESS_OFFSET);
		/*set rate to 26Mhz */
		clkVal =
		    readl(KONA_AON_CLK_VA +
			  KHUBAON_CLK_MGR_REG_PMU_BSC_DIV_OFFSET);
		clkVal &= ~KHUBAON_CLK_MGR_REG_PMU_BSC_DIV_PMU_BSC_DIV_MASK;
		writel(clkVal,
		       KONA_AON_CLK_VA +
		       KHUBAON_CLK_MGR_REG_PMU_BSC_DIV_OFFSET);
		/* Trigger the new clock frequency */
		regVal =
		    readl(KONA_AON_CLK_VA +
			  KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET);
		regVal &=
		    ~KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_PRIV_ACCESS_MODE_MASK;
		regVal |=
		    KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_PMU_BSC_TRIGGER_MASK;
		writel(regVal,
		       (KONA_AON_CLK_VA +
			KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET));
		while ((readl
			(KONA_AON_CLK_VA +
			 KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_OFFSET) &
			KHUBAON_CLK_MGR_REG_PERIPH_SEG_TRG_PMU_BSC_TRIGGER_MASK)) ;

		writel(reg_val,
		       KONA_AON_CLK_VA + KHUBAON_CLK_MGR_REG_WR_ACCESS_OFFSET);
		/* Configure PMU_BSC i2c timing to HS mode */
		pwmgr_pmu_bsc_init(PMU_BSC_BASE_ADDR,
				   PMU_BSC_BASE_ADDR + 0x100 - 1,
				   BCM_INT_ID_PM_I2C, &sts);
		if (sts)
			pr_info("init HS warning(%i)\n", sts);
	}
#endif

	capri_pwr_mgr_info.i2c_cmds = i2c_cmd;
	capri_pwr_mgr_info.num_i2c_cmds = ARRAY_SIZE(i2c_cmd);
	capri_pwr_mgr_info.i2c_var_data = pwrmgr_default_volt_lut;
	capri_pwr_mgr_info.num_i2c_var_data = VLT_LUT_SIZE;
	capri_pwr_mgr_info.i2c_cmd_ptr[VOLT0] = &v_ptr[0];
	capri_pwr_mgr_info.i2c_cmd_ptr[VOLT1] = &v_ptr[1];
	capri_pwr_mgr_info.i2c_cmd_ptr[VOLT2] = &v_ptr[2];

	capri_pwr_mgr_info.vc_ctrl = &capri_mm_ctrl;
#if defined(CONFIG_KONA_PWRMGR_REV2)
	capri_pwr_mgr_info.i2c_rd_off = SW_SEQ_I2C_RD_OFF;
	capri_pwr_mgr_info.i2c_rd_slv_id_off1 = SW_SEQ_I2C_RD_SLV_ID_OFF1;
	capri_pwr_mgr_info.i2c_rd_slv_id_off2 = SW_SEQ_I2C_RD_SLV_ID_OFF2;
	capri_pwr_mgr_info.i2c_rd_reg_addr_off = SW_SEQ_I2C_RD_REG_ADDR_OFF;
	capri_pwr_mgr_info.i2c_rd_fifo_off = SW_SEQ_I2C_RD_FIFO_OFF;
	capri_pwr_mgr_info.i2c_wr_off = SW_SEQ_I2C_WR_OFF;
	capri_pwr_mgr_info.i2c_wr_slv_id_off = SW_SEQ_I2C_WR_SLV_ID_OFF;
	capri_pwr_mgr_info.i2c_wr_reg_addr_off = SW_SEQ_I2C_WR_REG_ADDR_OFF;
	capri_pwr_mgr_info.i2c_wr_val_addr_off = SW_SEQ_I2C_WR_VAL_ADDR_OFF;
	capri_pwr_mgr_info.i2c_seq_timeout = SW_SEQ_TIMEOUT;
#ifdef CONFIG_CAPRI_WA_HWJIRA_1590
	capri_pwr_mgr_info.pc_toggle_off = SW_SEQ_I2C_PC_TOGGLE_OFF;
#endif
#endif
	pwr_mgr_init(&capri_pwr_mgr_info);
	capri_pi_mgr_init();

/*JIRA HWCAPRI-1447, it was observed that if MM CCU is switched to and from shutdown
 * state, it would break the DDR self refresh. work around for this from ASIC
 * team is to set the POWER_OK_MASK bit to 0 */

	pwr_mgr_ignore_power_ok_signal(false);

	/*JIRA HWCAPRI_501 */
	/*SW should keep POWER_OK_TIMER field in PI_DEFAULT_POWER_STATE
	   (0x35014024) register to its default value of 0x6. */
	pwr_mgr_set_power_ok_timer_max(0x6);

	/*Ignore Jtag power request when attaching Jtag(bit 21 in 0x35014024) */
	pwr_mgr_ignore_dap_powerup_request(true);

	/*clear all the event */
	pwr_mgr_event_clear_events(LCDTE_EVENT, EVENT_ID_ALL);

	pwr_mgr_event_set(SOFTWARE_2_EVENT, 1);
	pwr_mgr_event_set(SOFTWARE_0_EVENT, 1);

	pwr_mgr_pm_i2c_enable(true);

	/*Init event table */
	for (i = 0; i < ARRAY_SIZE(event_table); i++) {
		u32 event_id;

		event_id = event_table[i].event_id;

		pwr_mgr_event_trg_enable(event_table[i].event_id,
					 event_table[i].trig_type);

		cfg.policy = event_table[i].policy_modem;
		pwr_mgr_event_set_pi_policy(event_id, PI_MGR_PI_ID_MODEM, &cfg);

		cfg.policy = event_table[i].policy_arm_core;
		pwr_mgr_event_set_pi_policy(event_id, PI_MGR_PI_ID_ARM_CORE,
					    &cfg);

		cfg.policy = event_table[i].policy_arm_sub;
		pwr_mgr_event_set_pi_policy(event_id,
					    PI_MGR_PI_ID_ARM_SUB_SYSTEM, &cfg);

		cfg.policy = event_table[i].policy_hub_aon;
		pwr_mgr_event_set_pi_policy(event_id, PI_MGR_PI_ID_HUB_AON,
					    &cfg);

#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE

		cfg.policy = event_table[i].policy_esub;
		pwr_mgr_event_set_pi_policy(event_id, PI_MGR_PI_ID_ESUB, &cfg);

		cfg.policy = event_table[i].policy_esub_sub;
		pwr_mgr_event_set_pi_policy(event_id, PI_MGR_PI_ID_ESUB_SUB,
					    &cfg);
#endif
		cfg.policy = event_table[i].policy_hub_switchable;
		pwr_mgr_event_set_pi_policy(event_id,
					    PI_MGR_PI_ID_HUB_SWITCHABLE, &cfg);

		cfg.policy = event_table[i].policy_mm;
		pwr_mgr_event_set_pi_policy(event_id, PI_MGR_PI_ID_MM, &cfg);

		cfg.policy = event_table[i].policy_mm_sub;
		pwr_mgr_event_set_pi_policy(event_id, PI_MGR_PI_ID_MM_SUB,
					    &cfg);

		cfg.policy = event_table[i].policy_mm_sub2;
		pwr_mgr_event_set_pi_policy(event_id, PI_MGR_PI_ID_MM_SUB2,
					    &cfg);

	}
	/*Init all PIs */
/*Init all PIs*/
	for (i = 0; i < PI_MGR_PI_ID_MODEM; i++) {
		pi = pi_mgr_get(i);
		BUG_ON(pi == NULL);
		pi_init(pi);
	}

	capri_clock_init();

	return 0;
}

early_initcall(capri_pwr_mgr_init);

#ifdef CONFIG_DEBUG_FS

static struct pin_config pm_debugbus_config[] = {
	PIN_CFG(TRACEDT15, DEBUG_BUS15, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT14, DEBUG_BUS14, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT13, DEBUG_BUS13, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT12, DEBUG_BUS12, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT11, DEBUG_BUS11, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT10, DEBUG_BUS10, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT09, DEBUG_BUS09, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT08, DEBUG_BUS08, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT07, DEBUG_BUS07, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT06, DEBUG_BUS06, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT05, DEBUG_BUS05, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT04, DEBUG_BUS04, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT03, DEBUG_BUS03, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT02, DEBUG_BUS02, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT01, DEBUG_BUS01, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT00, DEBUG_BUS00, 0, OFF, OFF, 0, 1, 8MA),
};

static struct pin_config pm_is_idle_debugbus_config[] = {
	PIN_CFG(TRACEDT07, PM_DEBUG0, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT06, PM_DEBUG1, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT05, PM_DEBUG2, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT04, PM_DEBUG3, 0, OFF, OFF, 0, 1, 8MA),
};

void pwr_mgr_mach_debug_fs_init(int type)
{
	static bool mux_init;
	u32 reg_val, i;

	if (!mux_init) {

		for (i = 0; i < ARRAY_LEN(pm_debugbus_config); i++)
			pinmux_set_pin_config(&pm_debugbus_config[i]);
	}

	reg_val = readl(KONA_CHIPREG_VA + CHIPREG_PERIPH_MISC_REG1_OFFSET);
	reg_val &= ~CHIPREG_PERIPH_MISC_REG1_DEBUG_BUS_SEL_MASK;
	if (type == 0)
		reg_val |= 0x1 << CHIPREG_PERIPH_MISC_REG1_DEBUG_BUS_SEL_SHIFT;
	else if (type == 1)
		reg_val |= 0xD << CHIPREG_PERIPH_MISC_REG1_DEBUG_BUS_SEL_SHIFT;
	else
		BUG();
	writel(reg_val, KONA_CHIPREG_VA + CHIPREG_PERIPH_MISC_REG1_OFFSET);

}

/*enables TRACED07/6/5/4 to output is_idle bits.
Root CCU 0, KPS 1, KPM 2, ESUB 3, KHUB 4  */
void pwr_mgr_mach_debug_is_idle_fs_init(int type)
{
	u32 reg_val, i;
	/*RDB doesn't contain a sub DEBUG_IDLE_MASK define */
#define CHIPREG_PERIPH_MISC_REG1_DEBUG_IDLE_SEL_MASK \
				0x00F00000
#define	CHIPREG_PERIPH_MISC_REG1_DEBUG_IDLE_SEL_SHIFT \
				20

	if (type > 4) {
		pr_info("%s: %x type: 0-root, 1-kps, 2-kpm, 3-esub, 4-khub\n",
			__func__, type);
		return;
	}

	for (i = 0; i < ARRAY_LEN(pm_is_idle_debugbus_config); i++)
		pinmux_set_pin_config(&pm_is_idle_debugbus_config[i]);

	/*set CCU to debug */
	reg_val = readl(KONA_CHIPREG_VA + CHIPREG_PERIPH_MISC_REG1_OFFSET);
	reg_val &= ~CHIPREG_PERIPH_MISC_REG1_DEBUG_IDLE_SEL_MASK;
	reg_val |= (type + 8) << CHIPREG_PERIPH_MISC_REG1_DEBUG_IDLE_SEL_SHIFT;
	pr_info("%s: reg_val: %x\n", __func__, reg_val);
	writel(reg_val, KONA_CHIPREG_VA + CHIPREG_PERIPH_MISC_REG1_OFFSET);
}

#endif /*CONFIG_DEBUG_FS */

int capri_pwr_mgr_delayed_init(void)
{
	int i;
	struct pi *pi;
#ifdef CONFIG_DEBUG_FS
	u32 bmdm_pwr_mgr_base =
	    (u32)ioremap_nocache(BMDM_PWRMGR_BASE_ADDR, SZ_1K);
#endif
	/*All the initializations are done. Clear override bit here so that
	 * appropriate policies take effect*/
	for (i = 0; i < PI_MGR_PI_ID_MODEM; i++) {
		pi = pi_mgr_get(i);
		BUG_ON(pi == NULL);
		pi_init_state(pi);
	}

	/* Enable PI counters */
	for (i = 0; i < PI_MGR_PI_ID_MAX; i++)
		pwr_mgr_pi_counter_enable(i, 1);
	pm_mgr_pi_count_clear(1);
	pm_mgr_pi_count_clear(0);

	delayed_init_complete = 1;
#ifdef CONFIG_DEBUG_FS
	return pwr_mgr_debug_init(bmdm_pwr_mgr_base);
#else
	return 0;
#endif
}

#ifdef CONFIG_CAPRI_DELAYED_PM_INIT
static int param_set_pm_late_init(const char *val,
				  const struct kernel_param *kp)
{
	int ret = -1;
	int pm_delayed_init = 0;

	pr_info("%s\n", __func__);
	if (delayed_init_complete)
		return 0;
	if (!val)
		return -EINVAL;

	ret = sscanf(val, "%d", &pm_delayed_init);
	pr_info("%s, pm_delayed_init:%d\n", __func__, pm_delayed_init);
	if (pm_delayed_init == 1) {
		capri_pm_init();
		capri_pwr_mgr_delayed_init();
	}

	if (delay_arm_lpm.valid)
		pi_mgr_qos_request_remove(&delay_arm_lpm);

	return 0;
}
#endif

int __init capri_pwr_mgr_late_init(void)
{
#ifdef CONFIG_CAPRI_DELAYED_PM_INIT
	int ret;
	if (is_charging_state()) {
		pr_info("%s: power off charging, complete int here\n",
			__func__);
		capri_pm_init();
		capri_pwr_mgr_delayed_init();

	} else {
		ret = pi_mgr_qos_add_request(&delay_arm_lpm, "delay_arm_lpm",
				PI_MGR_PI_ID_ARM_CORE, 0);
		if (ret)
			pr_info("%s:request qos failed(%d)\n", __func__, ret);
	}
#else
	capri_pwr_mgr_delayed_init();
#endif
	return 0;
}

late_initcall(capri_pwr_mgr_late_init);
