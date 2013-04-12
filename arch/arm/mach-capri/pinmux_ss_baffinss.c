/************************************************************************************************/
/*                                                                                              */
/*  Copyright 2011  Broadcom Corporation                                                        */
/*                                                                                              */
/*     Unless you and Broadcom execute a separate written software license agreement governing  */
/*     use of this software, this software is licensed to you under the terms of the GNU        */
/*     General Public License version 2 (the GPL), available at                                 */
/*                                                                                              */
/*          http://www.broadcom.com/licenses/GPLv2.php                                          */
/*                                                                                              */
/*     with the following added to such license:                                                */
/*                                                                                              */
/*     As a special exception, the copyright holders of this software give you permission to    */
/*     link this software with independent modules, and to copy and distribute the resulting    */
/*     executable under terms of your choice, provided that you also meet, for each linked      */
/*     independent module, the terms and conditions of the license of that module.              */
/*     An independent module is a module which is not derived from this software.  The special  */
/*     exception does not apply to any modifications of the software.                           */
/*                                                                                              */
/*     Notwithstanding the above, under no circumstances may you combine this software in any   */
/*     way with any other Broadcom software provided under a license other than the GPL,        */
/*     without Broadcom's express prior written consent.                                        */
/*                                                                                              */
/************************************************************************************************/

#include <mach/pinmux.h>
#include <linux/kernel.h>

#include "pinmux_common.c"

/*************************************************************************  
 *             This is copied from Capri Stone definitions.                *
 *             Need to clean up and verify once Capri SS              *
 *             is available.                                             *
 *************************************************************************/

/* Board Specific Pin Groups */

