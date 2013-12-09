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

#include <linux/of_fdt.h>
#include <linux/mfd/bcmpmu.h>
#include <linux/kernel.h>

struct bcmpmu_dt_batt_data {
	int valid;
	char *model;
	uint32_t *voltcap_map_len;
	uint32_t *voltcap_map_width;
	unsigned long voltcap_map_size;
	uint32_t *voltcap_map;
	uint32_t *cpcty;
	uint32_t *eoc;
	uint32_t *chrg_1c;
	uint32_t *fg_zone_n20;
	unsigned long fg_zone_n20_size;
	uint32_t *fg_zone_n10;
	unsigned long fg_zone_n10_size;
	uint32_t *fg_zone_0;
	unsigned long fg_zone_0_size;
	uint32_t *fg_zone_p10;
	unsigned long fg_zone_p10_size;
	uint32_t *fg_zone_p20;
	unsigned long fg_zone_p20_size;
	uint32_t *eoc_curr_map_len;
	uint32_t *eoc_curr_map_width;
	unsigned long eoc_curr_map_size;
	uint32_t *eoc_curr_map;
	uint32_t *cutoff_volt_map_len;
	uint32_t *cutoff_volt_map_width;
	unsigned long cutoff_volt_map_size;
	uint32_t *cutoff_volt_map;
	uint32_t *max_volt;
	uint32_t *eoc_volt;
	uint32_t *resume_volt;
	uint32_t *low_volt;
	uint32_t *pre_eoc_min_curr;
};
struct bcmpmu_dt_batt_data battdata = {0};

struct bcmpmu_dt_pmu_data {
	int valid;
	uint32_t *fg_slp_curr_ua;
	uint32_t *fg_factor;
	uint32_t *fg_sns_res;
	uint32_t *reginit_len;
	uint32_t *reginit_width;
	uint32_t *max_batt_ov;
	unsigned long reginit_size;
	uint32_t *reginit_data;
	unsigned long curr_lmt_size;
	uint32_t *curr_lmt_data;
	uint32_t *ntc_upper_boundary;
	char *adc_ntc;
	char *adc_patemp;
	char *adc_32ktemp;
};
struct bcmpmu_dt_pmu_data pmudata = {0};

