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

#ifndef __BRCM_RDB_HSIC_PHY_H__
#define __BRCM_RDB_HSIC_PHY_H__

#define HSIC_PHY_CLKRST_CTRL_OFFSET                                       0x00000000
#define HSIC_PHY_CLKRST_CTRL_TYPE                                         UInt32
#define HSIC_PHY_CLKRST_CTRL_RESERVED_MASK                                0xFFFFF000
#define    HSIC_PHY_CLKRST_CTRL_SW_OHCI_ACCESS_EN_SHIFT                   11
#define    HSIC_PHY_CLKRST_CTRL_SW_OHCI_ACCESS_EN_MASK                    0x00000800
#define    HSIC_PHY_CLKRST_CTRL_MASK_PHY_RESETS_SHIFT                     10
#define    HSIC_PHY_CLKRST_CTRL_MASK_PHY_RESETS_MASK                      0x00000400
#define    HSIC_PHY_CLKRST_CTRL_AUTO_STOP_CLK12_48_SHIFT                  9
#define    HSIC_PHY_CLKRST_CTRL_AUTO_STOP_CLK12_48_MASK                   0x00000200
#define    HSIC_PHY_CLKRST_CTRL_CLK12_STOP_VAL_SHIFT                      4
#define    HSIC_PHY_CLKRST_CTRL_CLK12_STOP_VAL_MASK                       0x000001F0
#define    HSIC_PHY_CLKRST_CTRL_CLK48_REQ_SHIFT                           3
#define    HSIC_PHY_CLKRST_CTRL_CLK48_REQ_MASK                            0x00000008
#define    HSIC_PHY_CLKRST_CTRL_UTMIRESETN_SW_SHIFT                       1
#define    HSIC_PHY_CLKRST_CTRL_UTMIRESETN_SW_MASK                        0x00000006
#define    HSIC_PHY_CLKRST_CTRL_RESETN_SW_SHIFT                           0
#define    HSIC_PHY_CLKRST_CTRL_RESETN_SW_MASK                            0x00000001

#define HSIC_PHY_CORE_STRAP_CTRL_OFFSET                                   0x00000004
#define HSIC_PHY_CORE_STRAP_CTRL_TYPE                                     UInt32
#define HSIC_PHY_CORE_STRAP_CTRL_RESERVED_MASK                            0xFFFFE000
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_UTMI_BACKWARD_ENB_I_SHIFT          12
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_UTMI_BACKWARD_ENB_I_MASK           0x00001000
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_RESUME_UTMI_PLS_DIS_I_SHIFT        11
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_RESUME_UTMI_PLS_DIS_I_MASK         0x00000800
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_HUBSETUP_MIN_SHIFT                 10
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_HUBSETUP_MIN_MASK                  0x00000400
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_AUTOPPD_ON_OVERCUR_EN_SHIFT        9
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_AUTOPPD_ON_OVERCUR_EN_MASK         0x00000200
#define    HSIC_PHY_CORE_STRAP_CTRL_APP_START_CLK_SHIFT                   8
#define    HSIC_PHY_CORE_STRAP_CTRL_APP_START_CLK_MASK                    0x00000100
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_POWER_STATE_VALID_I_SHIFT          7
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_POWER_STATE_VALID_I_MASK           0x00000080
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_SIMULATION_MODE_I_SHIFT            6
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_SIMULATION_MODE_I_MASK             0x00000040
#define    HSIC_PHY_CORE_STRAP_CTRL_OHCI_0_CNTSEL_I_N_SHIFT               5
#define    HSIC_PHY_CORE_STRAP_CTRL_OHCI_0_CNTSEL_I_N_MASK                0x00000020
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_NXT_POWER_STATE_VALID_I_SHIFT      4
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_NXT_POWER_STATE_VALID_I_MASK       0x00000010
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_NEXT_POWER_STATE_I_SHIFT           2
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_NEXT_POWER_STATE_I_MASK            0x0000000C
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_POWER_STATE_I_SHIFT                0
#define    HSIC_PHY_CORE_STRAP_CTRL_SS_POWER_STATE_I_MASK                 0x00000003

