/*
 * Copyright 2011 Broadcom Corporation.  All rights reserved.
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
 *
 * Ported from CSP : chal_sdram_configs.h, chal_memc_ddr3.h for capri_pm_ddr3.S
 * - Removed LPDDR2 configuration (Sorry, I work on DDR3)
 *	      Alamy Liu <alamy.liu@broadcom.com>. Jun-15, 2012
 */

#ifndef _CHAL_SDRAM_CONFIGS_H_
#define _CHAL_SDRAM_CONFIGS_H_

/* ----------- Define (selecting) one memory configuration ----------------- */

#if defined(CONFIG_CAPRI_SYSEMI_DDR3_1V80)
#error Help! I Don't know the setting for 1v80 DDR3 SDRAM yet.

#elif defined(CONFIG_CAPRI_SYSEMI_DDR3_1V50)
	/* ***** Warning: The original voltage define for this chip is 1v35 */
#define CHAL_SDRAM_CONFIG_DDR3L_MICRON_MT41K128M16HA_2Gbx16_400MHz

#elif defined(CONFIG_CAPRI_SYSEMI_DDR3_1V35)
#define CHAL_SDRAM_CONFIG_DDR3L_MICRON_MT41K256M8HX_2Gbx8_400MHz

#else
#error Unsupported DDR3 voltage setting.

#endif

#define FALSE					(0)
#define TRUE					(1)

/* ------- define the immediate values used by memory configuration -------- */

/*
* Instead of including chal/chal_memc_ddr3.h (which causes a lot of
* compiling error problems). Just copy the necessary part of its definition
* to this file for compiling reason.
*	Jun-11, 2012. Alamy (alamy.liu@broadcom.com)
*/

#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR2_533B	(0)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR2_533C	(1)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR2_667C	(2)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR2_667D	(3)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR2_800C	(4)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR2_800D	(5)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR2_800E	(6)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR2_1066E	(7)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR2_1066F	(8)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_800D	(9)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_800E	(10)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1066E	(11)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1066F	(12)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1066G	(13)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1333F	(14)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1333G	(15)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1333H	(16)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1333J	(17)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1600G	(18)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1600H	(19)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1600J	(20)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1600K	(21)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1866J	(22)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1866K	(23)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1866L	(24)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1866M	(25)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_2133K	(26)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_2133L	(27)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_2133M	(28)
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_2133N	(29)

#define CHAL_MEMC_DDR3_JEDEC_TYPE_MAX		CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_2133N	/* This is different */
#define CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_MIN	CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_800D
#define CHAL_MEMC_DDR3_JEDEC_TYPE_IS_DDR3(t)	((t) >= CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_MIN)

/* DDR3- MR0 - Defintions from JEDEC STD 79-3D */
#define DDR3_MR0_BL_SHIFT		(0)	/* Burst Length */
#define DDR3_MR0_BL_MASK		(0x0003)
#define DDR3_MR0_CL_SHIFT		(2)	/* CAS Latency */
#define DDR3_MR0_CL_MASK		(0x0074)
#define DDR3_MR0_RBT_SHIFT		(3)	/* Read Burst Type */
#define DDR3_MR0_RBT_MASK		(0x0008)
#define DDR3_MR0_TM_SHIFT		(7)	/* Test Mode */
#define DDR3_MR0_TM_MASK		(0x0080)
#define DDR3_MR0_DLL_SHIFT		(8)	/* DLL Reset */
#define DDR3_MR0_DLL_MASK		(0x0100)
#define DDR3_MR0_WR_SHIFT		(9)	/* Write Recovery */
#define DDR3_MR0_WR_MASK		(0x0e00)
#define DDR3_MR0_PPD_SHIFT		(12)	/* Precharge PD DLL */
#define DDR3_MR0_PPD_MASK		(0x1000)

#define DDR3_MR0_BL_BL8_FIXED		(0 << DDR3_MR0_BL_SHIFT)
#define DDR3_MR0_BL_BC4_BL8_ONFLY	(1 << DDR3_MR0_BL_SHIFT)
#define DDR3_MR0_BL_BC4_FIXED		(2 << DDR3_MR0_BL_SHIFT)

#define DDR3_MR0_PDD_SLOW_EXIT		(0 << DDR3_MR0_PPD_SHIFT)
#define DDR3_MR0_PDD_FAST_EXIT		(1 << DDR3_MR0_PPD_SHIFT)