/* Primary Groups */
static struct pin_config clockout_config[] = {
	PIN_CFG(CLKREQ_IN_0, CLKREQ_IN_0, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(CLKOUT_0, CLKOUT_0, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(CLKOUT_1, VC_GPCLK_2, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(CLKOUT_2, VC_GPCLK_0, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(CLKOUT_3, VC_GPCLK_1, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(CWS_SYS_REQ1, GPIO_183, 1, ON, OFF, 0, 0, 8MA),
};

static struct pin_config cam_flash_config[] = {
	PIN_CFG(SSP3_FS, VC_I2S_WSIO, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(SSP3_RXD, VC_I2S_SDI, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP3_TXD, VC_I2S_SDO, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(SSP_EXTCLK, VC_CPG1, 0, OFF, OFF, 1, 1, 6MA),
};

/* Individual Pins */
static struct pin_config ungrouped_pins_config[] = {
	PIN_RGMII_CFG(RGMII_GPIO_0, RGMII_GPIO_0, 0x0),
	PIN_RGMII_CFG(RGMII_GPIO_1, RGMII_GPIO_1, 0x0),
	PIN_RGMII_CFG(RGMII_GPIO_2, RGMII_GPIO_2, 0x0),
	PIN_RGMII_CFG(RGMII_GPIO_3, RGMII_GPIO_3, 0x0),
	PIN_CFG(NAND_CLE, GPIO_048, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(KP_COL_IP_3, GPIO_064, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP2_FS_0, GPIO_095, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(SSP2_CLK, GPIO_099, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP2_TXD_0, GPIO_101, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_7, GPIO_138, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_3, GPIO_142, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_2, GPIO_143, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_1, GPIO_144, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_0, GPIO_145, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(GPIO12, GPIO_012, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(GPIO13, GPIO_013, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(CLKREQ_IN_1, PWM_O_0, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(MTX_SCAN_CLK, GPIO_175, 1, ON, OFF, 0, 0, 8MA),
};

/* Secondary Groups */
static struct pin_config sim2_config[] = {
	PIN_CFG(SIM2_RESETN, SIM2_RESETN, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SIM2_CLK, SIM2_CLK, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SIM2_DATA, SIM2_DATA, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SIM2_DET, SIM2_DET, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config arm3_i2c_config[] = {
	PIN_BSC_CFG(VC_CAM3_SCL, BSC3_SCL, 0x20),
	PIN_BSC_CFG(VC_CAM3_SDA, BSC3_SDA, 0x20),
};

static struct pin_config ssp2_config[] = {
	PIN_CFG(SSP2_FS_0, SSP2_FS_0, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP2_FS_1, SSP2_FS_1, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP2_FS_2, SSP2_FS_2, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP2_FS_3, SSP2_FS_3, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP2_CLK, SSP2_CLK, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP2_RXD_0, SSP2_RXD_0, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP2_TXD_0, SSP2_TXD_0, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP2_RXD_1, SSP2_RXD_1, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP2_TXD_1, SSP2_TXD_1, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config ssp3_2_config[] = {
	PIN_CFG(GPIO08, SSP3_FS, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(GPIO09, SSP3_CLK, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(GPIO10, SSP3_RXD, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(GPIO11, SSP3_TXD, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config hsi_config[] = {
	PIN_CFG(SSP4_FS, ACDATA, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP4_CLK, ACFLAG, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP4_RXD, ACREADY, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP4_TXD, ACWAKE, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP5_FS, CADATA, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP5_CLK, CAFLAG, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP5_RXD, CAREADY, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP5_TXD, CAWAKE, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config pm_debug_config[] = {
	PIN_CFG(SSP6_FS, PM_DEBUG0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP6_CLK, PM_DEBUG1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP6_RXD, PM_DEBUG2, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP6_TXD, PM_DEBUG3, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config mphi_config[] = {
	PIN_CFG(MPHI_DATA_10, MPHI_DATA_10, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_RUN0, MPHI_RUN0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_9, MPHI_DATA_9, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HCE0_N, MPHI_HCE0_N, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HWR_N, MPHI_HWR_N, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HRD_N, MPHI_HRD_N, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HA0, MPHI_HA0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HAT0, MPHI_HAT0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_8, MPHI_DATA_8, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_15, MPHI_DATA_15, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_14, MPHI_DATA_14, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_13, MPHI_DATA_13, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_12, MPHI_DATA_12, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_11, MPHI_DATA_11, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_RUN1, MPHI_RUN1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HCE1_N, MPHI_HCE1_N, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HAT1, MPHI_HAT1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_7, MPHI_DATA_7, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_6, MPHI_DATA_6, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_5, MPHI_DATA_5, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_4, MPHI_DATA_4, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_3, MPHI_DATA_3, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_2, MPHI_DATA_2, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_1, MPHI_DATA_1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_0, MPHI_DATA_0, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config avein_config[] = {
	PIN_CFG(LCD_G_7, AVEIN_VID15, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_6, AVEIN_VID14, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_5, AVEIN_VID13, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_4, AVEIN_VID12, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_3, AVEIN_VID11, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_2, AVEIN_VID10, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_1, AVEIN_VID9, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_0, AVEIN_VID8, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_7, AVEIN_VID7, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_6, AVEIN_VID6, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_5, AVEIN_VID5, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_4, AVEIN_VID4, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_HSYNC, AVEIN_HSYNC, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_VSYNC, AVEIN_VSYNC, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_OE, AVEIN_DSYNC, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_PCLK, AVEIN_VCLK, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config uart5_config[] = {
	PIN_CFG(LCD_R_3, LCD_R_3, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_2, LCD_R_2, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_1, LCD_R_1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_0, LCD_R_0, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config uart6_config[] = {
	PIN_CFG(LCD_R_7, LCD_R_7, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_6, LCD_R_6, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_5, LCD_R_5, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_4, LCD_R_4, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config mtx_scan_config[] = {
	PIN_CFG(MTX_SCAN_DATA, MTX_SCAN_DATA, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MTX_SCAN_CLK, MTX_SCAN_CLK, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config ic_usb_config[] = {
	PIN_CFG(IC_DP, IC_DP, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(IC_DM, IC_DM, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config pmu2_config[] = {
	PIN_CFG(STAT_1, STAT_1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(STAT_2, STAT_2, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(CLASSGPWR, CLASSGPWR, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config cir2_config[] = {
	PIN_CFG(GPIO02, CIR_TX, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(GPIO03, CIR_RX, 1, ON, OFF, 0, 0, 8MA),
};

static struct pin_config usb_host2_config[] = {
	PIN_CFG(GPIO00, USB1PFT, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(GPIO04, USB1PON, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config cam_pwm_config[] = {
	PIN_CFG(GPIO06, PWM_O_2, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(GPIO07, PWM_O_3, 0, OFF, OFF, 1, 1, 8MA),
};

/* Tertiary Groups */
static struct pin_config debug_bus_config[] = {
	PIN_CFG(NAND_ALE, DEBUG_BUS10, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(NAND_OEN, DEBUG_BUS09, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(NAND_WEN, DEBUG_BUS08, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(NAND_AD_7, DEBUG_BUS07, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(NAND_AD_6, DEBUG_BUS06, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(NAND_AD_5, DEBUG_BUS05, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(NAND_AD_4, DEBUG_BUS04, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(NAND_AD_3, DEBUG_BUS03, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(NAND_AD_2, DEBUG_BUS02, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(NAND_AD_1, DEBUG_BUS01, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(NAND_AD_0, DEBUG_BUS00, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config keypad_8x8_config[] = {
	PIN_CFG(KP_ROW_OP_0, KP_ROW_OP_0, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(KP_ROW_OP_1, KP_ROW_OP_1, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(KP_ROW_OP_2, KP_ROW_OP_2, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(KP_ROW_OP_3, KP_ROW_OP_3, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(KP_COL_IP_0, KP_COL_IP_0, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(KP_COL_IP_1, KP_COL_IP_1, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(KP_COL_IP_2, KP_COL_IP_2, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(KP_COL_IP_3, KP_COL_IP_3, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(UARTB3_URTS, KP_ROW_OP_6, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(UARTB3_UCTS, KP_COL_IP_6, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(UARTB3_URXD, KP_COL_IP_7, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(UARTB3_UTXD, KP_ROW_OP_7, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(SSP2_FS_2, KP_ROW_OP_4, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(SSP2_FS_3, KP_ROW_OP_5, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(SSP2_RXD_1, KP_COL_IP_4, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(SSP2_TXD_1, KP_COL_IP_5, 1, OFF, ON, 0, 0, 8MA),
};

static struct pin_config clkmon_config[] = {
	PIN_CFG(SIM_DET, CLKMON, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config sdio3_config[] = {
	PIN_CFG(MPHI_RUN1, SDIO3_CMD, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(MPHI_HCE1_N, SDIO3_RST_N, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(MPHI_HAT1, SDIO3_CLK, 0, OFF, OFF, 0, 0, 12MA),
	PIN_CFG(MPHI_DATA_7, SDIO3_DATA_7, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(MPHI_DATA_6, SDIO3_DATA_6, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(MPHI_DATA_5, SDIO3_DATA_5, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(MPHI_DATA_4, SDIO3_DATA_4, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(MPHI_DATA_3, SDIO3_DATA_3, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(MPHI_DATA_2, SDIO3_DATA_2, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(MPHI_DATA_1, SDIO3_DATA_1, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(MPHI_DATA_0, SDIO3_DATA_0, 0, OFF, ON, 0, 0, 12MA),
};

static struct pin_config eav_config[] = {
	PIN_CFG(MPHI_DATA_10, EAV_SD_0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_RUN0, EAV_WS_0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HCE0_N, EAV_BITCLK_0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HWR_N, EAV_BITCLK_1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HRD_N, EAV_SD_1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HA0, EAV_WS_1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HAT0, EAV_DTE_LTS, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_8, EAV_GPIO_LTS, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_9, MSTR_CLK_0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_15, MSTR_CLK_1, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config uart2_2_config[] = {
	PIN_CFG(MPHI_DATA_14, UARTB2_URXD, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_13, UARTB2_UTXD, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(MPHI_DATA_12, UARTB2_URTS, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(MPHI_DATA_11, UARTB2_UCTS, 1, OFF, ON, 0, 0, 8MA),
};

static struct pin_config pti_1_config[] = {
	PIN_CFG(TRACECLK, PTI_CLK, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(TRACEDT15, UARTB2_URXD, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(TRACEDT03, PTI_DAT3, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(TRACEDT02, PTI_DAT2, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(TRACEDT01, PTI_DAT1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(TRACEDT00, PTI_DAT0, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config pti_2_config[] = {
	PIN_CFG(SDIO4_CLK, PTI_CLK, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SDIO4_CMD, UARTB2_URXD, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SDIO4_DATA_3, PTI_DAT3, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SDIO4_DATA_2, PTI_DAT2, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SDIO4_DATA_1, PTI_DAT1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SDIO4_DATA_0, PTI_DAT0, 0, OFF, OFF, 0, 0, 8MA),
};

/* Complete List of Pin Groups for this Board */

const struct pin_group pin_config_list[] = {

	/* Primary Pin Groups */
	PIN_GROUP_CFG(PG_MODEM, modem_config, ARRAY_LEN(modem_config)),
	PIN_GROUP_CFG(PG_RF, rf_config, ARRAY_LEN(rf_config)),
	PIN_GROUP_CFG(PG_ETMTRACE, etm_trace_config,
		      ARRAY_LEN(etm_trace_config)),
	PIN_GROUP_CFG(PG_SDIO2, sdio2_config, ARRAY_LEN(sdio2_config)),
	PIN_GROUP_CFG(PG_SIM1, sim1_config, ARRAY_LEN(sim1_config)),
	PIN_GROUP_CFG(PG_KEYPAD_2X3, keypad_2x3_config,
		      ARRAY_LEN(keypad_2x3_config)),
	PIN_GROUP_CFG(PG_PMU_I2C, pmu_i2c_config, ARRAY_LEN(pmu_i2c_config)),
	PIN_GROUP_CFG(PG_CAM1_I2C, cam1_i2c_config, ARRAY_LEN(cam1_i2c_config)),
	PIN_GROUP_CFG(PG_CAM2_I2C, cam2_i2c_config, ARRAY_LEN(cam2_i2c_config)),
	PIN_GROUP_CFG(PG_CAM3_I2C, cam3_i2c_config, ARRAY_LEN(cam3_i2c_config)),
	PIN_GROUP_CFG(PG_HDMI_I2C, hdmi_i2c_config, ARRAY_LEN(hdmi_i2c_config)),
	PIN_GROUP_CFG(PG_ARM1_I2C, arm1_i2c_config, ARRAY_LEN(arm1_i2c_config)),
	PIN_GROUP_CFG(PG_ARM2_I2C, arm2_i2c_config, ARRAY_LEN(arm2_i2c_config)),
	PIN_GROUP_CFG(PG_UART1, uart1_config, ARRAY_LEN(uart1_config)),
	PIN_GROUP_CFG(PG_UART2, uart2_config, ARRAY_LEN(uart2_config)),
	PIN_GROUP_CFG(PG_UART3, uart3_config, ARRAY_LEN(uart3_config)),
	PIN_GROUP_CFG(PG_UART4, uart4_config, ARRAY_LEN(uart4_config)),
	PIN_GROUP_CFG(PG_SDIO1, sdio1_config, ARRAY_LEN(sdio1_config)),
	PIN_GROUP_CFG(PG_SDIO4, sdio4_config, ARRAY_LEN(sdio4_config)),
	PIN_GROUP_CFG(PG_SSP4, ssp4_config, ARRAY_LEN(ssp4_config)),
	PIN_GROUP_CFG(PG_SSP5, ssp5_config, ARRAY_LEN(ssp5_config)),
	PIN_GROUP_CFG(PG_SSP6, ssp6_config, ARRAY_LEN(ssp6_config)),
	PIN_GROUP_CFG(PG_CAM1, cam1_config, ARRAY_LEN(cam1_config)),
	PIN_GROUP_CFG(PG_CAM2, cam2_config, ARRAY_LEN(cam2_config)),
	PIN_GROUP_CFG(PG_CAM3, cam3_config, ARRAY_LEN(cam3_config)),
	PIN_GROUP_CFG(PG_CIR, cir_config, ARRAY_LEN(cir_config)),
	PIN_GROUP_CFG(PG_LCD, lcd_config, ARRAY_LEN(lcd_config)),
	PIN_GROUP_CFG(PG_CLOCKOUT, clockout_config, ARRAY_LEN(clockout_config)),
	PIN_GROUP_CFG(PG_DIGIMIC, digital_mic_config,
		      ARRAY_LEN(digital_mic_config)),
	PIN_GROUP_CFG(PG_PMU1, pmu1_config, ARRAY_LEN(pmu1_config)),
	PIN_GROUP_CFG(PG_USB_HOST1, usb_host1_config,
		      ARRAY_LEN(usb_host1_config)),
	PIN_GROUP_CFG(PG_GPS, gps_config, ARRAY_LEN(gps_config)),
	PIN_GROUP_CFG(PG_BT, bt_config, ARRAY_LEN(bt_config)),
	PIN_GROUP_CFG(PG_WL, wl_config, ARRAY_LEN(wl_config)),
	PIN_GROUP_CFG(PG_CAM_FLASH, cam_flash_config,
		      ARRAY_LEN(cam_flash_config)),
	PIN_GROUP_CFG(PG_NFC, nfc_config, ARRAY_LEN(nfc_config)),
	PIN_GROUP_CFG(PG_USB_HSIC, usb_hsic_config, ARRAY_LEN(usb_hsic_config)),
	PIN_GROUP_CFG(PG_STATUS_LED, status_led_config,
		      ARRAY_LEN(status_led_config)),
	PIN_GROUP_CFG(PG_TSC, tsc_config, ARRAY_LEN(tsc_config)),

	/* Secondary Pin Groups */
	PIN_GROUP_CFG(PG_SIM2, sim2_config, ARRAY_LEN(sim2_config)),
	PIN_GROUP_CFG(PG_ARM3_I2C, arm3_i2c_config, ARRAY_LEN(arm3_i2c_config)),
	PIN_GROUP_CFG(PG_SSP0, ssp0_config, ARRAY_LEN(ssp0_config)),
	PIN_GROUP_CFG(PG_SSP2, ssp2_config, ARRAY_LEN(ssp2_config)),
	PIN_GROUP_CFG(PG_SSP3_1, ssp3_1_config, ARRAY_LEN(ssp3_1_config)),
	PIN_GROUP_CFG(PG_SSP3_2, ssp3_2_config, ARRAY_LEN(ssp3_2_config)),
	PIN_GROUP_CFG(PG_HSI, hsi_config, ARRAY_LEN(hsi_config)),
	PIN_GROUP_CFG(PG_PM_DEBUG, pm_debug_config, ARRAY_LEN(pm_debug_config)),
	PIN_GROUP_CFG(PG_MPHI, mphi_config, ARRAY_LEN(mphi_config)),
	PIN_GROUP_CFG(PG_AVEIN, avein_config, ARRAY_LEN(avein_config)),
	PIN_GROUP_CFG(PG_UART5, uart5_config, ARRAY_LEN(uart5_config)),
	PIN_GROUP_CFG(PG_UART6, uart6_config, ARRAY_LEN(uart6_config)),
	PIN_GROUP_CFG(PG_MTX_SCAN, mtx_scan_config, ARRAY_LEN(mtx_scan_config)),
	PIN_GROUP_CFG(PG_IC_USB, ic_usb_config, ARRAY_LEN(ic_usb_config)),
	PIN_GROUP_CFG(PG_PMU2, pmu2_config, ARRAY_LEN(pmu2_config)),
	PIN_GROUP_CFG(PG_CIR2, cir2_config, ARRAY_LEN(cir2_config)),
	PIN_GROUP_CFG(PG_USB_HOST2, usb_host2_config,
		      ARRAY_LEN(usb_host2_config)),
	PIN_GROUP_CFG(PG_CAM_PWM, cam_pwm_config, ARRAY_LEN(cam_pwm_config)),
	PIN_GROUP_CFG(PG_BATT_CHRG, battery_charging_config,
		      ARRAY_LEN(battery_charging_config)),

	/* Tertiary Pin Groups */
	PIN_GROUP_CFG(PG_DEBUG_BUS, debug_bus_config,
		      ARRAY_LEN(debug_bus_config)),
	PIN_GROUP_CFG(PG_KEYPAD_8X8, keypad_8x8_config,
		      ARRAY_LEN(keypad_8x8_config)),
	PIN_GROUP_CFG(PG_CLK_MON, clkmon_config, ARRAY_LEN(clkmon_config)),
	PIN_GROUP_CFG(PG_VC4_JTAG, vc4_jtag_config, ARRAY_LEN(vc4_jtag_config)),
	PIN_GROUP_CFG(PG_SDIO3, sdio3_config, ARRAY_LEN(sdio3_config)),
	PIN_GROUP_CFG(PG_EAV, eav_config, ARRAY_LEN(eav_config)),
	PIN_GROUP_CFG(PG_UART2_2, uart2_2_config, ARRAY_LEN(uart2_2_config)),
	PIN_GROUP_CFG(PG_PTI1, pti_1_config, ARRAY_LEN(pti_1_config)),
	PIN_GROUP_CFG(PG_PTI2, pti_2_config, ARRAY_LEN(pti_2_config)),
};

/* board level init */
int __init pinmux_board_init(void)
{
	unsigned int i;

	/* Initialize Primary Pin Groups */
	pinmux_block_init(PG_MODEM);
	pinmux_block_init(PG_RF);
	pinmux_block_init(PG_ETMTRACE);
	pinmux_block_init(PG_SDIO2);
	pinmux_block_init(PG_SIM1);
	pinmux_block_init(PG_KEYPAD_2X3);
	pinmux_block_init(PG_PMU_I2C);
	pinmux_block_init(PG_CAM1_I2C);
	pinmux_block_init(PG_CAM2_I2C);
	pinmux_block_init(PG_CAM3_I2C);
	pinmux_block_init(PG_HDMI_I2C);
	pinmux_block_init(PG_ARM1_I2C);
	pinmux_block_init(PG_ARM2_I2C);
	pinmux_block_init(PG_UART1);
	pinmux_block_init(PG_UART2);
	pinmux_block_init(PG_UART3);
	pinmux_block_init(PG_UART4);
	pinmux_block_init(PG_SDIO1);
	pinmux_block_init(PG_SDIO4);
	pinmux_block_init(PG_SSP4);
	pinmux_block_init(PG_SSP5);
	pinmux_block_init(PG_SSP6);
	pinmux_block_init(PG_CAM1);
	pinmux_block_init(PG_CAM2);
	pinmux_block_init(PG_CAM3);
	pinmux_block_init(PG_CIR);
	pinmux_block_init(PG_LCD);
	pinmux_block_init(PG_CLOCKOUT);
	pinmux_block_init(PG_DIGIMIC);
	pinmux_block_init(PG_PMU1);
	pinmux_block_init(PG_USB_HOST1);
	pinmux_block_init(PG_GPS);
	pinmux_block_init(PG_BT);
	pinmux_block_init(PG_WL);
	pinmux_block_init(PG_CAM_FLASH);
	pinmux_block_init(PG_NFC);
	pinmux_block_init(PG_USB_HSIC);
	pinmux_block_init(PG_STATUS_LED);
	pinmux_block_init(PG_TSC);
	pinmux_block_init(PG_VC4_JTAG);

	/* Initialize All Other Primary Pins */
	for (i = 0; i < ARRAY_LEN(ungrouped_pins_config); i++)
		pinmux_set_pin_config(&ungrouped_pins_config[i]);

	return 0;
}

/* block level init */
int pinmux_block_init(int group_id)
{
	int i, len, index, rc;

	rc = 0;
	index = -1;
	len = ARRAY_LEN(pin_config_list);
	for (i = 0; i < len; i++) {
		if (pin_config_list[i].group_id == group_id) {
			index = i;
			break;
		}
	}

	if (index != -1) {
		struct pin_config *block_pin_config =
		    pin_config_list[index].listp;
		len = pin_config_list[index].num_pins;
		for (i = 0; i < len; i++)
			pinmux_set_pin_config(&block_pin_config[i]);
	} else {
		rc = 1;
	}

	return rc;
}
