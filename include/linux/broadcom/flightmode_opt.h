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

#ifndef FLIGHTMODE_OPT_H
#define FLIGHTMODE_OPT_H

#include <linux/ioctl.h>

#define FLIGHTMODE_OPT_MAGIC 0xcf

/* IOCTLs */
#define FLIGHTMODE_OPT_IOCTL_ENABLE_OPT  _IO(FLIGHTMODE_OPT_MAGIC, 0)
#define FLIGHTMODE_OPT_IOCTL_DISABLE_OPT _IO(FLIGHTMODE_OPT_MAGIC, 1)

#endif /* FLIGHTMODE_OPT_H */
