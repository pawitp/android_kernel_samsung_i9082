/*****************************************************************************
* Copyright 2001 - 2009 Broadcom Corporation.  All rights reserved.
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

#ifndef _SDIO_PLATFORM_H
#define _SDIO_PLATFORM_H

/* max number of SDIO devices */
#define SDIO_MAX_NUM_DEVICES 4

/*
 * SDIO device type
 */
enum sdio_devtype {
	SDIO_DEV_TYPE_SDMMC = 0,
	SDIO_DEV_TYPE_WIFI,
	SDIO_DEV_TYPE_EMMC,

	/* used for internal array indexing, DO NOT modify */
	SDIO_DEV_TYPE_MAX,
};

/*
 * SDIO WiFi GPIO configuration
 */
struct sdio_wifi_gpio_cfg {
	int reset;
	int shutdown;
	int reg;
	int host_wake;
	int reserved;
};

/*
 * SDIO Platform flags:
 *
 * Bit Map
 * -------
 *
 *  -----------------------------
 * | BIT 31 | ~~~~~~~~~~ | BIT 0 |
 *  -----------------------------
 *                          |
 *                           --------> 0 - Device connected to this controller
 *                                         is removable
 *                                     1 - Device connected to this controller
 *                                         is removable
 */
enum kona_sdio_plat_flags {
	KONA_SDIO_FLAGS_DEVICE_REMOVABLE = 0,
	KONA_SDIO_FLAGS_DEVICE_NON_REMOVABLE = 1 << 0,
	/* More flags can be added here */
};

struct sdio_platform_cfg {
	/* specify which SDIO device */
	unsigned id;

	/*
	* For boards without the SDIO pullup registers, data_pullup needs to set
	* to 1
	*/
	unsigned int data_pullup;

        /*
	 * Temporary flag to turn on UHS for eMMC until PMU regulator framework
	 * is integrated
	 */
	int tmp_uhs;

	/* for devices with 8-bit lines */
	int is_8bit;

	/* card detect gpio for SD/MMC cards */
	int cd_gpio;

	/* write protect gpio for SD/MMC cards */
	int wp_gpio;

	enum sdio_devtype devtype;

	/* Flags describing various platform options
	 * removable or not, for example while the SD is removable eMMC is not
	 */
	enum kona_sdio_plat_flags flags;

	/* clocks */
	unsigned long peri_clk_rate;

	/* need regulator to perform voltage switch for UHS mode */
	int need_regulator_uhs;

	struct sdio_wifi_gpio_cfg wifi_gpio;

	/* Call back added for unified DHD support */
#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT
	int (*register_status_notify) (void (*callback)
				       (int card_present, void *dev_id),
				       void *dev_id);
#endif

	int (*configure_sdio_pullup) (bool pull_up);
};

#endif  /* SDIO_PLATFORM_H */