#define HSIC_PHY_SS_FLADJ_VAL_HOST_I_OFFSET                               0x00000008
#define HSIC_PHY_SS_FLADJ_VAL_HOST_I_TYPE                                 UInt32
#define HSIC_PHY_SS_FLADJ_VAL_HOST_I_RESERVED_MASK                        0xFFFFFFC0
#define    HSIC_PHY_SS_FLADJ_VAL_HOST_I_SS_FLADJ_VAL_HOST_I_SHIFT         0
#define    HSIC_PHY_SS_FLADJ_VAL_HOST_I_SS_FLADJ_VAL_HOST_I_MASK          0x0000003F

#define HSIC_PHY_SS_FLADJ_VAL_1_OFFSET                                    0x0000000C
#define HSIC_PHY_SS_FLADJ_VAL_1_TYPE                                      UInt32
#define HSIC_PHY_SS_FLADJ_VAL_1_RESERVED_MASK                             0xFFFFFFC0
#define    HSIC_PHY_SS_FLADJ_VAL_1_SS_FLADJ_VAL_SHIFT                     0
#define    HSIC_PHY_SS_FLADJ_VAL_1_SS_FLADJ_VAL_MASK                      0x0000003F

#define HSIC_PHY_SS_FLADJ_VAL_2_OFFSET                                    0x00000010
#define HSIC_PHY_SS_FLADJ_VAL_2_TYPE                                      UInt32
#define HSIC_PHY_SS_FLADJ_VAL_2_RESERVED_MASK                             0xFFFFFFC0
#define    HSIC_PHY_SS_FLADJ_VAL_2_SS_FLADJ_VAL_SHIFT                     0
#define    HSIC_PHY_SS_FLADJ_VAL_2_SS_FLADJ_VAL_MASK                      0x0000003F

#define HSIC_PHY_HSIC_LDO_CTRL_OFFSET                                     0x00000014
#define HSIC_PHY_HSIC_LDO_CTRL_TYPE                                       UInt32
#define HSIC_PHY_HSIC_LDO_CTRL_RESERVED_MASK                              0xFFF80000
#define    HSIC_PHY_HSIC_LDO_CTRL_LDO_EN_SHIFT                            18
#define    HSIC_PHY_HSIC_LDO_CTRL_LDO_EN_MASK                             0x00040000
#define    HSIC_PHY_HSIC_LDO_CTRL_LDOCNTLEN_SHIFT                         17
#define    HSIC_PHY_HSIC_LDO_CTRL_LDOCNTLEN_MASK                          0x00020000
#define    HSIC_PHY_HSIC_LDO_CTRL_LDOCNTL_SHIFT                           0
#define    HSIC_PHY_HSIC_LDO_CTRL_LDOCNTL_MASK                            0x0001FFFF

#define HSIC_PHY_HSIC_PHY_CTRL_OFFSET                                     0x00000018
#define HSIC_PHY_HSIC_PHY_CTRL_TYPE                                       UInt32
#define HSIC_PHY_HSIC_PHY_CTRL_RESERVED_MASK                              0xFFFFFE00
#define    HSIC_PHY_HSIC_PHY_CTRL_PHY_ISO_SHIFT                           8
#define    HSIC_PHY_HSIC_PHY_CTRL_PHY_ISO_MASK                            0x00000100
#define    HSIC_PHY_HSIC_PHY_CTRL_UTMI_PWRDNB_SHIFT                       6
#define    HSIC_PHY_HSIC_PHY_CTRL_UTMI_PWRDNB_MASK                        0x000000C0
#define    HSIC_PHY_HSIC_PHY_CTRL_PHY_PWRDNB_SHIFT                        4
#define    HSIC_PHY_HSIC_PHY_CTRL_PHY_PWRDNB_MASK                         0x00000030
#define    HSIC_PHY_HSIC_PHY_CTRL_NON_DRIVING_SHIFT                       3
#define    HSIC_PHY_HSIC_PHY_CTRL_NON_DRIVING_MASK                        0x00000008
#define    HSIC_PHY_HSIC_PHY_CTRL_SOFT_RESETB_SHIFT                       1
#define    HSIC_PHY_HSIC_PHY_CTRL_SOFT_RESETB_MASK                        0x00000006
#define    HSIC_PHY_HSIC_PHY_CTRL_RESETB_SHIFT                            0
#define    HSIC_PHY_HSIC_PHY_CTRL_RESETB_MASK                             0x00000001

