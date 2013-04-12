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

#ifndef __CHIP_PINGROUP_H__
#define __CHIP_PINGROUP_H__

enum PIN_GROUP {
	PG_RGMII0,
	PG_RGMII1,
	PG_MODEM,
	PG_RF,
	PG_ETMTRACE,
	PG_SDIO2,
	PG_SIM1,
	PG_KEYPAD_2X3,
	PG_PMU_I2C,
	PG_CAM1_I2C,
	PG_CAM2_I2C,
	PG_CAM3_I2C,
	PG_HDMI_I2C,
	PG_ARM1_I2C,
	PG_ARM2_I2C,
	PG_UART1,
	PG_UART2,
	PG_UART3,
	PG_UART4,
	PG_SDIO1,
	PG_SDIO4,
	PG_SSP4,
	PG_SSP5,
	PG_SSP6,
	PG_CAM1,
	PG_CAM2,
	PG_CAM3,
	PG_CIR,
	PG_LCD,
	PG_CLOCKOUT,
	PG_DIGIMIC,
	PG_PMU1,
	PG_USB_HOST1,
	PG_GPS,
	PG_BT,
	PG_WL,
	PG_CAM_FLASH,
	PG_NFC,
	PG_USB_HSIC,
	PG_BATT_CHRG,
	PG_STATUS_LED,
	PG_TSC,

	PG_MII0,
	PG_MII1,
	PG_NAND,
	PG_SIM2,
	PG_ARM3_I2C,
	PG_SSP0,
	PG_SSP2,
	PG_SSP3_1,
	PG_SSP3_2,
	PG_HSI,
	PG_PM_DEBUG,
	PG_MPHI,
	PG_AVEIN,
	PG_UART5,
	PG_UART6,
	PG_MTX_SCAN,
	PG_IC_USB,
	PG_PMU2,
	PG_CIR2,
	PG_USB_HOST2,
	PG_CAM_PWM,
	PG_CAM_FLASH2,
	PG_GYRO,

	PG_DEBUG_BUS,
	PG_KEYPAD_8X8,
	PG_CLK_MON,
	PG_VC4_JTAG,
	PG_SDIO3,
	PG_EAV,
	PG_UART2_2,
	PG_PTI1,
	PG_PTI2,
};

#endif /* __CHIP_PINGROUP_H__ */