static void update_batt_temp_data(struct bcmpmu_platform_data *pdata)
{
	uint32_t *p, *p1;
	struct bcmpmu_fg_zone tbl;
	int i;
	if (battdata.fg_zone_n20) {
		if (battdata.fg_zone_n20_size !=
			offsetof(struct bcmpmu_fg_zone, temp))
			printk(KERN_INFO "%s: fg_zone_n20 data mismatch\n",
				__func__);
		else {
			p = battdata.fg_zone_n20;
			p1 = (uint32_t *)&tbl;
			for (i = 0; i < battdata.fg_zone_n20_size / 4; i++)
				*p1++ = be32_to_cpu(*p++);
			memcpy((void *)&pdata->fg_zone_info[FG_TMP_ZONE_n20],
				(void *)&tbl, battdata.fg_zone_n20_size);
			memcpy((void *)&pdata->fg_zone_info[FG_TMP_ZONE_n15],
				(void *)&tbl, battdata.fg_zone_n20_size);
		}
	}
	if (battdata.fg_zone_n10) {
		if (battdata.fg_zone_n10_size !=
			offsetof(struct bcmpmu_fg_zone, temp))
			printk(KERN_INFO "%s: fg_zone_n10 data mismatch\n",
				__func__);
		else {
			p = battdata.fg_zone_n10;
			p1 = (uint32_t *)&tbl;
			for (i = 0; i < battdata.fg_zone_n10_size / 4; i++)
				*p1++ = be32_to_cpu(*p++);
			memcpy((void *)&pdata->fg_zone_info[FG_TMP_ZONE_n10],
				(void *)&tbl, battdata.fg_zone_n10_size);
			memcpy((void *)&pdata->fg_zone_info[FG_TMP_ZONE_n5],
				(void *)&tbl, battdata.fg_zone_n10_size);
		}
	}
	if (battdata.fg_zone_0) {
		if (battdata.fg_zone_0_size !=
			offsetof(struct bcmpmu_fg_zone, temp))
			printk(KERN_INFO "%s: fg_zone_0 data mismatch\n",
				__func__);
		else {
			p = battdata.fg_zone_0;
			p1 = (uint32_t *)&tbl;
			for (i = 0; i < battdata.fg_zone_0_size / 4; i++)
				*p1++ = be32_to_cpu(*p++);
			memcpy((void *)&pdata->fg_zone_info[FG_TMP_ZONE_0],
				(void *)&tbl, battdata.fg_zone_0_size);
		}
	}
	if (battdata.fg_zone_p10) {
		if (battdata.fg_zone_p10_size !=
			offsetof(struct bcmpmu_fg_zone, temp))
			printk(KERN_INFO "%s: fg_zone_p10 data mismatch\n",
				__func__);
		else {
			p = battdata.fg_zone_p10;
			p1 = (uint32_t *)&tbl;
			for (i = 0; i < battdata.fg_zone_p10_size / 4; i++)
				*p1++ = be32_to_cpu(*p++);
			memcpy((void *)&pdata->fg_zone_info[FG_TMP_ZONE_p10],
				(void *)&tbl, battdata.fg_zone_p10_size);
			memcpy((void *)&pdata->fg_zone_info[FG_TMP_ZONE_p5],
				(void *)&tbl, battdata.fg_zone_p10_size);
		}
	}
	if (battdata.fg_zone_p20) {
		if (battdata.fg_zone_p20_size !=
			offsetof(struct bcmpmu_fg_zone, temp))
			printk(KERN_INFO "%s: fg_zone_p20 data mismatch\n",
				__func__);
		else {
			p = battdata.fg_zone_p20;
			p1 = (uint32_t *)&tbl;
			for (i = 0; i < battdata.fg_zone_p20_size / 4; i++)
				*p1++ = be32_to_cpu(*p++);
			memcpy((void *)&pdata->fg_zone_info[FG_TMP_ZONE_p20],
				(void *)&tbl, battdata.fg_zone_p20_size);
			memcpy((void *)&pdata->fg_zone_info[FG_TMP_ZONE_p15],
				(void *)&tbl, battdata.fg_zone_p20_size);
		}
	}
	pdata->fg_zone_info[FG_TMP_ZONE_n15].fct =
		(pdata->fg_zone_info[FG_TMP_ZONE_n20].fct +
			pdata->fg_zone_info[FG_TMP_ZONE_n10].fct) / 2;
	pdata->fg_zone_info[FG_TMP_ZONE_n5].fct =
		(pdata->fg_zone_info[FG_TMP_ZONE_n10].fct +
			pdata->fg_zone_info[FG_TMP_ZONE_0].fct) / 2;
	pdata->fg_zone_info[FG_TMP_ZONE_p5].fct =
		(pdata->fg_zone_info[FG_TMP_ZONE_p10].fct +
			pdata->fg_zone_info[FG_TMP_ZONE_0].fct) / 2;
	pdata->fg_zone_info[FG_TMP_ZONE_p15].fct =
		(pdata->fg_zone_info[FG_TMP_ZONE_p20].fct +
			pdata->fg_zone_info[FG_TMP_ZONE_p10].fct) / 2;

	for (i = FG_TMP_ZONE_MIN; i <= FG_TMP_ZONE_MAX; i++) {
		pdata->fg_zone_info[i].vcmap = pdata->batt_voltcap_map;
		pdata->fg_zone_info[i].maplen = pdata->batt_voltcap_map_len;
	}
}

