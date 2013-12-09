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

#ifndef _CAPH_PLATFORM_H
#define _CAPH_PLATFORM_H

struct extern_audio_platform_cfg {
	int ihf_ext_amp_gpio;
	int dock_aud_route_gpio;
	int mic_sel_aud_route_gpio;
#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN_CMCC)|| defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)	
	int mode_sel_aud_route_gpio;
	int bt_sel_aud_route_gpio;
#endif

};

struct audio_controller_platform_cfg {
	struct extern_audio_platform_cfg ext_aud_plat_cfg;
};

struct caph_platform_cfg {
	struct audio_controller_platform_cfg aud_ctrl_plat_cfg;
};

#endif /* CAPH_PLATFORM_H */
