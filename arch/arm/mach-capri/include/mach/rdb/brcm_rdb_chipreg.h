/************************************************************************************************/
/*                                                                                              */
/*  Copyright 2012  Broadcom Corporation                                                        */
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
/*     Date     : Generated on 3/20/2012 13:8:57                                             */
/*     RDB file : /projects/CHIP/revA0                                                                   */
/************************************************************************************************/

#ifndef __BRCM_RDB_CHIPREG_H__
#define __BRCM_RDB_CHIPREG_H__

#define CHIPREG_CHIPID_REG_OFFSET                                         0x00000000
#define CHIPREG_CHIPID_REG_TYPE                                           UInt32
#define CHIPREG_CHIPID_REG_RESERVED_MASK                                  0x00000000
#define    CHIPREG_CHIPID_REG_CHIPID_SHIFT                                0
#define    CHIPREG_CHIPID_REG_CHIPID_MASK                                 0xFFFFFFFF

#define CHIPREG_VC4_IDS_OFFSET                                            0x00000004
#define CHIPREG_VC4_IDS_TYPE                                              UInt32
#define CHIPREG_VC4_IDS_RESERVED_MASK                                     0xFFFFFFFF

#define CHIPREG_PROJECT_ID_OFFSET                                         0x00000008
#define CHIPREG_PROJECT_ID_TYPE                                           UInt32
#define CHIPREG_PROJECT_ID_RESERVED_MASK                                  0x00000000
#define    CHIPREG_PROJECT_ID_PROJECTID_SHIFT                             0
#define    CHIPREG_PROJECT_ID_PROJECTID_MASK                              0xFFFFFFFF

#define CHIPREG_STRAP_OFFSET                                              0x0000000C
#define CHIPREG_STRAP_TYPE                                                UInt32
#define CHIPREG_STRAP_RESERVED_MASK                                       0xFFFF0000
#define    CHIPREG_STRAP_RESERVED_STRAPS1_SHIFT                           14
#define    CHIPREG_STRAP_RESERVED_STRAPS1_MASK                            0x0000C000
#define    CHIPREG_STRAP_SYS_EMI_DDR3_MODE_SHIFT                          13
#define    CHIPREG_STRAP_SYS_EMI_DDR3_MODE_MASK                           0x00002000
#define    CHIPREG_STRAP_LPDDR2_POP_SHIFT                                 12
#define    CHIPREG_STRAP_LPDDR2_POP_MASK                                  0x00001000
#define    CHIPREG_STRAP_RESERVED_STRAPS2_SHIFT                           11
#define    CHIPREG_STRAP_RESERVED_STRAPS2_MASK                            0x00000800
#define    CHIPREG_STRAP_XTAL_OSC_BYPASS_SHIFT                            10
#define    CHIPREG_STRAP_XTAL_OSC_BYPASS_MASK                             0x00000400
#define    CHIPREG_STRAP_PLL_VCO_BYPASS_SHIFT                             9
#define    CHIPREG_STRAP_PLL_VCO_BYPASS_MASK                              0x00000200
#define    CHIPREG_STRAP_NAND_BOOT_PAGE_SIZE_SHIFT                        7
#define    CHIPREG_STRAP_NAND_BOOT_PAGE_SIZE_MASK                         0x00000180
#define    CHIPREG_STRAP_NAND_BOOT_ECC_SHIFT                              4
#define    CHIPREG_STRAP_NAND_BOOT_ECC_MASK                               0x00000070
#define    CHIPREG_STRAP_BOOT_OPTION_SHIFT                                0
#define    CHIPREG_STRAP_BOOT_OPTION_MASK                                 0x0000000F

#define CHIPREG_KONA_TZCFG_MAILBOX0_OFFSET                                0x00000010
#define CHIPREG_KONA_TZCFG_MAILBOX0_TYPE                                  UInt32
#define CHIPREG_KONA_TZCFG_MAILBOX0_RESERVED_MASK                         0x00000000
#define    CHIPREG_KONA_TZCFG_MAILBOX0_KONA_TZCFG_MAILBOX0_32_SHIFT       0
#define    CHIPREG_KONA_TZCFG_MAILBOX0_KONA_TZCFG_MAILBOX0_32_MASK        0xFFFFFFFF

#define CHIPREG_KONA_TZCFG_MAILBOX1_OFFSET                                0x00000014
#define CHIPREG_KONA_TZCFG_MAILBOX1_TYPE                                  UInt32
#define CHIPREG_KONA_TZCFG_MAILBOX1_RESERVED_MASK                         0x00000000
#define    CHIPREG_KONA_TZCFG_MAILBOX1_KONA_TZCFG_MAILBOX1_32_SHIFT       0
#define    CHIPREG_KONA_TZCFG_MAILBOX1_KONA_TZCFG_MAILBOX1_32_MASK        0xFFFFFFFF

#define CHIPREG_TMACT_DBG_CONTROL_OFFSET                                  0x00000018
#define CHIPREG_TMACT_DBG_CONTROL_TYPE                                    UInt32
#define CHIPREG_TMACT_DBG_CONTROL_RESERVED_MASK                           0xFFFFFFFE
#define    CHIPREG_TMACT_DBG_CONTROL_TMACT_DBG_ENABLE_SHIFT               0
#define    CHIPREG_TMACT_DBG_CONTROL_TMACT_DBG_ENABLE_MASK                0x00000001

#define CHIPREG_TZCONF_STATUS_OFFSET                                      0x0000001C
#define CHIPREG_TZCONF_STATUS_TYPE                                        UInt32
#define CHIPREG_TZCONF_STATUS_RESERVED_MASK                               0xFFFFFF7C
#define    CHIPREG_TZCONF_STATUS_TZCFG_OTP_EMU_MODE_SHIFT                 7
#define    CHIPREG_TZCONF_STATUS_TZCFG_OTP_EMU_MODE_MASK                  0x00000080
#define    CHIPREG_TZCONF_STATUS_TZCFG_OTP_EMU_SHIFT                      1
#define    CHIPREG_TZCONF_STATUS_TZCFG_OTP_EMU_MASK                       0x00000002
#define    CHIPREG_TZCONF_STATUS_TZCFG_OTP_PROD_SHIFT                     0
#define    CHIPREG_TZCONF_STATUS_TZCFG_OTP_PROD_MASK                      0x00000001

#define CHIPREG_SECURITY_SPARE_REG1_OFFSET                                0x00000020
#define CHIPREG_SECURITY_SPARE_REG1_TYPE                                  UInt32
#define CHIPREG_SECURITY_SPARE_REG1_RESERVED_MASK                         0x00000000
#define    CHIPREG_SECURITY_SPARE_REG1_SECURITY_SPARE_REG_SHIFT           0
#define    CHIPREG_SECURITY_SPARE_REG1_SECURITY_SPARE_REG_MASK            0xFFFFFFFF

#define CHIPREG_SECURITY_SPARE_REG2_OFFSET                                0x00000024
#define CHIPREG_SECURITY_SPARE_REG2_TYPE                                  UInt32
#define CHIPREG_SECURITY_SPARE_REG2_RESERVED_MASK                         0x00000000
#define    CHIPREG_SECURITY_SPARE_REG2_SECURITY_SPARE_REG_SHIFT           0
#define    CHIPREG_SECURITY_SPARE_REG2_SECURITY_SPARE_REG_MASK            0xFFFFFFFF

#define CHIPREG_PERIPH_MISC_REG3_OFFSET                                   0x00000028
#define CHIPREG_PERIPH_MISC_REG3_TYPE                                     UInt32
#define CHIPREG_PERIPH_MISC_REG3_RESERVED_MASK                            0xFFFFFFC0
#define    CHIPREG_PERIPH_MISC_REG3_ARM_RAM_PM_DISABLE_OVERRIDE_SHIFT     5
#define    CHIPREG_PERIPH_MISC_REG3_ARM_RAM_PM_DISABLE_OVERRIDE_MASK      0x00000020
#define    CHIPREG_PERIPH_MISC_REG3_BMODEM_GPEN_SWAP_SHIFT                4
#define    CHIPREG_PERIPH_MISC_REG3_BMODEM_GPEN_SWAP_MASK                 0x00000010
#define    CHIPREG_PERIPH_MISC_REG3_TRACECLK_DELAY_SHIFT                  0
#define    CHIPREG_PERIPH_MISC_REG3_TRACECLK_DELAY_MASK                   0x0000000F

#define CHIPREG_RING_OSC_CONTROL_OFFSET                                   0x0000002C
#define CHIPREG_RING_OSC_CONTROL_TYPE                                     UInt32
#define CHIPREG_RING_OSC_CONTROL_RESERVED_MASK                            0xFFFF0300
#define    CHIPREG_RING_OSC_CONTROL_HUB_OSC_OUT_ENABLE_SHIFT              15
#define    CHIPREG_RING_OSC_CONTROL_HUB_OSC_OUT_ENABLE_MASK               0x00008000
#define    CHIPREG_RING_OSC_CONTROL_HUB_IRD_MON_ENABLE_SHIFT              14
#define    CHIPREG_RING_OSC_CONTROL_HUB_IRD_MON_ENABLE_MASK               0x00004000
#define    CHIPREG_RING_OSC_CONTROL_ARM_OSC_OUT_ENABLE_SHIFT              12
#define    CHIPREG_RING_OSC_CONTROL_ARM_OSC_OUT_ENABLE_MASK               0x00003000
#define    CHIPREG_RING_OSC_CONTROL_ARM_IRD_MON_ENABLE_SHIFT              10
#define    CHIPREG_RING_OSC_CONTROL_ARM_IRD_MON_ENABLE_MASK               0x00000C00
#define    CHIPREG_RING_OSC_CONTROL_OSC2_SEL_SHIFT                        6
#define    CHIPREG_RING_OSC_CONTROL_OSC2_SEL_MASK                         0x000000C0
#define    CHIPREG_RING_OSC_CONTROL_OSC2_PW_ENABLE_SHIFT                  5
#define    CHIPREG_RING_OSC_CONTROL_OSC2_PW_ENABLE_MASK                   0x00000020
#define    CHIPREG_RING_OSC_CONTROL_OSC2_ENABLE_SHIFT                     4
#define    CHIPREG_RING_OSC_CONTROL_OSC2_ENABLE_MASK                      0x00000010
#define    CHIPREG_RING_OSC_CONTROL_OSC1_SELECT_SHIFT                     2
#define    CHIPREG_RING_OSC_CONTROL_OSC1_SELECT_MASK                      0x0000000C
#define    CHIPREG_RING_OSC_CONTROL_OSC1_PW_ENABLE_SHIFT                  1
#define    CHIPREG_RING_OSC_CONTROL_OSC1_PW_ENABLE_MASK                   0x00000002
#define    CHIPREG_RING_OSC_CONTROL_OSC1_ENABLE_SHIFT                     0
#define    CHIPREG_RING_OSC_CONTROL_OSC1_ENABLE_MASK                      0x00000001

