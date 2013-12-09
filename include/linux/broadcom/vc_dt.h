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

#if !defined(VC_DT_H)
#define VC_DT_H

int vc_dt_get_mem_config(uint32_t *base, uint32_t *load, uint32_t *size);
const char *vc_dt_get_pmu_config(void);
int vc_dt_get_fb_config(uint32_t *width, uint32_t *height, uint32_t *frame);
int vc_dt_get_vchiq_bulk_xfer_size(void);

#endif /* VC_DT_H */
