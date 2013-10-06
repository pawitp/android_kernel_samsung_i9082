/*****************************************************************************
*  Copyright 2012 Broadcom Corporation.  All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_fdt.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/broadcom/vc_dt.h>

#define VC_DT_PMU_LIST_LEN  63

#define VC_DT_FB_WIDTH_DEF  540
#define VC_DT_FB_HEIGHT_DEF 960
#define VC_DT_FB_FRAME_DEF  2

struct vc_dt_data {
	int valid;
	/* vc image parameter. */
	uint32_t  base;
	uint32_t  load;
	uint32_t  size;
	/* vc framebuffer parameters. */
	uint32_t  fb_width;
	uint32_t  fb_height;
	uint32_t  fb_frames;
	/* vc controlled pmu ldo's parameters. */
	char      pmu_list[VC_DT_PMU_LIST_LEN + 1];
	/* anticipated maximum bulk transfer size in bytes. */
	int       vchiq_bulk_xfer_size;
};
struct vc_dt_data vc_dt_data = {0};

int __init early_init_dt_scan_vc(unsigned long node, const char *uname,
				     int depth, void *data)
{
	const char *prop;
	unsigned long size;
	uint32_t *uprop;

	if (depth != 1 || strncmp(uname, "videocore", strlen("videocore")) != 0)
		return 0;

	prop = of_get_flat_dt_prop(node, "reg", &size);
	if (prop == NULL)
		printk(KERN_INFO "%s: videocore reg not found\n", __func__);
	else {
		uprop = (uint32_t *)prop;

		vc_dt_data.base = be32_to_cpu(uprop[0]);
		vc_dt_data.size = be32_to_cpu(uprop[1]);

		prop = of_get_flat_dt_prop(node, "load-addr", &size);
		if (prop != NULL) {
			uprop = (uint32_t *)prop;
			vc_dt_data.load = be32_to_cpu(uprop[0]);
		} else
			vc_dt_data.load = vc_dt_data.base;

		printk(KERN_INFO "videocore: base @0x%x, load @0x%x, 0x%x Mb\n",
			vc_dt_data.base, vc_dt_data.load, vc_dt_data.size);
	}

	prop = of_get_flat_dt_prop(node, "pmu-list", &size);
	if (prop == NULL) {
		vc_dt_data.pmu_list[0] = 0;
		printk(KERN_INFO "videocore: pmu-list <no list>\n");
	} else {
		if (size > VC_DT_PMU_LIST_LEN) {
			printk(KERN_WARNING "videocore: pmu-list too long %lu\n",
				size);
			size = VC_DT_PMU_LIST_LEN;
		}
		memcpy(vc_dt_data.pmu_list, prop, size);
		vc_dt_data.pmu_list[size] = 0;
		printk(KERN_INFO "videocore: pmu-list %s\n",
			vc_dt_data.pmu_list);
	}

	vc_dt_data.fb_width = VC_DT_FB_WIDTH_DEF;
	vc_dt_data.fb_height = VC_DT_FB_HEIGHT_DEF;
	vc_dt_data.fb_frames = VC_DT_FB_FRAME_DEF;
	prop = of_get_flat_dt_prop(node, "fb-width", &size);
	if (prop != NULL) {
		uprop = (uint32_t *)prop;
		vc_dt_data.fb_width = be32_to_cpu(uprop[0]);
	}
	prop = of_get_flat_dt_prop(node, "fb-height", &size);
	if (prop != NULL) {
		uprop = (uint32_t *)prop;
		vc_dt_data.fb_height = be32_to_cpu(uprop[0]);
	}
	prop = of_get_flat_dt_prop(node, "fb-frames", &size);
	if (prop != NULL) {
		uprop = (uint32_t *)prop;
		vc_dt_data.fb_frames = be32_to_cpu(uprop[0]);
	}

	prop = of_get_flat_dt_prop(node, "vchiq-bulk-xfer-size", &size);
	if (prop == NULL) {
		vc_dt_data.vchiq_bulk_xfer_size = 0;
		printk(KERN_INFO "videocore: vchiq-bulk-xfer-size not set\n");
	} else {
		uprop = (uint32_t *)prop;

		vc_dt_data.vchiq_bulk_xfer_size = (int)be32_to_cpu(uprop[0]);
		printk(KERN_INFO "videocore: vchiq-bulk-xfer-size %d\n",
			vc_dt_data.vchiq_bulk_xfer_size);
	}

	vc_dt_data.valid = 1;
	return 1;
}

int vc_dt_get_mem_config(uint32_t *base, uint32_t *load, uint32_t *size)
{
	if (!vc_dt_data.valid)
		return -EINVAL;

	if ((base == NULL) || (load == NULL) || (size == NULL))
		return -EINVAL;

	*base = vc_dt_data.base;
	*load = vc_dt_data.load;
	*size = vc_dt_data.size;

	return 0;
}
EXPORT_SYMBOL_GPL(vc_dt_get_mem_config);

const char *vc_dt_get_pmu_config(void)
{
	if (!vc_dt_data.valid)
		return NULL;

	return vc_dt_data.pmu_list;
}
EXPORT_SYMBOL_GPL(vc_dt_get_pmu_config);

int vc_dt_get_fb_config(uint32_t *width, uint32_t *height, uint32_t *frame)
{
	if (!vc_dt_data.valid)
		return -EINVAL;

	if ((width == NULL) || (height == NULL) || (frame == NULL))
		return -EINVAL;

	*width = vc_dt_data.fb_width;
	*height = vc_dt_data.fb_height;
	*frame = vc_dt_data.fb_frames;

	return 0;
}
EXPORT_SYMBOL_GPL(vc_dt_get_fb_config);

int vc_dt_get_vchiq_bulk_xfer_size(void)
{
	if (!vc_dt_data.valid)
		return 0;

	return vc_dt_data.vchiq_bulk_xfer_size;
}
EXPORT_SYMBOL_GPL(vc_dt_get_vchiq_bulk_xfer_size);
