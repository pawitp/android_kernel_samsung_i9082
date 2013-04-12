/*****************************************************************************
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
*****************************************************************************/
#ifndef _AL3006_H
#define _AL3006_H


#ifdef CONFIG_ARCH_KONA

#define AL3006_NAME "al3006"

/* platform data for the al3006 driver */
struct al3006_platform_data {
	int	irq_gpio;
};

#endif

#endif