#define CHIPREG_ARM_PERI_CONTROL_OFFSET                                   0x00000030
#define CHIPREG_ARM_PERI_CONTROL_TYPE                                     UInt32
#define CHIPREG_ARM_PERI_CONTROL_RESERVED_MASK                            0xFFFFF8E3
#define    CHIPREG_ARM_PERI_CONTROL_TPIU_CLK_IS_IDLE_SHIFT                10
#define    CHIPREG_ARM_PERI_CONTROL_TPIU_CLK_IS_IDLE_MASK                 0x00000400
#define    CHIPREG_ARM_PERI_CONTROL_PTI_CLK_IS_IDLE_SHIFT                 9
#define    CHIPREG_ARM_PERI_CONTROL_PTI_CLK_IS_IDLE_MASK                  0x00000200
#define    CHIPREG_ARM_PERI_CONTROL_DEBUG_HALT_REQ_SHIFT                  8
#define    CHIPREG_ARM_PERI_CONTROL_DEBUG_HALT_REQ_MASK                   0x00000100
#define    CHIPREG_ARM_PERI_CONTROL_DDR_POP_NOPOP_SHIFT                   4
#define    CHIPREG_ARM_PERI_CONTROL_DDR_POP_NOPOP_MASK                    0x00000010
#define    CHIPREG_ARM_PERI_CONTROL_SLV_TIMER_64BITMODE_SHIFT             3
#define    CHIPREG_ARM_PERI_CONTROL_SLV_TIMER_64BITMODE_MASK              0x00000008
#define    CHIPREG_ARM_PERI_CONTROL_HUB_TIMER_64BITMODE_SHIFT             2
#define    CHIPREG_ARM_PERI_CONTROL_HUB_TIMER_64BITMODE_MASK              0x00000004

#define CHIPREG_PERIPH_MISC_REG1_OFFSET                                   0x00000034
#define CHIPREG_PERIPH_MISC_REG1_TYPE                                     UInt32
#define CHIPREG_PERIPH_MISC_REG1_RESERVED_MASK                            0x00000000
#define    CHIPREG_PERIPH_MISC_REG1_A9_TO_FABRIC_SSTXF_MASK_SHIFT         31
#define    CHIPREG_PERIPH_MISC_REG1_A9_TO_FABRIC_SSTXF_MASK_MASK          0x80000000
#define    CHIPREG_PERIPH_MISC_REG1_MM_TO_HUB_SSTXF_MASK_SHIFT            30
#define    CHIPREG_PERIPH_MISC_REG1_MM_TO_HUB_SSTXF_MASK_MASK             0x40000000
#define    CHIPREG_PERIPH_MISC_REG1_FORCE_UPDATE_DISABLE_SHIFT            29
#define    CHIPREG_PERIPH_MISC_REG1_FORCE_UPDATE_DISABLE_MASK             0x20000000
#define    CHIPREG_PERIPH_MISC_REG1_AFCPDM_REQ_ENABLE_SHIFT               28
#define    CHIPREG_PERIPH_MISC_REG1_AFCPDM_REQ_ENABLE_MASK                0x10000000
#define    CHIPREG_PERIPH_MISC_REG1_DIS_VC4_EMI_SHIFT                     27
#define    CHIPREG_PERIPH_MISC_REG1_DIS_VC4_EMI_MASK                      0x08000000
#define    CHIPREG_PERIPH_MISC_REG1_CLKOUT_OUT_EN_SHIFT                   26
#define    CHIPREG_PERIPH_MISC_REG1_CLKOUT_OUT_EN_MASK                    0x04000000
#define    CHIPREG_PERIPH_MISC_REG1_FABRIC_SNOOP_SHIFT                    25
#define    CHIPREG_PERIPH_MISC_REG1_FABRIC_SNOOP_MASK                     0x02000000
#define    CHIPREG_PERIPH_MISC_REG1_SI_PERF_MON_HW_EN_SHIFT               24
#define    CHIPREG_PERIPH_MISC_REG1_SI_PERF_MON_HW_EN_MASK                0x01000000
#define    CHIPREG_PERIPH_MISC_REG1_DEBUG_BUS_SEL_SHIFT                   16
#define    CHIPREG_PERIPH_MISC_REG1_DEBUG_BUS_SEL_MASK                    0x00FF0000
#define    CHIPREG_PERIPH_MISC_REG1_CAPH_DBG_SEL_SHIFT                    0
#define    CHIPREG_PERIPH_MISC_REG1_CAPH_DBG_SEL_MASK                     0x0000FFFF

#define CHIPREG_PERIPH_MISC_REG2_OFFSET                                   0x00000038
#define CHIPREG_PERIPH_MISC_REG2_TYPE                                     UInt32
#define CHIPREG_PERIPH_MISC_REG2_RESERVED_MASK                            0x1F002000
#define    CHIPREG_PERIPH_MISC_REG2_PVTMON_MEASUREMENT_CALIBRATION_MODE_SHIFT 31
#define    CHIPREG_PERIPH_MISC_REG2_PVTMON_MEASUREMENT_CALIBRATION_MODE_MASK 0x80000000
#define    CHIPREG_PERIPH_MISC_REG2_PVTMON_MEASUREMENT_CALIBRATION_P_N_SHIFT 30
#define    CHIPREG_PERIPH_MISC_REG2_PVTMON_MEASUREMENT_CALIBRATION_P_N_MASK 0x40000000
#define    CHIPREG_PERIPH_MISC_REG2_PVTMON_MEASUREMENT_CALIBRATION_MAGNITUDE_SHIFT 29
#define    CHIPREG_PERIPH_MISC_REG2_PVTMON_MEASUREMENT_CALIBRATION_MAGNITUDE_MASK 0x20000000
#define    CHIPREG_PERIPH_MISC_REG2_XTAL_ELDO_MUX_SELECT_SHIFT            23
#define    CHIPREG_PERIPH_MISC_REG2_XTAL_ELDO_MUX_SELECT_MASK             0x00800000
#define    CHIPREG_PERIPH_MISC_REG2_SYS_EMI_CLOCK_DELAY_SELECT_UPPER_SHIFT 20
#define    CHIPREG_PERIPH_MISC_REG2_SYS_EMI_CLOCK_DELAY_SELECT_UPPER_MASK 0x00700000
#define    CHIPREG_PERIPH_MISC_REG2_DIFF26M_CLK_IS_IDLE_SHIFT             19
#define    CHIPREG_PERIPH_MISC_REG2_DIFF26M_CLK_IS_IDLE_MASK              0x00080000
#define    CHIPREG_PERIPH_MISC_REG2_SE26M_CLK_IS_IDLE_SHIFT               18
#define    CHIPREG_PERIPH_MISC_REG2_SE26M_CLK_IS_IDLE_MASK                0x00040000
#define    CHIPREG_PERIPH_MISC_REG2_MM_CRYSTAL_CLK_IS_IDLE_SHIFT          17
#define    CHIPREG_PERIPH_MISC_REG2_MM_CRYSTAL_CLK_IS_IDLE_MASK           0x00020000
#define    CHIPREG_PERIPH_MISC_REG2_HDMI_CRYSTAL_CLK_IS_IDLE_SHIFT        16
#define    CHIPREG_PERIPH_MISC_REG2_HDMI_CRYSTAL_CLK_IS_IDLE_MASK         0x00010000
#define    CHIPREG_PERIPH_MISC_REG2_DDR3_SW_RESET_SHIFT                   15
#define    CHIPREG_PERIPH_MISC_REG2_DDR3_SW_RESET_MASK                    0x00008000
#define    CHIPREG_PERIPH_MISC_REG2_DEBUG_BUS_SEL_SHIFT                   14
#define    CHIPREG_PERIPH_MISC_REG2_DEBUG_BUS_SEL_MASK                    0x00004000
#define    CHIPREG_PERIPH_MISC_REG2_I2C_PULLUP_ENABLE_SHIFT               12
#define    CHIPREG_PERIPH_MISC_REG2_I2C_PULLUP_ENABLE_MASK                0x00001000
#define    CHIPREG_PERIPH_MISC_REG2_SYS_EMI_CLOCK_DELAY_SELECT_LOWER_SHIFT 9
#define    CHIPREG_PERIPH_MISC_REG2_SYS_EMI_CLOCK_DELAY_SELECT_LOWER_MASK 0x00000E00
#define    CHIPREG_PERIPH_MISC_REG2_WGM_WAKEUP_MODEM_IRQ_SHIFT            8
#define    CHIPREG_PERIPH_MISC_REG2_WGM_WAKEUP_MODEM_IRQ_MASK             0x00000100
#define    CHIPREG_PERIPH_MISC_REG2_VC4_CLOCK_DELAY_SELECT_SHIFT          2
#define    CHIPREG_PERIPH_MISC_REG2_VC4_CLOCK_DELAY_SELECT_MASK           0x000000FC
#define    CHIPREG_PERIPH_MISC_REG2_RGMII_SEL_SHIFT                       0
#define    CHIPREG_PERIPH_MISC_REG2_RGMII_SEL_MASK                        0x00000003

#define CHIPREG_MDIO_CTRL_ADDR_WRDATA_OFFSET                              0x0000003C
#define CHIPREG_MDIO_CTRL_ADDR_WRDATA_TYPE                                UInt32
#define CHIPREG_MDIO_CTRL_ADDR_WRDATA_RESERVED_MASK                       0x00E00000
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_WRITE_START_SHIFT           31
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_WRITE_START_MASK            0x80000000
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_READ_START_SHIFT            30
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_READ_START_MASK             0x40000000
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_SM_SEL_SHIFT                29
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_SM_SEL_MASK                 0x20000000
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_ID_SHIFT                    24
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_ID_MASK                     0x1F000000
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_REG_ADDR_SHIFT              16
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_REG_ADDR_MASK               0x001F0000
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_REG_WR_DATA_SHIFT           0
#define    CHIPREG_MDIO_CTRL_ADDR_WRDATA_MDIO_REG_WR_DATA_MASK            0x0000FFFF

