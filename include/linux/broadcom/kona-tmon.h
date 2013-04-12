/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
*       @file   include/linux/broadcom/kona-tmon.h
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

/*
*
*****************************************************************************
*
* kona-tmon.h
*
* PURPOSE:
*
*
*
* NOTES:
*
* ****************************************************************************/

#ifndef __KONA_TMON_H__
#define __KONA_TMON_H__

enum threshold_enable {
	THRES_DISABLE,		/* threshold is disabled */
	THRES_ENABLE		/* threshold is enabled */
};

struct tmon_data {
	long critical_temp;
	long warning_temp_thresholds[4];
	long threshold_enable[4];
	long max_cpufreq[4];
	unsigned int polling_interval_ms;
	int temp_hysteresis;
};

#endif
