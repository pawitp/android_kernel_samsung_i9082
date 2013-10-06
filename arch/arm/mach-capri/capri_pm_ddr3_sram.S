/*
 *
 *  Copyright (c) 2012 Broadcom corporation.  All rights reserved.
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
 *
 * Ported from CSP source, mainly from chal_memc_ddr3.c
 *   Alamy Liu <alamy.liu@broadcom.com>, Jun, 2012.
 *
 */

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/asm-offsets.h>
#include <asm/system.h>					// Disable MMU
#include <mach/io_map.h>
#include <mach/rdb/brcm_rdb_csr.h>
#include <mach/rdb/brcm_rdb_root_rst_mgr_reg.h>
#include <mach/rdb/brcm_rdb_ddr3ctl.h>
#include <mach/rdb/brcm_rdb_ddr40_phy_addr_ctl.h>
#include <mach/rdb/brcm_rdb_ddr40_phy_word_lane.h>
#include <chal/chal_sdram_configs.h>
#include <mach/sec_api.h>
#include "capri_pm.h"

/*
 ==============================================================================
 */

/*
 * Instead of including chal/chal_memc_ddr3.h (which causes a lot of
 * compiling error problems) Just copy the necessary part of its definition
 * to this file for compiling reason.
 *	Jun-11, 2012. Alamy (alamy.liu@broadcom.com)
 */
#define WR_ACCESS_PASSWORD			(0x00A5A500)

/* CAUTION: Must match the column size of jeced_info table below */
#define	JEDEC_INFO_FIELD_NUM			(4)

/*
 * Cannot use i_phy_standby signal to enter PHY standby mode, so use command instead.
 * Only true for BCM11160 (Big Island). Not recommended for Capri.
 */
#undef	STANDBY_ENTER_REG_0xA

/*
 ==============================================================================
 */

/*
 *	SYS_EMI_OPEN (0x35008000) : DDR_POWER_DOWN_STATUS (0xF4) : POWER_DOWN_STATUS [1:0]
 */
	.equ	MEMC_ACTIVE_STATE,			0
	.equ	MEMC_ACTIVE_PWR_DOWN_STATE,		1
	.equ	MEMC_PRECHARGEPWR_DOWN_STATE,		2
	.equ	MEMC_SELF_REFRESH_STATE,		3

/*
 *	SYS_EMI_OPEN (0x35008000) : DRAM_INIT_CONTROL (0x14) : CS_BITS [21:20]
 */
	.equ	CSR_DRAM_INIT_CONTROL_CS_BITS_BOTH,	0
	.equ	CSR_DRAM_INIT_CONTROL_CS_BITS_CS1,	1
	.equ	CSR_DRAM_INIT_CONTROL_CS_BITS_CS0,	2

/*
 *	SYS_EMI_OPEN (0x35008000) : DRAM_INIT_CONTROL (0x14) : INIT_CMD [19:16]
 */
	.equ	CSR_DRAM_INIT_CONTROL_INIT_CMD_MRW,	0
	.equ	CSR_DRAM_INIT_CONTROL_INIT_CMD_MRR,	1
	.equ	CSR_DRAM_INIT_CONTROL_INIT_CMD_AREF,	2
	.equ	CSR_DRAM_INIT_CONTROL_INIT_CMD_PALL,	3

/*
 *	SYS_EMI_OPEN (0x35008000) : DRAM_INIT_CONTROL (0x14) : MR_ADDR [7:0]
 */
	.equ	CSR_DRAM_INIT_CONTROL_MR_ADDR_REG0,	0
	.equ	CSR_DRAM_INIT_CONTROL_MR_ADDR_REG1,	1
	.equ	CSR_DRAM_INIT_CONTROL_MR_ADDR_REG2,	2
	.equ	CSR_DRAM_INIT_CONTROL_MR_ADDR_REG3,	3

/*
 ==============================================================================
 */

/*
 * Used to use macro, LOAD_VA_FROM_PHYS, to load Virtual address.
 * Just keep the coding structure, and rename it to LOAD_IO_FROM_PA.
 */
	.macro	LOAD_IO_FROM_PA, reg, phys_addr
	ldr	\reg, =(\phys_addr)
	.endm

/*
 * Read DDR3 mode register
 */
	.macro	DDR3_READ_PHY_CTL_REG, addr_offset
	@ Return
	@	R0 contains the value

	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800
	ldr	r0, [r4, #\addr_offset]
	.endm
	#define	DDR3_READ_MODE_REG	DDR3_READ_PHY_CTL_REG	// Alias

/*
 * Write DDR3 mode register
 */
	.macro	DDR3_WRITE_MODE_REG, memc_cs, addr
	@ Parameter
	@	R0 contains the DATA value, which is retained.

	lsl	r1, r0, #CSR_DRAM_INIT_CONTROL_MRW_DATA_SHIFT
	and	r1, r1, #CSR_DRAM_INIT_CONTROL_MRW_DATA_MASK

	lsr	r2, r0, #8
	lsl	r2, r2, #(CSR_DRAM_INIT_CONTROL_MRW_DATA_DDR3_EXTN_SHIFT)
	and	r2, r2, #CSR_DRAM_INIT_CONTROL_MRW_DATA_DDR3_EXTN_MASK
	orr	r2, r2, r1

	mov	r1, #((\memc_cs << CSR_DRAM_INIT_CONTROL_CS_BITS_SHIFT) & CSR_DRAM_INIT_CONTROL_CS_BITS_MASK)
	orr	r2, r2, r1

	mov	r1, #((\addr << CSR_DRAM_INIT_CONTROL_MR_ADDR_SHIFT) & CSR_DRAM_INIT_CONTROL_MR_ADDR_MASK)
	orr	r2, r2, r1

	orr	r2, r2, #(CSR_DRAM_INIT_CONTROL_INIT_CMD_MRW << CSR_DRAM_INIT_CONTROL_INIT_CMD_SHIFT)

	LOAD_IO_FROM_PA	r4, (MEMC0_OPEN_BASE_ADDR)		// 0x3500_8000
	str	r2, [r4, #CSR_DRAM_INIT_CONTROL_OFFSET]
	.endm

/*
 ==============================================================================
 */

	.global	suspend_case_dormant_success;
	.global	ddr3_phy_mode

/*
 ==============================================================================
 */

	.text

/*
 * The following code would be run in SRAM, so that we could put DDR3 into self-refresh mode.
 */
/* ----------------------------------------------------------------------------
 *	void capri_ddr3_cpu_suspend( uint suspend_mode, uint param1, uint param2, uint param3 )
 *
 *		suspend_param0:	Suspend mode : DEEPSLEEP, DORMANT (RETENTION, WFI not supported)
 *		suspend_param1:	DEEPSLEEP) Not used; DORMANT) SMC buffer
 *		suspend_param2:	DEEPSLEEP) Not used; DORMANT) L2 control
 *		suspend_param3:	DEEPSLEEP) Not used; DORMANT) resume address
 *
 *	CAUTION: This function must be the 1st function.
 *		It is used to calculate the size (capri_ddr3_cpu_suspend_size) of code/data to be copied to SRAM
 */
ENTRY(capri_ddr3_cpu_suspend)
	str	lr, suspend_lr_register
	str	sp, suspend_stack_register
	str	r0, suspend_param0		@ suspend mode
	str	r1, suspend_param1		@ dormant) SMC buffer
	str	r2, suspend_param2		@ dormant) L2 control
	str	r3, suspend_param3		@ dormant) resume address

	/*
	 * Disable MMU
	 *
	 * There was TLB problem when we shutdown DDR3 (put it into self-refresh).
	 * To establish proper TLB & lock it doesn't seem to be a good way.
	 *
	 * Alternatively, we could avoid any of these cache problems by disabling MMU.
	 * Also, it makes the system state more symmetric (we are MMU disabled when entering/exiting suspend)
	 */
	ldr	lr, ddr3_after_mmu_off_jump_addr	@ PHYSICAL address of "ddr3_cpu_suspend_after_mmu_off"
	b	ddr3_v7_disable_mmu
