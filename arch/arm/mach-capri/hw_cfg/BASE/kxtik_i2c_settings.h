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

#ifndef KXTIK_I2C_SETTINGS_H
#define KXTIK_I2C_SETTINGS_H

#define KXTIK_GPIO_IRQ_PIN		(64)
#define KXTIK_I2C_BUS_ID		(0)

#define KXTIK_DEVICE_MAP		(2)
#define KXTIK_MAP_X			((KXTIK_DEVICE_MAP-1)%2)
#define KXTIK_MAP_Y			(KXTIK_DEVICE_MAP%2)
#define KXTIK_NEG_X			(((KXTIK_DEVICE_MAP+2)/2)%2)
#define KXTIK_NEG_Y			(((KXTIK_DEVICE_MAP+5)/4)%2)
#define KXTIK_NEG_Z			((KXTIK_DEVICE_MAP-1)/4)

#endif /* KXTIK_I2C_SETTINGS_H */
