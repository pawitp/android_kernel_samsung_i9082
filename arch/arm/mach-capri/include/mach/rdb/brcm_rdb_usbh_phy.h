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

#ifndef __BRCM_RDB_USBH_PHY_H__
#define __BRCM_RDB_USBH_PHY_H__

#define USBH_PHY_CLKRST_CTRL_OFFSET                                       0x00000000
#define USBH_PHY_CLKRST_CTRL_TYPE                                         UInt32
#define USBH_PHY_CLKRST_CTRL_RESERVED_MASK                                0xFFFFFFE0
#define    USBH_PHY_CLKRST_CTRL_SW_OHCI_ACCESS_EN_SHIFT                   4
#define    USBH_PHY_CLKRST_CTRL_SW_OHCI_ACCESS_EN_MASK                    0x00000010
#define    USBH_PHY_CLKRST_CTRL_MASK_PHY_RESETS_SHIFT                     3
#define    USBH_PHY_CLKRST_CTRL_MASK_PHY_RESETS_MASK                      0x00000008
#define    USBH_PHY_CLKRST_CTRL_UTMIRESETN_SW_SHIFT                       1
#define    USBH_PHY_CLKRST_CTRL_UTMIRESETN_SW_MASK                        0x00000006
#define    USBH_PHY_CLKRST_CTRL_RESETN_SW_SHIFT                           0
#define    USBH_PHY_CLKRST_CTRL_RESETN_SW_MASK                            0x00000001

#define USBH_PHY_CORE_STRAP_CTRL_OFFSET                                   0x00000004
#define USBH_PHY_CORE_STRAP_CTRL_TYPE                                     UInt32
#define USBH_PHY_CORE_STRAP_CTRL_RESERVED_MASK                            0xFFFFE000
#define    USBH_PHY_CORE_STRAP_CTRL_SS_UTMI_BACKWARD_ENB_I_SHIFT          12
#define    USBH_PHY_CORE_STRAP_CTRL_SS_UTMI_BACKWARD_ENB_I_MASK           0x00001000
#define    USBH_PHY_CORE_STRAP_CTRL_SS_RESUME_UTMI_PLS_DIS_I_SHIFT        11
#define    USBH_PHY_CORE_STRAP_CTRL_SS_RESUME_UTMI_PLS_DIS_I_MASK         0x00000800
#define    USBH_PHY_CORE_STRAP_CTRL_SS_HUBSETUP_MIN_SHIFT                 10
#define    USBH_PHY_CORE_STRAP_CTRL_SS_HUBSETUP_MIN_MASK                  0x00000400
#define    USBH_PHY_CORE_STRAP_CTRL_SS_AUTOPPD_ON_OVERCUR_EN_SHIFT        9
#define    USBH_PHY_CORE_STRAP_CTRL_SS_AUTOPPD_ON_OVERCUR_EN_MASK         0x00000200
#define    USBH_PHY_CORE_STRAP_CTRL_APP_START_CLK_SHIFT                   8
#define    USBH_PHY_CORE_STRAP_CTRL_APP_START_CLK_MASK                    0x00000100
#define    USBH_PHY_CORE_STRAP_CTRL_SS_POWER_STATE_VALID_I_SHIFT          7
#define    USBH_PHY_CORE_STRAP_CTRL_SS_POWER_STATE_VALID_I_MASK           0x00000080
#define    USBH_PHY_CORE_STRAP_CTRL_SS_SIMULATION_MODE_I_SHIFT            6
#define    USBH_PHY_CORE_STRAP_CTRL_SS_SIMULATION_MODE_I_MASK             0x00000040
#define    USBH_PHY_CORE_STRAP_CTRL_OHCI_0_CNTSEL_I_N_SHIFT               5
#define    USBH_PHY_CORE_STRAP_CTRL_OHCI_0_CNTSEL_I_N_MASK                0x00000020
#define    USBH_PHY_CORE_STRAP_CTRL_SS_NXT_POWER_STATE_VALID_I_SHIFT      4
#define    USBH_PHY_CORE_STRAP_CTRL_SS_NXT_POWER_STATE_VALID_I_MASK       0x00000010
#define    USBH_PHY_CORE_STRAP_CTRL_SS_NEXT_POWER_STATE_I_SHIFT           2
#define    USBH_PHY_CORE_STRAP_CTRL_SS_NEXT_POWER_STATE_I_MASK            0x0000000C
#define    USBH_PHY_CORE_STRAP_CTRL_SS_POWER_STATE_I_SHIFT                0
#define    USBH_PHY_CORE_STRAP_CTRL_SS_POWER_STATE_I_MASK                 0x00000003