/* DDR3- MR1 - Defintions from JEDEC STD 79-3D */
#define DDR3_MR1_RTT_NOM_0_SHIFT	(2)
#define DDR3_MR1_RTT_NOM_0_MASK		(1 << DDR3_MR1_RTT_NOM_0_SHIFT)
#define DDR3_MR1_RTT_NOM_1_SHIFT	(6)
#define DDR3_MR1_RTT_NOM_1_MASK		(1 << DDR3_MR1_RTT_NOM_1_SHIFT)
#define DDR3_MR1_RTT_NOM_2_SHIFT	(9)
#define DDR3_MR1_RTT_NOM_2_MASK		(1 << DDR3_MR1_RTT_NOM_2_SHIFT)
#define DDR3_MR1_RTT_NOM_MASK		(DDR3_MR1_RTT_NOM_0_MASK | DDR3_MR1_RTT_NOM_1_MASK | DDR3_MR1_RTT_NOM_2_MASK)
#define DDR3_MR1_RTT_NOM_VALUE_GET(r)	( ((((r) & DDR3_MR1_RTT_NOM_0_MASK) >> DDR3_MR1_RTT_NOM_0_SHIFT) << 0) | \
					  ((((r) & DDR3_MR1_RTT_NOM_1_MASK) >> DDR3_MR1_RTT_NOM_1_SHIFT) << 1) | \
					  ((((r) & DDR3_MR1_RTT_NOM_2_MASK) >> DDR3_MR1_RTT_NOM_2_SHIFT) << 2) \
					)
#define DDR3_MR1_RTT_NOM_VALUE_SET(v)	( ((((v) & 0x1) >> 0) << DDR3_MR1_RTT_NOM_0_SHIFT) | \
					  ((((v) & 0x2) >> 1) << DDR3_MR1_RTT_NOM_1_SHIFT) | \
					  ((((v) & 0x4) >> 2) << DDR3_MR1_RTT_NOM_2_SHIFT) \
					)
#define DDR3_MR1_RTT_NOM_DISABLED	DDR3_MR1_RTT_NOM_VALUE_SET(0)
#define DDR3_MR1_RTT_NOM_RZQ_DIV4	DDR3_MR1_RTT_NOM_VALUE_SET(1)
#define DDR3_MR1_RTT_NOM_RZQ_DIV2	DDR3_MR1_RTT_NOM_VALUE_SET(2)
#define DDR3_MR1_RTT_NOM_RZQ_DIV6	DDR3_MR1_RTT_NOM_VALUE_SET(3)
#define DDR3_MR1_RTT_NOM_RZQ_DIV12	DDR3_MR1_RTT_NOM_VALUE_SET(4)
#define DDR3_MR1_RTT_NOM_RZQ_DIV8	DDR3_MR1_RTT_NOM_VALUE_SET(5)

/* DDR3- MR2 - Defintions from JEDEC STD 79-3D */
#define DDR3_MR2_RTT_WR_SHIFT		(9)
#define DDR3_MR2_RTT_WR_MASK		(0x3 << DDR3_MR2_RTT_WR_SHIFT)
#define DDR3_MR2_RTT_WR_OFF		(0x0 << DDR3_MR2_RTT_WR_SHIFT)
#define DDR3_MR2_RTT_WR_RZQ_DIV4	(0x1 << DDR3_MR2_RTT_WR_SHIFT)
#define DDR3_MR2_RTT_WR_RZQ_DIV2	(0x2 << DDR3_MR2_RTT_WR_SHIFT)

#define CHAL_MEMC_DDR3_AD_WIDTH_13B	(0)
#define CHAL_MEMC_DDR3_AD_WIDTH_14B	(1)
#define CHAL_MEMC_DDR3_AD_WIDTH_15B	(2)
#define CHAL_MEMC_DDR3_AD_WIDTH_16B	(3)

#define CHAL_MEMC_DDR3_RANK_SINGLE	(0)
#define CHAL_MEMC_DDR3_RANK_DUAL	(1)

#define CHAL_MEMC_DDR3_BUS_WIDTH_32	(0)	/* BUS16 = 0, BUS8 = 0 */
#define CHAL_MEMC_DDR3_BUS_WIDTH_8	(1)	/* BUS16 = X, BUS8 = 1 */
#define CHAL_MEMC_DDR3_BUS_WIDTH_16	(2)	/* BUS16 = 1, BUS8 = 0 */

