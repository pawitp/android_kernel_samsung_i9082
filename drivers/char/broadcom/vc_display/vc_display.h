/*******************************************************************************
* Copyright 2012 Broadcom Corporation.  All rights reserved.
*
* Unless you and Broadcom execute a separate written software license agreement
* governing use of this software, this software is licensed to you under the
* terms of the GNU General Public License version 2, available at
* http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*******************************************************************************/

#ifndef VC_DISPLAY_H
#define VC_DISPLAY_H

int vc_display_bus_write(int unsigned display,
			 uint8_t destination,
			 const uint8_t *data, size_t count);

int vc_display_bus_read(int unsigned display,
			uint8_t source, uint8_t *data, size_t count);

#endif
