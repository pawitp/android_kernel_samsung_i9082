/*****************************************************************************
* Copyright 2011 Broadcom Corporation.  All rights reserved.
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

extern struct mmc_platform_data tuna_wifi_data;

int capri_wlan_init(void);

#define CAPRI_GPIO_HYS_EN		1
#define CAPRI_GPIO_HYS_DIS		0

#define CAPRI_GPIO_PULL_DN_EN	ON
#define CAPRI_GPIO_PULL_DN_DIS	OFF

#define CAPRI_GPIO_PULL_UP_EN	ON
#define CAPRI_GPIO_PULL_UP_DIS	OFF

#define CAPRI_GPIO_SLEW_EN		1
#define CAPRI_GPIO_SLEW_DIS		0

#define CAPRI_GPIO_INPUT_EN		1
#define CAPRI_GPIO_INPUT_DIS		0

#define CAPRI_GPIO_DRV_CURR_4	4MA
#define CAPRI_GPIO_DRV_CURR_6	6MA
#define CAPRI_GPIO_DRV_CURR_8	8MA
#define CAPRI_GPIO_DRV_CURR_10	10MA
#define CAPRI_GPIO_DRV_CURR_12	12MA
#define CAPRI_GPIO_DRV_CURR_16	16MA

/* int WARN_ON(int x);
*/

#define omap4_ctrl_wk_pad_writel(x, y)     0