#define USBH_PHY_SS_FLADJ_VAL_HOST_I_OFFSET                               0x00000008
#define USBH_PHY_SS_FLADJ_VAL_HOST_I_TYPE                                 UInt32
#define USBH_PHY_SS_FLADJ_VAL_HOST_I_RESERVED_MASK                        0xFFFFFFC0
#define    USBH_PHY_SS_FLADJ_VAL_HOST_I_SS_FLADJ_VAL_HOST_I_SHIFT         0
#define    USBH_PHY_SS_FLADJ_VAL_HOST_I_SS_FLADJ_VAL_HOST_I_MASK          0x0000003F

#define USBH_PHY_SS_FLADJ_VAL_1_OFFSET                                    0x0000000C
#define USBH_PHY_SS_FLADJ_VAL_1_TYPE                                      UInt32
#define USBH_PHY_SS_FLADJ_VAL_1_RESERVED_MASK                             0xFFFFFFC0
#define    USBH_PHY_SS_FLADJ_VAL_1_SS_FLADJ_VAL_SHIFT                     0
#define    USBH_PHY_SS_FLADJ_VAL_1_SS_FLADJ_VAL_MASK                      0x0000003F

#define USBH_PHY_SS_FLADJ_VAL_2_OFFSET                                    0x00000010
#define USBH_PHY_SS_FLADJ_VAL_2_TYPE                                      UInt32
#define USBH_PHY_SS_FLADJ_VAL_2_RESERVED_MASK                             0xFFFFFFC0
#define    USBH_PHY_SS_FLADJ_VAL_2_SS_FLADJ_VAL_SHIFT                     0
#define    USBH_PHY_SS_FLADJ_VAL_2_SS_FLADJ_VAL_MASK                      0x0000003F

#define USBH_PHY_AFE_CTRL_OFFSET                                          0x00000014
#define USBH_PHY_AFE_CTRL_TYPE                                            UInt32
#define USBH_PHY_AFE_CTRL_RESERVED_MASK                                   0xFFF00A00
#define    USBH_PHY_AFE_CTRL_AFE_PLL_BYPASS_SHIFT                         19
#define    USBH_PHY_AFE_CTRL_AFE_PLL_BYPASS_MASK                          0x00080000
#define    USBH_PHY_AFE_CTRL_SW_AFE_LDOBG_PWRDWNB_SHIFT                   18
#define    USBH_PHY_AFE_CTRL_SW_AFE_LDOBG_PWRDWNB_MASK                    0x00040000
#define    USBH_PHY_AFE_CTRL_AFE_LDOCNTL_1P2_2_SHIFT                      15
#define    USBH_PHY_AFE_CTRL_AFE_LDOCNTL_1P2_2_MASK                       0x00038000
#define    USBH_PHY_AFE_CTRL_AFE_LDOCNTL_1P2_1_SHIFT                      12
#define    USBH_PHY_AFE_CTRL_AFE_LDOCNTL_1P2_1_MASK                       0x00007000
#define    USBH_PHY_AFE_CTRL_AFE_LDOCNTLEN_1P2_1_SHIFT                    10
#define    USBH_PHY_AFE_CTRL_AFE_LDOCNTLEN_1P2_1_MASK                     0x00000400
#define    USBH_PHY_AFE_CTRL_SW_AFE_LDO_PWRDWNB_1_SHIFT                   8
#define    USBH_PHY_AFE_CTRL_SW_AFE_LDO_PWRDWNB_1_MASK                    0x00000100
#define    USBH_PHY_AFE_CTRL_AFE_LDOBG_OUTADJ_SHIFT                       4
#define    USBH_PHY_AFE_CTRL_AFE_LDOBG_OUTADJ_MASK                        0x000000F0
#define    USBH_PHY_AFE_CTRL_AFE_LDOBG_CADJ_SHIFT                         0
#define    USBH_PHY_AFE_CTRL_AFE_LDOBG_CADJ_MASK                          0x0000000F

