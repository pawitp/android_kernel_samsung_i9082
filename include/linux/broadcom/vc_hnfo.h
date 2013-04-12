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

#if !defined(VC_HNFO_H)
#define VC_HNFO_H

#include <linux/ioctl.h>

struct vc_hnfo_usr_info {
	/* Whether we want to power on explicit or power on
	 * detection only. */
	int power_on;
	/* Disable action on detection, useful for debug. */
	int dis_detect;
	/* Preferred default resolution. */
	int resolution;
	/* Disable cloning of the GUI explicitely. */
	int dis_clone;
	/* Enable special behavior for wifi-hdmi audio. */
	int whdmi_audio;
};

#define VC_HNFO_IOC_MAGIC  'H'

#define VC_HNFO_IOC_INFO \
	_IOR(VC_HNFO_IOC_MAGIC, 0, struct vc_hnfo_usr_info)

#endif /* VC_HNFO_H */
