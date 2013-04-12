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
#ifndef ESUB_CLKMGR_DEFS_H
#define ESUB_CLKMGR_DEFS_H

#include <mach/rdb/brcm_rdb_esub_clk_mgr_reg.h>

/* Mnemonic mappings to resolve inconsistencies between RDB files */
#define ESUB_CLK_MGR_REG_PLL_POST_RESETB_OFFSET \
	ESUB_CLK_MGR_REG_PLLE_POST_RESETB_OFFSET

#define ESUB_CLK_MGR_REG_PLL_POST_RESETB_I_POST_RESETB_MASK \
	ESUB_CLK_MGR_REG_PLLE_POST_RESETB_I_POST_RESETB_PLLE_MASK

#define ESUB_CLK_MGR_REG_PLL_RESETB_OFFSET \
	ESUB_CLK_MGR_REG_PLLE_RESETB_OFFSET

#define ESUB_CLK_MGR_REG_PLL_RESETB_I_PLL_RESETB_MASK \
	ESUB_CLK_MGR_REG_PLLE_RESETB_I_PLL_RESETB_PLLE_MASK

#define ESUB_CLK_MGR_REG_PLL_LOCK_PLL_LOCK_MASK \
	ESUB_CLK_MGR_REG_PLL_LOCK_PLL_LOCK_PLLE_MASK

#endif /* ESUB_CLKMGR_DEFS_H */
