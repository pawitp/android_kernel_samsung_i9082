/****************************************************************************
*
* Copyright 2010 --2012 Broadcom Corporation.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
*****************************************************************************/

#ifndef __ARM_ARCH_KONA_BLOCKER_H
#define __ARM_ARCH_KONA_BLOCKER_H

typedef int (*blocker_fn_t)(void *arg);
extern int pause_other_cpus(blocker_fn_t fn, void *arg);

#endif /*__ARM_ARCH_KONA_CLOCK_H*/