#define USBH_PHY_PHY_CTRL_OFFSET                                          0x00000018
#define USBH_PHY_PHY_CTRL_TYPE                                            UInt32
#define USBH_PHY_PHY_CTRL_RESERVED_MASK                                   0xFFFFFF80
#define    USBH_PHY_PHY_CTRL_WAKEUP_INT_INVERT_SHIFT                      5
#define    USBH_PHY_PHY_CTRL_WAKEUP_INT_INVERT_MASK                       0x00000060
#define    USBH_PHY_PHY_CTRL_WAKEUP_INT_MODE_SHIFT                        3
#define    USBH_PHY_PHY_CTRL_WAKEUP_INT_MODE_MASK                         0x00000018
#define    USBH_PHY_PHY_CTRL_SW_PHY_ISO_SHIFT                             2
#define    USBH_PHY_PHY_CTRL_SW_PHY_ISO_MASK                              0x00000004
#define    USBH_PHY_PHY_CTRL_PHY_IDDQ_SHIFT                               1
#define    USBH_PHY_PHY_CTRL_PHY_IDDQ_MASK                                0x00000002
#define    USBH_PHY_PHY_CTRL_SW_PHY_RESETB_SHIFT                          0
#define    USBH_PHY_PHY_CTRL_SW_PHY_RESETB_MASK                           0x00000001

#define USBH_PHY_PHY_P1CTRL_OFFSET                                        0x0000001C
#define USBH_PHY_PHY_P1CTRL_TYPE                                          UInt32
#define USBH_PHY_PHY_P1CTRL_RESERVED_MASK                                 0xFFFF0400
#define    USBH_PHY_PHY_P1CTRL_USB11_TX_TRISTATE_SHIFT                    15
#define    USBH_PHY_PHY_P1CTRL_USB11_TX_TRISTATE_MASK                     0x00008000
#define    USBH_PHY_PHY_P1CTRL_RXSYNC_DET_SHIFT                           12
#define    USBH_PHY_PHY_P1CTRL_RXSYNC_DET_MASK                            0x00007000
#define    USBH_PHY_PHY_P1CTRL_DFE_LOOPBACK_SHIFT                         11
#define    USBH_PHY_PHY_P1CTRL_DFE_LOOPBACK_MASK                          0x00000800
#define    USBH_PHY_PHY_P1CTRL_BC_PULLUPSEL_SHIFT                         9
#define    USBH_PHY_PHY_P1CTRL_BC_PULLUPSEL_MASK                          0x00000200
#define    USBH_PHY_PHY_P1CTRL_PHY_DISCONNECT_SHIFT                       8
#define    USBH_PHY_PHY_P1CTRL_PHY_DISCONNECT_MASK                        0x00000100
#define    USBH_PHY_PHY_P1CTRL_BC_DMPULLUP_SHIFT                          7
#define    USBH_PHY_PHY_P1CTRL_BC_DMPULLUP_MASK                           0x00000080
#define    USBH_PHY_PHY_P1CTRL_BC_DPPULLUP_SHIFT                          6
#define    USBH_PHY_PHY_P1CTRL_BC_DPPULLUP_MASK                           0x00000040
#define    USBH_PHY_PHY_P1CTRL_BC_DMPULLDOWN_SHIFT                        5
#define    USBH_PHY_PHY_P1CTRL_BC_DMPULLDOWN_MASK                         0x00000020
#define    USBH_PHY_PHY_P1CTRL_BC_DPPULLDOWN_SHIFT                        4
#define    USBH_PHY_PHY_P1CTRL_BC_DPPULLDOWN_MASK                         0x00000010
#define    USBH_PHY_PHY_P1CTRL_PHY_MODE_SHIFT                             2
#define    USBH_PHY_PHY_P1CTRL_PHY_MODE_MASK                              0x0000000C
#define    USBH_PHY_PHY_P1CTRL_SOFT_RESET_N_SHIFT                         1
#define    USBH_PHY_PHY_P1CTRL_SOFT_RESET_N_MASK                          0x00000002
#define    USBH_PHY_PHY_P1CTRL_NON_DRIVING_SHIFT                          0
#define    USBH_PHY_PHY_P1CTRL_NON_DRIVING_MASK                           0x00000001