#define CHIPREG_MDIO_CTRL_RD_DATA_OFFSET                                  0x00000040
#define CHIPREG_MDIO_CTRL_RD_DATA_TYPE                                    UInt32
#define CHIPREG_MDIO_CTRL_RD_DATA_RESERVED_MASK                           0xFFFF0000
#define    CHIPREG_MDIO_CTRL_RD_DATA_MDIO_REG_RD_DATA_SHIFT               0
#define    CHIPREG_MDIO_CTRL_RD_DATA_MDIO_REG_RD_DATA_MASK                0x0000FFFF

#define CHIPREG_RAM_STBY_CONTROL_OFFSET                                   0x00000044
#define CHIPREG_RAM_STBY_CONTROL_TYPE                                     UInt32
#define CHIPREG_RAM_STBY_CONTROL_RESERVED_MASK                            0x0080C000
#define    CHIPREG_RAM_STBY_CONTROL_NAND_STBY_1_SHIFT                     30
#define    CHIPREG_RAM_STBY_CONTROL_NAND_STBY_1_MASK                      0xC0000000
#define    CHIPREG_RAM_STBY_CONTROL_NAND_STBY_0_SHIFT                     28
#define    CHIPREG_RAM_STBY_CONTROL_NAND_STBY_0_MASK                      0x30000000
#define    CHIPREG_RAM_STBY_CONTROL_HSIC2_STBY_1_SHIFT                    26
#define    CHIPREG_RAM_STBY_CONTROL_HSIC2_STBY_1_MASK                     0x0C000000
#define    CHIPREG_RAM_STBY_CONTROL_USBH2_STBY_1_SHIFT                    24
#define    CHIPREG_RAM_STBY_CONTROL_USBH2_STBY_1_MASK                     0x03000000
#define    CHIPREG_RAM_STBY_CONTROL_ROM_STBY_SHIFT                        22
#define    CHIPREG_RAM_STBY_CONTROL_ROM_STBY_MASK                         0x00400000
#define    CHIPREG_RAM_STBY_CONTROL_UARTB6_STBY_SHIFT                     21
#define    CHIPREG_RAM_STBY_CONTROL_UARTB6_STBY_MASK                      0x00200000
#define    CHIPREG_RAM_STBY_CONTROL_UARTB5_STBY_SHIFT                     20
#define    CHIPREG_RAM_STBY_CONTROL_UARTB5_STBY_MASK                      0x00100000
#define    CHIPREG_RAM_STBY_CONTROL_UARTB4_STBY_SHIFT                     19
#define    CHIPREG_RAM_STBY_CONTROL_UARTB4_STBY_MASK                      0x00080000
#define    CHIPREG_RAM_STBY_CONTROL_UARTB3_STBY_SHIFT                     18
#define    CHIPREG_RAM_STBY_CONTROL_UARTB3_STBY_MASK                      0x00040000
#define    CHIPREG_RAM_STBY_CONTROL_UARTB2_STBY_SHIFT                     17
#define    CHIPREG_RAM_STBY_CONTROL_UARTB2_STBY_MASK                      0x00020000
#define    CHIPREG_RAM_STBY_CONTROL_UARTB1_STBY_SHIFT                     16
#define    CHIPREG_RAM_STBY_CONTROL_UARTB1_STBY_MASK                      0x00010000
#define    CHIPREG_RAM_STBY_CONTROL_SSP2_STBY_SHIFT                       13
#define    CHIPREG_RAM_STBY_CONTROL_SSP2_STBY_MASK                        0x00002000
#define    CHIPREG_RAM_STBY_CONTROL_SSP0_STBY_SHIFT                       12
#define    CHIPREG_RAM_STBY_CONTROL_SSP0_STBY_MASK                        0x00001000
#define    CHIPREG_RAM_STBY_CONTROL_SDIO4_RF_STBY_SHIFT                   11
#define    CHIPREG_RAM_STBY_CONTROL_SDIO4_RF_STBY_MASK                    0x00000800
#define    CHIPREG_RAM_STBY_CONTROL_SDIO3_RF_STBY_SHIFT                   10
#define    CHIPREG_RAM_STBY_CONTROL_SDIO3_RF_STBY_MASK                    0x00000400
#define    CHIPREG_RAM_STBY_CONTROL_SDIO2_RF_STBY_SHIFT                   9
#define    CHIPREG_RAM_STBY_CONTROL_SDIO2_RF_STBY_MASK                    0x00000200
#define    CHIPREG_RAM_STBY_CONTROL_SDIO1_RF_STBY_SHIFT                   8
#define    CHIPREG_RAM_STBY_CONTROL_SDIO1_RF_STBY_MASK                    0x00000100
#define    CHIPREG_RAM_STBY_CONTROL_STBY_RF_SHIFT                         7
#define    CHIPREG_RAM_STBY_CONTROL_STBY_RF_MASK                          0x00000080
#define    CHIPREG_RAM_STBY_CONTROL_PKA_STBY_SHIFT                        6
#define    CHIPREG_RAM_STBY_CONTROL_PKA_STBY_MASK                         0x00000040
#define    CHIPREG_RAM_STBY_CONTROL_STBY_SHIFT                            5
#define    CHIPREG_RAM_STBY_CONTROL_STBY_MASK                             0x00000020
#define    CHIPREG_RAM_STBY_CONTROL_USBH2_STBY_0_SHIFT                    3
#define    CHIPREG_RAM_STBY_CONTROL_USBH2_STBY_0_MASK                     0x00000018
#define    CHIPREG_RAM_STBY_CONTROL_HSIC2_STBY_0_SHIFT                    1
#define    CHIPREG_RAM_STBY_CONTROL_HSIC2_STBY_0_MASK                     0x00000006
#define    CHIPREG_RAM_STBY_CONTROL_AUDIOH_STBY_SHIFT                     0
#define    CHIPREG_RAM_STBY_CONTROL_AUDIOH_STBY_MASK                      0x00000001

#define CHIPREG_RAM_STBY_CONTROL1_OFFSET                                  0x00000048
#define CHIPREG_RAM_STBY_CONTROL1_TYPE                                    UInt32
#define CHIPREG_RAM_STBY_CONTROL1_RESERVED_MASK                           0x07F00C00
#define    CHIPREG_RAM_STBY_CONTROL1_RF_STBY_POWER_AWARE_ENABLE_SHIFT     31
#define    CHIPREG_RAM_STBY_CONTROL1_RF_STBY_POWER_AWARE_ENABLE_MASK      0x80000000
#define    CHIPREG_RAM_STBY_CONTROL1_RAM_STBY_POWER_AWARE_ENABLE_SHIFT    30
#define    CHIPREG_RAM_STBY_CONTROL1_RAM_STBY_POWER_AWARE_ENABLE_MASK     0x40000000
#define    CHIPREG_RAM_STBY_CONTROL1_RF_STBY_POWER_AWARE_SETTING_SHIFT    29
#define    CHIPREG_RAM_STBY_CONTROL1_RF_STBY_POWER_AWARE_SETTING_MASK     0x20000000
#define    CHIPREG_RAM_STBY_CONTROL1_RAM_STBY_POWER_AWARE_SETTING_SHIFT   27
#define    CHIPREG_RAM_STBY_CONTROL1_RAM_STBY_POWER_AWARE_SETTING_MASK    0x18000000
#define    CHIPREG_RAM_STBY_CONTROL1_SP_STBY_ESW_SHIFT                    18
#define    CHIPREG_RAM_STBY_CONTROL1_SP_STBY_ESW_MASK                     0x000C0000
#define    CHIPREG_RAM_STBY_CONTROL1_PD_STBY_ESW_SHIFT                    16
#define    CHIPREG_RAM_STBY_CONTROL1_PD_STBY_ESW_MASK                     0x00030000
#define    CHIPREG_RAM_STBY_CONTROL1_RAM_STBY_SHIFT                       14
#define    CHIPREG_RAM_STBY_CONTROL1_RAM_STBY_MASK                        0x0000C000
#define    CHIPREG_RAM_STBY_CONTROL1_STBY_SR_SHIFT                        12
#define    CHIPREG_RAM_STBY_CONTROL1_STBY_SR_MASK                         0x00003000
#define    CHIPREG_RAM_STBY_CONTROL1_KEK_STBY_RF_SHIFT                    8
#define    CHIPREG_RAM_STBY_CONTROL1_KEK_STBY_RF_MASK                     0x00000300
#define    CHIPREG_RAM_STBY_CONTROL1_HSOTG_DFIFO_STBY_SHIFT               6
#define    CHIPREG_RAM_STBY_CONTROL1_HSOTG_DFIFO_STBY_MASK                0x000000C0
#define    CHIPREG_RAM_STBY_CONTROL1_FSHOST_DFIFO_STBY_SHIFT              4
#define    CHIPREG_RAM_STBY_CONTROL1_FSHOST_DFIFO_STBY_MASK               0x00000030
#define    CHIPREG_RAM_STBY_CONTROL1_SCRATCHRAM_STBY_SHIFT                2
#define    CHIPREG_RAM_STBY_CONTROL1_SCRATCHRAM_STBY_MASK                 0x0000000C
#define    CHIPREG_RAM_STBY_CONTROL1_CPH_STBY_SHIFT                       0
#define    CHIPREG_RAM_STBY_CONTROL1_CPH_STBY_MASK                        0x00000003

#define CHIPREG_SYS_DDRLDO_CONTROL0_OFFSET                                0x0000004C
#define CHIPREG_SYS_DDRLDO_CONTROL0_TYPE                                  UInt32
#define CHIPREG_SYS_DDRLDO_CONTROL0_RESERVED_MASK                         0x00000000
#define    CHIPREG_SYS_DDRLDO_CONTROL0_SYS_DDRLDO_I_LDO_CONTL_SHIFT       16
#define    CHIPREG_SYS_DDRLDO_CONTROL0_SYS_DDRLDO_I_LDO_CONTL_MASK        0xFFFF0000
#define    CHIPREG_SYS_DDRLDO_CONTROL0_SYS_DDRLDO_I_BG_CONTL_SHIFT        0
#define    CHIPREG_SYS_DDRLDO_CONTROL0_SYS_DDRLDO_I_BG_CONTL_MASK         0x0000FFFF

#define CHIPREG_SYS_DDRLDO_CONTROL1_OFFSET                                0x00000050
#define CHIPREG_SYS_DDRLDO_CONTROL1_TYPE                                  UInt32
#define CHIPREG_SYS_DDRLDO_CONTROL1_RESERVED_MASK                         0xFFFFFFFC
#define    CHIPREG_SYS_DDRLDO_CONTROL1_SYS_DDRLDO_I_CNTL_EN_SHIFT         1
#define    CHIPREG_SYS_DDRLDO_CONTROL1_SYS_DDRLDO_I_CNTL_EN_MASK          0x00000002
#define    CHIPREG_SYS_DDRLDO_CONTROL1_SYS_DDRLDO_I_LDO_EN_SHIFT          0
#define    CHIPREG_SYS_DDRLDO_CONTROL1_SYS_DDRLDO_I_LDO_EN_MASK           0x00000001