#define CHAL_MEMC_DDR3_CHIP_WIDTH_8	(0)
#define CHAL_MEMC_DDR3_CHIP_WIDTH_16	(1)

#define CHAL_MEMC_DDR3_VDDQ_1P35V	(0)
#define CHAL_MEMC_DDR3_VDDQ_1P50V	(1)
#define CHAL_MEMC_DDR3_VDDQ_1P80V	(2)

/*
* SYS_EMI_DDR3_PHY_ADDR_CTL (0x35008800) : STANDBY_CONTROL (0xA4) : LDO_VOLTS [8:7]
*/
#define CHAL_MEMC_DDR3_LDO_1P80V	(0)
#define CHAL_MEMC_DDR3_LDO_RESERVED	(1)
#define CHAL_MEMC_DDR3_LDO_1P50V	(2)
#define CHAL_MEMC_DDR3_LDO_1P35V	(3)

#define CHAL_MEMC_DDR3_CHIP_SIZE_4Gb	(0)
#define CHAL_MEMC_DDR3_CHIP_SIZE_2Gb	(1)
#define CHAL_MEMC_DDR3_CHIP_SIZE_1Gb	(2)
#define CHAL_MEMC_DDR3_CHIP_SIZE_8Gb	(3)

/*----------- define the memory configuration types ------------------------*/

/****************************************
* Supported DDR3 configurations
****************************************/

/* Configuration for BCM911160SV and BCM911160Tablet board.
* Device: Micron: MT41J256M8HX-187E:D
* For ASIC at 400MHz (tCK 2.5ns), we need to use DDR3_800E PHY type.
* Note this is only valid for all parts down to 333MHz, at
* which point DDR3_800D is required.
*/
#if defined(CHAL_SDRAM_CONFIG_DDR3_2Gbx8_400MHz)
#define ddr3cfg_strap_override		(1)
#define ddr3cfg_jedec_type		CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_800E
#define ddr3cfg_chip_size		CHAL_MEMC_DDR3_CHIP_SIZE_2Gb
#define ddr3cfg_vddq			CHAL_MEMC_DDR3_VDDQ_1P50V
#define ddr3cfg_chip_width		CHAL_MEMC_DDR3_CHIP_WIDTH_8
#define ddr3cfg_bus_width		CHAL_MEMC_DDR3_BUS_WIDTH_32
#define ddr3cfg_rank			CHAL_MEMC_DDR3_RANK_SINGLE
#define ddr3cfg_ad_width		CHAL_MEMC_DDR3_AD_WIDTH_15B
#define ddr3cfg_clock_hz		(400000000)
#define ddr3cfg_odt_phy_enabled		(1)	/* Only 0 or 1 */
#define ddr3cfg_odt_sdram_mr1		DDR3_MR1_RTT_NOM_DISABLED
#define ddr3cfg_odt_sdram_mr2		DDR3_MR2_RTT_WR_OFF
#define ddr3cfg_ssc_percent		(0)
#define ddr3cfg_ssc_freq_hz		(20000)
#define ddr3cfg_virtual_vtt_enabled	(0)	/* Only 0 or 1 */
#endif

/* Configuration for Woodstock board.
*/
#if defined(CHAL_SDRAM_CONFIG_DDR3_1Gbx16_400MHz)
#define ddr3cfg_strap_override		(1)
#define ddr3cfg_jedec_type		CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_800E
#define ddr3cfg_chip_size		CHAL_MEMC_DDR3_CHIP_SIZE_1Gb
#define ddr3cfg_vddq			CHAL_MEMC_DDR3_VDDQ_1P50V
#define ddr3cfg_chip_width		CHAL_MEMC_DDR3_CHIP_WIDTH_16
#define ddr3cfg_bus_width		CHAL_MEMC_DDR3_BUS_WIDTH_32
#define ddr3cfg_rank			CHAL_MEMC_DDR3_RANK_SINGLE
#define ddr3cfg_ad_width		CHAL_MEMC_DDR3_AD_WIDTH_13B
#define ddr3cfg_clock_hz		(400000000)
#define ddr3cfg_odt_phy_enabled		(1)	/* Only 0 or 1 */
#define ddr3cfg_odt_sdram_mr1		DDR3_MR1_RTT_NOM_DISABLED
#define ddr3cfg_odt_sdram_mr2		DDR3_MR2_RTT_WR_OFF
#define ddr3cfg_ssc_percent		(0)
#define ddr3cfg_ssc_freq_hz		(20000)
#define ddr3cfg_virtual_vtt_enabled	(0)	/* Only 0 or 1 */
#endif

