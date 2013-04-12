/*****************************************************************************
* Copyright 2006 - 2011 Broadcom Corporation.  All rights reserved.
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

#ifndef KEYMAP_SETTINGS_H
#define KEYMAP_SETTINGS_H

#define HW_DEFAULT_KEYMAP \
{ \
	{ 0x00, 158 }, /* SEARCH */ \
	{ 0x01, 139 }, /* MENU */ \
	{ 0x02, 102 }, /* HOME */ \
	{ 0x03, 217 }, /* BACK */ \
}

#define HW_DEFAULT_POWEROFF { }	/* Power button goes to the PMU directly */

#endif
