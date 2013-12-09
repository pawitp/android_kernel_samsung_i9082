/*******************************************************************************
 * Copyright 2010,2011 Broadcom Corporation.  All rights reserved.
 *
 *	@file	arch/arm/plat-kona/kona_avs.c
 *
 * Unless you and Broadcom execute a separate written software license agreement
 * governing use of this software, this software is licensed to you under the
 * terms of the GNU General Public License version 2, available at
 * http://www.gnu.org/copyleft/gpl.html (the "GPL").
 *
 * Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *****************************************************************************/


#include <linux/kernel.h>
#include <linux/err.h>
#ifdef CONFIG_KONA_OTP
#include <plat/bcm_otp.h>
#endif
#include <plat/kona_avs.h>
#include <linux/platform_device.h>
#include <linux/io.h>


/*#define KONA_AVS_DEBUG*/

/*Should we move this to avs_param ?? */
#define MONITOR_VAL_MASK	   0xFF
#define MONITOR_VAL0_SHIFT	 8
#define MONITOR_VAL1_SHIFT	 16
#define MONITOR_VAL2_SHIFT	 24
#define MONITOR_VAL3_SHIFT	 0

#define AVS_ATE_MONTH_MASK  (0xF)
#define AVS_ATE_YEAR_MASK   (0xF0)
#define AVS_ATE_CSR_BIN_MASK	(0xF00)
#define AVS_ATE_MSR_BIN_MASK	(0xF)
#define AVS_ATE_VSR_BIN_MASK	(0xF0)
#define AVS_ATE_CRC_MASK	(0xF000)
#define AVS_ATE_IRDROP_MASK (0x3FF0000)

#define AVS_ATE_MONTH_SHIFT (0)
#define AVS_ATE_YEAR_SHIFT  (4)
#define AVS_ATE_CSR_BIN_SHIFT   (8)
#define AVS_ATE_MSR_BIN_SHIFT   (0)
#define AVS_ATE_VSR_BIN_SHIFT   (4)
#define AVS_ATE_CRC_SHIFT   (12)
#define AVS_ATE_IRDROP_SHIFT	(16)

#define avs_dbg(level, args...) \
	do { \
		if (debug_mask & level) { \
			pr_info(args); \
		} \
	} while (0)


enum {
	AVS_LOG_ERR  = 1 << 0,
	AVS_LOG_WARN = 1 << 1,
	AVS_LOG_INIT = 1 << 2,
	AVS_LOG_FLOW = 1 << 3,
	AVS_LOG_INFO = 1 << 4,
};

struct silicon_type {
	 u32 csr_type;
	 u32 msr_type;
	 u32 vsr_type;
};

struct avs_info {
	u32 monitor_val0;
	u32 monitor_val1;
	u32 monitor_val2;
	u32 monitor_val3;

	u32 silicon_type_csr;
	u32 silicon_type_msr;
	u32 silicon_type_vsr;


	u32 ate_silicon_type_csr;
	u32 ate_silicon_type_msr;
	u32 ate_silicon_type_vsr;

	u32 ate_csr_bin;
	u32 ate_msr_bin;
	u32 ate_vsr_bin;

	u32 avs_irdrop;

	u32 freq;
	u32 ate_crc;
	u32 year;
	u32 month;
	struct kona_avs_pdata *pdata;
};

struct avs_info avs_info = {.silicon_type_csr = SILICON_TS,
					.silicon_type_msr = SILICON_TS,
					.silicon_type_vsr = SILICON_TS, };

static int debug_mask = AVS_LOG_ERR | AVS_LOG_WARN | AVS_LOG_INIT |\
			AVS_LOG_FLOW | AVS_LOG_INFO;