/* Configuration for BCM9CHIPIT_4DDR3x8_EDC.
* Device: Micron: MT41J256M8HX-187E:D
* The clock speed is hardcoded in the FPGA image and not configurable via software
* Note that for FPGA, things are being run with DLL disabled.
* JEDEC only mandates that CL=6 CWL=6 need to be supported in this
* mode. This corresponds to DDR3_1066E in the PHY types.
*/
#if defined(CHAL_SDRAM_CONFIG_DDR3_BCM9CHIPIT_4DDR3x8_EDC)
#define ddr3cfg_strap_override		(1)
#define ddr3cfg_jedec_type		CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1066E
#define ddr3cfg_chip_size		CHAL_MEMC_DDR3_CHIP_SIZE_2Gb
#define ddr3cfg_vddq			CHAL_MEMC_DDR3_VDDQ_1P50V
#define ddr3cfg_chip_width		CHAL_MEMC_DDR3_CHIP_WIDTH_8
#define ddr3cfg_bus_width		CHAL_MEMC_DDR3_BUS_WIDTH_32
#define ddr3cfg_rank			CHAL_MEMC_DDR3_RANK_SINGLE
#define ddr3cfg_ad_width		CHAL_MEMC_DDR3_AD_WIDTH_15B
#define ddr3cfg_clock_hz		(13500000)
#define ddr3cfg_odt_phy_enabled		(1)	/* Only 0 or 1 */
#define ddr3cfg_odt_sdram_mr1		DDR3_MR1_RTT_NOM_DISABLED
#define ddr3cfg_odt_sdram_mr2		DDR3_MR2_RTT_WR_OFF
#define ddr3cfg_ssc_percent		(0)
#define ddr3cfg_ssc_freq_hz		(20000)
#define ddr3cfg_virtual_vtt_enabled	(0)	/* Only 0 or 1 */
#endif

/* Configuration for BCM9CHIPIT_4DDR3x8_EDC (Twin Die).
* Device: Micron: MT41J512M8THD-187E:D
* The clock speed is hardcoded in the FPGA image and not configurable via software
*/
#if defined(CHAL_SDRAM_CONFIG_DDR3_BCM9CHIPIT_4DDR3x8_TWIN_EDC)
#define ddr3cfg_strap_override		(1)
#define ddr3cfg_jedec_type		CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1066E
#define ddr3cfg_chip_size		CHAL_MEMC_DDR3_CHIP_SIZE_2Gb
#define ddr3cfg_vddq			CHAL_MEMC_DDR3_VDDQ_1P50V
#define ddr3cfg_chip_width		CHAL_MEMC_DDR3_CHIP_WIDTH_8
#define ddr3cfg_bus_width		CHAL_MEMC_DDR3_BUS_WIDTH_32
#define ddr3cfg_rank			CHAL_MEMC_DDR3_RANK_DUAL
#define ddr3cfg_ad_width		CHAL_MEMC_DDR3_AD_WIDTH_15B
#define ddr3cfg_clock_hz		(13500000)
#define ddr3cfg_odt_phy_enabled		(1)	/* Only 0 or 1 */
#define ddr3cfg_odt_sdram_mr1		DDR3_MR1_RTT_NOM_DISABLED
#define ddr3cfg_odt_sdram_mr2		DDR3_MR2_RTT_WR_OFF
#define ddr3cfg_ssc_percent		(0)
#define ddr3cfg_ssc_freq_hz		(20000)
#define ddr3cfg_virtual_vtt_enabled	(0)	/* Only 0 or 1 */
#endif

