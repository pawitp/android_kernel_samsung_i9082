/*****************************************************************************
*  Copyright 2011 - 2012 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/licenses/old-license/gpl-2.0.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/

#ifndef __ATAGDT_H_
#define __ATAGDT_H_


/*
  This is a Broadcom Kona specific ATAG for DT-blob.
  Put it here instead of setup.h to avoid conflict when merging upstream.
*/
#define ATAG_DTBLOB	0x54411122

extern int early_init_dt_scan_pinmux(unsigned long node, const
				char *uname, int depth, void *data);

extern int early_init_dt_scan_gpio(unsigned long node, const
				char *uname, int depth, void *data);

#ifdef CONFIG_KONA_DT_BCMPMU
extern int early_init_dt_scan_pmu(unsigned long node, const
				  char *uname, int depth, void *data);
extern int early_init_dt_scan_batt(unsigned long node, const
				   char *uname, int depth, void *data);
#endif

#ifdef CONFIG_BCM_VC_SERVICES
extern int early_init_dt_scan_vc(unsigned long node, const
				  char *uname, int depth, void *data);
#endif

#endif /* __ATAGDT_H_ */
