/*******************************************************************************
* Copyright 2010,2011 Broadcom Corporation.  All rights reserved.
*
*	@file	kona_avs.h
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

#ifndef __KONA_AVS___
#define __KONA_AVS___

#define ATE_FIELD_RESERVED	(0xFF)

#define AVS_NUM_MONITOR 4

enum {
	SILICON_SS,
	SILICON_TS,
	SILICON_TT,
	SILICON_TF,
	SILICON_FF
};

enum {
	AVS_TYPE_OPEN = 1,
	AVS_TYPE_BOOT = 1 << 1,
	AVS_READ_FROM_OTP = 1 << 2,	/*specify OTP row no as param */
	AVS_READ_FROM_MEM = 1 << 3,	/*specify physical mem addr as param */
	AVS_ATE_FEATURE_ENABLE = 1 << 4,
};

struct kona_ate_lut_entry_csr {
	u32 freq;
	u32 silicon_type;
};


struct kona_avs_lut_entry {
	u32 slow;
	u32 typical_slow;
	u32 typical_typical;
	u32 typical_fast;
	u32 fast;
};

struct kona_avs_pdata {
	u32 flags;
	u32 avs_mon_addr;
	u32 avs_ate_addr;
	int ate_default_silicon_type; /* default silicon type when CRC fails */
	int ate_default_cpu_freq;	/* default CPU FREQ when CRC fails */

	void (*silicon_type_notify) (u32 silicon_type_csr,
		u32 silicon_type_msr, u32 silicon_type_vsr, int freq_id);

	u32 irdrop_bin_size;

#if 0
	u32 nmos_bin_size;
	u32 pmos_bin_size;

	u32 *svt_pmos_bin;
	u32 *svt_nmos_bin;

	u32 *lvt_pmos_bin;
	u32 *lvt_nmos_bin;

	u32 *svt_silicon_type_lut;
	u32 *lvt_silicon_type_lut;
#endif

	struct kona_avs_lut_entry *avs_lut_irdrop;
	struct kona_avs_lut_entry *avs_lut_msr;
	struct kona_avs_lut_entry *avs_lut_vsr;

	struct kona_ate_lut_entry_csr *ate_lut_csr;
	u32 *ate_lut_msr;
	u32 *ate_lut_vsr;
};

/* u32 kona_avs_get_solicon_type(void); */
u8 *kona_avs_get_volt_table(void);

#endif	  /*__KONA_AVS___*/
