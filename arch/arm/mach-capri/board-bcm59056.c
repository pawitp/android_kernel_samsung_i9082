/*****************************************************************************
*  Copyright 2011 - 2012 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/licenses/old-license/gpl-2.0.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/
#include <linux/version.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/chipregHw_inline.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/mfd/bcmpmu.h>

#if defined(CONFIG_SEC_CHARGING_FEATURE)
#include <linux/spa_power.h>
#endif

#if defined(CONFIG_MACH_CAPRI_SS_S2VE)
#include <CAPRI_SS_S2VE/board-bcm59056_config.h>
#elif defined(CONFIG_MACH_CAPRI_TABLET)
#include <CAPRI_TABLET/board-bcm59056_config.h>
#elif defined(CONFIG_MACH_CAPRI_GARNET)
#include <CAPRI_GARNET/board-bcm59056_config.h>
#elif defined(CONFIG_MACH_CAPRI_SS_BAFFIN)
#include <CAPRI_SS_BAFFIN/board-bcm59056_config.h>
#elif defined(CONFIG_MACH_CAPRI_SS_CRATER)
#include <CAPRI_SS_CRATER/board-bcm59056_config.h>
#else
#include <BASE/board-bcm59056_config.h>
#endif

static struct platform_device bcmpmu_audio_device = {
	.name = "bcmpmu_audio",
	.id = -1,
	.dev.platform_data = NULL,
};

static struct platform_device bcmpmu_em_device = {
	.name = "bcmpmu_em",
	.id = -1,
	.dev.platform_data = NULL,
};

static struct platform_device bcmpmu_otg_xceiv_device = {
	.name = "bcmpmu_otg_xceiv",
	.id = -1,
	.dev.platform_data = NULL,
};

#ifdef CONFIG_BCMPMU_RPC
static struct platform_device bcmpmu_rpc = {
	.name = "bcmpmu_rpc",
	.id = -1,
	.dev.platform_data = NULL,
};
#endif

#ifdef CONFIG_CHARGER_BCMPMU_SPA
static struct platform_device bcmpmu_chrgr_spa_device = {
	.name = "bcmpmu_chrgr_pb",
	.id = -1,
	.dev.platform_data = NULL,
};
#endif

#define HW_FIXED_REG_NULL_PARAM \
	.supply_name = NULL, \
	.microvolts = 0, \
	.gpio = -1, \
	.startup_delay = 0, \
	.enable_high = 0, \
	.enabled_at_boot = 0, \
	.init_data = NULL

static struct fixed_voltage_config boost5v = {
#ifdef HW_BOOST_5V_PARAM
	HW_BOOST_5V_PARAM,
#else
	HW_FIXED_REG_NULL_PARAM,
#endif
};

static struct platform_device boost_device_5v = {
	.name = "reg-fixed-voltage",
	.id = -1,
	.dev.platform_data = &boost5v,
};

static struct platform_device *bcmpmu_client_devices[] = {
	&bcmpmu_audio_device,
#ifdef CONFIG_CHARGER_BCMPMU_SPA
	&bcmpmu_chrgr_spa_device,
#endif
	&bcmpmu_em_device,
	&bcmpmu_otg_xceiv_device,
#ifdef CONFIG_BCMPMU_RPC
	&bcmpmu_rpc,
#endif
};

static int __init bcmpmu_init_platform_hw(struct bcmpmu *bcmpmu)
{
	int i;
	printk(KERN_INFO "%s: called.\n", __func__);

	for (i = 0; i < ARRAY_SIZE(bcmpmu_client_devices); i++)
		bcmpmu_client_devices[i]->dev.platform_data = bcmpmu;
	platform_add_devices(bcmpmu_client_devices,
			     ARRAY_SIZE(bcmpmu_client_devices));

	if (get_chip_version() == CAPRI_A2 &&
		bcmpmu->pmu_rev == BCMPMU_REV_B0)
		platform_device_register(&boost_device_5v);

	return 0;
}

static int __init bcmpmu_exit_platform_hw(struct bcmpmu *bcmpmu)
{
	printk(KERN_INFO "%s called\n", __func__);

	return 0;
}

static struct i2c_board_info pmu_info_map1 = {
	I2C_BOARD_INFO("bcmpmu_map1", PMU_DEVICE_I2C_ADDR1),
};

static struct bcmpmu_adc_setting adc_setting = {
	.tx_rx_sel_addr = 0,
	.tx_delay = 0,
	.rx_delay = 0,
};

static struct bcmpmu_cutoff_map cutoff_cal_map[] = {
	{3480, 2, 0},
	{3440, 1, 0},
	{3400, 0, 0},
};

static struct bcmpmu_currcap_map eoc_cal_map[] = {
	{290, 90, 0},
	{270, 91, 0},
	{250, 92, 0},
	{228, 93, 0},
	{208, 94, 0},
	{185, 95, 0},
	{165, 96, 0},
	{145, 97, 0},
	{125, 98, 0},
	{105, 99, 0},
	{85, 100, 0},
	{0, 100, 0},
};

static int chrgr_curr_lmt[PMU_CHRGR_TYPE_MAX] = {
	[PMU_CHRGR_TYPE_NONE] = 0,
	[PMU_CHRGR_TYPE_SDP] = 0,
	[PMU_CHRGR_TYPE_CDP] = 1500,
	[PMU_CHRGR_TYPE_DCP] = 1500,
	[PMU_CHRGR_TYPE_TYPE1] = 700,
	[PMU_CHRGR_TYPE_TYPE2] = 700,
	[PMU_CHRGR_TYPE_PS2] = 100,
	[PMU_CHRGR_TYPE_ACA] = 0,
};

static struct bcmpmu_charge_zone chrg_zone[] = {
	{.tl = -50,.th = 600,.v = 3000,.fc = 10,.qc = 100},	/* Zone QC */
	{.tl = -50,.th = -1,.v = 4200,.fc = 100,.qc = 0},	/* Zone LL */
	{.tl = 0,.th = 99,.v = 4200,.fc = 100,.qc = 0},	/* Zone L */
	{.tl = 100,.th = 450,.v = 4200,.fc = 100,.qc = 0},	/* Zone N */
	{.tl = 451,.th = 500,.v = 4200,.fc = 100,.qc = 0},	/* Zone H */
	{.tl = 501,.th = 600,.v = 4200,.fc = 100,.qc = 0},	/* Zone HH */
	{.tl = -50,.th = 600,.v = 0,.fc = 0,.qc = 0},	/* Zone OUT */
};

