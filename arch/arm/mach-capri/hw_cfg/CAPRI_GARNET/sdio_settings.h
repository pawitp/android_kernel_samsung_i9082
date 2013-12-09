/*****************************************************************************
* Copyright 2010 - 2011 Broadcom Corporation.  All rights reserved.
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
*****************************************************************************/

#ifndef SDIO_SETTINGS_H
#define SDIO_SETTINGS_H

/*
 * Refer to mach/sdio_platform.h for details
 */

#define HW_WLAN_GPIO_RESET_PIN	103	/* WL_REG_ON/WL_RST_B */
#define HW_WLAN_GPIO_HOST_WAKE_PIN	102	/* WL_HOSTWAKE */

/*
 * #define HW_WLAN_GPIO_SHUTDOWN_PIN
 * #define HW_WLAN_GPIO_REG_PIN
 */

/*
 * HW_SDIO_PARAM defines the array of the struct sdio_platform_cfg data
 * structure, with each element in the array representing a SDIO device setting
 */
/*
 * Set SDIO1's peri_clk_rate to 48Mhz, since it is connected to WIFI chip and
 * the max sdio clock rate should not exceed 50Mhz.
 */
#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT
#define HW_SDIO_PARAM \
{ \
	{ /* SDIO1 */ \
		.id = 0, \
		.peri_clk_rate = 48000000, \
		.devtype = SDIO_DEV_TYPE_WIFI, \
		.register_status_notify = capri_wifi_status_register, \
	}, \
	{ /* SDIO2 */ \
		.id = 1, \
		.is_8bit = 1, \
		.peri_clk_rate = 52000000, \
		.devtype = SDIO_DEV_TYPE_EMMC, \
	}, \
	{ /* SDIO4 */ \
		.id = 3, \
		.tmp_uhs = 1, \
		.need_regulator_uhs = 1, \
		.peri_clk_rate = 48000000, \
		.cd_gpio = 14, /* SDIO_CARD_DET_B */ \
		.wp_gpio = -1, \
		.devtype = SDIO_DEV_TYPE_SDMMC, \
	}, \
}
#else
#define HW_SDIO_PARAM \
{ \
	{ /* SDIO1 */ \
		.id = 0, \
		.peri_clk_rate = 48000000, \
		.devtype = SDIO_DEV_TYPE_WIFI, \
	}, \
	{ /* SDIO2 */ \
		.id = 1, \
		.is_8bit = 1, \
		.peri_clk_rate = 52000000, \
		.devtype = SDIO_DEV_TYPE_EMMC, \
	}, \
	{ /* SDIO4 */ \
		.id = 3, \
		.tmp_uhs = 1, \
		.need_regulator_uhs = 1, \
		.peri_clk_rate = 48000000, \
		.cd_gpio = 14, /* SDIO_CARD_DET_B */ \
		.wp_gpio = -1, \
		.devtype = SDIO_DEV_TYPE_SDMMC, \
	}, \
}

#endif /* CONFIG_BRCM_UNIFIED_DHD_SUPPORT */

#endif /* SDIO_SETTINGS_H */