#define CHIPREG_VC4_DDRLDO_CONTROL0_OFFSET                                0x00000054
#define CHIPREG_VC4_DDRLDO_CONTROL0_TYPE                                  UInt32
#define CHIPREG_VC4_DDRLDO_CONTROL0_RESERVED_MASK                         0x00000000
#define    CHIPREG_VC4_DDRLDO_CONTROL0_VC4_DDRLDO_I_LDO_CONTL_SHIFT       16
#define    CHIPREG_VC4_DDRLDO_CONTROL0_VC4_DDRLDO_I_LDO_CONTL_MASK        0xFFFF0000
#define    CHIPREG_VC4_DDRLDO_CONTROL0_VC4_DDRLDO_I_BG_CONTL_SHIFT        0
#define    CHIPREG_VC4_DDRLDO_CONTROL0_VC4_DDRLDO_I_BG_CONTL_MASK         0x0000FFFF

#define CHIPREG_VC4_DDRLDO_CONTROL1_OFFSET                                0x00000058
#define CHIPREG_VC4_DDRLDO_CONTROL1_TYPE                                  UInt32
#define CHIPREG_VC4_DDRLDO_CONTROL1_RESERVED_MASK                         0xFFFFFFFC
#define    CHIPREG_VC4_DDRLDO_CONTROL1_VC4_DDRLDO_I_CNTL_EN_SHIFT         1
#define    CHIPREG_VC4_DDRLDO_CONTROL1_VC4_DDRLDO_I_CNTL_EN_MASK          0x00000002
#define    CHIPREG_VC4_DDRLDO_CONTROL1_VC4_DDRLDO_I_LDO_EN_SHIFT          0
#define    CHIPREG_VC4_DDRLDO_CONTROL1_VC4_DDRLDO_I_LDO_EN_MASK           0x00000001

#define CHIPREG_XTAL_DDRLDO_CONTROL0_OFFSET                               0x0000005C
#define CHIPREG_XTAL_DDRLDO_CONTROL0_TYPE                                 UInt32
#define CHIPREG_XTAL_DDRLDO_CONTROL0_RESERVED_MASK                        0x00000000
#define    CHIPREG_XTAL_DDRLDO_CONTROL0_XTAL_DDRLDO_I_LDO_CONTL_SHIFT     16
#define    CHIPREG_XTAL_DDRLDO_CONTROL0_XTAL_DDRLDO_I_LDO_CONTL_MASK      0xFFFF0000
#define    CHIPREG_XTAL_DDRLDO_CONTROL0_XTAL_DDRLDO_I_BG_CONTL_SHIFT      0
#define    CHIPREG_XTAL_DDRLDO_CONTROL0_XTAL_DDRLDO_I_BG_CONTL_MASK       0x0000FFFF

#define CHIPREG_XTAL_DDRLDO_CONTROL1_OFFSET                               0x00000060
#define CHIPREG_XTAL_DDRLDO_CONTROL1_TYPE                                 UInt32
#define CHIPREG_XTAL_DDRLDO_CONTROL1_RESERVED_MASK                        0xFFFFFFFC
#define    CHIPREG_XTAL_DDRLDO_CONTROL1_XTAL_DDRLDO_I_CNTL_EN_SHIFT       1
#define    CHIPREG_XTAL_DDRLDO_CONTROL1_XTAL_DDRLDO_I_CNTL_EN_MASK        0x00000002
#define    CHIPREG_XTAL_DDRLDO_CONTROL1_XTAL_DDRLDO_I_LDO_EN_SHIFT        0
#define    CHIPREG_XTAL_DDRLDO_CONTROL1_XTAL_DDRLDO_I_LDO_EN_MASK         0x00000001

#define CHIPREG_MM_SWITCH_CONTROL_OFFSET                                  0x00000064
#define CHIPREG_MM_SWITCH_CONTROL_TYPE                                    UInt32
#define CHIPREG_MM_SWITCH_CONTROL_RESERVED_MASK                           0xFFFFE080
#define    CHIPREG_MM_SWITCH_CONTROL_PI_MM_POWER_OK_SHIFT                 12
#define    CHIPREG_MM_SWITCH_CONTROL_PI_MM_POWER_OK_MASK                  0x00001000
#define    CHIPREG_MM_SWITCH_CONTROL_MM_PWRCMP_MASK_SHIFT                 8
#define    CHIPREG_MM_SWITCH_CONTROL_MM_PWRCMP_MASK_MASK                  0x00000F00
#define    CHIPREG_MM_SWITCH_CONTROL_MM_PWRCMP_DELAY_SEL_SHIFT            4
#define    CHIPREG_MM_SWITCH_CONTROL_MM_PWRCMP_DELAY_SEL_MASK             0x00000070
#define    CHIPREG_MM_SWITCH_CONTROL_MM_PWRCMP_THRES_SEL_SHIFT            2
#define    CHIPREG_MM_SWITCH_CONTROL_MM_PWRCMP_THRES_SEL_MASK             0x0000000C
#define    CHIPREG_MM_SWITCH_CONTROL_MM_SWITCH_CTRL_SHIFT                 0
#define    CHIPREG_MM_SWITCH_CONTROL_MM_SWITCH_CTRL_MASK                  0x00000003

#define CHIPREG_ESUB_SWITCH_CONTROL_OFFSET                                0x00000068
#define CHIPREG_ESUB_SWITCH_CONTROL_TYPE                                  UInt32
#define CHIPREG_ESUB_SWITCH_CONTROL_RESERVED_MASK                         0xFFFFE080
#define    CHIPREG_ESUB_SWITCH_CONTROL_PI_ESUB_POWER_OK_SHIFT             12
#define    CHIPREG_ESUB_SWITCH_CONTROL_PI_ESUB_POWER_OK_MASK              0x00001000
#define    CHIPREG_ESUB_SWITCH_CONTROL_ESUB_PWRCMP_MASK_SHIFT             8
#define    CHIPREG_ESUB_SWITCH_CONTROL_ESUB_PWRCMP_MASK_MASK              0x00000F00
#define    CHIPREG_ESUB_SWITCH_CONTROL_ESUB_PWRCMP_DELAY_SEL_SHIFT        4
#define    CHIPREG_ESUB_SWITCH_CONTROL_ESUB_PWRCMP_DELAY_SEL_MASK         0x00000070
#define    CHIPREG_ESUB_SWITCH_CONTROL_ESUB_PWRCMP_THRES_SEL_SHIFT        2
#define    CHIPREG_ESUB_SWITCH_CONTROL_ESUB_PWRCMP_THRES_SEL_MASK         0x0000000C
#define    CHIPREG_ESUB_SWITCH_CONTROL_ESUB_SWITCH_CTRL_SHIFT             0
#define    CHIPREG_ESUB_SWITCH_CONTROL_ESUB_SWITCH_CTRL_MASK              0x00000003

#define CHIPREG_PTI_CONTROL_OFFSET                                        0x00000074
#define CHIPREG_PTI_CONTROL_TYPE                                          UInt32
#define CHIPREG_PTI_CONTROL_RESERVED_MASK                                 0xFFFFFFFE
#define    CHIPREG_PTI_CONTROL_PTI_MUX_CONTROL_SHIFT                      0
#define    CHIPREG_PTI_CONTROL_PTI_MUX_CONTROL_MASK                       0x00000001

#define CHIPREG_BMODEM_SW_INT_SET_OFFSET                                  0x00000078
#define CHIPREG_BMODEM_SW_INT_SET_TYPE                                    UInt32
#define CHIPREG_BMODEM_SW_INT_SET_RESERVED_MASK                           0xFFFF0000
#define    CHIPREG_BMODEM_SW_INT_SET_BMDM_SW_INT_SET_SHIFT                0
#define    CHIPREG_BMODEM_SW_INT_SET_BMDM_SW_INT_SET_MASK                 0x0000FFFF

#define CHIPREG_BMODEM_SW_INT_CLEAR_OFFSET                                0x0000007C
#define CHIPREG_BMODEM_SW_INT_CLEAR_TYPE                                  UInt32
#define CHIPREG_BMODEM_SW_INT_CLEAR_RESERVED_MASK                         0xFFFF0000
#define    CHIPREG_BMODEM_SW_INT_CLEAR_BMDM_SW_INT_CLEAR_SHIFT            0
#define    CHIPREG_BMODEM_SW_INT_CLEAR_BMDM_SW_INT_CLEAR_MASK             0x0000FFFF