#ifdef CONFIG_MACH_CAPRI_TABLET
static struct bcmpmu_voltcap_map batt_voltcap_map[] = {
	/*
	* Average Open-Cell Voltage vs Remaining Capacity for
	* Capri BRT 3.7V 6700mAh Li-ion batteries measured @ 25C
	* data provided by MAP hardware team
	*/
	{4166, 100},
	{4138, 99},
	{4119, 98},
	{4106, 97},
	{4095, 96},
	{4085, 95},
	{4074, 94},
	{4065, 93},
	{4055, 92},
	{4046, 91},
	{4038, 90},
	{3990, 85},
	{3948, 80},
	{3911, 75},
	{3877, 70},
	{3847, 65},
	{3821, 60},
	{3799, 55},
	{3781, 50},
	{3765, 45},
	{3752, 40},
	{3739, 35},
	{3725, 30},
	{3707, 25},
	{3684, 20},
	{3662, 15},
	{3599, 10},
	{3583, 9},
	{3561, 8},
	{3528, 7},
	{3479, 6},
	{3412, 5},
	{3313, 4},
	{3131, 3},
};
#else
static struct bcmpmu_voltcap_map batt_voltcap_map[] = {
	/*
	* volt capacity
	*/
	{4179, 100},
	{4130, 95},
	{4086, 90},
	{4045, 85},
	{4005, 81},
	{3973, 76},
	{3943, 71},
	{3916, 66},
	{3873, 61},
	{3837, 56},
	{3817, 51},
	{3802, 46},
	{3790, 42},
	{3782, 37},
	{3776, 32},
	{3769, 27},
	{3747, 22},
	{3714, 17},
	{3691, 12},
	{3690, 11},
	{3689, 10},
	{3688, 9},
	{3687, 8},
	{3684, 7},
	{3676, 6},
	{3654, 4},
	{3616, 3},
	{3564, 2},
	{3497, 1},
	{3394, 0},
};
#endif

