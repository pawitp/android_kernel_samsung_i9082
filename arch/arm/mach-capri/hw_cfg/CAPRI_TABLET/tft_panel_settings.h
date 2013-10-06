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

#ifndef TFT_PANEL_H
#define TFT_PANEL_H

#define TFT_PANEL_SETTINGS \
{ \
	.gpio_lcd_pwr_en     = 4,	/* LCD_PWR_EN */ \
	.gpio_lcd_reset      = 5,	/* LCD_RST_B */ \
	.gpio_bl_en          = -1,	/* LCD_BL_EN */ \
	.gpio_bl_pwr_en      = 100,	/* LCD_BL_BST_EN */ \
	.gpio_bl_pwm         = 6,	/* LCD_BL_PWM */ \
}

#endif /* TFT_PANEL_H */