#define HSIC_PHY_HSIC_CFG1_OFFSET                                         0x0000001C
#define HSIC_PHY_HSIC_CFG1_TYPE                                           UInt32
#define HSIC_PHY_HSIC_CFG1_RESERVED_MASK                                  0xFF800000
#define    HSIC_PHY_HSIC_CFG1_EB_IDLE_DET_SHIFT                           22
#define    HSIC_PHY_HSIC_CFG1_EB_IDLE_DET_MASK                            0x00400000
#define    HSIC_PHY_HSIC_CFG1_EXTEND_ENDOFIDLE_SHIFT                      21
#define    HSIC_PHY_HSIC_CFG1_EXTEND_ENDOFIDLE_MASK                       0x00200000
#define    HSIC_PHY_HSIC_CFG1_CON_LINESTATE_1CYC_SHIFT                    20
#define    HSIC_PHY_HSIC_CFG1_CON_LINESTATE_1CYC_MASK                     0x00100000
#define    HSIC_PHY_HSIC_CFG1_DELAY_MONCON_EN_SHIFT                       19
#define    HSIC_PHY_HSIC_CFG1_DELAY_MONCON_EN_MASK                        0x00080000
#define    HSIC_PHY_HSIC_CFG1_CONFSM_SRST_N_SHIFT                         18
#define    HSIC_PHY_HSIC_CFG1_CONFSM_SRST_N_MASK                          0x00040000
#define    HSIC_PHY_HSIC_CFG1_PAD_SLEW_SHIFT                              16
#define    HSIC_PHY_HSIC_CFG1_PAD_SLEW_MASK                               0x00030000
#define    HSIC_PHY_HSIC_CFG1_ENABLE_BUS_KEEPERS_SHIFT                    15
#define    HSIC_PHY_HSIC_CFG1_ENABLE_BUS_KEEPERS_MASK                     0x00008000
#define    HSIC_PHY_HSIC_CFG1_CONNECT_SIG_LEN_SHIFT                       13
#define    HSIC_PHY_HSIC_CFG1_CONNECT_SIG_LEN_MASK                        0x00006000
#define    HSIC_PHY_HSIC_CFG1_RX_ERROR_MASK_SHIFT                         11
#define    HSIC_PHY_HSIC_CFG1_RX_ERROR_MASK_MASK                          0x00001800
#define    HSIC_PHY_HSIC_CFG1_DEV_LINESTATE_SE0_SHIFT                     10
#define    HSIC_PHY_HSIC_CFG1_DEV_LINESTATE_SE0_MASK                      0x00000400
#define    HSIC_PHY_HSIC_CFG1_LINESTATE_BYPASS_SHIFT                      9
#define    HSIC_PHY_HSIC_CFG1_LINESTATE_BYPASS_MASK                       0x00000200
#define    HSIC_PHY_HSIC_CFG1_HSIC_ENABLE_SHIFT                           8
#define    HSIC_PHY_HSIC_CFG1_HSIC_ENABLE_MASK                            0x00000100
#define    HSIC_PHY_HSIC_CFG1_PLL_SUSPEND_EN_SHIFT                        7
#define    HSIC_PHY_HSIC_CFG1_PLL_SUSPEND_EN_MASK                         0x00000080
#define    HSIC_PHY_HSIC_CFG1_UTMI_TXBITSTUFF_ENABLE_SHIFT                6
#define    HSIC_PHY_HSIC_CFG1_UTMI_TXBITSTUFF_ENABLE_MASK                 0x00000040
#define    HSIC_PHY_HSIC_CFG1_ISSUE_DEVICE_CONNECT_SHIFT                  5
#define    HSIC_PHY_HSIC_CFG1_ISSUE_DEVICE_CONNECT_MASK                   0x00000020
#define    HSIC_PHY_HSIC_CFG1_DRIVE_SE0_SHIFT                             4
#define    HSIC_PHY_HSIC_CFG1_DRIVE_SE0_MASK                              0x00000010
#define    HSIC_PHY_HSIC_CFG1_ASSERT_IDLE_SHIFT                           3
#define    HSIC_PHY_HSIC_CFG1_ASSERT_IDLE_MASK                            0x00000008
#define    HSIC_PHY_HSIC_CFG1_ASSERT_SE0_SHIFT                            2
#define    HSIC_PHY_HSIC_CFG1_ASSERT_SE0_MASK                             0x00000004
#define    HSIC_PHY_HSIC_CFG1_CONNECT_FSM_EN_SHIFT                        1
#define    HSIC_PHY_HSIC_CFG1_CONNECT_FSM_EN_MASK                         0x00000002
#define    HSIC_PHY_HSIC_CFG1_DIG_PLL_LOCK_EN_SHIFT                       0
#define    HSIC_PHY_HSIC_CFG1_DIG_PLL_LOCK_EN_MASK                        0x00000001

