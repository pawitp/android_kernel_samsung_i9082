/*
 * Copyright (C) 2013 Samsung Electronics
 * Jeongrae Kim <jryu.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef _TSU6721_H_
#define _TSU6721_H_

enum {
	TSU6721_DETACHED,
	TSU6721_ATTACHED
};

enum {
	DISABLE,
	ENABLE
};

enum {
	DOCK_UI_DESK = 1,
	DOCK_UI_CAR
};

#define TSU6721_I2C_BUS_ID 8
#define GPIO_USB_I2C_SDA 131
#define GPIO_USB_I2C_SCL 132
#define GPIO_USB_INT 100


struct tsu6721_platform_data {

	void (*cfg_gpio) (void);
	void (*otg_cb) (bool attached);
	void (*usb_cb) (bool attached);
	void (*uart_cb) (bool attached);
	void (*charger_cb) (bool attached);
	void (*jig_cb) (bool attached);
	void (*deskdock_cb) (bool attached);
	void (*cardock_cb) (bool attached);
	void (*mhl_cb) (bool attached);
	void (*reset_cb) (void);
	void (*set_init_flag) (void);
	
	void (*callback)(enum cable_type_t cable_type, int attached);
	void (*ovp_cb)(bool attached);
	void (*mhl_sel) (bool onoff);
	int	(*dock_init) (void);
};

extern int poweroff_charging;

extern void tsu6721_otg_detach(void);
extern int check_jig_state(void);

#if defined(CONFIG_VIDEO_MHL_V2)
extern int dock_det(void);
#endif

extern struct class *sec_class;

#endif /* _TSU6721_H_ */


