/*****************************************************************************
* Copyright 2009-2010 Broadcom Corporation.  All rights reserved.
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

#ifndef _TANGO_TS_H_
#define _TANGO_TS_H_

#define I2C_TS_DRIVER_NAME			"tango_ts"
#define TANGO_S32_SLAVE_ADDR		0x5C
#define TANGO_M29_SLAVE_ADDR		0x45
#define TANGO_M29_SLAVE_ADDR_1		0x46
#define TANGO_S32_LAYOUT			(X_RIGHT_Y_UP)
#define TANGO_M29_LAYOUT			(X_RIGHT_Y_DOWN)

/*  y
 *  ^             +---->x
 *  |      or     |
 *  |             |
 *  +--->x        v
 *                y
 * X_RIGHT_Y_UP  X_RIGHT_Y_DOWN
 */
typedef enum
{
	X_RIGHT_Y_UP,
	X_RIGHT_Y_DOWN,
	X_LEFT_Y_UP,
	X_LEFT_Y_DOWN,
} SCREEN_XY_LAYOUT_e;

struct TANGO_I2C_TS_t
{
	int gpio_irq_pin;
	int gpio_reset_pin;
	int x_max_value;
	int y_max_value;
	SCREEN_XY_LAYOUT_e layout;
	int is_multi_touch;
	int is_resetable;
	int max_finger_val;
	int panel_width;  /* LCD panel width in millimeters */
};

#endif    /* _TANGO_TS_H_ */

