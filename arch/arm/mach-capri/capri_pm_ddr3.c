/*
 *  Copyright (c) 2012 Broadcom corporation.  All rights reserved.
 *    Alamy Liu <alamy.liu@broadcom.com>, Aug, 2012.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  Unless you and Broadcom execute a separate written software license
 *  agreement governing use of this software, this software is licensed to you
 *  under the terms of the GNU General Public License version 2, available at
 *  http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
 *
 * Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a
 * license other than the GPL, without Broadcom's express prior written
 * consent.
 *
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <mach/io_map.h>
#include "capri_pm.h"

extern unsigned int capri_ddr3_cpu_suspend_size;

void capri_suspend_ddr3(uint suspend_mode, uint param1, uint param2,
			uint param3)
{
	void (*capri_ddr3_cpu_suspend_sram_ptr) (uint suspend_mode, uint param1,
						 uint param2, uint param3);
	void *vDest;

	/*
	 * saving portion of SRAM to be used by suspend function.
	 */
	/* Kernel team has a reserved SRAM region could use */

	/* make sure SRAM copy gets physically written into SDRAM.
	   SDRAM will be placed into self-refresh during power down */
	/* flush_cache_all(); *//* We didn't actually copy data */

	/* copy ddr3 suspend function into SRAM */
	if (capri_ddr3_cpu_suspend_size > DDR3_SRAM_CODE_SIZE) {
		/* ***** Warning: need to restore_console, and return fail.
		 * use "no_console_suspend" command line options
		 * to see this message.
		 */
		pr_err("Error: Unable to move code to SRAM (0x%x > 0x%x)\n",
		       capri_ddr3_cpu_suspend_size, DDR3_SRAM_CODE_SIZE);

		/* Generate an exception */
		/*      Doesn't work (system already down) */
		/* *(int *)0 = 0; */

		/* directly Return */
		/*      Doesn't work (something wrong in dormant code */
		return;
	}

	capri_ddr3_cpu_suspend_sram_ptr =
	    (void *)(KONA_SRAM_VA + DDR3_SRAM_CODE_OFFSET);
	vDest =
	    memcpy(capri_ddr3_cpu_suspend_sram_ptr, capri_ddr3_cpu_suspend,
		   capri_ddr3_cpu_suspend_size);

	/* Flush I-cache / D-cache to make sure the code is copied
	 * over to sram ?
	 */

	/* Jump to the ddr3 suspend function */
	capri_ddr3_cpu_suspend_sram_ptr(suspend_mode, param1, param2, param3);

	/* restoring portion of SRAM that was used by suspend function */
	/* memcpy(capri_ddr3_cpu_suspend_sram_ptr,
	 *        saved_sram, capri_ddr3_cpu_suspend_size);
	 */
}
