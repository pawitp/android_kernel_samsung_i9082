/* include/linux/hscdtd.h
 *
 * GeoMagneticField device driver
 *
 * Copyright (C) 2012 ALPS ELECTRIC CO., LTD. All Rights Reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef HSCDTD_H
#define HSCDTD_H

#define HSCDTD_DRIVER_NAME	"HSCDTD006A"

#define HSCDTD_X_INDX 0
#define HSCDTD_Y_INDX 1
#define HSCDTD_Z_INDX 2
#define HSCDTD_DIR_NORMAL  1
#define HSCDTD_DIR_INVERT -1

struct hscdtd_platform_data {
	uint8_t x_indx;
	uint8_t y_indx;
	uint8_t z_indx;
	int8_t x_dir;
	int8_t y_dir;
	int8_t z_dir;
	void (*power_mode)(int enable);
};

#endif