#define CHIPREG_BMODEM_SW_INT_SEL_OFFSET                                  0x00000080
#define CHIPREG_BMODEM_SW_INT_SEL_TYPE                                    UInt32
#define CHIPREG_BMODEM_SW_INT_SEL_RESERVED_MASK                           0xFFFF0000
#define    CHIPREG_BMODEM_SW_INT_SEL_BMDM_SW_INT_SEL_SHIFT                0
#define    CHIPREG_BMODEM_SW_INT_SEL_BMDM_SW_INT_SEL_MASK                 0x0000FFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_SET0_OFFSET                          0x00000084
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET0_TYPE                            UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET0_RESERVED_MASK                   0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET0_INTR_EN_4_PMU_SET0_SHIFT     0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET0_INTR_EN_4_PMU_SET0_MASK      0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_SET1_OFFSET                          0x00000088
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET1_TYPE                            UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET1_RESERVED_MASK                   0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET1_INTR_EN_4_PMU_SET1_SHIFT     0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET1_INTR_EN_4_PMU_SET1_MASK      0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_SET2_OFFSET                          0x0000008C
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET2_TYPE                            UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET2_RESERVED_MASK                   0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET2_INTR_EN_4_PMU_SET2_SHIFT     0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET2_INTR_EN_4_PMU_SET2_MASK      0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_SET3_OFFSET                          0x00000090
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET3_TYPE                            UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET3_RESERVED_MASK                   0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET3_INTR_EN_4_PMU_SET3_SHIFT     0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET3_INTR_EN_4_PMU_SET3_MASK      0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_SET4_OFFSET                          0x00000094
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET4_TYPE                            UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET4_RESERVED_MASK                   0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET4_INTR_EN_4_PMU_SET4_SHIFT     0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET4_INTR_EN_4_PMU_SET4_MASK      0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_SET5_OFFSET                          0x00000098
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET5_TYPE                            UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET5_RESERVED_MASK                   0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET5_INTR_EN_4_PMU_SET5_SHIFT     0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET5_INTR_EN_4_PMU_SET5_MASK      0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_SET6_OFFSET                          0x0000009C
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET6_TYPE                            UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_SET6_RESERVED_MASK                   0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET6_INTR_EN_4_PMU_SET6_SHIFT     0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_SET6_INTR_EN_4_PMU_SET6_MASK      0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR0_OFFSET                        0x000000A0
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR0_TYPE                          UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR0_RESERVED_MASK                 0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR0_INTR_EN_4_PMU_CLEAR0_SHIFT 0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR0_INTR_EN_4_PMU_CLEAR0_MASK  0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR1_OFFSET                        0x000000A4
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR1_TYPE                          UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR1_RESERVED_MASK                 0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR1_INTR_EN_4_PMU_CLEAR1_SHIFT 0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR1_INTR_EN_4_PMU_CLEAR1_MASK  0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR2_OFFSET                        0x000000A8
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR2_TYPE                          UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR2_RESERVED_MASK                 0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR2_INTR_EN_4_PMU_CLEAR2_SHIFT 0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR2_INTR_EN_4_PMU_CLEAR2_MASK  0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR3_OFFSET                        0x000000AC
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR3_TYPE                          UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR3_RESERVED_MASK                 0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR3_INTR_EN_4_PMU_CLEAR3_SHIFT 0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR3_INTR_EN_4_PMU_CLEAR3_MASK  0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR4_OFFSET                        0x000000B0
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR4_TYPE                          UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR4_RESERVED_MASK                 0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR4_INTR_EN_4_PMU_CLEAR4_SHIFT 0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR4_INTR_EN_4_PMU_CLEAR4_MASK  0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR5_OFFSET                        0x000000B4
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR5_TYPE                          UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR5_RESERVED_MASK                 0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR5_INTR_EN_4_PMU_CLEAR5_SHIFT 0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR5_INTR_EN_4_PMU_CLEAR5_MASK  0xFFFFFFFF

#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR6_OFFSET                        0x000000B8
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR6_TYPE                          UInt32
#define CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR6_RESERVED_MASK                 0x00000000
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR6_INTR_EN_4_PMU_CLEAR6_SHIFT 0
#define    CHIPREG_INTERRUPT_EVENT_4_PM_CLEAR6_INTR_EN_4_PMU_CLEAR6_MASK  0xFFFFFFFF

#define CHIPREG_AUXDAC_CONTROL_OFFSET                                     0x000000BC
#define CHIPREG_AUXDAC_CONTROL_TYPE                                       UInt32
#define CHIPREG_AUXDAC_CONTROL_RESERVED_MASK                              0xFFFFFFFF

#define CHIPREG_A9_DORMANT_BOOT_ADDR_REG0_OFFSET                          0x000000C0
#define CHIPREG_A9_DORMANT_BOOT_ADDR_REG0_TYPE                            UInt32
#define CHIPREG_A9_DORMANT_BOOT_ADDR_REG0_RESERVED_MASK                   0x00000000
#define    CHIPREG_A9_DORMANT_BOOT_ADDR_REG0_A9_DORMANT_BOOT_ADDRESS0_SHIFT 0
#define    CHIPREG_A9_DORMANT_BOOT_ADDR_REG0_A9_DORMANT_BOOT_ADDRESS0_MASK 0xFFFFFFFF

#define CHIPREG_A9_DORMANT_BOOT_ADDR_REG1_OFFSET                          0x000000C4
#define CHIPREG_A9_DORMANT_BOOT_ADDR_REG1_TYPE                            UInt32
#define CHIPREG_A9_DORMANT_BOOT_ADDR_REG1_RESERVED_MASK                   0x00000000
#define    CHIPREG_A9_DORMANT_BOOT_ADDR_REG1_A9_DORMANT_BOOT_ADDRESS1_SHIFT 0
#define    CHIPREG_A9_DORMANT_BOOT_ADDR_REG1_A9_DORMANT_BOOT_ADDRESS1_MASK 0xFFFFFFFF

#define CHIPREG_EAV_CONTROL_OFFSET                                        0x000000C8
#define CHIPREG_EAV_CONTROL_TYPE                                          UInt32
#define CHIPREG_EAV_CONTROL_RESERVED_MASK                                 0xFFFFFFFC
#define    CHIPREG_EAV_CONTROL_EAV_FORCE_TEST_MODE_SHIFT                  1
#define    CHIPREG_EAV_CONTROL_EAV_FORCE_TEST_MODE_MASK                   0x00000002
#define    CHIPREG_EAV_CONTROL_EAV_DUMP_CONTROL_SHIFT                     0
#define    CHIPREG_EAV_CONTROL_EAV_DUMP_CONTROL_MASK                      0x00000001

#define CHIPREG_SPIS_DIAG_CONTROL_OFFSET                                  0x000000CC
#define CHIPREG_SPIS_DIAG_CONTROL_TYPE                                    UInt32
#define CHIPREG_SPIS_DIAG_CONTROL_RESERVED_MASK                           0xFFFFFF00
#define    CHIPREG_SPIS_DIAG_CONTROL_SPIS_DIAG_SEL_SHIFT                  0
#define    CHIPREG_SPIS_DIAG_CONTROL_SPIS_DIAG_SEL_MASK                   0x000000FF

#define CHIPREG_USB3_DEBUG_CONTROL_OFFSET                                 0x000000D0
#define CHIPREG_USB3_DEBUG_CONTROL_TYPE                                   UInt32
#define CHIPREG_USB3_DEBUG_CONTROL_RESERVED_MASK                          0xFFFFFFFF

#define CHIPREG_USBH_DEBUG_CONTROL_OFFSET                                 0x000000D4
#define CHIPREG_USBH_DEBUG_CONTROL_TYPE                                   UInt32
#define CHIPREG_USBH_DEBUG_CONTROL_RESERVED_MASK                          0xFFFFFFFF

#define CHIPREG_HSIC2_DIAG_CONTROL_OFFSET                                 0x000000D8
#define CHIPREG_HSIC2_DIAG_CONTROL_TYPE                                   UInt32
#define CHIPREG_HSIC2_DIAG_CONTROL_RESERVED_MASK                          0xFFFFFF00
#define    CHIPREG_HSIC2_DIAG_CONTROL_HSIC2_DIAG_SELECT_SHIFT             0
#define    CHIPREG_HSIC2_DIAG_CONTROL_HSIC2_DIAG_SELECT_MASK              0x000000FF

#define CHIPREG_VC4_AUX_CONTROL_OFFSET                                    0x000000DC
#define CHIPREG_VC4_AUX_CONTROL_TYPE                                      UInt32
#define CHIPREG_VC4_AUX_CONTROL_RESERVED_MASK                             0xFFFFFFF8
#define    CHIPREG_VC4_AUX_CONTROL_VC4_SELF_REFRESH_ENTER_SHIFT           2
#define    CHIPREG_VC4_AUX_CONTROL_VC4_SELF_REFRESH_ENTER_MASK            0x00000004
#define    CHIPREG_VC4_AUX_CONTROL_VC4_PHY_STANDBY_SHIFT                  1
#define    CHIPREG_VC4_AUX_CONTROL_VC4_PHY_STANDBY_MASK                   0x00000002
#define    CHIPREG_VC4_AUX_CONTROL_VC4_SREF_IS_DPD_SHIFT                  0
#define    CHIPREG_VC4_AUX_CONTROL_VC4_SREF_IS_DPD_MASK                   0x00000001

#define CHIPREG_SYS_SREF_IS_DPD_OFFSET                                    0x000000E0
#define CHIPREG_SYS_SREF_IS_DPD_TYPE                                      UInt32
#define CHIPREG_SYS_SREF_IS_DPD_RESERVED_MASK                             0xFFFFFFFE
#define    CHIPREG_SYS_SREF_IS_DPD_SYS_SREF_IS_DPD_SHIFT                  0
#define    CHIPREG_SYS_SREF_IS_DPD_SYS_SREF_IS_DPD_MASK                   0x00000001

#define CHIPREG_USBH2_DIAG_CONTROL_OFFSET                                 0x000000E4
#define CHIPREG_USBH2_DIAG_CONTROL_TYPE                                   UInt32
#define CHIPREG_USBH2_DIAG_CONTROL_RESERVED_MASK                          0xFFFFFF00
#define    CHIPREG_USBH2_DIAG_CONTROL_USBH2_DIAG_SELECT_SHIFT             0
#define    CHIPREG_USBH2_DIAG_CONTROL_USBH2_DIAG_SELECT_MASK              0x000000FF

#define CHIPREG_RGMII_VREF_CONTROL_OFFSET                                 0x000000E8
#define CHIPREG_RGMII_VREF_CONTROL_TYPE                                   UInt32
#define CHIPREG_RGMII_VREF_CONTROL_RESERVED_MASK                          0xFFFFFFFE
#define    CHIPREG_RGMII_VREF_CONTROL_REF_INT_EN_RGMII_VREF_SHIFT         0
#define    CHIPREG_RGMII_VREF_CONTROL_REF_INT_EN_RGMII_VREF_MASK          0x00000001

#define CHIPREG_TOP_LEVEL_CONTROL_OFFSET                                  0x000000FC
#define CHIPREG_TOP_LEVEL_CONTROL_TYPE                                    UInt32
#define CHIPREG_TOP_LEVEL_CONTROL_RESERVED_MASK                           0xE00101FF
#define    CHIPREG_TOP_LEVEL_CONTROL_JTAG_MUX_SEL_CONTROL_DAP_SHIFT       28
#define    CHIPREG_TOP_LEVEL_CONTROL_JTAG_MUX_SEL_CONTROL_DAP_MASK        0x10000000
#define    CHIPREG_TOP_LEVEL_CONTROL_JTAG_MUX_SEL_CONTROL_VC_SHIFT        26
#define    CHIPREG_TOP_LEVEL_CONTROL_JTAG_MUX_SEL_CONTROL_VC_MASK         0x0C000000
#define    CHIPREG_TOP_LEVEL_CONTROL_JTAG_MUX_SEL_CONTROL_MODEM_SHIFT     24
#define    CHIPREG_TOP_LEVEL_CONTROL_JTAG_MUX_SEL_CONTROL_MODEM_MASK      0x03000000
#define    CHIPREG_TOP_LEVEL_CONTROL_PLLD_CTRL_UPPER27_21_SHIFT           17
#define    CHIPREG_TOP_LEVEL_CONTROL_PLLD_CTRL_UPPER27_21_MASK            0x00FE0000
#define    CHIPREG_TOP_LEVEL_CONTROL_PLLC_CTRL_UPPER27_21_SHIFT           9
#define    CHIPREG_TOP_LEVEL_CONTROL_PLLC_CTRL_UPPER27_21_MASK            0x0000FE00