/* Configuration for BCM9CHIPIT_DDR3x16_EDC.
* Device: Micron: MT41J128M16HA-15E:D
* The clock speed is hardcoded in the FPGA image and not configurable via software
*/
#if defined(CHAL_SDRAM_CONFIG_DDR3_BCM9CHIPIT_DDR3x16_EDC)
#define ddr3cfg_strap_override		(1)
#define ddr3cfg_jedec_type		CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_1066E
#define ddr3cfg_chip_size		CHAL_MEMC_DDR3_CHIP_SIZE_2Gb
#define ddr3cfg_vddq			CHAL_MEMC_DDR3_VDDQ_1P50V
#define ddr3cfg_chip_width		CHAL_MEMC_DDR3_CHIP_WIDTH_16
#define ddr3cfg_bus_width		CHAL_MEMC_DDR3_BUS_WIDTH_32
#define ddr3cfg_rank			CHAL_MEMC_DDR3_RANK_DUAL
#define ddr3cfg_ad_width		CHAL_MEMC_DDR3_AD_WIDTH_14B
#define ddr3cfg_clock_hz		(13500000)
#define ddr3cfg_odt_phy_enabled		(1)	/* Only 0 or 1 */
#define ddr3cfg_odt_sdram_mr1		DDR3_MR1_RTT_NOM_DISABLED
#define ddr3cfg_odt_sdram_mr2		DDR3_MR2_RTT_WR_OFF
#define ddr3cfg_ssc_percent		(0)
#define ddr3cfg_ssc_freq_hz		(20000)
#define ddr3cfg_virtual_vtt_enabled	(0)	/* Only 0 or 1 */
#endif

/* Configuration for BCM911130_CPU board with the BCM9_RAY_CAPRI
* Device: Micron: MT41K128M16HA-15E:D
*/
#if defined(CHAL_SDRAM_CONFIG_DDR3L_MICRON_MT41K128M16HA_2Gbx16_400MHz)
#define ddr3cfg_strap_override		(1)
#define ddr3cfg_jedec_type		CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_800E
#define ddr3cfg_chip_size		CHAL_MEMC_DDR3_CHIP_SIZE_2Gb
	/* ***** Warning: The original voltage define for this chip is 1v35 */
	/* #define ddr3cfg_vddq                 CHAL_MEMC_DDR3_VDDQ_1P35V */
#define ddr3cfg_vddq			CHAL_MEMC_DDR3_VDDQ_1P50V
#define ddr3cfg_chip_width		CHAL_MEMC_DDR3_CHIP_WIDTH_16
#define ddr3cfg_bus_width		CHAL_MEMC_DDR3_BUS_WIDTH_32
#define ddr3cfg_rank			CHAL_MEMC_DDR3_RANK_SINGLE
#define ddr3cfg_ad_width		CHAL_MEMC_DDR3_AD_WIDTH_14B
#define ddr3cfg_clock_hz		(400000000)
#define ddr3cfg_odt_phy_enabled		(0)	/* Only 0 or 1 */
#define ddr3cfg_odt_sdram_mr1		DDR3_MR1_RTT_NOM_DISABLED
#define ddr3cfg_odt_sdram_mr2		DDR3_MR2_RTT_WR_OFF
#define ddr3cfg_ssc_percent		(0)
#define ddr3cfg_ssc_freq_hz		(20000)
#define ddr3cfg_virtual_vtt_enabled	(1)	/* Only 0 or 1 */
#endif

/* Configuration for BCM9CAPRI_CPU14DC board with the BCM9_RAY_CAPRI
* Device: Micron: MT41K256M8HX-15E
*/
#if defined(CHAL_SDRAM_CONFIG_DDR3L_MICRON_MT41K256M8HX_2Gbx8_400MHz)
#define ddr3cfg_strap_override		(1)
#define ddr3cfg_jedec_type		CHAL_MEMC_DDR3_JEDEC_TYPE_DDR3_800E
#define ddr3cfg_chip_size		CHAL_MEMC_DDR3_CHIP_SIZE_2Gb
#define ddr3cfg_vddq			CHAL_MEMC_DDR3_VDDQ_1P35V
#define ddr3cfg_chip_width		CHAL_MEMC_DDR3_CHIP_WIDTH_8
#define ddr3cfg_bus_width		CHAL_MEMC_DDR3_BUS_WIDTH_32
#define ddr3cfg_rank			CHAL_MEMC_DDR3_RANK_SINGLE
#define ddr3cfg_ad_width		CHAL_MEMC_DDR3_AD_WIDTH_15B
#define ddr3cfg_clock_hz		(400000000)
#define ddr3cfg_odt_phy_enabled		(0)	/* Only 0 or 1 */
#define ddr3cfg_odt_sdram_mr1		DDR3_MR1_RTT_NOM_DISABLED
#define ddr3cfg_odt_sdram_mr2		DDR3_MR2_RTT_WR_OFF
#define ddr3cfg_ssc_percent		(0)
#define ddr3cfg_ssc_freq_hz		(20000)
#define ddr3cfg_virtual_vtt_enabled	(1)	/* Only 0 or 1 */
#endif

#endif /* _CHAL_SDRAM_CONFIGS_H_ */
