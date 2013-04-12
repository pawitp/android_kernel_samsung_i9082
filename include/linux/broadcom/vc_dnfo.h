/*****************************************************************************
* Copyright 2010 - 2011 Broadcom Corporation.  All rights reserved.
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

#if !defined(VC_DNFO_H)
#define VC_DNFO_H

#include <linux/ioctl.h>

/* These need to be kept consistent. */
#define VC_DNFO_FRAC_MULTIPLIER 100000
#define VC_DNFO_FRAC_DIGITS     5

struct vc_dnfo_display_info {
	unsigned int width;	/* Base width */
	unsigned int height;	/* Base height */

	unsigned int scale;	/* Whether to scale or not the base dimension */
	unsigned int swidth;	/* Scaled width */
	unsigned int sheight;	/* Scaled height */

	unsigned int bpp;	/* Bits per pixel for display */
	unsigned int layer;	/* VC layer to put the display on */

	/* Dots Per Inch values * VC_DNFO_FRAC_MULTIPLIER */
	unsigned int xdpi;
	unsigned int ydpi;

	unsigned int smtex;	/* Shared memory for texture upload */

	/* Frames per second * VC_DNFO_FRAC_MULTIPLIER */
	unsigned int fps;

	unsigned int frac_scale; /* VC_DNFO_FRAC_MULTIPLIER */
};

#define VC_DNFO_IOC_MAGIC  'I'

#define VC_DNFO_IOC_DISPLAY_INFO \
	_IOR(VC_DNFO_IOC_MAGIC, 0, struct vc_dnfo_display_info)

#endif /* VC_DNFO_H */
