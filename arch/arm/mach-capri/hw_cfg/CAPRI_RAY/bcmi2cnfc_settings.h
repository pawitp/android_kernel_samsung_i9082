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

#include <linux/i2c.h>

#define BCMI2CNFC_BUSID  0
#define BCMI2CNFC_ADDR   0x1FA
#define BCMI2CNFC_WAKE   141
#define BCMI2CNFC_EN     139
#define BCMI2CNFC_IRQ    140
#define BCMI2CNFC_FLAGS  I2C_CLIENT_TEN