#define HSIC_PHY_HSIC_CFG2_OFFSET                                         0x00000020
#define HSIC_PHY_HSIC_CFG2_TYPE                                           UInt32
#define HSIC_PHY_HSIC_CFG2_RESERVED_MASK                                  0xFFFF0000
#define    HSIC_PHY_HSIC_CFG2_PVT_CTRL_SHIFT                              0
#define    HSIC_PHY_HSIC_CFG2_PVT_CTRL_MASK                               0x0000FFFF

#define HSIC_PHY_HSIC_PLL_CTRL_OFFSET                                     0x00000024
#define HSIC_PHY_HSIC_PLL_CTRL_TYPE                                       UInt32
#define HSIC_PHY_HSIC_PLL_CTRL_RESERVED_MASK                              0xFC000000
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_LOCK_SHIFT                          25
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_LOCK_MASK                           0x02000000
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_NDIV_INT_SHIFT                      15
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_NDIV_INT_MASK                       0x01FF8000
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_P1DIV_SHIFT                         12
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_P1DIV_MASK                          0x00007000
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_KA_SHIFT                            9
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_KA_MASK                             0x00000E00
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_KI_SHIFT                            6
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_KI_MASK                             0x000001C0
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_KP_SHIFT                            2
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_KP_MASK                             0x0000003C
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_POWERDOWNB_SHIFT                    1
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_POWERDOWNB_MASK                     0x00000002
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_RESET_SHIFT                         0
#define    HSIC_PHY_HSIC_PLL_CTRL_PLL_RESET_MASK                          0x00000001

#define HSIC_PHY_HSIC_PLL_NDIV_FRAC_OFFSET                                0x00000028
#define HSIC_PHY_HSIC_PLL_NDIV_FRAC_TYPE                                  UInt32
#define HSIC_PHY_HSIC_PLL_NDIV_FRAC_RESERVED_MASK                         0xFFF00000
#define    HSIC_PHY_HSIC_PLL_NDIV_FRAC_PLL_NDIV_FRAC_SHIFT                0
#define    HSIC_PHY_HSIC_PLL_NDIV_FRAC_PLL_NDIV_FRAC_MASK                 0x000FFFFF

