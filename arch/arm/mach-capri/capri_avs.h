/*****************************************************************************
*
* Power Manager config parameters for Rhea platform
*
* Copyright 2010 Broadcom Corporation.  All rights reserved.
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

#ifndef __CAPRI_AVS_H__
#define __CAPRI_AVS_H__

#define SR_VLT_LUT_SIZE 16

#define ARRAY_LIST(...) {__VA_ARGS__}

/*supported A9 freqs*/
enum {
	A9_FREQ_1_GHZ,
	A9_FREQ_1200_MHZ,
	A9_FREQ_1400_MHZ,
	A9_FREQ_MAX
};

enum {
	OFF = 0,
	RETENTION,
	WAKEUP,
	CSR_ECON1,
	CSR_ECON2,
	MSR_ECON,
	UNUSED_ROW6,
	CSR_NORMAL,
	MSR_NORMAL,
	UNUSED_ROW9,
	CSR_TURBO1,
	MSR_TURBO,
	UNUSED_ROW12,
	UNUSED_ROW13,
	VSR_NORMAL,
	CSR_TURBO2
};

/*This API should be defined in appropriate PMU board file*/
extern const u8 *bcmpmu_get_sr_vlt_table(int sr, u32 freq_inx,
					 u32 silicon_type_csr,
					 u32 silicon_type_msr,
					 u32 silicon_type_vsr);
extern int pm_init_pmu_sr_vlt_map_table(u32 silicon_type_csr,
					u32 silicon_type_msr,
					u32 silicon_type_vsr, int freq_id);

#endif	/*__PM_PARAMS_H__*/