#define USBH_PHY_PHY_P2CTRL_OFFSET                                        0x00000020
#define USBH_PHY_PHY_P2CTRL_TYPE                                          UInt32
#define USBH_PHY_PHY_P2CTRL_RESERVED_MASK                                 0xFFFF0400
#define    USBH_PHY_PHY_P2CTRL_USB11_TX_TRISTATE_SHIFT                    15
#define    USBH_PHY_PHY_P2CTRL_USB11_TX_TRISTATE_MASK                     0x00008000
#define    USBH_PHY_PHY_P2CTRL_RXSYNC_DET_SHIFT                           12
#define    USBH_PHY_PHY_P2CTRL_RXSYNC_DET_MASK                            0x00007000
#define    USBH_PHY_PHY_P2CTRL_DFE_LOOPBACK_SHIFT                         11
#define    USBH_PHY_PHY_P2CTRL_DFE_LOOPBACK_MASK                          0x00000800
#define    USBH_PHY_PHY_P2CTRL_BC_PULLUPSEL_SHIFT                         9
#define    USBH_PHY_PHY_P2CTRL_BC_PULLUPSEL_MASK                          0x00000200
#define    USBH_PHY_PHY_P2CTRL_PHY_DISCONNECT_SHIFT                       8
#define    USBH_PHY_PHY_P2CTRL_PHY_DISCONNECT_MASK                        0x00000100
#define    USBH_PHY_PHY_P2CTRL_BC_DMPULLUP_SHIFT                          7
#define    USBH_PHY_PHY_P2CTRL_BC_DMPULLUP_MASK                           0x00000080
#define    USBH_PHY_PHY_P2CTRL_BC_DPPULLUP_SHIFT                          6
#define    USBH_PHY_PHY_P2CTRL_BC_DPPULLUP_MASK                           0x00000040
#define    USBH_PHY_PHY_P2CTRL_BC_DMPULLDOWN_SHIFT                        5
#define    USBH_PHY_PHY_P2CTRL_BC_DMPULLDOWN_MASK                         0x00000020
#define    USBH_PHY_PHY_P2CTRL_BC_DPPULLDOWN_SHIFT                        4
#define    USBH_PHY_PHY_P2CTRL_BC_DPPULLDOWN_MASK                         0x00000010
#define    USBH_PHY_PHY_P2CTRL_PHY_MODE_SHIFT                             2
#define    USBH_PHY_PHY_P2CTRL_PHY_MODE_MASK                              0x0000000C
#define    USBH_PHY_PHY_P2CTRL_SOFT_RESET_N_SHIFT                         1
#define    USBH_PHY_PHY_P2CTRL_SOFT_RESET_N_MASK                          0x00000002
#define    USBH_PHY_PHY_P2CTRL_NON_DRIVING_SHIFT                          0
#define    USBH_PHY_PHY_P2CTRL_NON_DRIVING_MASK                           0x00000001

#define USBH_PHY_PLL_CTRL_OFFSET                                          0x00000024
#define USBH_PHY_PLL_CTRL_TYPE                                            UInt32
#define USBH_PHY_PLL_CTRL_RESERVED_MASK                                   0xFC000000
#define    USBH_PHY_PLL_CTRL_PLL_LOCK_SHIFT                               25
#define    USBH_PHY_PLL_CTRL_PLL_LOCK_MASK                                0x02000000
#define    USBH_PHY_PLL_CTRL_PLL_NDIV_INT_SHIFT                           15
#define    USBH_PHY_PLL_CTRL_PLL_NDIV_INT_MASK                            0x01FF8000
#define    USBH_PHY_PLL_CTRL_PLL_PDIV_SHIFT                               12
#define    USBH_PHY_PLL_CTRL_PLL_PDIV_MASK                                0x00007000
#define    USBH_PHY_PLL_CTRL_PLL_KA_SHIFT                                 9
#define    USBH_PHY_PLL_CTRL_PLL_KA_MASK                                  0x00000E00
#define    USBH_PHY_PLL_CTRL_PLL_KI_SHIFT                                 6
#define    USBH_PHY_PLL_CTRL_PLL_KI_MASK                                  0x000001C0
#define    USBH_PHY_PLL_CTRL_PLL_KP_SHIFT                                 2
#define    USBH_PHY_PLL_CTRL_PLL_KP_MASK                                  0x0000003C
#define    USBH_PHY_PLL_CTRL_PLL_SUSPENDN_SHIFT                           1
#define    USBH_PHY_PLL_CTRL_PLL_SUSPENDN_MASK                            0x00000002
#define    USBH_PHY_PLL_CTRL_PLL_RESETB_SHIFT                             0
#define    USBH_PHY_PLL_CTRL_PLL_RESETB_MASK                              0x00000001

