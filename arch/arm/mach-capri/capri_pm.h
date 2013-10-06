/*
 *  Copyright (C) 2012 Broadcom Corporation.
 *    Alamy Liu <alamy.liu@broadcom.com>, Aug, 2012.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/types.h>


#if defined(CONFIG_CAPRI_SYSEMI_DDR3)

#ifndef __ASSEMBLY__

void capri_suspend_ddr3(
	uint suspend_mode
			/* DEEPSLEEP, DORMANT (RETENTION, WFI not supported) */
	, uint param1	/* DEEPSLEEP) Not used; DORMANT) SMC buffer */
	, uint param2	/* DEEPSLEEP) Not used; DORMANT) L2 control */
	, uint param3	/* DEEPSLEEP) Not used; DORMANT) resume address */
);

void capri_ddr3_cpu_suspend(
	uint suspend_mode
	, uint param1
	, uint param2
	, uint param3
);

#endif	/* __ASSEMBLY__ */


/* We have C800 ~ EFFF (10K), from region 25 to 29 ( 2K / per region) */
#define	KERNEL_TEAM_SRAM_OFFSET		(0x0000C800)
#define	KERNEL_TEAM_SRAM_SIZE		(1024 * 10)

/*
 * Hint: Could steal the rear area of SMC buffer used by Suspend-Dormant
 * for log_buffer ~3404C800
 * When exits dormant successfully, we refresh stack anyway.
 * The other cases doesn't use stack size that much.
 */
/* Region 25 (C800 ~ D000) */
#define	DDR3_SRAM_STACK_OFFSET		(0x0000C800)
#define	DDR3_SRAM_STACK_SIZE		(SZ_2K)

/* Region 26~27 (D000 ~ E000) */
#define	DDR3_SRAM_CODE_OFFSET		(0x0000D000)
#define	DDR3_SRAM_CODE_SIZE		(SZ_4K)

#endif	/* CONFIG_CAPRI_SYSEMI_DDR3 */

