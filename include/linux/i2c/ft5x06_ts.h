/*
* drivers/input/touchscreen/ft5x0x_ts.c
*
* FocalTech ft5x0x TouchScreen driver.
*
* Copyright (c) 2010 Focal tech Ltd.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
*/

#ifndef _FT5X06_TS_H_
#define _FT5X06_TS_H_

#define I2C_FT5X06_DRIVER_NAME		"ft5x06-ts"
#define FT5X06_SLAVE_ADDR		(0x70 >> 1)


#define FT5X06_IS_MULTI_TOUCH		1
#define FT5X06_MAX_NUM_FINGERS		5

#define FT5X06_MAX_X			720
#define FT5X06_MAX_Y			1280

#define FT5X06_PRESS			0x7F
#define FT5X06_PRESS_MAX		0xFF


#define FT5X06_LAYOUT			(FT5X0506_X_RIGHT_Y_DOWN)

/*  y
 *  ^             +---->x
 *  |      or     |
 *  |             |
 *  +--->x        v
 *                y
 * X_RIGHT_Y_UP  X_RIGHT_Y_DOWN
 */
enum FT5X06_SCREEN_XY_LAYOUT_e {
	FT5X0506_X_RIGHT_Y_UP,
	FT5X0506_X_RIGHT_Y_DOWN,
	FT5X0506_X_LEFT_Y_UP,
	FT5X0506_X_LEFT_Y_DOWN,
} ;

struct ft5x06_platform_data {
	int gpio_irq_pin;
	int gpio_reset_pin;
	int x_max_value;
	int y_max_value;
	enum FT5X06_SCREEN_XY_LAYOUT_e layout;
	int is_multi_touch;
	int is_resetable;
	int max_finger_val;
	int press_max;
	int press_value;
};

#endif    /* _FT5X06_TS_H_ */