#define USBH_PHY_TP_SEL_OFFSET                                            0x00000028
#define USBH_PHY_TP_SEL_TYPE                                              UInt32
#define USBH_PHY_TP_SEL_RESERVED_MASK                                     0xFFFFFF00
#define    USBH_PHY_TP_SEL_TP_SEL_SHIFT                                   0
#define    USBH_PHY_TP_SEL_TP_SEL_MASK                                    0x000000FF

#define USBH_PHY_TP_OUT_OFFSET                                            0x0000002C
#define USBH_PHY_TP_OUT_TYPE                                              UInt32
#define USBH_PHY_TP_OUT_RESERVED_MASK                                     0xFF000000
#define    USBH_PHY_TP_OUT_TP_OUT_SHIFT                                   0
#define    USBH_PHY_TP_OUT_TP_OUT_MASK                                    0x00FFFFFF

#define USBH_PHY_SUSPEND_WRAP_CTRL_OFFSET                                 0x00000030
#define USBH_PHY_SUSPEND_WRAP_CTRL_TYPE                                   UInt32
#define USBH_PHY_SUSPEND_WRAP_CTRL_RESERVED_MASK                          0xFFF000C0
#define    USBH_PHY_SUSPEND_WRAP_CTRL_POST_COUNT_LOAD_SHIFT               16
#define    USBH_PHY_SUSPEND_WRAP_CTRL_POST_COUNT_LOAD_MASK                0x000F0000
#define    USBH_PHY_SUSPEND_WRAP_CTRL_MAIN_COUNT_LOAD_SHIFT               8
#define    USBH_PHY_SUSPEND_WRAP_CTRL_MAIN_COUNT_LOAD_MASK                0x0000FF00
#define    USBH_PHY_SUSPEND_WRAP_CTRL_PLL_RESETB_MASK_SHIFT               5
#define    USBH_PHY_SUSPEND_WRAP_CTRL_PLL_RESETB_MASK_MASK                0x00000020
#define    USBH_PHY_SUSPEND_WRAP_CTRL_PHY_RESETB_MASK_SHIFT               4
#define    USBH_PHY_SUSPEND_WRAP_CTRL_PHY_RESETB_MASK_MASK                0x00000010
#define    USBH_PHY_SUSPEND_WRAP_CTRL_PHY_CLK_REQ_CLR_SHIFT               3
#define    USBH_PHY_SUSPEND_WRAP_CTRL_PHY_CLK_REQ_CLR_MASK                0x00000008
#define    USBH_PHY_SUSPEND_WRAP_CTRL_PHY_CLK_REQ_SHIFT                   2
#define    USBH_PHY_SUSPEND_WRAP_CTRL_PHY_CLK_REQ_MASK                    0x00000004
#define    USBH_PHY_SUSPEND_WRAP_CTRL_SUSPEND_MASK_SHIFT                  1
#define    USBH_PHY_SUSPEND_WRAP_CTRL_SUSPEND_MASK_MASK                   0x00000002
#define    USBH_PHY_SUSPEND_WRAP_CTRL_SW_MODE_SHIFT                       0
#define    USBH_PHY_SUSPEND_WRAP_CTRL_SW_MODE_MASK                        0x00000001

#endif /* __BRCM_RDB_USBH_PHY_H__ */


