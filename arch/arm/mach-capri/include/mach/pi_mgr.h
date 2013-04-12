/****************************************************************************
*
* Copyright 2010 --2011 Broadcom Corporation.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
*****************************************************************************/

#ifndef __CAPRI_PI_MNGR_H__
#define __CAPRI_PI_MNGR_H__

enum {
	/*PI_MGR_PI_ID_MM, */
	PI_MGR_PI_ID_HUB_SWITCHABLE,
	PI_MGR_PI_ID_HUB_AON,
	PI_MGR_PI_ID_ARM_CORE,
	PI_MGR_PI_ID_ARM_SUB_SYSTEM,
#ifdef CONFIG_MACH_CAPRI_ESUB_ENABLE
	PI_MGR_PI_ID_ESUB,
	PI_MGR_PI_ID_ESUB_SUB,
#endif
	PI_MGR_PI_ID_MM,
	PI_MGR_PI_ID_MM_SUB,
	PI_MGR_PI_ID_MM_SUB2,
	PI_MGR_PI_ID_MODEM,
	PI_MGR_PI_ID_MAX
};

enum {
	PI_STATE_ACTIVE,
	PI_STATE_RETENTION,
	PI_STATE_SHUTDOWN,
};

enum {
	ARM_CORE_STATE_ACTIVE,
	ARM_CORE_STATE_SUSPEND,
	ARM_CORE_STATE_RETENTION,
	ARM_CORE_STATE_DORMANT,
};

enum {
	PI_OPP_ECONOMY,
	PI_OPP_NORMAL,
	PI_OPP_TURBO,
	PI_OPP_MAX
};

enum {
#ifdef CONFIG_CAPRI_156M
	PI_PROC_OPP_ECONOMY,
	PI_PROC_OPP_ECONOMY1,
	PI_PROC_OPP_NORMAL,
	PI_PROC_OPP_TURBO1,
	PI_PROC_OPP_TURBO,
	PI_PROC_OPP_MAX
#else
	PI_PROC_OPP_ECONOMY1,
	PI_PROC_OPP_NORMAL,
	PI_PROC_OPP_TURBO1,
	PI_PROC_OPP_TURBO,
	PI_PROC_OPP_MAX
#endif
};
void capri_pi_mgr_init(void);
int capri_pi_mgr_print_act_pis(void);

#endif /*__CAPRI_PI_MNGR_H__*/