ddr3_after_mmu_off_jump_addr:
	.word(INT_SRAM_BASE + DDR3_SRAM_CODE_OFFSET \
		+ ddr3_after_mmu_off_jump_loc - capri_ddr3_cpu_suspend)
ddr3_after_mmu_off_jump_loc:

	/* ***** Warning: Discuss/Document the STACK usage in SRAM */
	LOAD_IO_FROM_PA	sp, (INT_SRAM_BASE + DDR3_SRAM_STACK_OFFSET + DDR3_SRAM_STACK_SIZE)

	bl	memc_ddr3_self_refresh_enter
	bl	memc_ddr3_phy_idle_standby_enter
	bl	memc_ddr3_shutoff_pll_enter

suspend_case_begin:
/*
	ldr	r0, suspend_param0
	cmp	r0, #SUSPEND_DORMANT
	beq	suspend_case_dormant
	cmp	r0, #SUSPEND_DEEPSLEEP
	beq	suspend_case_deepsleep

	b	suspend_case_default
*/
	@ Fall through: as we support Dormant mode only

suspend_case_dormant:
	// local_secure_api(SSAPI_DORMANT_ENTRY_SERV, ...)
	adr	r6, dormant_smc_params
	ldr	r4, suspend_param1
	str	r4, [r6, #0x00]			@ param0: point to buffer
	str	r6, [r6, #0x04]			@ param1: point to parameters
	ldr	r2, suspend_param2
	str	r2, [r6, #0x08]			@ param2: L2 control

	adr	r2, suspend_case_dormant_success
	str	r2, [r4, #0x00]			@ buffer[0]: core0 reset address
	ldr	r2, suspend_param3
	str	r2, [r4, #0x04]			@ buffer[1]: core1 reset address
	/* Warning: The case more than 2 CPU cores */

2:	mov	r4, #SSAPI_DORMANT_ENTRY_SERV	@ service id
	mov	r5, #3				@ Keep IRQ and FIQ off in SMC
	adr	r6, dormant_smc_params
#ifdef REQUIRES_SEC
	.arch_extension sec
#endif
	smc	0				@ Security Monitor Call --- put CPU0 into dormant mode
	cmp	r12, #1				@ SMC exit normally
	bne	2b

suspend_case_dormant_fail:
	b	suspend_case_end

	@ We use ADR to get the address. It could not be too far from it.
dormant_smc_params:
	.word(0)				@ param0 : point to buffer
	.word(0)				@ param1 : point to params
	.word(0)				@ param2 : L2 control

suspend_case_dormant_success:
	/* We must have come here through a reset of a successful dormant exit */

	/* ***** Warning: Discuss/Document the STACK usage in SRAM */
	LOAD_IO_FROM_PA	sp, (INT_SRAM_BASE + DDR3_SRAM_STACK_OFFSET + DDR3_SRAM_STACK_SIZE)

	bl	memc_ddr3_shutoff_pll_exit
	bl	memc_ddr3_phy_idle_standby_exit
	bl	memc_ddr3_self_refresh_exit

	ldr	sp, suspend_stack_register	@ Safe: relative address
	ldr	pc, suspend_param3		@ Safe: relative address

suspend_case_deepsleep:
	/* Dummy instructions: 30 times of "mov r0, r0" (nop) */
	mov	r0, #(30 >> 1)
3:	subs	r0, r0, #1
	bne	3b

	/* WFI put CPUs into suspend */
	dmb
	wfi
	dmb

	/* Dummy instructions: 34 times of "mov r0, r0" (nop) */
	mov	r0, #(34 >> 1)
4:	subs	r0, r0, #1
	bne	4b
	b	suspend_case_end

suspend_case_default:

	/* ***** Warning: suspend mode is not supported */

	@ Fall through as this is the last case

suspend_case_end:

	bl	memc_ddr3_shutoff_pll_exit
	bl	memc_ddr3_phy_idle_standby_exit
	bl	memc_ddr3_self_refresh_exit

	/*
	 * Enable MMU
	 *
	 * As we disabled MMU, it must be re-enalbed.
	 * ***** Warning: This path is not verified (Aug-22, 2012. Alamy)
	 */
	ldr	lr, ddr3_after_mmu_on_jump_addr		@ VIRTUAL address of "ddr3_after_mmu_on_jump_loc"
	b	ddr3_v7_enable_mmu
ddr3_after_mmu_on_jump_addr:
	.word(KONA_SRAM_VA + DDR3_SRAM_CODE_OFFSET \
		+ ddr3_after_mmu_on_jump_loc - capri_ddr3_cpu_suspend)
ddr3_after_mmu_on_jump_loc:

	mov	r0, #0
	ldr	sp, suspend_stack_register
	ldr	pc, suspend_lr_register


/* ----------------------------------------------------------------------------
 *
 * CAUTION
 *	- This part of codes is architecture related (refer to arch/arm/mm/cache-xxx.S)
 *	- We have no stack, must retain r0-r3 which are parameters passing "capri_ddr3_cpu_suspend"
 */
ddr3_v7_disable_mmu:
	.func

	/*
	 * Disable MMU, D-Cache.
	 * Keep I-Cache alive (performance), but flush it.
	 */
	ldr	r6, ddr3_v7_mmu_off_jump_addr	@ Load PA from data (we are VA here)
	mrc	p15, 0, r7, c1, c0, 0		@ Read control register
	bic	r7, r7, #(CR_M)			@ MMU
	bic	r7, r7, #(CR_C)			@ D-Cache
	mcr	p15, 0, r7, c1, c0, 0		@ Write back control register
	/*
	 * Although it works without the "jump" after MMU is disabled
	 * But it's better to have it as enabling MMU does
	 */
	mov	r6, r6
	mov	pc, r6
ddr3_v7_mmu_off_jump_addr:
	.word(INT_SRAM_BASE + DDR3_SRAM_CODE_OFFSET \
		+ ddr3_v7_mmu_on_off_jump_loc - capri_ddr3_cpu_suspend)

ddr3_v7_enable_mmu:
	/*
	 * Enable MMU, D-Cache
	 */
	ldr	r6, ddr3_v7_mmu_on_jump_addr	@ Load VA from data (we are PA here)
	mrc	p15, 0, r7, c1, c0, 0		@ Read control register
	orr	r7, r7, #(CR_M)			@ MMU
	orr	r7, r7, #(CR_C)			@ D-Cache
	mcr	p15, 0, r7, c1, c0, 0		@ Write back control register
	mov	r6, r6
	mov	pc, r6
ddr3_v7_mmu_on_jump_addr:
	.word(KONA_SRAM_VA + DDR3_SRAM_CODE_OFFSET \
		+ ddr3_v7_mmu_on_off_jump_loc - capri_ddr3_cpu_suspend)

ddr3_v7_mmu_on_off_jump_loc:
ddr3_v7_invalidate_all:
	@ <Rt> is ignored for BPIALL, BPIALLIS, ICIALLUIS, and ICIALLU
	mcr	p15, 0, r6, c7, c5, 0		@ ICIALLU  - Invalidate entire I-Cache, and flushes branch target cache
//	mcr	p15, 0, r6, c7, c5, 6		@ BPIALL   - Invalidate all branch predictor
//	mcr	p15, 0, r6, c7, c1, 6		@ BPIALLIS - Invalidate all branch predictor Inner Shareable

	@ <Rt> is ignored for DTLBIALL, ITLBIALL, TLBIALL, & TLBIALLIS
	mcr	p15, 0, r6, c8, c6, 0		@ DTLBIALL - Invalidate entire data TLB
	mcr	p15, 0, r6, c8, c5, 0		@ ITLBIALL - Invalidate entire instruction TLB
	mcr	p15, 0, r6, c8, c7, 0		@ TLBIALL  - Invalidate entire unified TLB
//	mcr	p15, 0, r6, c8, c3, 0		@ TLBIALLIS- Invalidate entire unified TLB Inner Shareable

ddr3_v7_flush_dcache_all:
	/*
	 * Don't bother to flush D-cache because of the following reasons
	 *
	 * Case of dormant success
	 *	- CPU being reset later (*)
	 *	- D-Cache is disabled
	 *	- Flushing D-Cache is not just few instructions task
	 * Case of others
	 *	- Data could be remain unchanged.
	 */

	mov	pc, lr
	.endfunc

/* ----------------------------------------------------------------------------
 *	void memc_ddr3_self_refresh_enter(void)
 *
 *	Instruct MEMC to enter self-refresh
 */
memc_ddr3_self_refresh_enter:
	.func

	LOAD_IO_FROM_PA	r4, (ROOT_RST_BASE_ADDR)		// 0x3500_1F00
	LOAD_IO_FROM_PA	r5, (MEMC0_OPEN_BASE_ADDR)		// 0x3500_8000

	movw	r2, #:lower16:WR_ACCESS_PASSWORD
	movt	r2, #:upper16:WR_ACCESS_PASSWORD
	orr	r2, r2, #ROOT_RST_MGR_REG_WR_ACCESS_RSTMGR_ACC_MASK
	str	r2, [r4, #ROOT_RST_MGR_REG_WR_ACCESS_OFFSET]

	/* self-refresh enable */
	ldr	r2, [r4, #ROOT_RST_MGR_REG_DDR3_REFRESH_CNTL_OFFSET]
	bic	r2, r2, #(ROOT_RST_MGR_REG_DDR3_REFRESH_CNTL_PRIV_ACCESS_MODE_MASK)
	orr	r2, r2, #(ROOT_RST_MGR_REG_DDR3_REFRESH_CNTL_SYS_EMI_DDR3_SW_SELF_REF_ENTER_MASK)
	str	r2, [r4, #ROOT_RST_MGR_REG_DDR3_REFRESH_CNTL_OFFSET]

	/* Waiting for self-refresh */
2:	ldr	r0, [r5, #CSR_DDR_POWER_DOWN_STATUS_OFFSET]
	and	r0, r0, #CSR_DDR_POWER_DOWN_STATUS_POWER_DOWN_STATUS_MASK
	lsr	r0, r0, #CSR_DDR_POWER_DOWN_STATUS_POWER_DOWN_STATUS_SHIFT
	cmp	r0, #(MEMC_SELF_REFRESH_STATE)
	bne	2b

	mov	pc, lr
	.endfunc

/* ----------------------------------------------------------------------------
 *	void memc_ddr3_self_refresh_exit(void)
 *
 *	Instruct MEMC to exit self-refresh
 */
memc_ddr3_self_refresh_exit:
	.func
	stmfd	sp!, {lr}

	/* writel( WR_ACCESS_PASSWORD | ROOT_RST_MGR_REG_WR_ACCESS_RSTMGR_ACC_MASK,
		KONA_ROOT_RST_VA + ROOT_RST_MGR_REG_WR_ACCESS_OFFSET ); */
	LOAD_IO_FROM_PA	r4, (ROOT_RST_BASE_ADDR)		// 0x3500_1F00

	movw	r2, #:lower16:WR_ACCESS_PASSWORD
	movt	r2, #:upper16:WR_ACCESS_PASSWORD
	orr	r2, r2, #ROOT_RST_MGR_REG_WR_ACCESS_RSTMGR_ACC_MASK
	str	r2, [r4, #ROOT_RST_MGR_REG_WR_ACCESS_OFFSET]

	/* writel( mode, KONA_ROOT_RST_VA + ROOT_RST_MGR_REG_DDR3_REFRESH_CNTL_OFFSET ); */
	ldr	r2, [r4, #ROOT_RST_MGR_REG_DDR3_REFRESH_CNTL_OFFSET]
	bic	r2, r2, #(ROOT_RST_MGR_REG_DDR3_REFRESH_CNTL_PRIV_ACCESS_MODE_MASK | \
			ROOT_RST_MGR_REG_DDR3_REFRESH_CNTL_SYS_EMI_DDR3_SW_SELF_REF_ENTER_MASK)
	str	r2, [r4, #ROOT_RST_MGR_REG_DDR3_REFRESH_CNTL_OFFSET]

	/* Check if PHY has changed mode registers --- if so, need to restore */
	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800

	ldr	r0, [r4, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_OFFSET]
	tst	r0, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_PWRDOWN_SKIP_MRS_MASK
	beq	2f
	/* The PHY changed the mode registers. Need to ensure they are put
	 * back into a usable state. Have to wait until MEMC is ready
	 * to exit its self-refresh mode before this can be done.
	 * Also should only see this happen if SKIP_MRS is not set.
	 */
	bl	ddr3_config_mode_registers
2:

	ldmfd	sp!, {pc}
	.endfunc

/* ----------------------------------------------------------------------------
 *	void ddr3_config_mode_registers(void)
 */
ddr3_config_mode_registers:
	.func

	/* Configure Mode Register 0 */
	DDR3_READ_MODE_REG	DDR40_PHY_ADDR_CTL_MODE_REG0_OFFSET
	bic	r0, r0, #(DDR3_MR0_BL_MASK)		@ &= ~DDR3_MR0_BL_MASK
	orr	r0, r0, #(DDR3_MR0_PDD_FAST_EXIT)	@ |= DDR3_MR0_PDD_FAST_EXIT
	orr	r0, r0, #(DDR3_MR0_BL_BC4_BL8_ONFLY)	@ |= DDR3_MR0_BL_BC4_BL8_ONFLY
	DDR3_WRITE_MODE_REG	CSR_DRAM_INIT_CONTROL_CS_BITS_BOTH, CSR_DRAM_INIT_CONTROL_MR_ADDR_REG0

#if	defined (CSR_DDR_SW_POWER_DOWN_CONTROL_DDR3_DLL_PPD_MASK)
	/* The MEMC open DLL PDD has to match the MR0 setting */
	LOAD_IO_FROM_PA	r4, (MEMC0_OPEN_BASE_ADDR)		// 0x3500_8000
	ldr	r2, [r4, #CSR_DDR_SW_POWER_DOWN_CONTROL_OFFSET]

	tst	r0, #(DDR3_MR0_PPD_MASK)
	beq	2f
	orr	r2, r2, #(CSR_DDR_SW_POWER_DOWN_CONTROL_DDR3_DLL_PPD_MASK)
	b	4f
2:	bic	r2, r2, #(CSR_DDR_SW_POWER_DOWN_CONTROL_DDR3_DLL_PPD_MASK)
4:	str	r2, [r4, #CSR_DDR_SW_POWER_DOWN_CONTROL_OFFSET]
#endif

	/* Configure Mode Register 1 */
	DDR3_READ_MODE_REG	DDR40_PHY_ADDR_CTL_MODE_REG1_OFFSET
	bic	r0, r0, #(DDR3_MR1_RTT_NOM_MASK)
	orr	r0, r0, #(ddr3cfg_odt_sdram_mr1)
	DDR3_WRITE_MODE_REG	CSR_DRAM_INIT_CONTROL_CS_BITS_BOTH, CSR_DRAM_INIT_CONTROL_MR_ADDR_REG1

	/* Configure Mode Register 2 */
	DDR3_READ_MODE_REG	DDR40_PHY_ADDR_CTL_MODE_REG1_OFFSET
	bic	r0, r0, #(DDR3_MR2_RTT_WR_MASK)
	orr	r0, r0, #(ddr3cfg_odt_sdram_mr2)
	DDR3_WRITE_MODE_REG	CSR_DRAM_INIT_CONTROL_CS_BITS_BOTH, CSR_DRAM_INIT_CONTROL_MR_ADDR_REG2

	mov	pc, lr
	.endfunc


/* ----------------------------------------------------------------------------
 *	void memc_ddr3_phy_idle_standby_enter(void)
 *		Put PHY into Standby low-power state / IDLE state
 *
 *	Wrapper function for
 *		memc_ddr3_phy_idle_enter(void)
 *		memc_ddr3_phy_standby_enter(void)
 */
memc_ddr3_phy_idle_standby_enter:
	.func
	stmfd	sp!, {lr}

	ldr	r2, ddr3_phy_mode
	cmp	r2, #0
	beq	2f
	bl	memc_ddr3_phy_idle_enter	@ ddr3_phy_mode == 1
	b	4f
2:	bl	memc_ddr3_phy_standby_enter	@ ddr3_phy_mode == 0
4:
	ldmfd	sp!, {pc}
	.endfunc

/* ----------------------------------------------------------------------------
 *	void memc_ddr3_phy_idle_standby_exit(void)
 *		Take PHY out of Standby / IDLE lower-powre state
 *
 *	Wrapper function for
 *		memc_ddr3_phy_idle_exit(void)
 *		memc_ddr3_phy_standby_exit(void)
 */
memc_ddr3_phy_idle_standby_exit:
	.func
	stmfd	sp!, {lr}

	ldr	r2, ddr3_phy_mode
	cmp	r2, #0
	beq	2f
	bl	memc_ddr3_phy_idle_exit		@ ddr3_phy_mode == 1
	b	4f
2:	bl	memc_ddr3_phy_standby_exit	@ ddr3_phy_mode == 0
4:
	ldmfd	sp!, {pc}
	.endfunc


/* ----------------------------------------------------------------------------
 *	void memc_ddr3_shutoff_pll_enter(void)
 *
 *	Procedure for PHY IDLE mode
 *	1. 0x3500_8814 : PLL_CONFIG[1:0] = 0b11 (Reset & Power Off PLL)
 *	2. 0x3500_8804 : CLK_PM_CTRL[0] = 0b1 (Disable DDR clocks)
 *	3. 0x3500_8430 : PLL_CTL5[1] = 0b1 (Power down DDR3 PLL)
 *	4. 0x3500_8104 : SW_CLK_SWITCH_CNTRL_REG[1] = 0b1 (Shut off PLL clock)
 */
memc_ddr3_shutoff_pll_enter:
	.func

	LOAD_IO_FROM_PA	r5, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800
	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_CTL_BASE_ADDR)		// 0x3500_8400
	LOAD_IO_FROM_PA	r3, (MEMC0_OPEN_BASE_ADDR)		// 0x3500_8000

	ldr	r2, ddr3_phy_mode
	cmp	r2, #0
	beq	shutoff_pll_enter_phy_standby_mode

shutoff_pll_enter_phy_idle_mode:
	/* Power down: PLL_CONFIG[1:0] = 0b11 */
	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_PLL_CONFIG_OFFSET]
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_PLL_CONFIG_RESET_MASK | \
			DDR40_PHY_ADDR_CTL_PLL_CONFIG_PWRDN_MASK)
	str	r2, [r5, #DDR40_PHY_ADDR_CTL_PLL_CONFIG_OFFSET]

	/* Disable clock: CLK_PM_CTRL[0] = 0b1 */
	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_CLK_PM_CTRL_OFFSET]
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_CLK_PM_CTRL_DIS_DDR_CLK_MASK)
	str	r2, [r5, #DDR40_PHY_ADDR_CTL_CLK_PM_CTRL_OFFSET]

	b	shutoff_pll_enter_phy_mode_end

shutoff_pll_enter_phy_standby_mode:
	/* power down */
	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_PLL_CONFIG_OFFSET]
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_PLL_CONFIG_PWRDN_MASK)
	str	r2, [r5, #DDR40_PHY_ADDR_CTL_PLL_CONFIG_OFFSET]

	/* Disable clock: CLK_PM_CTRL[0] = 0b1 */
	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_CLK_PM_CTRL_OFFSET]
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_CLK_PM_CTRL_DIS_DDR_CLK_MASK)
	str	r2, [r5, #DDR40_PHY_ADDR_CTL_CLK_PM_CTRL_OFFSET]

	/* Power down PHY PLL at the controller level */
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	orr	r2, r2, #DDR3CTL_PHY_CONFIG_MC2IOB_PHY_PLL_PWRDN_MASK
	str	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	/* dummy read */
//	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	/* disable PHY clock */
	orr	r2, r2, #DDR3CTL_PHY_CONFIG_MC2IOB_PHY_CK_DIS_MASK
	str	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]

	/*
	 * Finally put the PHY into its power down state.
	 *
	 * Note that you cannot access the DDR40_PHY_ADDR_CTL and DDR40_PHY_WORD_LANE registers
	 * until the PHY is taken out of its power down state.
	 */
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	bic	r2, r2, #(DDR3CTL_PHY_CONFIG_MC2IOB_STANDBY_EXIT_L_MASK)
	str	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]

shutoff_pll_enter_phy_mode_end:

	/* PLL_CTL5[1] = 0b1 */
	ldr	r2, [r4, #DDR3CTL_PLL_CTL5_OFFSET]
	orr	r2, r2, #(DDR3CTL_PLL_CTL5_PLL_PWRDN_MASK)
	str	r2, [r4, #DDR3CTL_PLL_CTL5_OFFSET]

	/* SW_CLK_SWITCH_CNTRL_REG[1] = 0b1 */
	ldr	r2, [r3, #CSR_SW_CLK_SWITCH_CNTRL_REG_OFFSET]
	orr	r2, r2, #(CSR_SW_CLK_SWITCH_CNTRL_REG_SW2CCU_SYS_PLL_CLK_IS_IDLE_MASK)
	str	r2, [r3, #CSR_SW_CLK_SWITCH_CNTRL_REG_OFFSET]

	mov	pc, lr
	.endfunc

/* ----------------------------------------------------------------------------
 *	void memc_ddr3_shutoff_pll_exit
 *
 *	Reverse prodedure of shutting off PLLs & wait for PLL lock
 */
memc_ddr3_shutoff_pll_exit:
	.func

	LOAD_IO_FROM_PA	r5, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800
	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_CTL_BASE_ADDR)		// 0x3500_8400
	LOAD_IO_FROM_PA	r3, (MEMC0_OPEN_BASE_ADDR)		// 0x3500_8000

	/* Release PLL clk_is_idle: SW_CLK_SWITCH_CNTRL_REG[1] = 0b0 */
	ldr	r2, [r3, #CSR_SW_CLK_SWITCH_CNTRL_REG_OFFSET]
	bic	r2, r2, #(CSR_SW_CLK_SWITCH_CNTRL_REG_SW2CCU_SYS_PLL_CLK_IS_IDLE_MASK)
	str	r2, [r3, #CSR_SW_CLK_SWITCH_CNTRL_REG_OFFSET]

	ldr	r2, [r4, #DDR3CTL_PLL_CTL5_OFFSET]
	/* Release power down: PLL_CTL5[1] = 0b0 */
	bic	r2, r2, #(DDR3CTL_PLL_CTL5_PLL_PWRDN_MASK)
	str	r2, [r4, #DDR3CTL_PLL_CTL5_OFFSET]
	/* Put PLL in reset: PLL_CTL5[0] = 0b0 */
	bic	r2, r2, #(DDR3CTL_PLL_CTL5_PLL_RESETB_MASK)
	str	r2, [r4, #DDR3CTL_PLL_CTL5_OFFSET]
	/* Hold post dividers in reset: PLL_CTL5[2] = 0b0 */
	bic	r2, r2, #(DDR3CTL_PLL_CTL5_PLL_POST_RESETB_MASK)
	str	r2, [r4, #DDR3CTL_PLL_CTL5_OFFSET]

	/* Check/wait until unlocked: PLL_STATUS[13] == 0b0 */
2:	ldr	r1, [r4, #DDR3CTL_PLL_STATUS_OFFSET]
	tst	r1, #DDR3CTL_PLL_STATUS_PLL_LOCK_MASK
	bne	2b

	/* Release PLL from reset: PLL_CTL5[0] = 0b1 */
	ldr	r2, [r4, #DDR3CTL_PLL_CTL5_OFFSET]
	orr	r2, r2, #(DDR3CTL_PLL_CTL5_PLL_RESETB_MASK)
	str	r2, [r4, #DDR3CTL_PLL_CTL5_OFFSET]

	/* Wait until locked */
4:	ldr	r1, [r4, #DDR3CTL_PLL_STATUS_OFFSET]
	tst	r1, #DDR3CTL_PLL_STATUS_PLL_LOCK_MASK
	beq	4b

	/* Release post dividers */
	ldr	r2, [r4, #DDR3CTL_PLL_CTL5_OFFSET]
	orr	r2, r2, #(DDR3CTL_PLL_CTL5_PLL_POST_RESETB_MASK)
	str	r2, [r4, #DDR3CTL_PLL_CTL5_OFFSET]

	ldr	r2, ddr3_phy_mode
	cmp	r2, #0
	beq	shutoff_pll_exit_phy_standby_mode

shutoff_pll_exit_phy_idle_mode:
	/* Enable clock: CLK_PM_CTRL[0] = 0b0 */
	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_CLK_PM_CTRL_OFFSET]
	bic	r2, r2, #DDR40_PHY_ADDR_CTL_CLK_PM_CTRL_DIS_DDR_CLK_MASK
	str	r2, [r5, #DDR40_PHY_ADDR_CTL_CLK_PM_CTRL_OFFSET]

	/* Power up PLL: PLL_CONFIG[1:0] = 0b00 */
	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_PLL_CONFIG_OFFSET]
	bic	r2, r2, #(DDR40_PHY_ADDR_CTL_PLL_CONFIG_RESET_MASK | \
			DDR40_PHY_ADDR_CTL_PLL_CONFIG_PWRDN_MASK)
	str	r2, [r5, #DDR40_PHY_ADDR_CTL_PLL_CONFIG_OFFSET]

	/* Wait for lock: PLL_STATUS[0] = 0b1 */
6:	ldr	r1, [r5, #DDR40_PHY_ADDR_CTL_PLL_STATUS_OFFSET]
	tst	r1, #(DDR40_PHY_ADDR_CTL_PLL_STATUS_LOCK_MASK)
	beq	6b
	b	shutoff_pll_exit_phy_mode_end

shutoff_pll_exit_phy_standby_mode:
	/* Release PHY PLL from power down */
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	bic	r2, r2, #DDR3CTL_PHY_CONFIG_MC2IOB_PHY_PLL_PWRDN_MASK
	str	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	/* Enable PHY clock */
	bic	r2, r2, #DDR3CTL_PHY_CONFIG_MC2IOB_PHY_CK_DIS_MASK	@ Save reading instruation
	str	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	/* dummy reads (why, and how many ?) */
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]

shutoff_pll_exit_phy_mode_end:

	mov	pc, lr
	.endfunc


/* ----------------------------------------------------------------------------
 *	void memc_ddr3_phy_idle_enter(void)
 *
 *	Config  IDLE_PAD_CONTROL regs (PHY IDLE enter, or going to the lowest
 *	power state)
 *
 *	1. All _oeb, _reb, _rxenb should be disabled (0b1) in IDLE_PAD_CONTROL
 *	   register of SYS_EMI_DDR3_PHY_WL_0 (0x3500_8A00)
 *	           and SYS_EMI_DDR3_PHY_WL_1 (0x3500_8C00)
 *
 *	2. Then set the IDLE bit(bit 31) of IDLE_PAD_CONTROL, for both
 *	   SYS_EMI_DDR3_PHY_WL_0 & SYS_EMI_DDR3_PHY_WL_1
 *
 *	3. Set IDLE bit(bit 31) in IDLE_PAD_CONTROL register
 *	   in SYS_EMI_DDR3_PHY_ADDR_CTL
 */
memc_ddr3_phy_idle_enter:
	.func

	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_WL_0_BASE_ADDR)	// 0x3500_8A00
	LOAD_IO_FROM_PA	r5, (MEMC0_DDR3_PHY_WL_1_BASE_ADDR)	// 0x3500_8C00

	/* Prepare to disable all _oeb, _reb, _rxenb bits */
	/* bit 0,1,3, 4,5,7, 8,9,11, 12,13,15, 16,17,19 */
	mov	r3, #(DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_CLK0_OEB_MASK | \
		      DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_CLK0_REB_MASK | \
		      DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_CLK0_RXENB_MASK)		@ clk0 (r3 = 0xB)
@	orr	r3, r3, r3, lsl 4		@ r3 = 0xBB
@	orr	r3, r3, r3, lsl 8		@ r3 = 0xBBBB
	orr	r3, #(DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_CLK1_OEB_MASK | \
		      DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_CLK1_REB_MASK | \
		      DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_CLK1_RXENB_MASK)		@ clk1 (r3 = 0xBB)
	orr	r3, #(DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_DQS_OEB_MASK | \
		      DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_DQS_REB_MASK | \
		      DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_DQS_RXENB_MASK)		@ dqs (r3 = 0xBBB)
	orr	r3, #(DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_READ_ENB_OEB_MASK | \
		      DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_READ_ENB_REB_MASK | \
		      DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_READ_ENB_RXENB_MASK)		@ read_enb (r3 = 0xBBBB)
	orr	r3, #(DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_DQ_OEB_MASK | \
		      DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_DQ_REB_MASK | \
		      DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_DQ_RXENB_MASK)		@ dq (r3 = 0xBBBBB)

	/* IDLE_PAD_CONTROL[lane0] (0x1A0) */
	ldr	r2, [r4, #DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_OFFSET]			@ ICE: r2 = 0x005F2FAA
	str	r2, phy_idle_pad_ctrl_reg_wl0

	orr	r2, r3									@ ICE: r2 = 0x005FBFBB
	str	r2, [r4, #DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_OFFSET]
	orr	r2, r2, #DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_IDLE_MASK			@ bit-31 = 1
	str	r2, [r4, #DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_OFFSET]			@ lane0 idle state (276->262 mA)

	/* IDLE_PAD_CONTROL[lane1] (0x1A0) */
	ldr	r2, [r5, #DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_OFFSET]			@ ICE: r2 = 0x005F2FAA
	str	r2, phy_idle_pad_ctrl_reg_wl1

	orr	r2, r3									@ ICE: r2 = 0x005FBFBB
	str	r2, [r5, #DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_OFFSET]
	orr	r2, r2, #DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_IDLE_MASK			@ bit-31 = 1
	str	r2, [r5, #DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_OFFSET]			@ lane1 idle state (262->248 mA)

	/* Enable global idle: [DDR3_PHY_ADDR_CTL : IDLE_PAD_CONTROL] */
	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800
	ldr	r2, [r4, #DDR40_PHY_ADDR_CTL_IDLE_PAD_CONTROL_OFFSET]			@ ICE: r2 = 0x00000132
	orr	r2, r2, #DDR40_PHY_ADDR_CTL_IDLE_PAD_CONTROL_IDLE_MASK			@ bit-31 = 1
	str	r2, [r4, #DDR40_PHY_ADDR_CTL_IDLE_PAD_CONTROL_OFFSET]			@ ICE: 249->228 mA

	mov	pc, lr
	.endfunc

/* ----------------------------------------------------------------------------
 *	void memc_ddr3_phy_idle_exit(void)
 *
 *	Restoring all IDLE_PAD_CONTROL regs(PHY IDLE exit, going out of lowest power state)
 */
memc_ddr3_phy_idle_exit:
	.func

	/* Disable global idle: [DDR3_PHY_ADDR_CTL : IDLE_PAD_CONTROL] */
	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800
	ldr	r2, [r4, #DDR40_PHY_ADDR_CTL_IDLE_PAD_CONTROL_OFFSET]
	bic	r2, r2, #DDR40_PHY_ADDR_CTL_IDLE_PAD_CONTROL_IDLE_MASK			@ bit-31 = 0
	str	r2, [r4, #DDR40_PHY_ADDR_CTL_IDLE_PAD_CONTROL_OFFSET]

	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_WL_0_BASE_ADDR)	// 0x3500_8A00
	LOAD_IO_FROM_PA	r5, (MEMC0_DDR3_PHY_WL_1_BASE_ADDR)	// 0x3500_8C00

	/* IDLE_PAD_CONTROL[lane1] */
	ldr	r2, phy_idle_pad_ctrl_reg_wl1
	str	r2, [r5, #DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_OFFSET]

	/* IDLE_PAD_CONTROL[lane0] */
	ldr	r2, phy_idle_pad_ctrl_reg_wl0
	str	r2, [r4, #DDR40_PHY_WORD_LANE_IDLE_PAD_CONTROL_OFFSET]

	mov	pc, lr
	.endfunc


/* ----------------------------------------------------------------------------
 *	void memc_ddr3_phy_standby_enter(void)
 */
memc_ddr3_phy_standby_enter:
	.func

	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_CTL_BASE_ADDR)		// 0x3500_8400
	LOAD_IO_FROM_PA	r5, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800

	/* Wait until OK to perform PHY standby operations */
2:	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_OFFSET]
	tst	r2, #(DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_STANDBY_READY_MASK)
	beq	2b

	/* Put the PHY into the ARMED state with CKE low and RST_N high.
	 * We are going to use the PHY standby pin to exit the STANDBY
	 * standby state instead of VDDC. Need to make sure this is
	 * high beforehand, and then low once in STANDBY state.
	 * We also have already configured the device, so don't need
	 * to re-issue the Mode Register Set (MRS) when we exit.
	 */
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	orr	r2, r2, #DDR3CTL_PHY_CONFIG_MC2IOB_STANDBY_EXIT_L_MASK
	str	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]

	/* Remap to PHY addr standby control values */
#if defined (CONFIG_CAPRI_SYSEMI_DDR3_1V80)
	mov	r2, #(CHAL_MEMC_DDR3_LDO_1P80V << DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_LDO_VOLTS_SHIFT)

#elif defined (CONFIG_CAPRI_SYSEMI_DDR3_1V50)
	mov	r2, #(CHAL_MEMC_DDR3_LDO_1P50V << DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_LDO_VOLTS_SHIFT)

#elif defined (CONFIG_CAPRI_SYSEMI_DDR3_1V35)
	mov	r2, #(CHAL_MEMC_DDR3_LDO_1P35V << DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_LDO_VOLTS_SHIFT)

#else
	#error Please find out the DDR3 you are using
#endif
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_RST_N_MASK | \
			DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_SKIP_MRS_MASK)
	orr	r2, r2, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_STANDBY_EXIT_PIN_EN_MASK
	orr	r2, r2, #(0x5)					@ arm standby
	str	r2, [r5, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_OFFSET]

	/* Wait PHY standby armed */
4:	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_OFFSET]
	tst	r2, #(DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_ARMED_MASK)
	beq	4b

#if defined (STANDBY_ENTER_REG_0xA)
	@ BCM11140 is not this case
	/* Clear the previous standby command 0x5 before writing the next command 0xA */
	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_OFFSET]
	bic	r2, r2, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_STANDBY_MASK
	orr	r2, r2, #0x0A
	str	r2, [r5, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_OFFSET]
#else
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	orr	r2, r2, #DDR3CTL_PHY_CONFIG_MC2IOB_PHY_STANDBY_MASK
	str	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
#endif

	/* Wait PHY standby active */
6:	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_OFFSET]
	tst	r2, #(DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_STANDBY_ACTIVE_MASK)
	beq	6b

	mov	pc, lr
	.endfunc

/* ----------------------------------------------------------------------------
 *	void memc_ddr3_phy_standby_exit(void)
 */
memc_ddr3_phy_standby_exit:
	.func
	stmfd	sp!, {lr}

	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_CTL_BASE_ADDR)		// 0x3500_8400
	LOAD_IO_FROM_PA	r5, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800

#if !defined (STANDBY_ENTER_REG_0xA)
	@ BCM911140 is this case
	/* The i_phy standby signal was used to put the PHY into standby active state.
	 * Make sure the signal is now de-asserted.
	 */
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	bic	r2, r2, #DDR3CTL_PHY_CONFIG_MC2IOB_PHY_STANDBY_MASK
	str	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
#endif

	/* Take the PHY out its power down state */
	ldr	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]
	orr	r2, r2, #DDR3CTL_PHY_CONFIG_MC2IOB_STANDBY_EXIT_L_MASK
	str	r2, [r4, #DDR3CTL_PHY_CONFIG_OFFSET]

	/* Wait until PHY power up status indicates that PHY POR (Power On Reset) is complete
	 * You will be able to access the DDR40_PHY_ADDR_CTL and DDR40_PHY_WORD_LANE registers again
	 * after this.
	 */
#if defined(DDR3CTL_PHY_STATUS_IOB2MC_PHY_PWRUP_RSB_MASK) && defined(DDR3CTL_PHY_STATUS_IOB2MC_PHY_PWRUP_RSB_MASK)
	@ BCM911140 is this case
	/* Wait PHY Powerup */
2:	ldr	r2, [r4, #DDR3CTL_PHY_STATUS_OFFSET]
	tst	r2, #(DDR3CTL_PHY_STATUS_IOB2MC_PHY_PWRUP_RSB_MASK)
	beq	2b
#else
	/* PHY power up status is not available on BigIsland/Hana
	 * Delay loop instead
	 */
	DELAY	1	/* wait 1 us (must retain r5, r4) */
#endif

	/* Wait PHY standby warm-start */
4:	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_OFFSET]
	tst	r2, #(DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_WARMSTART_MASK)
	beq	4b

	/* Wait PHY standby ready */
6:	ldr	r2, [r5, #DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_OFFSET]
	tst	r2, #(DDR40_PHY_ADDR_CTL_STANDBY_CONTROL_STANDBY_READY_MASK)
	beq	6b

	/* Putting the PHY into its power down state essentially resets the PHY.
	 * We need to re-initialize things to bring it back to an operable state.
	 */
	bl	ddr3_strap_config
	mov	r0, #(DDR40_PHY_ADDR_CTL_VDL_CALIBRATE_SKIP_RST_MASK | \
		      DDR40_PHY_ADDR_CTL_VDL_CALIBRATE_EXIT_IN_SR_MASK)
	bl	ddr3_auto_calibrate
	bl	ddr3_config_phy_odt
	bl	ddr3_config_wr_preamble
	bl	ddr3_config_virtual_vtt

	ldmfd	sp!, {pc}
	.endfunc

/* ----------------------------------------------------------------------------
 *	void ddr3_strap_config(void)
 */
ddr3_strap_config:
	.func

#if (ddr3cfg_jedec_type > CHAL_MEMC_DDR3_JEDEC_TYPE_MAX)
	#error Invalid JEDEC type
#endif

#if (ddr3cfg_strap_override != 1)
	#error Strap override must be enabled (set to 1)
#endif

	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800

	/* r3 = jedec_info [ddr3cfg_jedec_type] */
	adr	r3, jedec_info
	add	r3, r3, #(ddr3cfg_jedec_type * (JEDEC_INFO_FIELD_NUM << 2))

	/*
	 * JIRA HWBIGISLAND-891
         *
	 * I don't feel like to compile in the code when it's already fixed in H.W.
	 */
#if defined(JIRA_HWBIGISLAND_891)

#else
	ldr	r2, [r3, #0]                    @ r2 = jedec_info[*].MHz
	lsl	r2, r2, #DDR40_PHY_ADDR_CTL_STRAP_CONTROL_MHZ_SHIFT
#endif
	orr	r2, r2, #((ddr3cfg_ad_width       << DDR40_PHY_ADDR_CTL_STRAP_CONTROL_AD_WIDTH_SHIFT)   | \
			  (ddr3cfg_rank           << DDR40_PHY_ADDR_CTL_STRAP_CONTROL_DUAL_RANK_SHIFT)  | \
			  (ddr3cfg_bus_width      << DDR40_PHY_ADDR_CTL_STRAP_CONTROL_BUS8_SHIFT)       | \
			  (ddr3cfg_chip_width     << DDR40_PHY_ADDR_CTL_STRAP_CONTROL_CHIP_WIDTH_SHIFT) | \
			  (ddr3cfg_vddq           << DDR40_PHY_ADDR_CTL_STRAP_CONTROL_VDDQ_SHIFT))
	orr	r2, r2, #((ddr3cfg_chip_size      << DDR40_PHY_ADDR_CTL_STRAP_CONTROL_CHIP_SIZE_SHIFT)  | \
			  (ddr3cfg_jedec_type     << DDR40_PHY_ADDR_CTL_STRAP_CONTROL_JEDEC_TYPE_SHIFT) | \
			  (ddr3cfg_strap_override << DDR40_PHY_ADDR_CTL_STRAP_CONTROL_STRAPS_VALID_SHIFT))
	str	r2, [r4, #DDR40_PHY_ADDR_CTL_STRAP_CONTROL_OFFSET]

#if CHAL_MEMC_DDR3_JEDEC_TYPE_IS_DDR3(ddr3cfg_jedec_type)
	mov	r2, #DDR40_PHY_ADDR_CTL_STRAP_CONTROL2_DDR3_MASK
#else
	mov	r2, #0
#endif
	orr	r2, r2, #(0 << DDR40_PHY_ADDR_CTL_STRAP_CONTROL2_AL_SHIFT)

	ldr	r1, [r3, #4]                    @ r1 = jedec_info[*].CL
	orr	r2, r1, lsl #DDR40_PHY_ADDR_CTL_STRAP_CONTROL2_CL_SHIFT

	ldr	r1, [r3, #8]                    @ r1 = jedec_info[*].CWL
	orr	r2, r1, lsl #DDR40_PHY_ADDR_CTL_STRAP_CONTROL2_CWL_SHIFT

	ldr	r1, [r3, #0xC]                  @ r1 = jedec_info[*].WR
	orr	r2, r1, lsl #DDR40_PHY_ADDR_CTL_STRAP_CONTROL2_WR_SHIFT

	str	r2, [r4, #DDR40_PHY_ADDR_CTL_STRAP_CONTROL2_OFFSET]

	/* Populate remainder of device info structure */
	// Do we really need this ?  ***** WARNING: review later

	mov	pc, lr
	.endfunc

/* ----------------------------------------------------------------------------
 *	r0(=status) ddr3_auto_calibrate(r0 = option)
 */
ddr3_auto_calibrate:
	.func

	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800

	/* Ensure that audio_init is cleared.  JIRA HWCAPRI-1389 */
	ldr	r2, [r4, #DDR40_PHY_ADDR_CTL_VDL_CALIBRATE_OFFSET]
	bic	r2, r2, #(DDR40_PHY_ADDR_CTL_VDL_CALIBRATE_AUTO_INIT_MASK)
	str	r2, [r4, #DDR40_PHY_ADDR_CTL_VDL_CALIBRATE_OFFSET]

	/* Auto-Calibrate */
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_VDL_CALIBRATE_USE_STRAPS_MASK | \
			DDR40_PHY_ADDR_CTL_VDL_CALIBRATE_AUTO_INIT_MASK)
	str	r2, [r4, #DDR40_PHY_ADDR_CTL_VDL_CALIBRATE_OFFSET]

	/* Wait PHY auto-init - Polling done bit */
2:	ldr	r2, [r4, #DDR40_PHY_ADDR_CTL_VDL_CALIB_STATUS_OFFSET]
	tst	r2, #(DDR40_PHY_ADDR_CTL_VDL_CALIB_STATUS_AUTO_INIT_DONE_MASK)
	beq	2b

	/* Verify PHY PLL has locked */
4:	ldr	r2, [r4, #DDR40_PHY_ADDR_CTL_PLL_STATUS_OFFSET]
	tst	r2, #(DDR40_PHY_ADDR_CTL_PLL_STATUS_LOCK_MASK)
	beq	4b

	/* Return the Fail status */
	ldr	r2, [r4, #DDR40_PHY_ADDR_CTL_VDL_CALIB_STATUS_OFFSET]
	and	r2, r2, #DDR40_PHY_ADDR_CTL_VDL_CALIB_STATUS_AUTO_INIT_FAIL_MASK
	lsr	r0, r2, #DDR40_PHY_ADDR_CTL_VDL_CALIB_STATUS_AUTO_INIT_FAIL_SHIFT

	mov	pc, lr
	.endfunc

/* ----------------------------------------------------------------------------
 *	void ddr3_config_phy_odt(void)
 */
ddr3_config_phy_odt:
	.func

	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_WL_0_BASE_ADDR)	// 0x3500_8A00
	LOAD_IO_FROM_PA	r5, (MEMC0_DDR3_PHY_WL_1_BASE_ADDR)	// 0x3500_8C00

	/* Turn ODT on or off for the PHY DQ/DQS. This is the only variable part of
	 * the PHY ODT cofiguration.
	 *
	 * (In Assembly, since the configuration is defined, the code is fixed by pre-processor)
	 */
#if (ddr3cfg_odt_phy_enabled == 1)
	mov	r2, #(DDR40_PHY_WORD_LANE_READ_CONTROL_DQ_ODT_ENABLE_MASK | \
		      DDR40_PHY_WORD_LANE_READ_CONTROL_DQ_ODT_TE_ADJ_MASK)
#else
	mov	r2, #0
#endif
	str	r2, [r4, #DDR40_PHY_WORD_LANE_READ_CONTROL_OFFSET]
	str	r2, [r5, #DDR40_PHY_WORD_LANE_READ_CONTROL_OFFSET]

	/* Set the PHY word lane VDDO to something more appropriate for the h/w. We will also
	 * set the terminations, but that should not matter if ODT was disabled above.
	 */
	mov	r2, #((ddr3cfg_vddq << DDR40_PHY_WORD_LANE_DRIVE_PAD_CTL_VDDO_VOLTS_SHIFT) | \
		      (DDR40_PHY_WORD_LANE_DRIVE_PAD_CTL_RT60B_MASK))   // 120 ohm
	str	r2, [r4, #DDR40_PHY_WORD_LANE_DRIVE_PAD_CTL_OFFSET]
	str	r2, [r5, #DDR40_PHY_WORD_LANE_DRIVE_PAD_CTL_OFFSET]

	/* Set the PHY ADDR/CTL terminations to something more appropriate for the h/w */
	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800
	mov	r2, #((ddr3cfg_vddq << DDR40_PHY_ADDR_CTL_DRIVE_PAD_CTL_VDDO_VOLTS_SHIFT) | \
		      DDR40_PHY_ADDR_CTL_DRIVE_PAD_CTL_RT60B_MASK)      // 120 ohm
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_DRIVE_PAD_CTL_AUTO_OEB_MASK | \
			  DDR40_PHY_ADDR_CTL_DRIVE_PAD_CTL_IDDQ_A15_MASK)
	str	r2, [r4, #DDR40_PHY_ADDR_CTL_DRIVE_PAD_CTL_OFFSET]

	mov	pc, lr
	.endfunc

/* ----------------------------------------------------------------------------
 *	void ddr3_config_wr_preabmble(void)
 */
ddr3_config_wr_preamble:
	.func

	/* Write preamble long enabled */

	/* Required for JIRA HWCAPRI-1385 */
	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_WL_0_BASE_ADDR)	// 0x3500_8A00
	LOAD_IO_FROM_PA	r5, (MEMC0_DDR3_PHY_WL_1_BASE_ADDR)	// 0x3500_8C00
	mov	r2, #(DDR40_PHY_WORD_LANE_WR_PREAMBLE_MODE_LONG_MASK)

	str	r2, [r4, #DDR40_PHY_WORD_LANE_WR_PREAMBLE_MODE_OFFSET]
	str	r2, [r5, #DDR40_PHY_WORD_LANE_WR_PREAMBLE_MODE_OFFSET]

	mov	pc, lr
	.endfunc

/* ----------------------------------------------------------------------------
 *	void ddr3_config_virtual_vtt(void)
 */
ddr3_config_virtual_vtt:
	.func

	LOAD_IO_FROM_PA	r4, (MEMC0_DDR3_PHY_ADDR_CTL_BASE_ADDR)	// 0x3500_8800
	eor	r2, r2				@ r2 = 0

#if (ddr3cfg_virtual_vtt_enabled == 1)
	/* Confiugre r2 (VTT_CONNNECTIONS) */

	/* Enable ba[2], ba[1], ba[0] */
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_BA2_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_BA1_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_BA0_MASK)
	/* Enable ras, cas, we */
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_RAS_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_CAS_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_WE_MASK)

	ldr	r3, [r4, #DDR40_PHY_ADDR_CTL_STRAP_STATUS_OFFSET]
	tst	r3, #DDR40_PHY_ADDR_CTL_STRAP_STATUS_STRAPS_VALID_MASK
	bne	2f
	mov	r0, #0
	mov	pc, lr
2:

	and	r1, r3, #DDR40_PHY_ADDR_CTL_STRAP_STATUS_AD_WIDTH_MASK
	lsr	r1, r1, #DDR40_PHY_ADDR_CTL_STRAP_STATUS_AD_WIDTH_SHIFT
	movw	r0, #:lower16:(0x00001FFF)
	lsl	r0, r0, r1                      // AD_WIDTH_13B (0) --> 0x1FFF, AD_WIDTH_14B (1) --> 3FFF
	orr	r0, r0, #(0xF)                  // AD_WIDTH_15B (2) --> 0x7FFF, AD_WIDTH_16B (3) --> 7FFF
	orr	r2, r2, r0

	and	r1, r3, #DDR40_PHY_ADDR_CTL_STRAP_STATUS_DUAL_RANK_MASK
	lsr	r1, r1, #DDR40_PHY_ADDR_CTL_STRAP_STATUS_DUAL_RANK_SHIFT
	cmp	r1, #0
	beq	4f
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_AUX2_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_AUX1_MASK)
	b	6f
4:
	#if defined(CFG_GLOBAL_DDR3_SWAP_CS0_CS1) && CFG_GLOBAL_DDR3_SWAP_CS0_CS1
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_AUX1_MASK)
	#else
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_AUX0_MASK)
	#endif
6:
	str	r2, [r4, #DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_OFFSET]

	/* Configure VTT_OVERRIDES */
	/* Use the VTT_CONNECTIONS but don't configure ras, cas, we, cs_n */
	bic	r1, r2, #(DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_RAS_MASK  | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_CAS_MASK  | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_WE_MASK   | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_AUX2_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_AUX1_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONNECTIONS_AUX0_MASK)
	str	r1, [r4, #DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_OVERRIDE_OFFSET]

	ldr	r2, [r4, #DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONTROL_OFFSET]
	orr	r2, r2, #(DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONTROL_ENABLE_CTL_IDLE_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONTROL_ENABLE_CS_IDLE_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONTROL_ENABLE_CKE_IDLE_MASK)
	str	r2, [r4, #DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONTROL_OFFSET]

#else
	ldr	r2, [r4, #DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONTROL_OFFSET]
	bic	r2, r2, #(DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONTROL_ENABLE_CTL_IDLE_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONTROL_ENABLE_CS_IDLE_MASK | \
			  DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONTROL_ENABLE_CKE_IDLE_MASK)
	str	r2, [r4, #DDR40_PHY_ADDR_CTL_VIRTUAL_VTT_CONTROL_OFFSET]
#endif

	mov	r0, #0
	mov	pc, lr
	.endfunc


/*
 ==============================================================================
 */

/* ----------------------------------------------------------------------------
 *	Literal Pool
 */
static_variable:
	.LTORG

jedec_info:
	/*       MHz      CL    CWL    WR */
	.word    266  ,    3  ,  0  ,   4       @ // 0   DDR2-533B
	.word    266  ,    4  ,  0  ,   4       @ // 1   DDR2-533C
	.word    333  ,    4  ,  0  ,   5       @ // 2   DDR2-667C
	.word    333  ,    5  ,  0  ,   5       @ // 3   DDR2-667D
	.word    400  ,    4  ,  0  ,   6       @ // 4   DDR2-800C
	.word    400  ,    5  ,  0  ,   6       @ // 5   DDR2-800D
	.word    400  ,    6  ,  0  ,   6       @ // 6   DDR2-800E
	.word    533  ,    6  ,  0  ,   8       @ // 7   DDR2-1066E
	.word    533  ,    7  ,  0  ,   8       @ // 8   DDR2-1066F
	.word    400  ,    5  ,  5  ,   6       @ // 9   DDR3-800D
	.word    400  ,    6  ,  5  ,   6       @ // 10  DDR3-800E
	.word    533  ,    6  ,  6  ,   8       @ // 11  DDR3-1066E
	.word    533  ,    7  ,  6  ,   8       @ // 12  DDR3-1066F
	.word    533  ,    8  ,  6  ,   8       @ // 13  DDR3-1066G
	.word    667  ,    7  ,  7  ,  10       @ // 14  DDR3-1333F
	.word    667  ,    8  ,  7  ,  10       @ // 15  DDR3-1333G
	.word    667  ,    9  ,  7  ,  10       @ // 16  DDR3-1333H
	.word    667  ,   10  ,  7  ,  10       @ // 17  DDR3-1333J
	.word    800  ,    8  ,  8  ,  12       @ // 18  DDR3-1600G
	.word    800  ,    9  ,  8  ,  12       @ // 19  DDR3-1600H
	.word    800  ,   10  ,  8  ,  12       @ // 20  DDR3-1600J
	.word    800  ,   11  ,  8  ,  12       @ // 21  DDR3-1600K
	.word    933  ,   10  ,  9  ,  14       @ // 22  DDR3-1866J
	.word    933  ,   11  ,  9  ,  14       @ // 23  DDR3-1866K
	.word    933  ,   12  ,  9  ,  14       @ // 24  DDR3-1866L
	.word    933  ,   13  ,  9  ,  14       @ // 25  DDR3-1866M
	.word   1066  ,   11  , 10  ,  16       @ // 26  DDR3-2133K
	.word   1066  ,   12  , 10  ,  16       @ // 27  DDR3-2133L
	.word   1066  ,   13  , 10  ,  16       @ // 28  DDR3-2133M
	.word   1066  ,   14  , 10  ,  16       @ // 29  DDR3-2133N

suspend_lr_register:
	.word(0)
suspend_stack_register:
	.word(0)
suspend_param0:
	.word(0)
suspend_param1:
	.word(0)
suspend_param2:
	.word(0)
suspend_param3:
	.word(0)

ddr3_phy_mode:
	.word(1)
phy_idle_pad_ctrl_reg_wl0:
	.word(0)
phy_idle_pad_ctrl_reg_wl1:
	.word(0)

/*
 ==============================================================================
 */

/* CAUTION: capri_ddr3_cpu_suspend_size MUST be the last data.
 * So that we won't leave anything behind in DRAM not being copied to SRAM
 */
ENTRY(capri_ddr3_cpu_suspend_size)
	.word	. - capri_ddr3_cpu_suspend

	.end