int __init early_init_dt_scan_pmu(unsigned long node, const char *uname,
				     int depth, void *data)
{
	const char *prop;
	unsigned long size;
	uint32_t *p;

	if (depth != 1 || strcmp(uname, "bcmpmu") != 0)
		return 0;

	prop = of_get_flat_dt_prop(node, "fg_slp_curr_ua", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: fg_slp_curr_ua not found\n", __func__);
	else
		pmudata.fg_slp_curr_ua = (uint32_t *)prop;

	prop = of_get_flat_dt_prop(node, "fg_factor", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: fg_factor not found\n", __func__);
	else
		pmudata.fg_factor = (uint32_t *)prop;

	prop = of_get_flat_dt_prop(node, "fg_sns_res", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: fg_sns_res not found\n", __func__);
	else
		pmudata.fg_sns_res = (uint32_t *)prop;

	prop = of_get_flat_dt_prop(node, "reginit", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: pmu reginit not found\n", __func__);
	else {
		p = (uint32_t *)prop;
		pmudata.reginit_width = p;
		p++;
		pmudata.reginit_len = p;
	}

	prop = of_get_flat_dt_prop(node, "initdata", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: pmu initdata found\n", __func__);
	else {
		pmudata.reginit_size = size/4;
		pmudata.reginit_data = (uint32_t *)prop;
	}

	prop = of_get_flat_dt_prop(node, "chrgr_curr_lmt", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: chrgr_curr_lmt not found\n", __func__);
	else {
		pmudata.curr_lmt_size = size/4;
		pmudata.curr_lmt_data = (uint32_t *)prop;
	}

	prop = of_get_flat_dt_prop(node, "ntc_upper_boundary", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: ntc_upper_boundary not found\n", __func__);
	else
		pmudata.ntc_upper_boundary = (uint32_t *)prop;

	prop = of_get_flat_dt_prop(node, "max_batt_ov", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: max_batt_ov not found\n", __func__);
	else
		pmudata.max_batt_ov = (uint32_t *)prop;

	prop = of_get_flat_dt_prop(node, "adc-chnl-ntc", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: adc-chnl-ntc not found\n", __func__);
	else
		pmudata.adc_ntc = (char *)prop;

	prop = of_get_flat_dt_prop(node, "adc-chnl-patemp", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: adc-chnl-patemp not found\n", __func__);
	else
		pmudata.adc_patemp = (char *)prop;

	prop = of_get_flat_dt_prop(node, "adc-chnl-32ktemp", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: adc-chnl-32ktemp not found\n", __func__);
	else
		pmudata.adc_32ktemp = (char *)prop;

	pmudata.valid = 1;

	return 1;
}

int __init early_init_dt_scan_batt(unsigned long node, const char *uname,
				     int depth, void *data)
{
	const char *prop;
	unsigned long size;
	uint32_t *p;

	if (depth != 1 || strcmp(uname, "battery") != 0)
		return 0;

	prop = of_get_flat_dt_prop(node, "charge-eoc", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: charge-eoc not found\n", __func__);
	else
		battdata.eoc = (uint32_t *)prop;

	prop = of_get_flat_dt_prop(node, "charge-1c", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: charge-1c not found\n", __func__);
	else
		battdata.chrg_1c = (uint32_t *)prop;

	prop = of_get_flat_dt_prop(node, "capacity", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: battery capacity not found\n", __func__);
	else
		battdata.cpcty = (uint32_t *)prop;

	prop = of_get_flat_dt_prop(node, "pre_eoc_min_curr", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: pre_eoc_min_curr not found\n", __func__);
	else
		battdata.pre_eoc_min_curr = (uint32_t *)prop;

	prop = of_get_flat_dt_prop(node, "vcmap-size", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: battery vcmap-size not found\n",
		__func__);
	else {
		p = (uint32_t *)prop;
		battdata.voltcap_map_width = p;
		p++;
		battdata.voltcap_map_len = p;
	}
	prop = of_get_flat_dt_prop(node, "vcmap", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: battery vcmap not found\n", __func__);
	else {
		battdata.voltcap_map = (uint32_t *)prop;
		battdata.voltcap_map_size = size/4;
	}

	prop = of_get_flat_dt_prop(node, "model", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: battery model not found\n", __func__);
	else
		battdata.model = (char *)prop;


	prop = of_get_flat_dt_prop(node, "fg-zone-n20", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: battery fg-zone-n20 not found\n", __func__);
	else {
		battdata.fg_zone_n20 = (uint32_t *)prop;
		battdata.fg_zone_n20_size = size;
	}

	prop = of_get_flat_dt_prop(node, "fg-zone-n10", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: battery fg-zone-n10 not found\n", __func__);
	else {
		battdata.fg_zone_n10 = (uint32_t *)prop;
		battdata.fg_zone_n10_size = size;
	}

	prop = of_get_flat_dt_prop(node, "fg-zone-0", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: battery fg-zone-0 not found\n", __func__);
	else {
		battdata.fg_zone_0 = (uint32_t *)prop;
		battdata.fg_zone_0_size = size;
	}

	prop = of_get_flat_dt_prop(node, "fg-zone-p10", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: battery fg-zone-p10 not found\n", __func__);
	else {
		battdata.fg_zone_p10 = (uint32_t *)prop;
		battdata.fg_zone_p10_size = size;
	}

	prop = of_get_flat_dt_prop(node, "fg-zone-p20", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: battery fg-zone-p20 not found\n", __func__);
	else {
		battdata.fg_zone_p20 = (uint32_t *)prop;
		battdata.fg_zone_p20_size = size;
	}

	prop = of_get_flat_dt_prop(node, "eoc-curr-map-size", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: eoc-curr-map-size not found\n",
		__func__);
	else {
		p = (uint32_t *)prop;
		battdata.eoc_curr_map_width = p;
		p++;
		battdata.eoc_curr_map_len = p;
	}
	prop = of_get_flat_dt_prop(node, "eoc-curr-map", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: eoc-curr-map not found\n", __func__);
	else {
		battdata.eoc_curr_map = (uint32_t *)prop;
		battdata.eoc_curr_map_size = size/4;
	}


	prop = of_get_flat_dt_prop(node, "cutoff-volt-map-size", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: cutoff-volt-map-size not found\n",
		__func__);
	else {
		p = (uint32_t *)prop;
		battdata.cutoff_volt_map_width = p;
		p++;
		battdata.cutoff_volt_map_len = p;
	}
	prop = of_get_flat_dt_prop(node, "cutoff-volt-map", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: cutoff-volt-map not found\n", __func__);
	else {
		battdata.cutoff_volt_map = (uint32_t *)prop;
		battdata.cutoff_volt_map_size = size/4;
	}

	prop = of_get_flat_dt_prop(node, "max-volt", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: max-volt not found\n", __func__);
	else
		battdata.max_volt = (uint32_t *)prop;
	prop = of_get_flat_dt_prop(node, "eoc-volt", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: eoc-volt not found\n", __func__);
	else
		battdata.eoc_volt = (uint32_t *)prop;
	prop = of_get_flat_dt_prop(node, "resume-volt", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: resume-volt not found\n", __func__);
	else
		battdata.resume_volt = (uint32_t *)prop;
	prop = of_get_flat_dt_prop(node, "low-volt", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: low-volt not found\n", __func__);
	else
		battdata.low_volt = (uint32_t *)prop;

	battdata.valid = 1;

	return 1;
}

void bcmpmu_update_pdata_dt_batt(struct bcmpmu_platform_data *pdata)
{
	uint32_t len = 0, wid = 0;
	uint32_t *p, *p1;
	void *tbl;
	int i;

	if (battdata.valid == 0)
		return;

	if (battdata.eoc != 0)
		pdata->chrg_eoc = be32_to_cpu(*battdata.eoc);
	if (battdata.chrg_1c != NULL)
		pdata->chrg_1c_rate = be32_to_cpu(*battdata.chrg_1c);
	if (battdata.cpcty != 0)
		pdata->fg_capacity_full = be32_to_cpu(*battdata.cpcty) * 3600;
	if (battdata.pre_eoc_min_curr != 0)
		pdata->pre_eoc_min_curr =
			be32_to_cpu(*battdata.pre_eoc_min_curr);
	if (battdata.model != 0)
		pdata->batt_model = battdata.model;
	if (battdata.max_volt != NULL)
		pdata->max_vfloat = be32_to_cpu(*battdata.max_volt);
	if (battdata.eoc_volt != NULL)
		pdata->fg_fbat_lvl = be32_to_cpu(*battdata.eoc_volt);
	if (battdata.resume_volt != NULL)
		pdata->chrg_resume_lvl = be32_to_cpu(*battdata.resume_volt);
	if (battdata.low_volt != NULL)
		pdata->fg_lbat_lvl = be32_to_cpu(*battdata.low_volt);
	if (battdata.voltcap_map_width != 0)
		wid = be32_to_cpu(*battdata.voltcap_map_width);
	if (battdata.voltcap_map_len != 0)
		len = be32_to_cpu(*battdata.voltcap_map_len);

	if (wid * len != battdata.voltcap_map_size)
		printk(KERN_INFO "%s: batt v-c tbl err, w=0x%X, l=0x%X, size=%ld\n",
			__func__, wid, len, battdata.voltcap_map_size);
	else {
		tbl = (struct bcmpmu_voltcap_map *)tbl;
		tbl = kzalloc(len * sizeof(struct bcmpmu_voltcap_map), GFP_KERNEL);
		if (tbl == NULL)
			printk(KERN_INFO
			"%s: failed to alloc mem for pdata->batt_voltcap_map.\n",
			__func__);
		else {
			pdata->batt_voltcap_map_len = len;
			p = battdata.voltcap_map;
			p1 = tbl;
			for (i = 0; i < battdata.voltcap_map_size; i++)
				*p1++ = be32_to_cpu(*p++);
			pdata->batt_voltcap_map =
				(struct bcmpmu_voltcap_map *)tbl;
		}
	}

	if (battdata.eoc_curr_map_width != 0)
		wid = be32_to_cpu(*battdata.eoc_curr_map_width);
	if (battdata.eoc_curr_map_len != 0)
		len = be32_to_cpu(*battdata.eoc_curr_map_len);
	if (wid * len != battdata.eoc_curr_map_size)
		printk(KERN_INFO "%s: eoc_curr_map tbl err,"
			"w=0x%X, l=0x%X, size=%ld\n",
			__func__, wid, len, battdata.eoc_curr_map_size);
	else {
		tbl = (struct bcmpmu_currcap_map *)tbl;
		tbl = kzalloc(len * sizeof(struct bcmpmu_currcap_map), GFP_KERNEL);
		if (tbl == NULL)
			printk(KERN_INFO
			"%s: failed to alloc mem for pdata->eoc_cal_map.\n",
			__func__);
		else {
			pdata->eoc_cal_map_len = len;
			p = battdata.eoc_curr_map;
			p1 = tbl;
			for (i = 0; i < battdata.eoc_curr_map_size; i++)
				*p1++ = be32_to_cpu(*p++);
			pdata->eoc_cal_map =
				(struct bcmpmu_currcap_map *)tbl;
		}
	}


	if (battdata.cutoff_volt_map_width != 0)
		wid = be32_to_cpu(*battdata.cutoff_volt_map_width);
	if (battdata.cutoff_volt_map_len != 0)
		len = be32_to_cpu(*battdata.cutoff_volt_map_len);

	if (wid * len != battdata.cutoff_volt_map_size)
		printk(KERN_INFO "%s: cutoff_volt_map tbl err,"
			"w=0x%X, l=0x%X, size=%ld\n",
			__func__, wid, len, battdata.cutoff_volt_map_size);
	else {
		tbl = (struct bcmpmu_cutoff_map *)tbl;
		tbl = kzalloc(len * sizeof(struct bcmpmu_cutoff_map), GFP_KERNEL);
		if (tbl == NULL)
			printk(KERN_INFO
			"%s: failed to alloc mem for pdata->cutoff_volt_map.\n",
			__func__);
		else {
			pdata->cutoff_cal_map_len = len;
			p = battdata.cutoff_volt_map;
			p1 = tbl;
			for (i = 0; i < battdata.cutoff_volt_map_size; i++)
				*p1++ = be32_to_cpu(*p++);
			pdata->cutoff_cal_map =
				(struct bcmpmu_cutoff_map *)tbl;
		}
	}
	update_batt_temp_data(pdata);
}

void bcmpmu_update_pdata_dt_pmu(struct bcmpmu_platform_data *pdata)
{
	uint32_t *p, *p1;
	void *tbl;
	uint32_t len = 0, wid = 0;
	int i;

	if (pmudata.valid == 0)
		return;

	if (pmudata.fg_slp_curr_ua != NULL)
		pdata->fg_slp_curr_ua = be32_to_cpu(*pmudata.fg_slp_curr_ua);
	if (pmudata.fg_factor != 0)
		pdata->fg_factor = be32_to_cpu(*pmudata.fg_factor);
	if (pmudata.max_batt_ov != 0)
		pdata->max_batt_ov = be32_to_cpu(*pmudata.max_batt_ov);
	if (pmudata.fg_sns_res != 0)
		pdata->fg_sns_res = be32_to_cpu(*pmudata.fg_sns_res);

	if (pmudata.reginit_width != 0)
		wid = be32_to_cpu(*pmudata.reginit_width);
	if (pmudata.reginit_len != 0)
		len = be32_to_cpu(*pmudata.reginit_len);

	if (wid * len != pmudata.reginit_size)
		printk(KERN_INFO "%s: pmu reg init table error, w=%d, l=%d size=%ld\n",
			__func__, wid, len, pmudata.reginit_size);
	else {
		tbl = (struct bcmpmu_rw_data *)tbl;
		tbl = kzalloc(len * sizeof(struct bcmpmu_rw_data), GFP_KERNEL);
		if (tbl == NULL)
			printk(KERN_INFO
				"%s: failed to alloc mem for pdata->init_data.\n",
				__func__);
		else {
			pdata->init_max = len;
			p = pmudata.reginit_data;
			p1 = tbl;
			for (i = 0; i < pmudata.reginit_size; i++)
				*p1++ = be32_to_cpu(*p++);
			pdata->init_data = (struct bcmpmu_rw_data *)tbl;
		}
	}

	if (pmudata.curr_lmt_size != PMU_CHRGR_TYPE_MAX)
		printk(KERN_INFO "%s: current limit table error, size=%ld\n",
			__func__, pmudata.curr_lmt_size);
	else {
		tbl = (int *)tbl;
		tbl = kzalloc(PMU_CHRGR_TYPE_MAX * sizeof(int), GFP_KERNEL);
		if (tbl == NULL)
			printk(KERN_INFO
			"%s: failed to alloc mem for pdata->chrgr_curr_lmt.\n",
			__func__);
		else {
			p = pmudata.curr_lmt_data;
			p1 = tbl;
			for (i = 0; i < pmudata.curr_lmt_size; i++)
				*p1++ = be32_to_cpu(*p++);
			pdata->chrgr_curr_lmt = (int *)tbl;
		}
	}
	if (pmudata.ntc_upper_boundary != 0)
		pdata->ntc_upper_boundary =
			be32_to_cpu(*pmudata.ntc_upper_boundary);
	if (pmudata.adc_ntc != 0) {
		if (strncmp(pmudata.adc_ntc, "ntc", 3) == 0)
			pdata->adc_channel_ntc = PMU_ADC_NTC;
		else if (strncmp(pmudata.adc_ntc, "patemp", 6) == 0)
			pdata->adc_channel_ntc = PMU_ADC_PATEMP;
		else if (strncmp(pmudata.adc_ntc, "32ktemp", 7) == 0)
			pdata->adc_channel_ntc = PMU_ADC_32KTEMP;
	}
	if (pmudata.adc_patemp != 0) {
		if (strncmp(pmudata.adc_patemp, "ntc", 3) == 0)
			pdata->adc_channel_patemp = PMU_ADC_NTC;
		else if (strncmp(pmudata.adc_patemp, "patemp", 6) == 0)
			pdata->adc_channel_patemp = PMU_ADC_PATEMP;
		else if (strncmp(pmudata.adc_patemp, "32ktemp", 7) == 0)
			pdata->adc_channel_patemp = PMU_ADC_32KTEMP;
	}
	if (pmudata.adc_32ktemp != 0) {
		if (strncmp(pmudata.adc_32ktemp, "ntc", 3) == 0)
			pdata->adc_channel_32ktemp = PMU_ADC_NTC;
		else if (strncmp(pmudata.adc_32ktemp, "patemp", 6) == 0)
			pdata->adc_channel_32ktemp = PMU_ADC_PATEMP;
		else if (strncmp(pmudata.adc_32ktemp, "32ktemp", 7) == 0)
			pdata->adc_channel_32ktemp = PMU_ADC_32KTEMP;
	}
}