static struct bcmpmu_fg_zone fg_zone[FG_TMP_ZONE_MAX + 1] = {
/* This table is default data, the real data from board file or device tree*/
	{.temp = -200,
	 .reset = 1, .fct = 26, .guardband = 30,
	 .esr_vl_lvl = 3788,.esr_vm_lvl = 3814,.esr_vh_lvl = 4088,
	 .esr_vl_slope = -31479,.esr_vl_offset = 121154,
	 .esr_vm_slope = -7916,.esr_vm_offset = 31900,
	 .esr_vh_slope = -597,.esr_vh_offset = 3985,
	 .esr_vf_slope = -7664,.esr_vf_offset = 32875,
	 .vcmap = &batt_voltcap_map[0],
	 .maplen = ARRAY_SIZE(batt_voltcap_map)},	/* -20 */
	{.temp = -150,
	 .reset = 0, .fct = 210, .guardband = 50,
	 .esr_vl_lvl = 3788,.esr_vm_lvl = 3814,.esr_vh_lvl = 4088,
	 .esr_vl_slope = -31479,.esr_vl_offset = 121154,
	 .esr_vm_slope = -7916,.esr_vm_offset = 31900,
	 .esr_vh_slope = -597,.esr_vh_offset = 3985,
	 .esr_vf_slope = -7664,.esr_vf_offset = 32875,
	 .vcmap = &batt_voltcap_map[0],
	 .maplen = ARRAY_SIZE(batt_voltcap_map)},	/* -15 */
	{.temp = -100,
	 .reset = 1, .fct = 394, .guardband = 30,
	 .esr_vl_lvl = 3742,.esr_vm_lvl = 3798,.esr_vh_lvl = 4088,
	 .esr_vl_slope = -8481,.esr_vl_offset = 33090,
	 .esr_vm_slope = -6677,.esr_vm_offset = 26338,
	 .esr_vh_slope = -611,.esr_vh_offset = 3297,
	 .esr_vf_slope = -2255,.esr_vf_offset = 10018,
	 .vcmap = &batt_voltcap_map[0],
	 .maplen = ARRAY_SIZE(batt_voltcap_map)},	/* -10 */
	{.temp = -50,
	 .reset = 0, .fct = 565, .guardband = 50,
	 .esr_vl_lvl = 3742,.esr_vm_lvl = 3798,.esr_vh_lvl = 4088,
	 .esr_vl_slope = -8481,.esr_vl_offset = 33090,
	 .esr_vm_slope = -6677,.esr_vm_offset = 26338,
	 .esr_vh_slope = -611,.esr_vh_offset = 3297,
	 .esr_vf_slope = -2255,.esr_vf_offset = 10018,
	 .vcmap = &batt_voltcap_map[0],
	 .maplen = ARRAY_SIZE(batt_voltcap_map)},	/* -5 */
	{.temp = 0,
	 .reset = 1, .fct = 736, .guardband = 30,
	 .esr_vl_lvl = 3679,.esr_vm_lvl = 3788,.esr_vh_lvl = 4088,
	 .esr_vl_slope = -31497,.esr_vl_offset = 117159,
	 .esr_vm_slope = -6612,.esr_vm_offset = 25599,
	 .esr_vh_slope = -393,.esr_vh_offset = 2042,
	 .esr_vf_slope = -1918,.esr_vf_offset = 8277,
	 .vcmap = &batt_voltcap_map[0],
	 .maplen = ARRAY_SIZE(batt_voltcap_map)},	/* 0 */
	{.temp = 50,
	 .reset = 0, .fct = 811, .guardband = 50,
	 .esr_vl_lvl = 3663,.esr_vm_lvl = 3679,.esr_vh_lvl = 3798,
	 .esr_vl_slope = -18140,.esr_vl_offset = 67518,
	 .esr_vm_slope = -28827,.esr_vm_offset = 106665,
	 .esr_vh_slope = -2252,.esr_vh_offset = 8887,
	 .esr_vf_slope = -370,.esr_vf_offset = 1739,
	 .vcmap = &batt_voltcap_map[0],
	 .maplen = ARRAY_SIZE(batt_voltcap_map)},	/* 5 */
	{.temp = 100,
	 .reset = 1, .fct = 887, .guardband = 30,
	 .esr_vl_lvl = 3663,.esr_vm_lvl = 3679,.esr_vh_lvl = 3798,
	 .esr_vl_slope = -18140,.esr_vl_offset = 67518,
	 .esr_vm_slope = -28827,.esr_vm_offset = 106665,
	 .esr_vh_slope = -2252,.esr_vh_offset = 8887,
	 .esr_vf_slope = -370,.esr_vf_offset = 1739,
	 .vcmap = &batt_voltcap_map[0],
	 .maplen = ARRAY_SIZE(batt_voltcap_map)},	/* 10 */
	{.temp = 150,
	 .reset = 1, .fct = 943, .guardband = 30,
	 .esr_vl_lvl = 3668,.esr_vm_lvl = 3687,.esr_vh_lvl = 3896,
	 .esr_vl_slope = -908,.esr_vl_offset = 3574,
	 .esr_vm_slope = -1936,.esr_vm_offset = 7344,
	 .esr_vh_slope = 124,.esr_vh_offset = -251,
	 .esr_vf_slope = -417,.esr_vf_offset = 1857,
	 .vcmap = &batt_voltcap_map[0],
	 .maplen = ARRAY_SIZE(batt_voltcap_map)},	/* 15 */
	{.temp = 200,
	 .reset = 1, .fct = 1000, .guardband = 30,
	 .esr_vl_lvl = 3668,.esr_vm_lvl = 3687,.esr_vh_lvl = 3896,
	 .esr_vl_slope = -908,.esr_vl_offset = 3574,
	 .esr_vm_slope = -1936,.esr_vm_offset = 7344,
	 .esr_vh_slope = 124,.esr_vh_offset = -251,
	 .esr_vf_slope = -417,.esr_vf_offset = 1857,
	 .vcmap = &batt_voltcap_map[0],
	 .maplen = ARRAY_SIZE(batt_voltcap_map)},	/* 20 */
};