#define CHIPREG_CHIP_SW_STRAP_OFFSET                                      0x00000100
#define CHIPREG_CHIP_SW_STRAP_TYPE                                        UInt32
#define CHIPREG_CHIP_SW_STRAP_RESERVED_MASK                               0x00000000
#define    CHIPREG_CHIP_SW_STRAP_SW_STRAPS_SHIFT                          0
#define    CHIPREG_CHIP_SW_STRAP_SW_STRAPS_MASK                           0xFFFFFFFF

#define CHIPREG_GP_STATUS0_OFFSET                                         0x00000104
#define CHIPREG_GP_STATUS0_TYPE                                           UInt32
#define CHIPREG_GP_STATUS0_RESERVED_MASK                                  0x00000000
#define    CHIPREG_GP_STATUS0_GP_STATUS0_SHIFT                            0
#define    CHIPREG_GP_STATUS0_GP_STATUS0_MASK                             0xFFFFFFFF

#define CHIPREG_GP_STATUS1_OFFSET                                         0x00000108
#define CHIPREG_GP_STATUS1_TYPE                                           UInt32
#define CHIPREG_GP_STATUS1_RESERVED_MASK                                  0x00000000
#define    CHIPREG_GP_STATUS1_GP_STATUS1_SHIFT                            0
#define    CHIPREG_GP_STATUS1_GP_STATUS1_MASK                             0xFFFFFFFF

#define CHIPREG_INT_STATUS0_OFFSET                                        0x00000118
#define CHIPREG_INT_STATUS0_TYPE                                          UInt32
#define CHIPREG_INT_STATUS0_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_STATUS0_ISR0N_SHIFT                                0
#define    CHIPREG_INT_STATUS0_ISR0N_MASK                                 0xFFFFFFFF

#define CHIPREG_INT_STATUS1_OFFSET                                        0x0000011C
#define CHIPREG_INT_STATUS1_TYPE                                          UInt32
#define CHIPREG_INT_STATUS1_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_STATUS1_ISR1N_SHIFT                                0
#define    CHIPREG_INT_STATUS1_ISR1N_MASK                                 0xFFFFFFFF

#define CHIPREG_INT_STATUS2_OFFSET                                        0x00000120
#define CHIPREG_INT_STATUS2_TYPE                                          UInt32
#define CHIPREG_INT_STATUS2_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_STATUS2_ISR2N_SHIFT                                0
#define    CHIPREG_INT_STATUS2_ISR2N_MASK                                 0xFFFFFFFF

#define CHIPREG_INT_STATUS3_OFFSET                                        0x00000124
#define CHIPREG_INT_STATUS3_TYPE                                          UInt32
#define CHIPREG_INT_STATUS3_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_STATUS3_ISR3N_SHIFT                                0
#define    CHIPREG_INT_STATUS3_ISR3N_MASK                                 0xFFFFFFFF

#define CHIPREG_INT_STATUS4_OFFSET                                        0x00000128
#define CHIPREG_INT_STATUS4_TYPE                                          UInt32
#define CHIPREG_INT_STATUS4_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_STATUS4_ISR4N_SHIFT                                0
#define    CHIPREG_INT_STATUS4_ISR4N_MASK                                 0xFFFFFFFF

#define CHIPREG_INT_STATUS5_OFFSET                                        0x0000012C
#define CHIPREG_INT_STATUS5_TYPE                                          UInt32
#define CHIPREG_INT_STATUS5_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_STATUS5_ISR5N_SHIFT                                0
#define    CHIPREG_INT_STATUS5_ISR5N_MASK                                 0xFFFFFFFF

#define CHIPREG_INT_STATUS6_OFFSET                                        0x00000130
#define CHIPREG_INT_STATUS6_TYPE                                          UInt32
#define CHIPREG_INT_STATUS6_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_STATUS6_ISR6N_SHIFT                                0
#define    CHIPREG_INT_STATUS6_ISR6N_MASK                                 0xFFFFFFFF

#define CHIPREG_INT_ENABLE0_OFFSET                                        0x00000134
#define CHIPREG_INT_ENABLE0_TYPE                                          UInt32
#define CHIPREG_INT_ENABLE0_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_ENABLE0_MDM_IMR0N_SHIFT                            0
#define    CHIPREG_INT_ENABLE0_MDM_IMR0N_MASK                             0xFFFFFFFF

#define CHIPREG_INT_ENABLE1_OFFSET                                        0x00000138
#define CHIPREG_INT_ENABLE1_TYPE                                          UInt32
#define CHIPREG_INT_ENABLE1_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_ENABLE1_MDM_IMR1N_SHIFT                            0
#define    CHIPREG_INT_ENABLE1_MDM_IMR1N_MASK                             0xFFFFFFFF

#define CHIPREG_INT_ENABLE2_OFFSET                                        0x0000013C
#define CHIPREG_INT_ENABLE2_TYPE                                          UInt32
#define CHIPREG_INT_ENABLE2_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_ENABLE2_MDM_IMR2N_SHIFT                            0
#define    CHIPREG_INT_ENABLE2_MDM_IMR2N_MASK                             0xFFFFFFFF

#define CHIPREG_INT_ENABLE3_OFFSET                                        0x00000140
#define CHIPREG_INT_ENABLE3_TYPE                                          UInt32
#define CHIPREG_INT_ENABLE3_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_ENABLE3_MDM_IMR3N_SHIFT                            0
#define    CHIPREG_INT_ENABLE3_MDM_IMR3N_MASK                             0xFFFFFFFF

#define CHIPREG_INT_ENABLE4_OFFSET                                        0x00000144
#define CHIPREG_INT_ENABLE4_TYPE                                          UInt32
#define CHIPREG_INT_ENABLE4_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_ENABLE4_MDM_IMR4N_SHIFT                            0
#define    CHIPREG_INT_ENABLE4_MDM_IMR4N_MASK                             0xFFFFFFFF

#define CHIPREG_INT_ENABLE5_OFFSET                                        0x00000148
#define CHIPREG_INT_ENABLE5_TYPE                                          UInt32
#define CHIPREG_INT_ENABLE5_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_ENABLE5_MDM_IMR5N_SHIFT                            0
#define    CHIPREG_INT_ENABLE5_MDM_IMR5N_MASK                             0xFFFFFFFF

#define CHIPREG_INT_ENABLE6_OFFSET                                        0x0000014C
#define CHIPREG_INT_ENABLE6_TYPE                                          UInt32
#define CHIPREG_INT_ENABLE6_RESERVED_MASK                                 0x00000000
#define    CHIPREG_INT_ENABLE6_MDM_IMR6N_SHIFT                            0
#define    CHIPREG_INT_ENABLE6_MDM_IMR6N_MASK                             0xFFFFFFFF

#define CHIPREG_INT_MASKED_STATUS0_OFFSET                                 0x00000150
#define CHIPREG_INT_MASKED_STATUS0_TYPE                                   UInt32
#define CHIPREG_INT_MASKED_STATUS0_RESERVED_MASK                          0x00000000
#define    CHIPREG_INT_MASKED_STATUS0_MDM_IMSR0N_SHIFT                    0
#define    CHIPREG_INT_MASKED_STATUS0_MDM_IMSR0N_MASK                     0xFFFFFFFF

#define CHIPREG_INT_MASKED_STATUS1_OFFSET                                 0x00000154
#define CHIPREG_INT_MASKED_STATUS1_TYPE                                   UInt32
#define CHIPREG_INT_MASKED_STATUS1_RESERVED_MASK                          0x00000000
#define    CHIPREG_INT_MASKED_STATUS1_MDM_IMSR1N_SHIFT                    0
#define    CHIPREG_INT_MASKED_STATUS1_MDM_IMSR1N_MASK                     0xFFFFFFFF

#define CHIPREG_INT_MASKED_STATUS2_OFFSET                                 0x00000158
#define CHIPREG_INT_MASKED_STATUS2_TYPE                                   UInt32
#define CHIPREG_INT_MASKED_STATUS2_RESERVED_MASK                          0x00000000
#define    CHIPREG_INT_MASKED_STATUS2_MDM_IMSR2N_SHIFT                    0
#define    CHIPREG_INT_MASKED_STATUS2_MDM_IMSR2N_MASK                     0xFFFFFFFF

#define CHIPREG_INT_MASKED_STATUS3_OFFSET                                 0x0000015C
#define CHIPREG_INT_MASKED_STATUS3_TYPE                                   UInt32
#define CHIPREG_INT_MASKED_STATUS3_RESERVED_MASK                          0x00000000
#define    CHIPREG_INT_MASKED_STATUS3_MDM_IMSR3N_SHIFT                    0
#define    CHIPREG_INT_MASKED_STATUS3_MDM_IMSR3N_MASK                     0xFFFFFFFF

#define CHIPREG_INT_MASKED_STATUS4_OFFSET                                 0x00000160
#define CHIPREG_INT_MASKED_STATUS4_TYPE                                   UInt32
#define CHIPREG_INT_MASKED_STATUS4_RESERVED_MASK                          0x00000000
#define    CHIPREG_INT_MASKED_STATUS4_MDM_IMSR4N_SHIFT                    0
#define    CHIPREG_INT_MASKED_STATUS4_MDM_IMSR4N_MASK                     0xFFFFFFFF

#define CHIPREG_INT_MASKED_STATUS5_OFFSET                                 0x00000164
#define CHIPREG_INT_MASKED_STATUS5_TYPE                                   UInt32
#define CHIPREG_INT_MASKED_STATUS5_RESERVED_MASK                          0x00000000
#define    CHIPREG_INT_MASKED_STATUS5_MDM_IMSR5N_SHIFT                    0
#define    CHIPREG_INT_MASKED_STATUS5_MDM_IMSR5N_MASK                     0xFFFFFFFF

#define CHIPREG_INT_MASKED_STATUS6_OFFSET                                 0x00000168
#define CHIPREG_INT_MASKED_STATUS6_TYPE                                   UInt32
#define CHIPREG_INT_MASKED_STATUS6_RESERVED_MASK                          0x00000000
#define    CHIPREG_INT_MASKED_STATUS6_MDM_IMSR6N_SHIFT                    0
#define    CHIPREG_INT_MASKED_STATUS6_MDM_IMSR6N_MASK                     0xFFFFFFFF

