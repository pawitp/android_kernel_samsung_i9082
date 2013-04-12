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

/* Common Pin Groups */

static struct pin_config modem_config[] = {
	PIN_CFG(MDMGPIO0, GPEN00, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(MDMGPIO1, GPEN01, 0, OFF, OFF, 1, 0, 8MA),
	PIN_CFG(MDMGPIO2, GPEN02, 0, OFF, OFF, 1, 0, 8MA),
	PIN_CFG(MDMGPIO3, GPEN03, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(MDMGPIO4, GPEN04, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(MDMGPIO5, GPEN05, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(MDMGPIO6, GPEN06, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(MDMGPIO7, GPEN07, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(MDMGPIO8, GPEN08, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(ADCSYNC, GPEN09, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(GPS_PABLANK, GPEN12, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(TXPWRIND, GPEN11, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(GPS_TMARK, GPEN10, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(GPEN13, GPEN13, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(GPEN14, GPEN14, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(GPEN15, GPEN15, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config rf_config[] = {
	PIN_CFG(SRI_C, SRI_C, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SRI_E, SRI_E, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SRI_D, SRI_D, 0, OFF, OFF, 1, 0, 8MA),
	PIN_CFG(RFST2G_MTSLOTEN3G, RFST2G_MTSLOTEN3G, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(TXDATA3G0, TXDATA3G0, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(RTXDATA2G_TXDATA3G1, RTXDATA2G_TXDATA3G1, 0, OFF, OFF, 1, 1,
		8MA),
	PIN_CFG(RTXEN2G_TXDATA3G2, RTXEN2G_TXDATA3G2, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(RXDATA3G0, RXDATA3G0, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(RXDATA3G1, RXDATA3G1, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(RXDATA3G2, RXDATA3G2, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(CLK_CX8, CLK_CX8, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SYSCLKEN, SYSCLKEN, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config etm_trace_config[] = {
	PIN_CFG(TRACECLK, TRACECLK, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT15, TRACEDT15, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT14, TRACEDT14, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT13, TRACEDT13, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT12, TRACEDT12, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT11, TRACEDT11, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT10, TRACEDT10, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT09, TRACEDT09, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT08, TRACEDT08, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT07, TRACEDT07, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT06, TRACEDT06, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT05, TRACEDT05, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT04, TRACEDT04, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT03, TRACEDT03, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT02, TRACEDT02, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT01, TRACEDT01, 0, OFF, OFF, 0, 1, 8MA),
	PIN_CFG(TRACEDT00, TRACEDT00, 0, OFF, OFF, 0, 1, 8MA),
};

static struct pin_config sdio2_config[] = {
	PIN_CFG(NAND_ALE, SDIO2_CLK, 0, OFF, OFF, 0, 0, 12MA),
	PIN_CFG(NAND_OEN, SDIO2_CMD, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(NAND_WEN, SDIO2_RST_N, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(NAND_AD_7, SDIO2_DATA_7, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(NAND_AD_6, SDIO2_DATA_6, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(NAND_AD_5, SDIO2_DATA_5, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(NAND_AD_4, SDIO2_DATA_4, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(NAND_AD_3, SDIO2_DATA_3, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(NAND_AD_2, SDIO2_DATA_2, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(NAND_AD_1, SDIO2_DATA_1, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(NAND_AD_0, SDIO2_DATA_0, 0, OFF, ON, 0, 0, 12MA),
};

static struct pin_config sim1_config[] = {
	PIN_CFG(SIM_RESETN, SIM_RESETN, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SIM_CLK, SIM_CLK, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SIM_DATA, SIM_DATA, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SIM_DET, SIM_DET, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config keypad_2x3_config[] = {
	PIN_CFG(KP_ROW_OP_0, KP_ROW_OP_0, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(KP_ROW_OP_1, KP_ROW_OP_1, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(KP_COL_IP_0, KP_COL_IP_0, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(KP_COL_IP_1, KP_COL_IP_1, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(KP_COL_IP_2, KP_COL_IP_2, 1, OFF, ON, 0, 0, 8MA),
};

static struct pin_config pmu_i2c_config[] = {
	PIN_BSC_CFG(PMU_SCL, PMU_SCL, 0x20),
	PIN_BSC_CFG(PMU_SDA, PMU_SDA, 0x20),
};

static struct pin_config cam1_i2c_config[] = {
	PIN_BSC_CFG(VC_CAM1_SCL, VC_CAM1_SCL, 0x20),
	PIN_BSC_CFG(VC_CAM1_SDA, VC_CAM1_SDA, 0x20),
};

static struct pin_config cam2_i2c_config[] = {
	PIN_BSC_CFG(VC_CAM2_SCL, VC_CAM2_SCL, 0x20),
	PIN_BSC_CFG(VC_CAM2_SDA, VC_CAM2_SDA, 0x20),
};

static struct pin_config cam3_i2c_config[] = {
	PIN_BSC_CFG(VC_CAM3_SCL, VC_CAM3_SCL, 0x20),
	PIN_BSC_CFG(VC_CAM3_SDA, VC_CAM3_SDA, 0x20),
};

static struct pin_config hdmi_i2c_config[] = {
	PIN_BSC_CFG(HDMI_SCL, HDMI_SCL, 0x0),
	PIN_BSC_CFG(HDMI_SDA, HDMI_SDA, 0x0),
};

static struct pin_config arm1_i2c_config[] = {
	PIN_BSC_CFG(BSC1_SCL, BSC1_SCL, 0x20),
	PIN_BSC_CFG(BSC1_SDA, BSC1_SDA, 0x20),
};

static struct pin_config arm2_i2c_config[] = {
	PIN_BSC_CFG(BSC2_SCL, BSC2_SCL, 0x20),
	PIN_BSC_CFG(BSC2_SDA, BSC2_SDA, 0x20),
};

static struct pin_config uart1_config[] = {
	PIN_CFG(UARTB1_URXD, UARTB1_URXD, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(UARTB1_UTXD, UARTB1_UTXD, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config uart2_config[] = {
	PIN_CFG(UARTB2_URXD, UARTB2_URXD, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(UARTB2_UTXD, UARTB2_UTXD, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config uart3_config[] = {
	PIN_CFG(UARTB3_URTS, UARTB3_URTS, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(UARTB3_UCTS, UARTB3_UCTS, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(UARTB3_URXD, UARTB3_URXD, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(UARTB3_UTXD, UARTB3_UTXD, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config uart4_config[] = {
	PIN_CFG(UARTB4_URTS, UARTB4_URTS, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(UARTB4_UCTS, UARTB4_UCTS, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(UARTB4_URXD, UARTB4_URXD, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(UARTB4_UTXD, UARTB4_UTXD, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config sdio1_config[] = {
	PIN_CFG(SDIO1_DATA_3, SDIO1_DATA_3, 0, OFF, ON, 0, 0, 8MA),
	PIN_CFG(SDIO1_DATA_2, SDIO1_DATA_2, 0, OFF, ON, 0, 0, 8MA),
	PIN_CFG(SDIO1_DATA_1, SDIO1_DATA_1, 0, OFF, ON, 0, 0, 8MA),
	PIN_CFG(SDIO1_DATA_0, SDIO1_DATA_0, 0, OFF, ON, 0, 0, 8MA),
	PIN_CFG(SDIO1_CMD, SDIO1_CMD, 0, OFF, ON, 0, 0, 8MA),
	PIN_CFG(SDIO1_CLK, SDIO1_CLK, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config sdio4_config[] = {
	PIN_CFG(SDIO4_DATA_3, SDIO4_DATA_3, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(SDIO4_DATA_2, SDIO4_DATA_2, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(SDIO4_DATA_1, SDIO4_DATA_1, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(SDIO4_DATA_0, SDIO4_DATA_0, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(SDIO4_CMD, SDIO4_CMD, 0, OFF, ON, 0, 0, 12MA),
	PIN_CFG(SDIO4_CLK, SDIO4_CLK, 0, OFF, OFF, 0, 0, 12MA),
	PIN_CFG(GPIO14, GPIO_014, 1, OFF, ON, 1, 0, 8MA),
	PIN_CFG(MPHI_HAT1, GPIO_137, 1, OFF, ON, 1, 0, 8MA),
};

static struct pin_config ssp4_config[] = {
	PIN_CFG(SSP4_FS, SSP4_FS, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP4_CLK, SSP4_CLK, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP4_RXD, SSP4_RXD, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP4_TXD, SSP4_TXD, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config ssp6_config[] = {
	PIN_CFG(SSP6_FS, SSP6_FS, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP6_CLK, SSP6_CLK, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP6_RXD, SSP6_RXD, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP6_TXD, SSP6_TXD, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config cam1_config[] = {
	PIN_CFG(MPHI_DATA_10, CAM1_CAM_CLK_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_RUN0, CAM1_CAM_VSYNC_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_9, CAM1_CAM_HSYNC_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HCE0_N, CAM1_CAM_DATA0_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HWR_N, CAM1_CAM_DATA1_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HRD_N, CAM1_CAM_DATA2_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HA0, CAM1_CAM_DATA3_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HAT0, CAM1_CAM_DATA4_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_8, CAM1_CAM_DATA5_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_15, CAM1_CAM_DATA6_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_14, CAM1_CAM_DATA7_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_13, CAM1_CAM_DATA8_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_12, CAM1_CAM_DATA9_CPI, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(GPIO08, DBI_C_DIN, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(GPIO09, DBI_C_DOUT, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config cam2_config[] = {
	PIN_CFG(GPIO10, VC_TE0, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(GPIO11, VC_TE1, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config cam3_config[] = {
	PIN_CFG(IC_DP, VC_UTXD, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(IC_DM, VC_URXD, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config cir_config[] = {
	PIN_CFG(MPHI_RUN1, CIR_RX, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_HCE1_N, CIR_TX, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config digital_mic_config[] = {
	PIN_CFG(DIGMIC1_CLK, DIGMIC1_CLK, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(DIGMIC1_DQ, DIGMIC1_DQ, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(DIGMIC2_CLK, DIGMIC2_CLK, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(DIGMIC2_DQ, DIGMIC2_DQ, 1, ON, OFF, 0, 0, 8MA),
};

static struct pin_config usb_host1_config[] = {
	PIN_CFG(NAND_RDY_1, USB0PON, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(NAND_CEN_1, USB0PFT, 1, OFF, ON, 0, 0, 8MA),
};

static struct pin_config gps_config[] = {
	PIN_CFG(KP_ROW_OP_2, GPIO_059, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(KP_ROW_OP_3, GPIO_060, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config bt_config[] = {
	PIN_CFG(SSP2_FS_1, GPIO_096, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP2_FS_2, GPIO_097, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(SSP2_FS_3, GPIO_098, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config wl_config[] = {
	PIN_CFG(SSP2_RXD_1, GPIO_102, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP2_TXD_1, GPIO_103, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config nfc_config[] = {
	PIN_CFG(MPHI_DATA_6, GPIO_139, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(MPHI_DATA_5, GPIO_140, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_4, GPIO_141, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config usb_hsic_config[] = {
	PIN_CFG(MTX_SCAN_DATA, GPIO_174, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config status_led_config[] = {
	PIN_CFG(NAND_CEN_0, GPIO_045, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(NAND_RDY_0, GPIO_046, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(NAND_WP, GPIO_047, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config tsc_config[] = {
	PIN_CFG(GPIO00, GPIO_000, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(GPIO01, GPIO_001, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config ssp0_config[] = {
	PIN_CFG(SSP0_FS, SSP0_FS, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP0_CLK, SSP0_CLK, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP0_RXD, SSP0_RXD, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP0_TXD, SSP0_TXD, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config ssp3_1_config[] = {
	PIN_CFG(SSP3_FS, SSP3_FS, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP3_CLK, SSP3_CLK, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP3_RXD, SSP3_RXD, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP3_TXD, SSP3_TXD, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP_EXTCLK, SSP_EXTCLK, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config vc4_jtag_config[] = {
	PIN_CFG(SSP4_FS, VC_TMS, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP4_CLK, VC_TCK, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP4_RXD, VC_TDI, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP4_TXD, VC_TDO, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP5_FS, VC_TRSTB, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP5_CLK, RTCK, 0, OFF, OFF, 0, 0, 8MA),
};

static struct pin_config battery_charging_config[] = {
	PIN_CFG(SSP0_FS, GPIO_091, 1, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP0_CLK, GPIO_092, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(SSP0_RXD, GPIO_093, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(SSP0_TXD, GPIO_094, 0, OFF, OFF, 1, 1, 6MA),
};

static struct pin_config lcd_config[] = {
	PIN_CFG(LCD_R_7, LCD_R_7, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_6, LCD_R_6, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_5, LCD_R_5, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_4, LCD_R_4, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_3, LCD_R_3, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_2, LCD_R_2, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_1, LCD_R_1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_R_0, LCD_R_0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_7, LCD_G_7, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_6, LCD_G_6, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_5, LCD_G_5, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_4, LCD_G_4, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_3, LCD_G_3, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_2, LCD_G_2, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_1, LCD_G_1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_G_0, LCD_G_0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_7, LCD_B_7, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_6, LCD_B_6, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_5, LCD_B_5, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_4, LCD_B_4, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_3, LCD_B_3, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_2, LCD_B_2, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_1, LCD_B_1, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_B_0, LCD_B_0, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_HSYNC, LCD_HSYNC, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_VSYNC, LCD_VSYNC, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_OE, LCD_OE, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(LCD_PCLK, LCD_PCLK, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(GPIO04, GPIO_004, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(GPIO05, GPIO_005, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(GPIO06, PWM_O_2, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(GPIO07, GPIO_007, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP2_RXD_0, GPIO_100, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP0_FS, GPIO_091, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP0_CLK, GPIO_092, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP0_RXD, GPIO_093, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(SSP0_TXD, GPIO_094, 0, OFF, OFF, 0, 0, 8MA),
	PIN_CFG(MPHI_DATA_11, DBI_C_SCL, 0, OFF, OFF, 0, 0, 8MA),	/* LCD_TN_COL1 */
	PIN_CFG(CWS_SYS_REQ2, VC_CPG0, 0, OFF, OFF, 0, 0, 8MA),	/* LCD_TN_COL2 */
	PIN_CFG(SSP3_CLK, VC_I2S_SCK, 0, OFF, OFF, 0, 0, 8MA),	/* LCD_TN_SEG1 */
	PIN_CFG(CWS_SYS_REQ3, VC_GPIO_6, 0, OFF, OFF, 0, 0, 8MA),	/* LCD_TN_SEG2 */
};

static struct pin_config ssp5_config[] = {
	PIN_CFG(SSP5_FS, SSP5_FS, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP5_CLK, SSP5_CLK, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(SSP5_RXD, SSP5_RXD, 0, ON, OFF, 0, 0, 8MA),
	PIN_CFG(SSP5_TXD, SSP5_TXD, 0, OFF, OFF, 1, 1, 8MA),
};

static struct pin_config pmu1_config[] = {
	PIN_CFG(BAT_RM, BAT_RM, 0, OFF, ON, 0, 0, 8MA),
	PIN_CFG(PMU_INT, PMU_INT, 1, OFF, ON, 0, 0, 8MA),
	PIN_CFG(STAT_1, SIM_LDO_EN, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(STAT_2, SIM2_LDO_EN, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(CLASSGPWR, PC3, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(PC1, PC1, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(PC2, PC2, 0, OFF, OFF, 1, 1, 8MA),
	PIN_CFG(UARTB1_URTS, THERMAL_SHUTDOWN, 0, OFF, OFF, 1, 1, 6MA),
	PIN_CFG(UARTB1_UCTS, PC3, 0, OFF, OFF, 1, 1, 6MA),
};