#define HSIC_PHY_TEST_MUX_SEL_OFFSET                                      0x0000002C
#define HSIC_PHY_TEST_MUX_SEL_TYPE                                        UInt32
#define HSIC_PHY_TEST_MUX_SEL_RESERVED_MASK                               0xFFFFFF00
#define    HSIC_PHY_TEST_MUX_SEL_TM_SEL_SHIFT                             0
#define    HSIC_PHY_TEST_MUX_SEL_TM_SEL_MASK                              0x000000FF

#define HSIC_PHY_TEST_MUX_OUT_OFFSET                                      0x00000030
#define HSIC_PHY_TEST_MUX_OUT_TYPE                                        UInt32
#define HSIC_PHY_TEST_MUX_OUT_RESERVED_MASK                               0xFFFF0000
#define    HSIC_PHY_TEST_MUX_OUT_TM_OUT_SHIFT                             0
#define    HSIC_PHY_TEST_MUX_OUT_TM_OUT_MASK                              0x0000FFFF

#define HSIC_PHY_HSIC_STS_OFFSET                                          0x00000034
#define HSIC_PHY_HSIC_STS_TYPE                                            UInt32
#define HSIC_PHY_HSIC_STS_RESERVED_MASK                                   0x7FE00000
#define    HSIC_PHY_HSIC_STS_PLL_LOCK_SHIFT                               31
#define    HSIC_PHY_HSIC_STS_PLL_LOCK_MASK                                0x80000000
#define    HSIC_PHY_HSIC_STS_PVT_COMP_ACK_SHIFT                           20
#define    HSIC_PHY_HSIC_STS_PVT_COMP_ACK_MASK                            0x00100000
#define    HSIC_PHY_HSIC_STS_PVT_COMP_DONE_SHIFT                          19
#define    HSIC_PHY_HSIC_STS_PVT_COMP_DONE_MASK                           0x00080000
#define    HSIC_PHY_HSIC_STS_PVT_COMP_ERROR_SHIFT                         13
#define    HSIC_PHY_HSIC_STS_PVT_COMP_ERROR_MASK                          0x0007E000
#define    HSIC_PHY_HSIC_STS_PVT_PDONE_2CORE_SHIFT                        12
#define    HSIC_PHY_HSIC_STS_PVT_PDONE_2CORE_MASK                         0x00001000
#define    HSIC_PHY_HSIC_STS_PVT_NDONE_2CORE_SHIFT                        11
#define    HSIC_PHY_HSIC_STS_PVT_NDONE_2CORE_MASK                         0x00000800
#define    HSIC_PHY_HSIC_STS_PVT_PCOMP_CODE_2CORE_SHIFT                   7
#define    HSIC_PHY_HSIC_STS_PVT_PCOMP_CODE_2CORE_MASK                    0x00000780
#define    HSIC_PHY_HSIC_STS_PVT_NCOMP_CODE_2CORE_SHIFT                   3
#define    HSIC_PHY_HSIC_STS_PVT_NCOMP_CODE_2CORE_MASK                    0x00000078
#define    HSIC_PHY_HSIC_STS_PVT_DIV_CLK_2CORE_SHIFT                      2
#define    HSIC_PHY_HSIC_STS_PVT_DIV_CLK_2CORE_MASK                       0x00000004
#define    HSIC_PHY_HSIC_STS_PVT_PCOMP_ENB_2CORE_SHIFT                    1
#define    HSIC_PHY_HSIC_STS_PVT_PCOMP_ENB_2CORE_MASK                     0x00000002
#define    HSIC_PHY_HSIC_STS_PVT_NCOMP_ENB_2CORE_SHIFT                    0
#define    HSIC_PHY_HSIC_STS_PVT_NCOMP_ENB_2CORE_MASK                     0x00000001

#endif /* __BRCM_RDB_HSIC_PHY_H__ */