#ifdef CONFIG_CHARGER_BCMPMU_EXT
static struct bcmpmu_ext_chrgr_info ext_chrgr_info = {
	.ac5v = 56,
	.iusb = 94,
	.cen = 48,
	.usus = 93,
	.uok = 55,
	.flt = 92,
	.chg = 91,
};
#endif

#ifdef CONFIG_CHARGER_BCMPMU_SPA
static void notify_spa(enum bcmpmu_event_t event, int data);

#endif

static struct bcmpmu_platform_data bcmpmu_plat_data = {
#ifdef CONFIG_KONA_PMU_BSC_HS_MODE
	/*
	 * PMU in High Speed (HS) mode. I2C CLK is 3.25MHz
	 * derived from 26MHz input clock.
	 *
	 * Rhea: PMBSC is always in HS mode, i2c_pdata is not in use.
	 */
	.i2c_pdata = ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_HS),
#else
	.i2c_pdata = ADD_I2C_SLAVE_SPEED(BSC_BUS_SPEED_400K),
#endif
	.init = bcmpmu_init_platform_hw,
	.exit = bcmpmu_exit_platform_hw,
	.i2c_board_info_map1 = &pmu_info_map1,
	.i2c_adapter_id = PMU_DEVICE_I2C_BUSNO,
	.i2c_pagesize = 256,
	.init_data = &register_init_data[0],
	.init_max = ARRAY_SIZE(register_init_data),
	.batt_temp_in_celsius = 1,
	.batt_temp_map = &batt_temp_map[0],
	.batt_temp_map_len = ARRAY_SIZE(batt_temp_map),
	.adc_setting = &adc_setting,
	.adc_rtm_for_hk = 1,
	.adc_channel_ntc = PMU_ADC_NTC,
	.adc_channel_patemp = PMU_ADC_PATEMP,
	.adc_channel_32ktemp = PMU_ADC_32KTEMP,
	.num_of_regl = ARRAY_SIZE(bcm59056_regulators),
	.regulator_init_data = &bcm59056_regulators[0],
	.fg_smpl_rate = 2083,
	.fg_slp_rate = 32000,
	.fg_slp_curr_ua = 1000,
	.fg_factor = 976,
	.max_batt_ov = 100,
	.pre_eoc_min_curr = 300,
	.fg_sns_res = 10,
	.batt_voltcap_map = &batt_voltcap_map[0],
	.batt_voltcap_map_len = ARRAY_SIZE(batt_voltcap_map),
	.cutoff_cal_map = &cutoff_cal_map[0],
	.cutoff_cal_map_len = ARRAY_SIZE(cutoff_cal_map),
	.eoc_cal_map = &eoc_cal_map[0],
	.eoc_cal_map_len = ARRAY_SIZE(eoc_cal_map),
	.chrgr_curr_lmt = &chrgr_curr_lmt[0],
	.chrg_zone_map = &chrg_zone[0],
	.support_hw_eoc = 0,
	.support_chrg_maint = 1,
	.chrg_resume_lvl = 4100,
#ifdef CONFIG_CHARGER_BCMPMU_EXT
	.fg_support_tc = 0,
#else
	.fg_support_tc = 1,
