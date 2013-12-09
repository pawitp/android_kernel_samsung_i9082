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

#ifndef _ISL29044_SETTINGS_H_
#define _ISL29044_SETTINGS_H_

#define ISL29044_I2C_BUS_ID     0
#define ISL29044_I2C_ADDRESS    0x44

#define ISL29044_PLATFORM_DATA \
{\
	.ps_max_thr = 100,\
	.ps_reflect_thr = 200,\
	.als_diff = 10,\
}

#endif /* _ISL29044_SETTINGS_H_ */
