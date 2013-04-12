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

#ifndef USBH_SETTINGS_H
#define USBH_SETTINGS_H

/*
 * Refer to mach/usbh.h for details
 */

#define HW_USBH_PARAM \
{ \
	.peri_clk_name = USBH_48M_PERI_CLK_NAME_STR, \
	.ahb_clk_name = USBH_AHB_BUS_CLK_NAME_STR, \
	.opt_clk_name = USBH_12M_PERI_CLK_NAME_STR, \
	.num_ports = 1, /* 1 because 2nd port need mod. */ \
	.port = { \
		[0] = { \
			.pwr_gpio = 43,		/* USBH1_PWR_EN */ \
			.pwr_flt_gpio = 44,	/* USBH1_PWR_FLT */ \
			.reset_gpio = -1,	/* Internal Phy */ \
		}, \
		[1] = { \
			.pwr_gpio = 4,		/* USBH2_PWR_EN */ \
			.pwr_flt_gpio = 0,	/* USBH2_PWR_FLT */ \
			.reset_gpio = -1,	/* Internal Phy */ \
		}, \
	}, \
}

/* HSIC */
#define HW_USBH_HSIC_PARAM \
{ \
	.peri_clk_name = USBH_48M_PERI_CLK_NAME_STR, \
	.ahb_clk_name = USBHSIC_AHB_BUS_CLK_NAME_STR, \
	.opt_clk_name = USBH_12M_PERI_CLK_NAME_STR, \
	.num_ports = 2, \
	.port = { \
		[0] = { \
			.pwr_gpio = -1,		/* HSIC1_PWR_EN */ \
			.pwr_flt_gpio = -1,	/* HSIC1_PWR_FLT */ \
			.reset_gpio = 103,	/* Device reset */ \
		}, \
		[1] = { \
			.pwr_gpio = -1,		/* HSIC2_PWR_EN */ \
			.pwr_flt_gpio = -1,	/* HSIC2_PWR_FLT */ \
			.reset_gpio = 32,	/* Device reset */ \
		}, \
	}, \
}

#endif /* USBH_SETTINGS_H */