#define CHIPREG_DDR23_MODE_CONTROL_OFFSET                                 0x0000016C
#define CHIPREG_DDR23_MODE_CONTROL_TYPE                                   UInt32
#define CHIPREG_DDR23_MODE_CONTROL_RESERVED_MASK                          0xFFFFFFF8
#define    CHIPREG_DDR23_MODE_CONTROL_DDR23_MODE_SHIFT                    2
#define    CHIPREG_DDR23_MODE_CONTROL_DDR23_MODE_MASK                     0x00000004
#define    CHIPREG_DDR23_MODE_CONTROL_DDR23_MODE_OVERRIDE_SHIFT           1
#define    CHIPREG_DDR23_MODE_CONTROL_DDR23_MODE_OVERRIDE_MASK            0x00000002
#define    CHIPREG_DDR23_MODE_CONTROL_DDR23_MODE_OVERRIDE_VALUE_SHIFT     0
#define    CHIPREG_DDR23_MODE_CONTROL_DDR23_MODE_OVERRIDE_VALUE_MASK      0x00000001

#define CHIPREG_BOOT_2ND_ADDR_OFFSET                                      0x0000017C
#define CHIPREG_BOOT_2ND_ADDR_TYPE                                        UInt32
#define CHIPREG_BOOT_2ND_ADDR_RESERVED_MASK                               0x00000000
#define    CHIPREG_BOOT_2ND_ADDR_A9_CORE_BOOT_2ND_ADDR_SHIFT              2
#define    CHIPREG_BOOT_2ND_ADDR_A9_CORE_BOOT_2ND_ADDR_MASK               0xFFFFFFFC
#define    CHIPREG_BOOT_2ND_ADDR_A9_NON_PRIMARY_CORE_ENABLE_SHIFT         0
#define    CHIPREG_BOOT_2ND_ADDR_A9_NON_PRIMARY_CORE_ENABLE_MASK          0x00000003

#define CHIPREG_CORE0_SEMAPHORE_STS_OFFSET                                0x00000180
#define CHIPREG_CORE0_SEMAPHORE_STS_TYPE                                  UInt32
#define CHIPREG_CORE0_SEMAPHORE_STS_RESERVED_MASK                         0x00000000
#define    CHIPREG_CORE0_SEMAPHORE_STS_CORE0_SEMAPHORE_STATUS_SHIFT       0
#define    CHIPREG_CORE0_SEMAPHORE_STS_CORE0_SEMAPHORE_STATUS_MASK        0xFFFFFFFF

#define CHIPREG_CORE0_SEMAPHORE_L_OFFSET                                  0x00000184
#define CHIPREG_CORE0_SEMAPHORE_L_TYPE                                    UInt32
#define CHIPREG_CORE0_SEMAPHORE_L_RESERVED_MASK                           0x00000000
#define    CHIPREG_CORE0_SEMAPHORE_L_CORE0_SEMAPHORE_LOCK_SHIFT           0
#define    CHIPREG_CORE0_SEMAPHORE_L_CORE0_SEMAPHORE_LOCK_MASK            0xFFFFFFFF

#define CHIPREG_CORE0_SEMAPHORE_UL_OFFSET                                 0x00000188
#define CHIPREG_CORE0_SEMAPHORE_UL_TYPE                                   UInt32
#define CHIPREG_CORE0_SEMAPHORE_UL_RESERVED_MASK                          0x00000000
#define    CHIPREG_CORE0_SEMAPHORE_UL_CORE0_SEMAPHORE_UNLOCK_SHIFT        0
#define    CHIPREG_CORE0_SEMAPHORE_UL_CORE0_SEMAPHORE_UNLOCK_MASK         0xFFFFFFFF

#define CHIPREG_CORE0_RAW_SEMAPHORE_ST_OFFSET                             0x0000018C
#define CHIPREG_CORE0_RAW_SEMAPHORE_ST_TYPE                               UInt32
#define CHIPREG_CORE0_RAW_SEMAPHORE_ST_RESERVED_MASK                      0x00000000
#define    CHIPREG_CORE0_RAW_SEMAPHORE_ST_CORE0_RAW_SEMAPHORE_STATUS_SHIFT 0
#define    CHIPREG_CORE0_RAW_SEMAPHORE_ST_CORE0_RAW_SEMAPHORE_STATUS_MASK 0xFFFFFFFF

#define CHIPREG_CORE1_SEMAPHORE_STS_OFFSET                                0x00000190
#define CHIPREG_CORE1_SEMAPHORE_STS_TYPE                                  UInt32
#define CHIPREG_CORE1_SEMAPHORE_STS_RESERVED_MASK                         0x00000000
#define    CHIPREG_CORE1_SEMAPHORE_STS_CORE1_SEMAPHORE_STATUS_SHIFT       0
#define    CHIPREG_CORE1_SEMAPHORE_STS_CORE1_SEMAPHORE_STATUS_MASK        0xFFFFFFFF

#define CHIPREG_CORE1_SEMAPHORE_L_OFFSET                                  0x00000194
#define CHIPREG_CORE1_SEMAPHORE_L_TYPE                                    UInt32
#define CHIPREG_CORE1_SEMAPHORE_L_RESERVED_MASK                           0x00000000
#define    CHIPREG_CORE1_SEMAPHORE_L_CORE1_SEMAPHORE_LOCK_SHIFT           0
#define    CHIPREG_CORE1_SEMAPHORE_L_CORE1_SEMAPHORE_LOCK_MASK            0xFFFFFFFF

#define CHIPREG_CORE1_SEMAPHORE_UL_OFFSET                                 0x00000198
#define CHIPREG_CORE1_SEMAPHORE_UL_TYPE                                   UInt32
#define CHIPREG_CORE1_SEMAPHORE_UL_RESERVED_MASK                          0x00000000
#define    CHIPREG_CORE1_SEMAPHORE_UL_CORE1_SEMAPHORE_UNLOCK_SHIFT        0
#define    CHIPREG_CORE1_SEMAPHORE_UL_CORE1_SEMAPHORE_UNLOCK_MASK         0xFFFFFFFF

#define CHIPREG_CORE1_RAW_SEMAPHORE_ST_OFFSET                             0x0000019C
#define CHIPREG_CORE1_RAW_SEMAPHORE_ST_TYPE                               UInt32
#define CHIPREG_CORE1_RAW_SEMAPHORE_ST_RESERVED_MASK                      0x00000000
#define    CHIPREG_CORE1_RAW_SEMAPHORE_ST_CORE1_RAW_SEMAPHORE_STATUS_SHIFT 0
#define    CHIPREG_CORE1_RAW_SEMAPHORE_ST_CORE1_RAW_SEMAPHORE_STATUS_MASK 0xFFFFFFFF

#define CHIPREG_CORE2_SEMAPHORE_STS_OFFSET                                0x000001A0
#define CHIPREG_CORE2_SEMAPHORE_STS_TYPE                                  UInt32
#define CHIPREG_CORE2_SEMAPHORE_STS_RESERVED_MASK                         0x00000000
#define    CHIPREG_CORE2_SEMAPHORE_STS_CORE2_SEMAPHORE_STATUS_SHIFT       0
#define    CHIPREG_CORE2_SEMAPHORE_STS_CORE2_SEMAPHORE_STATUS_MASK        0xFFFFFFFF

#define CHIPREG_CORE2_SEMAPHORE_L_OFFSET                                  0x000001A4
#define CHIPREG_CORE2_SEMAPHORE_L_TYPE                                    UInt32
#define CHIPREG_CORE2_SEMAPHORE_L_RESERVED_MASK                           0x00000000
#define    CHIPREG_CORE2_SEMAPHORE_L_CORE2_SEMAPHORE_LOCK_SHIFT           0
#define    CHIPREG_CORE2_SEMAPHORE_L_CORE2_SEMAPHORE_LOCK_MASK            0xFFFFFFFF

#define CHIPREG_CORE2_SEMAPHORE_UL_OFFSET                                 0x000001A8
#define CHIPREG_CORE2_SEMAPHORE_UL_TYPE                                   UInt32
#define CHIPREG_CORE2_SEMAPHORE_UL_RESERVED_MASK                          0x00000000
#define    CHIPREG_CORE2_SEMAPHORE_UL_CORE2_SEMAPHORE_UNLOCK_SHIFT        0
#define    CHIPREG_CORE2_SEMAPHORE_UL_CORE2_SEMAPHORE_UNLOCK_MASK         0xFFFFFFFF

#define CHIPREG_CORE2_RAW_SEMAPHORE_ST_OFFSET                             0x000001AC
#define CHIPREG_CORE2_RAW_SEMAPHORE_ST_TYPE                               UInt32
#define CHIPREG_CORE2_RAW_SEMAPHORE_ST_RESERVED_MASK                      0x00000000
#define    CHIPREG_CORE2_RAW_SEMAPHORE_ST_CORE2_RAW_SEMAPHORE_STATUS_SHIFT 0
#define    CHIPREG_CORE2_RAW_SEMAPHORE_ST_CORE2_RAW_SEMAPHORE_STATUS_MASK 0xFFFFFFFF

#define CHIPREG_CORE3_SEMAPHORE_STS_OFFSET                                0x000001B0
#define CHIPREG_CORE3_SEMAPHORE_STS_TYPE                                  UInt32
#define CHIPREG_CORE3_SEMAPHORE_STS_RESERVED_MASK                         0x00000000
#define    CHIPREG_CORE3_SEMAPHORE_STS_CORE3_SEMAPHORE_STATUS_SHIFT       0
#define    CHIPREG_CORE3_SEMAPHORE_STS_CORE3_SEMAPHORE_STATUS_MASK        0xFFFFFFFF

#define CHIPREG_CORE3_SEMAPHORE_L_OFFSET                                  0x000001B4
#define CHIPREG_CORE3_SEMAPHORE_L_TYPE                                    UInt32
#define CHIPREG_CORE3_SEMAPHORE_L_RESERVED_MASK                           0x00000000
#define    CHIPREG_CORE3_SEMAPHORE_L_CORE3_SEMAPHORE_LOCK_SHIFT           0
#define    CHIPREG_CORE3_SEMAPHORE_L_CORE3_SEMAPHORE_LOCK_MASK            0xFFFFFFFF

#define CHIPREG_CORE3_SEMAPHORE_UL_OFFSET                                 0x000001B8
#define CHIPREG_CORE3_SEMAPHORE_UL_TYPE                                   UInt32
#define CHIPREG_CORE3_SEMAPHORE_UL_RESERVED_MASK                          0x00000000
#define    CHIPREG_CORE3_SEMAPHORE_UL_CORE3_SEMAPHORE_UNLOCK_SHIFT        0
#define    CHIPREG_CORE3_SEMAPHORE_UL_CORE3_SEMAPHORE_UNLOCK_MASK         0xFFFFFFFF