#endif
	.fg_tc_dn_lvl = 50,	/* 5c */
	.fg_tc_up_lvl = 200, /* 20c */
	.fg_zone_settle_tm = 60,
	.fg_zone_info = &fg_zone[0],
	.fg_poll_hbat = 112000,
	.fg_poll_lbat = 5000,
	.fg_lbat_lvl = 3500,
	.fg_fbat_lvl = 4140,
	.fg_low_cal_lvl = 3550,
#ifdef CONFIG_USB_SWITCH_FSA9485
	.bc = BCMPMU_BC_FSA,
#else
	.bc = BCMPMU_BC_BB_BC12,
#endif
	.batt_model = "SS,1650mAH",
	.cutoff_volt = 3400,
	.cutoff_count_max = 3,
#ifdef CONFIG_CHARGER_BCMPMU_EXT
	.batt_impedence = 200,
	.chrg_1c_rate = 3350,
	.chrg_eoc = 168,
	.fg_capacity_full = 6700 * 3600,
	.support_ext_chrgr = 1,
	.ext_chrgr_info = &ext_chrgr_info,
#else
	.batt_impedence = 150,
	.sys_impedence = 33,
	.chrg_1c_rate = 1650,
	.chrg_eoc = 100,
	.fg_capacity_full = 1650*3600,
#endif
	.non_pse_charging = 1,
	.max_vfloat = 4200,
	.irq = PMU_INT_IRQ_NUM,
#ifdef CONFIG_CHARGER_BCMPMU_SPA
	.piggyback_chrg = 1,
	.piggyback_chrg_name = "bcm59056_charger",
	.piggyback_notify = notify_spa,
	.piggyback_work = NULL,
	.spafifo = NULL,
	.spalock = NULL,
#endif
	.support_die_temp_monitor = 0,
	.die_temp_threshold = 650,
	.die_temp_hysteresis = 50,
	.die_temp_throttle = 400,
	.die_temp_slope = 497,
	.die_temp_offset = -275700,
};

#ifdef CONFIG_CHARGER_BCMPMU_SPA
static void notify_spa(enum bcmpmu_event_t event, int data)
{
	if (bcmpmu_plat_data.spafifo) {
		mutex_lock(bcmpmu_plat_data.spalock);
		if (!bcmpmu_plat_data.spafifo->fifo_full) {
			if(event == BCMPMU_CHRGR_EVENT_CHGR_DETECTION)
				bcmpmu_plat_data.spafifo->chrg_on = (data != POWER_SUPPLY_TYPE_BATTERY)?true:false;

			printk(KERN_INFO "%s event=%d, data=%d, pchrgr_on=0x%x\n",
				__func__, event, data, bcmpmu_plat_data.spafifo->chrg_on);

			if(!bcmpmu_plat_data.spafifo->chrg_on && (event == BCMPMU_CHRGR_EVENT_EOC))
				mutex_unlock(bcmpmu_plat_data.spalock);
			else {
				bcmpmu_plat_data.spafifo->event[bcmpmu_plat_data.spafifo->head] = event;
				bcmpmu_plat_data.spafifo->data[bcmpmu_plat_data.spafifo->head] = data;
				//SPA_FIFO_HEAD[bcmpmu_plat_data.spafifo];
				bcmpmu_plat_data.spafifo->head = ((bcmpmu_plat_data.spafifo->head+1)
				 & (bcmpmu_plat_data.spafifo->length-1));

				if(bcmpmu_plat_data.spafifo->head == bcmpmu_plat_data.spafifo->tail)
				bcmpmu_plat_data.spafifo->fifo_full = true;
			mutex_unlock(bcmpmu_plat_data.spalock);

	if (bcmpmu_plat_data.piggyback_work)
		schedule_delayed_work(bcmpmu_plat_data.piggyback_work, 0);
			}
		} else {
			printk(KERN_INFO "%s: fifo full.\n", __func__);
			mutex_unlock(bcmpmu_plat_data.spalock);
		}
	}
}
#endif
static struct i2c_board_info __initdata pmu_info[] = {
	{
	 I2C_BOARD_INFO("bcmpmu", PMU_DEVICE_I2C_ADDR),
	 .platform_data = &bcmpmu_plat_data,
	 },
};

__init int board_pmu_init(void)
{

#ifdef CONFIG_KONA_DT_BCMPMU
	bcmpmu_update_pdata_dt_batt(&bcmpmu_plat_data);
	bcmpmu_update_pdata_dt_pmu(&bcmpmu_plat_data);
#endif

	i2c_register_board_info(PMU_DEVICE_I2C_BUSNO,
				pmu_info, ARRAY_SIZE(pmu_info));
	return 0;
}

arch_initcall(board_pmu_init);
