/*****************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
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

#ifndef GPIO_KEYS_SETTINGS_H
#define GPIO_KEYS_SETTINGS_H

/*
 * GPIO keys configurations
 */
#if defined (CONFIG_KEYBOARD_GPIO_REV03)
#define GPIO_KEYS_SETTINGS { \
	{ KEY_HOME, 63, 1, "HOME", EV_KEY, 0, 64}, \
	{ KEY_VOLUMEUP, 125, 1, "Volume up", EV_KEY, 0, 64}, \
	{ KEY_VOLUMEDOWN, 62, 1, "Volume down", EV_KEY, 0, 64}, \
}
#else
#define GPIO_KEYS_SETTINGS { \
	{ KEY_HOME, 63, 1, "HOME", EV_KEY, 0, 64}, \
	{ KEY_VOLUMEUP, 61, 1, "Volume up", EV_KEY, 0, 64}, \
	{ KEY_VOLUMEDOWN, 62, 1, "Volume down", EV_KEY, 0, 64}, \
}
#endif

#endif /* GPIO_KEYS_SETTINGS_H */
