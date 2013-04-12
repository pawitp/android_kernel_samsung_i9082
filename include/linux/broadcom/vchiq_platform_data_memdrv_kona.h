/*****************************************************************************
* Copyright 2008 - 2010 Broadcom Corporation.  All rights reserved.
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

#ifndef VCHIQ_PLATFORM_DATA_MEMDRV_KONA_H_
#define VCHIQ_PLATFORM_DATA_MEMDRV_KONA_H_

#include "vchiq_platform_data_memdrv.h"

/*
 * This structure mirrors the vc4_pin_config_t in u-boot, although there is
 * no requirement that it actually be the same.
 */
typedef struct {
	int gpio;
	int input;
	int initial_level;
	const char *description;

} VCHIQ_PLATFORM_MEMDRV_KONA_DATA_GPIO_T;

typedef struct {
	VCHIQ_PLATFORM_DATA_MEMDRV_T memdrv;

	unsigned int ipcIrq;

	unsigned int num_gpio_configs;
	VCHIQ_PLATFORM_MEMDRV_KONA_DATA_GPIO_T *gpio_config;

} VCHIQ_PLATFORM_DATA_MEMDRV_KONA_T;

#endif /* VCHIQ_PLATFORM_DATA_MEMDRV_KONA_H_ */