module_param_named(avs_mon_val0, avs_info.monitor_val0, int,
					S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(avs_mon_val1, avs_info.monitor_val1, int,
					S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(avs_mon_val2, avs_info.monitor_val2, int,
					S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(avs_mon_val3, avs_info.monitor_val3, int,
					S_IRUGO | S_IWUSR | S_IWGRP);

module_param_named(avs_ate_csr_bin, avs_info.ate_csr_bin, int,
					S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(avs_ate_msr_bin, avs_info.ate_msr_bin, int,
					S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(avs_ate_vsr_bin, avs_info.ate_vsr_bin, int,
					S_IRUGO | S_IWUSR | S_IWGRP);

module_param_named(avs_irdrop, avs_info.avs_irdrop, int,
					S_IRUGO | S_IWUSR | S_IWGRP);


module_param_named(year, avs_info.year, int,
					S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(month, avs_info.month, int,
					S_IRUGO | S_IWUSR | S_IWGRP);
module_param_named(ate_crc, avs_info.ate_crc, int,
					S_IRUGO | S_IWUSR | S_IWGRP);


module_param_named(debug_mask, debug_mask, int,
					S_IRUGO | S_IWUSR | S_IWGRP);

struct trigger_avs {
	int dummy;
};

#define __param_check_trigger_avs(name, p, type) \
	static inline struct type *__check_##name(void) { return (p); }

#define param_check_trigger_avs(name, p) \
	__param_check_trigger_avs(name, p, trigger_avs)

static int param_set_trigger_avs(const char *val,
			const struct kernel_param *kp);
static struct kernel_param_ops param_ops_trigger_avs = {
	.set = param_set_trigger_avs,
};

static struct trigger_avs trigger_avs;
module_param_named(trigger_avs, trigger_avs, trigger_avs,
				S_IWUSR | S_IWGRP);


struct mon_val {

	u32 val0;
	u32 val1;
};

struct ate_val {
	u32 val0;
	u32 val1;
};

#if defined(KONA_AVS_DEBUG)

static int otp_read_mon_val(int row, struct mon_val *mon_val)
{
	avs_dbg(AVS_LOG_INFO, "%s:row = %d\n", __func__, row);

	if (row < 0)
		return -EINVAL;
	mon_val->val0 =
		(0x7f << MONITOR_VAL0_SHIFT) | (0xb2 << MONITOR_VAL1_SHIFT) |
			(0x76 << MONITOR_VAL2_SHIFT);
	mon_val->val1 = 0x8f;

	avs_dbg(AVS_LOG_INFO, "%s:mon_val->val0 = %d, mon_val->val1 = %d\n",
			 __func__, mon_val->val0, mon_val->val1);

	return 0;
}


static int otp_read_ate_val(int row, struct ate_val *ate_val)
{
	avs_dbg(AVS_LOG_INFO, "%s:row = %d\n", __func__, row);

	if (row < 0)
		return -EINVAL;
	ate_val->val0 =
		(0x09 << AVS_ATE_MONTH_SHIFT) |
		(0x02 << AVS_ATE_YEAR_SHIFT) |
		(0x0b << AVS_ATE_CSR_BIN_SHIFT)  |
		(0x1D7 << AVS_ATE_IRDROP_SHIFT);

	ate_val->val1 =
		(0x03 << AVS_ATE_VSR_BIN_SHIFT)  |
		(0x02 << AVS_ATE_MSR_BIN_SHIFT);

	avs_dbg(AVS_LOG_INFO, "%s:ate_val->val0 = %d, ate_val->val1 = %d\n",
			 __func__, ate_val->val0, ate_val->val1);

	return 0;
}

#endif
#if 0
u32 kona_avs_get_solicon_type(void)
{
	BUG_ON(avs_info.pdata == NULL);
	return avs_info.silicon_type;
}
EXPORT_SYMBOL(kona_avs_get_solicon_type);
#endif

/**
 * converts interger to string radix 2 (binary
 * number string)
 */
static void int2bin(unsigned int num, char *str)
{
	int i = 0;
	int j = 0;
	char temp[33];

	while (num != 0) {
		temp[i] = num % 2 ? '1' : '0';
		num /= 2;
		i++;
	}
	str[i] = 0;
	temp[i] = 0;

	/* reverse the string */
	while (i) {
		i--;
		str[j] = temp[i];
		j++;
	}
}

/**
 * 4-bit Linear feeback shift register implementation
 * based on primitive polynomial x^4 + x + 1
 *
 * bitstring should be null terminate
 * bit stream (exp: "10111111")
 */
static void cal_crc(const char *bitstring, char *crc_res)
{
	char crc[4];
	int i;
	char do_invert;

	avs_dbg(AVS_LOG_INFO, "%s:\n",  __func__);

	memset(crc, 0x0, sizeof(crc));

	for (i = 0; i < strlen(bitstring); ++i) {
		do_invert = ('1' == bitstring[i]) ^ crc[3];
		crc[3] = crc[2];
		crc[2] = crc[1];
		crc[1] = crc[0] ^ do_invert;
		crc[0] = do_invert;
	}

	for (i = 0; i < 4; ++i)
		crc_res[3-i] = crc[i] ? '1' : '0';
	crc_res[4] = 0; /* Null Terminated */
}

static int kona_avs_get_mon_val(struct avs_info *avs_inf_ptr)
{
	struct mon_val mon_val;
	int ret = -EINVAL;

	avs_dbg(AVS_LOG_INFO, "%s:\n",  __func__);

	if (avs_inf_ptr->pdata->flags & AVS_TYPE_BOOT) {
		avs_dbg(AVS_LOG_ERR,
				"%s:AVS_TYPE_BOOT not supported !!!\n",
				__func__);
		return -EINVAL;
	}

	if (avs_inf_ptr->pdata->flags & AVS_READ_FROM_MEM) {
		void __iomem *mem_ptr;
		avs_dbg(AVS_LOG_INIT, "    AVS_READ_FROM_MEM => mem adr = %x\n",
				avs_inf_ptr->pdata->avs_mon_addr);
		BUG_ON(avs_inf_ptr->pdata->avs_mon_addr == 0);


		mem_ptr =
			ioremap_nocache(avs_inf_ptr->pdata->avs_mon_addr,
					sizeof(struct mon_val));

		avs_dbg(AVS_LOG_INIT,
				"    AVS_READ_FROM_MEM => virtual addr = %p\n",
				mem_ptr);
		if (mem_ptr) {
			memcpy(&mon_val, mem_ptr, sizeof(struct mon_val));
			iounmap(mem_ptr);
			ret = 0;
		} else {
			ret = -ENOMEM;
			BUG_ON(mem_ptr == NULL);
		}
	} else {
		avs_dbg(AVS_LOG_INIT,
				"    AVS_READ_FROM_OTP => row = %x\n",
				avs_inf_ptr->pdata->avs_mon_addr);

#if defined(KONA_AVS_DEBUG) || defined(CONFIG_KONA_OTP)
		ret =
		otp_read_mon_val(avs_inf_ptr->pdata->avs_mon_addr, &mon_val);
#endif
	}

	if (!ret) {

		avs_dbg(AVS_LOG_INIT, "    opt:val0 = %x; val1 = %x\n",
				mon_val.val0,
				mon_val.val1);
		avs_inf_ptr->monitor_val0 =
			(mon_val.val0 >> MONITOR_VAL0_SHIFT) & MONITOR_VAL_MASK;
		avs_inf_ptr->monitor_val1 =
			(mon_val.val0 >> MONITOR_VAL1_SHIFT) & MONITOR_VAL_MASK;
		avs_inf_ptr->monitor_val2 =
			(mon_val.val0 >> MONITOR_VAL2_SHIFT) & MONITOR_VAL_MASK;
		avs_inf_ptr->monitor_val3 =
			(mon_val.val1 >> MONITOR_VAL3_SHIFT) & MONITOR_VAL_MASK;

		avs_dbg(AVS_LOG_INIT,
				"    monitor_val0 = %d\n"
				"    monitor_val1 = %d\n"
				"    monitor_val2 = %d\n"
				"    monitor_val3 = %d\n",
				avs_inf_ptr->monitor_val0,
				avs_inf_ptr->monitor_val1,
				avs_inf_ptr->monitor_val2,
				avs_inf_ptr->monitor_val3);
	}
	return ret;
}

static int kona_avs_get_ate_val(struct avs_info *avs_inf_ptr)
{
	struct ate_val ate_val;
	int ret = -EINVAL;
	void __iomem *mem_ptr;

	avs_dbg(AVS_LOG_FLOW, "%s:\n", __func__);

	if (avs_inf_ptr->pdata->flags & AVS_TYPE_BOOT) {
		avs_dbg(AVS_LOG_ERR, "%s: AVS_TYPE_BOOT not supported !!!\n",
				__func__);
		return -EINVAL;
	}

	if (avs_inf_ptr->pdata->flags & AVS_READ_FROM_MEM) {
		BUG_ON(avs_inf_ptr->pdata->avs_ate_addr == 0);
		avs_dbg(AVS_LOG_INIT,
				"    AVS_READ_FROM_MEM => mem adr = %x\n",
				avs_inf_ptr->pdata->avs_ate_addr);

		mem_ptr = ioremap_nocache(avs_inf_ptr->pdata->avs_ate_addr,
				sizeof(ate_val));
		avs_dbg(AVS_LOG_INIT,
				"    AVS_READ_FROM_MEM => virtual addr = %p\n",
				mem_ptr);
		if (mem_ptr) {
			memcpy(&ate_val, mem_ptr, sizeof(ate_val));
			iounmap(mem_ptr);
			ret = 0;
		} else {
			ret = -ENOMEM;
			BUG_ON(mem_ptr == NULL);
		}
	}
	if (!ret) {
		avs_dbg(AVS_LOG_INIT, "    ATE val0 = %x; val1 = %x\n",
				ate_val.val0,
				ate_val.val1);
		avs_inf_ptr->year = ((ate_val.val0 & AVS_ATE_YEAR_MASK) >>
				AVS_ATE_YEAR_SHIFT);
		avs_inf_ptr->month = ((ate_val.val0 & AVS_ATE_MONTH_MASK) >>
				AVS_ATE_MONTH_SHIFT);
		avs_dbg(AVS_LOG_INFO, "    AVS Year & Month of Manufacturing:"
				"%d %d\n",
				((avs_inf_ptr->year == 0) ? 2012 :
				 (2010 + avs_inf_ptr->year)),
				(avs_inf_ptr->month));

		avs_inf_ptr->ate_csr_bin =
				((ate_val.val0 & AVS_ATE_CSR_BIN_MASK) >>
				AVS_ATE_CSR_BIN_SHIFT);
		avs_inf_ptr->ate_msr_bin =
				((ate_val.val1 & AVS_ATE_MSR_BIN_MASK) >>
				AVS_ATE_MSR_BIN_SHIFT);
		avs_inf_ptr->ate_vsr_bin =
				((ate_val.val1 & AVS_ATE_VSR_BIN_MASK) >>
				AVS_ATE_VSR_BIN_SHIFT);

		avs_inf_ptr->ate_crc = ((ate_val.val0 & AVS_ATE_CRC_MASK) >>
				AVS_ATE_CRC_SHIFT);

		avs_inf_ptr->avs_irdrop =
				((ate_val.val0 & AVS_ATE_IRDROP_MASK) >>
				AVS_ATE_IRDROP_SHIFT);

		avs_dbg(AVS_LOG_INIT,
							"    ATE_CSR_BIN[3 : 0] = 0x%x\n"
							"    ATE_MSR_BIN[3 : 0] = 0x%x\n"
							"    ATE_VSR_BIN[3 : 0] = 0x%x\n"
							"    YEAR[3 : 0]        = 0x%x\n"
							"    MONTH[3 : 0]       = 0x%x\n"
							"    ATE_IRDROP_BIN[9:0] = 0x%x\n"
							"    CRC[3 : 0]         = 0x%x\n",
				avs_inf_ptr->ate_csr_bin,
				avs_inf_ptr->ate_msr_bin,
				avs_inf_ptr->ate_vsr_bin,
				avs_inf_ptr->year,
				avs_inf_ptr->month,
				avs_inf_ptr->avs_irdrop,
				avs_inf_ptr->ate_crc);
	}
	return ret;
}

static int kona_avs_ate_get_type(struct avs_info *avs_inf_ptr)
{
	struct kona_avs_pdata *pdata = avs_inf_ptr->pdata;
	char str[33];
	char pack[64];
	char crc[5];
	u32 temp1;
	u32 temp2;
	u32 temp3;
	long crc_val;
	int err;
	int i;

	memset(pack, 0, sizeof(pack));


	avs_dbg(AVS_LOG_INIT, "%s:\n"
			"   avs_inf_ptr->ate_csr_bin = %d\n"
			"   avs_inf_ptr->ate_msr_bin = %d\n"
			"   avs_inf_ptr->ate_vsr_bin = %d\n"
			"   avs_inf_ptr->ate_crc = 0x%x\n",
			__func__,
			avs_inf_ptr->ate_csr_bin,
			avs_inf_ptr->ate_msr_bin,
			avs_inf_ptr->ate_vsr_bin,
			avs_inf_ptr->ate_crc);

	if ((avs_inf_ptr->ate_csr_bin + avs_inf_ptr->ate_msr_bin +
		avs_inf_ptr->ate_vsr_bin == 0) && (avs_inf_ptr->ate_crc == 0)) {
		avs_dbg(AVS_LOG_INFO, "%s: No ATE data. Return\n", __func__);

		return -ENODATA;
	}

	BUG_ON(pdata->ate_lut_csr[avs_inf_ptr->ate_csr_bin].silicon_type ==
			ATE_FIELD_RESERVED);
	/**
	 * pack {IRDROP[9:0], CSR_ATE_AVS_BIN[3:0], Year[3:0],
	 * Month[3:0], VM3[7:0], VM2[7:0], VM1[7:0], VM0[7:0],
	 * VSR_ATE_AVS_BIN[3:0], MSR_ATE_AVS_BIN[3:0]} and calculate CRC
	 */
	temp1 = ((avs_inf_ptr->avs_irdrop << 12) |
			 (avs_inf_ptr->ate_csr_bin << 8) |
			 (avs_inf_ptr->year << 4) | (avs_inf_ptr->month));
	avs_dbg(AVS_LOG_INFO, "pack [IRDROP:ATE_CSR_BIN:YEAR:MONTH] = 0x%x\n",
						 temp1);
/* bit pattern length of IRDROP[9:0], CSR_ATE_AVS_BIN[3:0], Year[3:0],
	 * Month[3:0]*/
#define BIT_PATTERN_LENGTH1 (10 + 4 + 4 + 4)

/* bit pattern length of VM3[7:0], VM2[7:0], VM1[7:0], VM0[7:0] */
#define BIT_PATTERN_LENGTH2 (8 + 8 + 8 + 8)

/* bit pattern length of VSR_ATE_AVS_BIN[3:0], MSR_ATE_AVS_BIN[3:0] */
#define BIT_PATTERN_LENGTH3 (4 + 4)

	int2bin(temp1, str);
	for (i = 0; i < BIT_PATTERN_LENGTH1 - strlen(str); i++)
		strcat(pack, "0");

	strcat(pack, str);

	temp2 = ((avs_inf_ptr->monitor_val3 << 24)  |
			 (avs_inf_ptr->monitor_val2 << 16)  |
			 (avs_inf_ptr->monitor_val1 << 8)   |
			 (avs_inf_ptr->monitor_val0));
	avs_dbg(AVS_LOG_INFO, "pack [VM3:2:1:0] = 0x%x\n", temp2);
	int2bin(temp2, str);
	for (i = 0; i < BIT_PATTERN_LENGTH2 - strlen(str); i++)
		strcat(pack, "0");

	strcat(pack, str);
	avs_dbg(AVS_LOG_INFO, "packed [ATE:VM] string for CRC :\n%s\n", pack);

	temp3 = ((avs_inf_ptr->ate_vsr_bin << 4) |
			 (avs_inf_ptr->ate_msr_bin));
	avs_dbg(AVS_LOG_INFO, "pack [ATE_VSR_BIN:ATE_MSR_BIN] = 0x%x\n", temp3);
	int2bin(temp3, str);
	for (i = 0; i < BIT_PATTERN_LENGTH3 - strlen(str); i++)
		strcat(pack, "0");

	strcat(pack, str);
	avs_dbg(AVS_LOG_INFO, "packed [ATE:VM] string for CRC :\n%s\n", pack);


	cal_crc(pack, crc);
	/**
	 * if CRC fails, we will assume 1000MHZ and typical slow silicon type
	 */
	err = kstrtol(crc, 2, &crc_val);
	if (err) {
		avs_dbg(AVS_LOG_ERR, "kstrtol returned error\n");
		return err;
	}

	avs_dbg(AVS_LOG_INIT, "Calcualted ATE CRC value = 0x%x\n",
		(u32)crc_val);

	if (avs_inf_ptr->ate_crc != crc_val) {
		avs_dbg(AVS_LOG_ERR, "ATE CRC Failed. "
				"Assuming default silicon type %d\n",
				pdata->ate_default_silicon_type);

#ifdef CONFIG_CAPRI_28145
		avs_inf_ptr->ate_silicon_type_csr =  SILICON_SS;
		avs_inf_ptr->ate_silicon_type_msr =  SILICON_SS;
		avs_inf_ptr->ate_silicon_type_vsr =  SILICON_SS;
#else
		avs_inf_ptr->ate_silicon_type_csr =
		 pdata->ate_default_silicon_type;
		avs_inf_ptr->ate_silicon_type_msr =
		 pdata->ate_default_silicon_type;
		avs_inf_ptr->ate_silicon_type_vsr =
		 pdata->ate_default_silicon_type;
#endif
		avs_inf_ptr->freq = pdata->ate_default_cpu_freq;

	} else {
		avs_inf_ptr->ate_silicon_type_csr =
		pdata->ate_lut_csr[avs_inf_ptr->ate_csr_bin].silicon_type;

		avs_inf_ptr->ate_silicon_type_msr =
			pdata->ate_lut_msr[avs_inf_ptr->ate_msr_bin];

		avs_inf_ptr->ate_silicon_type_vsr =
			pdata->ate_lut_vsr[avs_inf_ptr->ate_vsr_bin];

		avs_inf_ptr->freq =
			pdata->ate_lut_csr[avs_inf_ptr->ate_csr_bin].freq;
	}
	avs_dbg(AVS_LOG_INIT, "%s:\n"
			"   ate silicon type csr = %d\n"
			"   ate silicon type msr = %d\n"
			"   ate silicon type vsr = %d\n"
			"   freq = %d\n",
			__func__,
			avs_inf_ptr->ate_silicon_type_csr,
			avs_inf_ptr->ate_silicon_type_msr,
			avs_inf_ptr->ate_silicon_type_vsr,
			avs_inf_ptr->freq);
	return 0;
}

static u32 kona_avs_get_avs_type(u32 avs_val,
			 struct kona_avs_lut_entry *avs_lut)
{
	u32 bin = SILICON_TS;

	avs_dbg(AVS_LOG_INFO, "%s:\n",  __func__);

	if (avs_val < avs_lut->slow) {
		bin = SILICON_SS;
	} else if (avs_val >= avs_lut->slow &&
			  avs_val < avs_lut->typical_slow) {
		bin = SILICON_TS;
	} else if (avs_val >= avs_lut->typical_slow &&
			  avs_val < avs_lut->typical_typical) {
		bin = SILICON_TT;
	} else if (avs_val >= avs_lut->typical_typical &&
			  avs_val < avs_lut->typical_fast) {
		bin = SILICON_TF;
	} else if (avs_val >= avs_lut->typical_fast) {
		bin = SILICON_FF;
	}

	avs_dbg(AVS_LOG_INFO, "    bin = %d\n", bin);

	return bin;
}

static u32 kona_avs_get_csr_type(struct avs_info *avs_inf_ptr)
{
	u32 irdrop_bin = SILICON_TS;

	struct kona_avs_pdata *pdata = avs_inf_ptr->pdata;

	avs_dbg(AVS_LOG_INFO, "%s: avs_inf_ptr->avs_irdrop = %d\n",
				__func__, avs_inf_ptr->avs_irdrop);

	irdrop_bin = kona_avs_get_avs_type(avs_inf_ptr->avs_irdrop,
				 pdata->avs_lut_irdrop);

	avs_dbg(AVS_LOG_INFO, "%s: irdrop_bin = %d\n",
		__func__, irdrop_bin);

	return irdrop_bin;
}

static u32 kona_avs_get_msr_type(struct avs_info *avs_inf_ptr)
{
	u32 monitor0_bin, monitor1_bin, monitor2_bin, monitor3_bin = SILICON_TS;
	u32 avs_msr_type = SILICON_TS;

	struct kona_avs_pdata *pdata = avs_inf_ptr->pdata;

	avs_dbg(AVS_LOG_INIT, "%s:\n", __func__);
	avs_dbg(AVS_LOG_INIT, "    avs_inf_ptr->monitor_val0 = %d\n",
	 avs_inf_ptr->monitor_val0);
	avs_dbg(AVS_LOG_INIT, "    avs_inf_ptr->monitor_val1 = %d\n",
	 avs_inf_ptr->monitor_val1);
	avs_dbg(AVS_LOG_INIT, "    avs_inf_ptr->monitor_val2 = %d\n",
	 avs_inf_ptr->monitor_val2);
	avs_dbg(AVS_LOG_INIT, "    avs_inf_ptr->monitor_val3 = %d\n",
	 avs_inf_ptr->monitor_val3);

	monitor0_bin = kona_avs_get_avs_type(avs_inf_ptr->monitor_val0,
						&(pdata->avs_lut_msr[0]));

	monitor1_bin = kona_avs_get_avs_type(avs_inf_ptr->monitor_val1,
						&(pdata->avs_lut_msr[1]));

	monitor2_bin = kona_avs_get_avs_type(avs_inf_ptr->monitor_val2,
						&(pdata->avs_lut_msr[2]));

	monitor3_bin = kona_avs_get_avs_type(avs_inf_ptr->monitor_val3,
						&(pdata->avs_lut_msr[3]));


	avs_msr_type = min(monitor0_bin, min(monitor1_bin,
					 min(monitor2_bin, monitor3_bin)));

	avs_dbg(AVS_LOG_INFO, "%s:avs_msr_type = %d\n",
		__func__, avs_msr_type);

	return avs_msr_type;
}

static u32 kona_avs_get_vsr_type(struct avs_info *avs_inf_ptr)
{

	u32 monitor0_bin, monitor1_bin, monitor2_bin, monitor3_bin = SILICON_TS;
	u32 avs_vsr_type = SILICON_TS;

	struct kona_avs_pdata *pdata = avs_inf_ptr->pdata;

	avs_dbg(AVS_LOG_INIT, "%s:\n", __func__);
	avs_dbg(AVS_LOG_INIT, "    avs_inf_ptr->monitor_val0 = %d\n",
	 avs_inf_ptr->monitor_val0);
	avs_dbg(AVS_LOG_INIT, "    avs_inf_ptr->monitor_val1 = %d\n",
	 avs_inf_ptr->monitor_val1);
	avs_dbg(AVS_LOG_INIT, "    avs_inf_ptr->monitor_val2 = %d\n",
	 avs_inf_ptr->monitor_val2);
	avs_dbg(AVS_LOG_INIT, "    avs_inf_ptr->monitor_val3 = %d\n",
	 avs_inf_ptr->monitor_val3);

	monitor0_bin = kona_avs_get_avs_type(avs_inf_ptr->monitor_val0,
					&(pdata->avs_lut_vsr[0]));

	monitor1_bin = kona_avs_get_avs_type(avs_inf_ptr->monitor_val1,
					&(pdata->avs_lut_vsr[1]));

	monitor2_bin = kona_avs_get_avs_type(avs_inf_ptr->monitor_val2,
					&(pdata->avs_lut_vsr[2]));

	monitor3_bin = kona_avs_get_avs_type(avs_inf_ptr->monitor_val3,
					&(pdata->avs_lut_vsr[3]));


	avs_vsr_type = min(monitor0_bin, min(monitor1_bin,
					 min(monitor2_bin, monitor3_bin)));

	avs_dbg(AVS_LOG_INFO, "%s: avs_vsr_type = %d\n",
		__func__, avs_vsr_type);

	return avs_vsr_type;
}


static int avs_find_silicon_type(void)
{
	int ret = 0;
	int ate_enabled = 0;

	avs_dbg(AVS_LOG_INIT, "%s:\n", __func__);

	avs_dbg(AVS_LOG_INIT, "    avs_info.monitor_val0 = %d\n",
	 avs_info.monitor_val0);
	avs_dbg(AVS_LOG_INIT, "    avs_info.monitor_val1 = %d\n",
	 avs_info.monitor_val1);
	avs_dbg(AVS_LOG_INIT, "    avs_info.monitor_val2 = %d\n",
	 avs_info.monitor_val2);
	avs_dbg(AVS_LOG_INIT, "    avs_info.monitor_val3 = %d\n",
	 avs_info.monitor_val3);

	avs_dbg(AVS_LOG_INIT, "    avs_info.avs_irdrop = %d\n",
	  avs_info.avs_irdrop);
	avs_dbg(AVS_LOG_INIT, "    avs_info.ate_csr_bin = %d\n",
	  avs_info.ate_csr_bin);
	avs_dbg(AVS_LOG_INIT, "    avs_info.ate_msr_bin = %d\n",
	  avs_info.ate_msr_bin);
	avs_dbg(AVS_LOG_INIT, "    avs_info.ate_vsr_bin = %d\n",
	  avs_info.ate_vsr_bin);
	avs_dbg(AVS_LOG_INIT, "    avs_info.freq = %d\n",
	  avs_info.freq);
	avs_dbg(AVS_LOG_INIT, "    avs_info.year = %d\n",
	  avs_info.year);
	avs_dbg(AVS_LOG_INIT, "    avs_info.month = %d\n",
	  avs_info.month);

	if (!avs_info.pdata)
		return  -EPERM;

	avs_info.silicon_type_csr = kona_avs_get_csr_type(&avs_info);

	avs_info.silicon_type_msr = kona_avs_get_msr_type(&avs_info);

	avs_info.silicon_type_vsr = kona_avs_get_vsr_type(&avs_info);

	ate_enabled = avs_info.pdata->flags & AVS_ATE_FEATURE_ENABLE;

	if (ate_enabled)
		ret = kona_avs_ate_get_type(&avs_info);

	if (ate_enabled && !ret) {
		avs_info.silicon_type_csr = min(avs_info.ate_silicon_type_csr,
				avs_info.silicon_type_csr);

		avs_info.silicon_type_msr = min(avs_info.ate_silicon_type_msr,
				avs_info.silicon_type_msr);

		avs_info.silicon_type_vsr = min(avs_info.ate_silicon_type_vsr,
				avs_info.silicon_type_vsr);
	} else {
		avs_info.freq = -1;
	}

	if (avs_info.pdata->silicon_type_notify)
		avs_info.pdata->silicon_type_notify(avs_info.silicon_type_csr,
				avs_info.silicon_type_msr,
				avs_info.silicon_type_vsr,
				avs_info.freq);

	avs_dbg(AVS_LOG_INIT,
			"%s: AVS Complete",
			__func__);

	return 0;
}

static int param_set_trigger_avs(const char *val, const struct kernel_param *kp)
{
	int trig;
	int ret = -1;

	avs_dbg(AVS_LOG_FLOW, "%s\n", __func__);
	if (!val)
		return -EINVAL;
	if (!avs_info.pdata) {
		avs_dbg(AVS_LOG_ERR,
				"%s : invalid paltform data !!\n", __func__);
		return  -EPERM;
	}
	ret = sscanf(val, "%d", &trig);
	avs_dbg(AVS_LOG_INFO, "%s, trig:%d\n", __func__, trig);
	if (trig == 1)
		avs_find_silicon_type();

	return 0;
}

static int kona_avs_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct kona_avs_pdata *pdata = pdev->dev.platform_data;

	avs_dbg(AVS_LOG_INIT, "%s: AVS Start\n", __func__);

	if (!pdata) {
		avs_dbg(AVS_LOG_ERR,
				"%s : invalid paltform data !!\n", __func__);
		ret = -EPERM;
		goto error;
	}

	avs_info.pdata = pdata;


	BUG_ON((pdata->flags & AVS_TYPE_OPEN)
		   && (pdata->flags & AVS_TYPE_BOOT));
	BUG_ON((pdata->flags & AVS_READ_FROM_OTP)
		   && (pdata->flags & AVS_READ_FROM_MEM));

	ret = kona_avs_get_mon_val(&avs_info);
	if (ret)
		goto error;
	ret = kona_avs_get_ate_val(&avs_info);
	if (ret)
		goto error;

	avs_find_silicon_type();


error:
	return ret;
}

static int __devexit kona_avs_drv_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver kona_avs_driver = {
	.probe = kona_avs_drv_probe,
	.remove = __devexit_p(kona_avs_drv_remove),
	.driver = {.name = "kona-avs",},
};

static int __init kona_avs_drv_init(void)
{
	return platform_driver_register(&kona_avs_driver);
}

subsys_initcall_sync(kona_avs_drv_init);

static void __exit kona_avs_drv_exit(void)
{
	platform_driver_unregister(&kona_avs_driver);
}

module_exit(kona_avs_drv_exit);

MODULE_ALIAS("platform:kona_avs_drv");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AVS driver for BRCM Kona based Chipsets");
