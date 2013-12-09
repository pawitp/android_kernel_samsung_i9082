/*
	$License:
	Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	$
 */

#ifndef __ISL29044_H_
#define __ISL29044_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define ISL29044_MODULE_NAME	"isl29044"

typedef struct isl29044_val
{
	int8_t type;
	int32_t val;
} isl29044_val_t;

#define ISL29044_EVENT_NUM 64
#define ALS_EVENT 0
#define PS_EVENT 1

#define ISL29044_IOCTL (0x12) /* Magic number for ISL29044 iocts */
/* IOCTL commands for /dev/isl29044 */

#define ISL29044_ALS_ENABLE	_IOW(ISL29044_IOCTL, 0x01, int8_t)
#define ISL29044_PS_ENABLE	_IOW(ISL29044_IOCTL, 0x02, int8_t)
#define ISL29044_SET_ALS_DELAY	_IOW(ISL29044_IOCTL, 0x03, int64_t)
#define ISL29044_SET_PS_DELAY	_IOW(ISL29044_IOCTL, 0x04, int64_t)

struct isl29044_platform_data
{
	uint8_t ps_max_thr;
	uint8_t ps_reflect_thr;
	uint16_t als_diff;
};

#endif /* __ISL29044_H_ */
