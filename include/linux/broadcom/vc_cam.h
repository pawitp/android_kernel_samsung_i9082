/*****************************************************************************
* Copyright 2012 Broadcom Corporation.  All rights reserved.
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

#if !defined(VC_CAM_H)
#define VC_CAM_H

#include <linux/ioctl.h>

#define VC_CAM_IOC_MAGIC 0xd1

#ifdef __KERNEL__
extern void __init vc_cam_early_init(void);
#endif

/* IOCTL commands */
enum vc_fw_cmd_e {
	VC_CAM_FW_MODE,
	VC_CAM_FW_LAST            /* Do no delete */
};

enum vc_fw_ctrl_action_e {
	VC_CAM_FW_CTRL_UPDATE,
	VC_CAM_FW_CTRL_DUMP,
};

/* IOCTL Data structures */
struct vc_cam_fw_ioctl {
	enum vc_fw_ctrl_action_e action;
};

#define VC_CAM_IOCTL_FW_MODE \
	_IOR(VC_CAM_IOC_MAGIC, VC_CAM_FW_MODE,\
	struct vc_cam_fw_ioctl)

#endif /* VC_CAM_H */