#define CHIPREG_CORE3_RAW_SEMAPHORE_ST_OFFSET                             0x000001BC
#define CHIPREG_CORE3_RAW_SEMAPHORE_ST_TYPE                               UInt32
#define CHIPREG_CORE3_RAW_SEMAPHORE_ST_RESERVED_MASK                      0x00000000
#define    CHIPREG_CORE3_RAW_SEMAPHORE_ST_CORE3_RAW_SEMAPHORE_STATUS_SHIFT 0
#define    CHIPREG_CORE3_RAW_SEMAPHORE_ST_CORE3_RAW_SEMAPHORE_STATUS_MASK 0xFFFFFFFF

#define CHIPREG_CORE4_SEMAPHORE_STS_OFFSET                                0x000001C0
#define CHIPREG_CORE4_SEMAPHORE_STS_TYPE                                  UInt32
#define CHIPREG_CORE4_SEMAPHORE_STS_RESERVED_MASK                         0x00000000
#define    CHIPREG_CORE4_SEMAPHORE_STS_CORE4_SEMAPHORE_STATUS_SHIFT       0
#define    CHIPREG_CORE4_SEMAPHORE_STS_CORE4_SEMAPHORE_STATUS_MASK        0xFFFFFFFF

#define CHIPREG_CORE4_SEMAPHORE_L_OFFSET                                  0x000001C4
#define CHIPREG_CORE4_SEMAPHORE_L_TYPE                                    UInt32
#define CHIPREG_CORE4_SEMAPHORE_L_RESERVED_MASK                           0x00000000
#define    CHIPREG_CORE4_SEMAPHORE_L_CORE4_SEMAPHORE_LOCK_SHIFT           0
#define    CHIPREG_CORE4_SEMAPHORE_L_CORE4_SEMAPHORE_LOCK_MASK            0xFFFFFFFF

#define CHIPREG_CORE4_SEMAPHORE_UL_OFFSET                                 0x000001C8
#define CHIPREG_CORE4_SEMAPHORE_UL_TYPE                                   UInt32
#define CHIPREG_CORE4_SEMAPHORE_UL_RESERVED_MASK                          0x00000000
#define    CHIPREG_CORE4_SEMAPHORE_UL_CORE4_SEMAPHORE_UNLOCK_SHIFT        0
#define    CHIPREG_CORE4_SEMAPHORE_UL_CORE4_SEMAPHORE_UNLOCK_MASK         0xFFFFFFFF

#define CHIPREG_CORE4_RAW_SEMAPHORE_ST_OFFSET                             0x000001CC
#define CHIPREG_CORE4_RAW_SEMAPHORE_ST_TYPE                               UInt32
#define CHIPREG_CORE4_RAW_SEMAPHORE_ST_RESERVED_MASK                      0x00000000
#define    CHIPREG_CORE4_RAW_SEMAPHORE_ST_CORE4_RAW_SEMAPHORE_STATUS_SHIFT 0
#define    CHIPREG_CORE4_RAW_SEMAPHORE_ST_CORE4_RAW_SEMAPHORE_STATUS_MASK 0xFFFFFFFF

#define CHIPREG_CORE5_SEMAPHORE_STS_OFFSET                                0x000001D0
#define CHIPREG_CORE5_SEMAPHORE_STS_TYPE                                  UInt32
#define CHIPREG_CORE5_SEMAPHORE_STS_RESERVED_MASK                         0x00000000
#define    CHIPREG_CORE5_SEMAPHORE_STS_CORE5_SEMAPHORE_STATUS_SHIFT       0
#define    CHIPREG_CORE5_SEMAPHORE_STS_CORE5_SEMAPHORE_STATUS_MASK        0xFFFFFFFF

#define CHIPREG_CORE5_SEMAPHORE_L_OFFSET                                  0x000001D4
#define CHIPREG_CORE5_SEMAPHORE_L_TYPE                                    UInt32
#define CHIPREG_CORE5_SEMAPHORE_L_RESERVED_MASK                           0x00000000
#define    CHIPREG_CORE5_SEMAPHORE_L_CORE5_SEMAPHORE_LOCK_SHIFT           0
#define    CHIPREG_CORE5_SEMAPHORE_L_CORE5_SEMAPHORE_LOCK_MASK            0xFFFFFFFF

#define CHIPREG_CORE5_SEMAPHORE_UL_OFFSET                                 0x000001D8
#define CHIPREG_CORE5_SEMAPHORE_UL_TYPE                                   UInt32
#define CHIPREG_CORE5_SEMAPHORE_UL_RESERVED_MASK                          0x00000000
#define    CHIPREG_CORE5_SEMAPHORE_UL_CORE5_SEMAPHORE_UNLOCK_SHIFT        0
#define    CHIPREG_CORE5_SEMAPHORE_UL_CORE5_SEMAPHORE_UNLOCK_MASK         0xFFFFFFFF

#define CHIPREG_CORE5_RAW_SEMAPHORE_ST_OFFSET                             0x000001DC
#define CHIPREG_CORE5_RAW_SEMAPHORE_ST_TYPE                               UInt32
#define CHIPREG_CORE5_RAW_SEMAPHORE_ST_RESERVED_MASK                      0x00000000
#define    CHIPREG_CORE5_RAW_SEMAPHORE_ST_CORE5_RAW_SEMAPHORE_STATUS_SHIFT 0
#define    CHIPREG_CORE5_RAW_SEMAPHORE_ST_CORE5_RAW_SEMAPHORE_STATUS_MASK 0xFFFFFFFF

#define CHIPREG_CORE6_SEMAPHORE_STS_OFFSET                                0x000001E0
#define CHIPREG_CORE6_SEMAPHORE_STS_TYPE                                  UInt32
#define CHIPREG_CORE6_SEMAPHORE_STS_RESERVED_MASK                         0x00000000
#define    CHIPREG_CORE6_SEMAPHORE_STS_CORE6_SEMAPHORE_STATUS_SHIFT       0
#define    CHIPREG_CORE6_SEMAPHORE_STS_CORE6_SEMAPHORE_STATUS_MASK        0xFFFFFFFF

#define CHIPREG_CORE6_SEMAPHORE_L_OFFSET                                  0x000001E4
#define CHIPREG_CORE6_SEMAPHORE_L_TYPE                                    UInt32
#define CHIPREG_CORE6_SEMAPHORE_L_RESERVED_MASK                           0x00000000
#define    CHIPREG_CORE6_SEMAPHORE_L_CORE6_SEMAPHORE_LOCK_SHIFT           0
#define    CHIPREG_CORE6_SEMAPHORE_L_CORE6_SEMAPHORE_LOCK_MASK            0xFFFFFFFF

#define CHIPREG_CORE6_SEMAPHORE_UL_OFFSET                                 0x000001E8
#define CHIPREG_CORE6_SEMAPHORE_UL_TYPE                                   UInt32
#define CHIPREG_CORE6_SEMAPHORE_UL_RESERVED_MASK                          0x00000000
#define    CHIPREG_CORE6_SEMAPHORE_UL_CORE6_SEMAPHORE_UNLOCK_SHIFT        0
#define    CHIPREG_CORE6_SEMAPHORE_UL_CORE6_SEMAPHORE_UNLOCK_MASK         0xFFFFFFFF

#define CHIPREG_CORE6_RAW_SEMAPHORE_ST_OFFSET                             0x000001EC
#define CHIPREG_CORE6_RAW_SEMAPHORE_ST_TYPE                               UInt32
#define CHIPREG_CORE6_RAW_SEMAPHORE_ST_RESERVED_MASK                      0x00000000
#define    CHIPREG_CORE6_RAW_SEMAPHORE_ST_CORE6_RAW_SEMAPHORE_STATUS_SHIFT 0
#define    CHIPREG_CORE6_RAW_SEMAPHORE_ST_CORE6_RAW_SEMAPHORE_STATUS_MASK 0xFFFFFFFF

#define CHIPREG_CORE7_SEMAPHORE_STS_OFFSET                                0x000001F0
#define CHIPREG_CORE7_SEMAPHORE_STS_TYPE                                  UInt32
#define CHIPREG_CORE7_SEMAPHORE_STS_RESERVED_MASK                         0x00000000
#define    CHIPREG_CORE7_SEMAPHORE_STS_CORE7_SEMAPHORE_STATUS_SHIFT       0
#define    CHIPREG_CORE7_SEMAPHORE_STS_CORE7_SEMAPHORE_STATUS_MASK        0xFFFFFFFF

#define CHIPREG_CORE7_SEMAPHORE_L_OFFSET                                  0x000001F4
#define CHIPREG_CORE7_SEMAPHORE_L_TYPE                                    UInt32
#define CHIPREG_CORE7_SEMAPHORE_L_RESERVED_MASK                           0x00000000
#define    CHIPREG_CORE7_SEMAPHORE_L_CORE7_SEMAPHORE_LOCK_SHIFT           0
#define    CHIPREG_CORE7_SEMAPHORE_L_CORE7_SEMAPHORE_LOCK_MASK            0xFFFFFFFF

#define CHIPREG_CORE7_SEMAPHORE_UL_OFFSET                                 0x000001F8
#define CHIPREG_CORE7_SEMAPHORE_UL_TYPE                                   UInt32
#define CHIPREG_CORE7_SEMAPHORE_UL_RESERVED_MASK                          0x00000000
#define    CHIPREG_CORE7_SEMAPHORE_UL_CORE7_SEMAPHORE_UNLOCK_SHIFT        0
#define    CHIPREG_CORE7_SEMAPHORE_UL_CORE7_SEMAPHORE_UNLOCK_MASK         0xFFFFFFFF

#define CHIPREG_CORE7_RAW_SEMAPHORE_ST_OFFSET                             0x000001FC
#define CHIPREG_CORE7_RAW_SEMAPHORE_ST_TYPE                               UInt32
#define CHIPREG_CORE7_RAW_SEMAPHORE_ST_RESERVED_MASK                      0x00000000
#define    CHIPREG_CORE7_RAW_SEMAPHORE_ST_CORE7_RAW_SEMAPHORE_STATUS_SHIFT 0
#define    CHIPREG_CORE7_RAW_SEMAPHORE_ST_CORE7_RAW_SEMAPHORE_STATUS_MASK 0xFFFFFFFF

#endif /* __BRCM_RDB_CHIPREG_H__ */


