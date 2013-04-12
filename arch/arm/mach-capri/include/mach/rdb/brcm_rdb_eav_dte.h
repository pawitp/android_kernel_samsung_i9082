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

#ifndef __BRCM_RDB_EAV_DTE_H__
#define __BRCM_RDB_EAV_DTE_H__

#define EAV_DTE_AV_TE_OFFSET                                              0x00000000
#define EAV_DTE_AV_TE_TYPE                                                UInt32
#define EAV_DTE_AV_TE_RESERVED_MASK                                       0x00000000
#define    EAV_DTE_AV_TE_TE_REG_SHIFT                                     0
#define    EAV_DTE_AV_TE_TE_REG_MASK                                      0xFFFFFFFF

#define EAV_DTE_AV_GLB_CTRL_OFFSET                                        0x00000004
#define EAV_DTE_AV_GLB_CTRL_TYPE                                          UInt32
#define EAV_DTE_AV_GLB_CTRL_RESERVED_MASK                                 0xFFFFF000
#define    EAV_DTE_AV_GLB_CTRL_STAT_DBG_SEL_SHIFT                         8
#define    EAV_DTE_AV_GLB_CTRL_STAT_DBG_SEL_MASK                          0x00000F00
#define    EAV_DTE_AV_GLB_CTRL_DYN_DGB_SEL_SHIFT                          4
#define    EAV_DTE_AV_GLB_CTRL_DYN_DGB_SEL_MASK                           0x000000F0
#define    EAV_DTE_AV_GLB_CTRL_DYN_DBG_EN_SHIFT                           3
#define    EAV_DTE_AV_GLB_CTRL_DYN_DBG_EN_MASK                            0x00000008
#define    EAV_DTE_AV_GLB_CTRL_P1_CLK_P0_SHIFT                            2
#define    EAV_DTE_AV_GLB_CTRL_P1_CLK_P0_MASK                             0x00000004
#define    EAV_DTE_AV_GLB_CTRL_P1_SYNC_EXT_CLK_SHIFT                      1
#define    EAV_DTE_AV_GLB_CTRL_P1_SYNC_EXT_CLK_MASK                       0x00000002
#define    EAV_DTE_AV_GLB_CTRL_P0_SYNC_EXT_CLK_SHIFT                      0
#define    EAV_DTE_AV_GLB_CTRL_P0_SYNC_EXT_CLK_MASK                       0x00000001

#define EAV_DTE_GLB_STATIC_DEBUG_OFFSET                                   0x00000008
#define EAV_DTE_GLB_STATIC_DEBUG_TYPE                                     UInt32
#define EAV_DTE_GLB_STATIC_DEBUG_RESERVED_MASK                            0x00000000
#define    EAV_DTE_GLB_STATIC_DEBUG_STATIC_DBG_DATA_SHIFT                 0
#define    EAV_DTE_GLB_STATIC_DEBUG_STATIC_DBG_DATA_MASK                  0xFFFFFFFF

#define EAV_DTE_P0_AV_CSR_OFFSET                                          0x00000100
#define EAV_DTE_P0_AV_CSR_TYPE                                            UInt32
#define EAV_DTE_P0_AV_CSR_RESERVED_MASK                                   0x00020000
#define    EAV_DTE_P0_AV_CSR_SW_RST_SHIFT                                 31
#define    EAV_DTE_P0_AV_CSR_SW_RST_MASK                                  0x80000000
#define    EAV_DTE_P0_AV_CSR_IO_MODE_SHIFT                                30
#define    EAV_DTE_P0_AV_CSR_IO_MODE_MASK                                 0x40000000
#define    EAV_DTE_P0_AV_CSR_SAMP_SHFT_SHIFT                              27
#define    EAV_DTE_P0_AV_CSR_SAMP_SHFT_MASK                               0x38000000
#define    EAV_DTE_P0_AV_CSR_INV_ICLK_SHIFT                               26
#define    EAV_DTE_P0_AV_CSR_INV_ICLK_MASK                                0x04000000
#define    EAV_DTE_P0_AV_CSR_INV_WCLK_SHIFT                               25
#define    EAV_DTE_P0_AV_CSR_INV_WCLK_MASK                                0x02000000
#define    EAV_DTE_P0_AV_CSR_WCLK_SEL_SHIFT                               24
#define    EAV_DTE_P0_AV_CSR_WCLK_SEL_MASK                                0x01000000
#define    EAV_DTE_P0_AV_CSR_ICLK_SEL_SHIFT                               23
#define    EAV_DTE_P0_AV_CSR_ICLK_SEL_MASK                                0x00800000
#define    EAV_DTE_P0_AV_CSR_FB_POS_SHIFT                                 21
#define    EAV_DTE_P0_AV_CSR_FB_POS_MASK                                  0x00600000
#define    EAV_DTE_P0_AV_CSR_AV_SER_OE_SHIFT                              20
#define    EAV_DTE_P0_AV_CSR_AV_SER_OE_MASK                               0x00100000
#define    EAV_DTE_P0_AV_CSR_AV_ICLK_OE_SHIFT                             19
#define    EAV_DTE_P0_AV_CSR_AV_ICLK_OE_MASK                              0x00080000
#define    EAV_DTE_P0_AV_CSR_AV_WCLK_OE_SHIFT                             18
#define    EAV_DTE_P0_AV_CSR_AV_WCLK_OE_MASK                              0x00040000
#define    EAV_DTE_P0_AV_CSR_MODE_SHIFT                                   14
#define    EAV_DTE_P0_AV_CSR_MODE_MASK                                    0x0001C000
#define    EAV_DTE_P0_AV_CSR_MUTE_ALL_SHIFT                               13
#define    EAV_DTE_P0_AV_CSR_MUTE_ALL_MASK                                0x00002000
#define    EAV_DTE_P0_AV_CSR_LSB_MSB_SHIFT                                12
#define    EAV_DTE_P0_AV_CSR_LSB_MSB_MASK                                 0x00001000
#define    EAV_DTE_P0_AV_CSR_CH_ACT_MASK_SHIFT                            4
#define    EAV_DTE_P0_AV_CSR_CH_ACT_MASK_MASK                             0x00000FF0
#define    EAV_DTE_P0_AV_CSR_NUM_CHAN_SHIFT                               0
#define    EAV_DTE_P0_AV_CSR_NUM_CHAN_MASK                                0x0000000F

#define EAV_DTE_P0_AV_OCD_CSR_OFFSET                                      0x00000104
#define EAV_DTE_P0_AV_OCD_CSR_TYPE                                        UInt32
#define EAV_DTE_P0_AV_OCD_CSR_RESERVED_MASK                               0xFFFF0230
#define    EAV_DTE_P0_AV_OCD_CSR_ICLK_D_SEL_SHIFT                         15
#define    EAV_DTE_P0_AV_OCD_CSR_ICLK_D_SEL_MASK                          0x00008000
#define    EAV_DTE_P0_AV_OCD_CSR_WCLK_NE_SEL_SHIFT                        14
#define    EAV_DTE_P0_AV_OCD_CSR_WCLK_NE_SEL_MASK                         0x00004000
#define    EAV_DTE_P0_AV_OCD_CSR_SAMP_SHFT_DIR_SHIFT                      13
#define    EAV_DTE_P0_AV_OCD_CSR_SAMP_SHFT_DIR_MASK                       0x00002000
#define    EAV_DTE_P0_AV_OCD_CSR_LTE_SEL_SHIFT                            10
#define    EAV_DTE_P0_AV_OCD_CSR_LTE_SEL_MASK                             0x00001C00
#define    EAV_DTE_P0_AV_OCD_CSR_INV_WCLK_SHIFT                           8
#define    EAV_DTE_P0_AV_OCD_CSR_INV_WCLK_MASK                            0x00000100
#define    EAV_DTE_P0_AV_OCD_CSR_INV_ICLK_SHIFT                           7
#define    EAV_DTE_P0_AV_OCD_CSR_INV_ICLK_MASK                            0x00000080
#define    EAV_DTE_P0_AV_OCD_CSR_ICLK_SRC_SEL_SHIFT                       6
#define    EAV_DTE_P0_AV_OCD_CSR_ICLK_SRC_SEL_MASK                        0x00000040
#define    EAV_DTE_P0_AV_OCD_CSR_SYNC_SEL_SHIFT                           3
#define    EAV_DTE_P0_AV_OCD_CSR_SYNC_SEL_MASK                            0x00000008
#define    EAV_DTE_P0_AV_OCD_CSR_BCLK_SRC_SEL_SHIFT                       0
#define    EAV_DTE_P0_AV_OCD_CSR_BCLK_SRC_SEL_MASK                        0x00000007

#define EAV_DTE_P0_AV_FIFO_STAT_OFFSET                                    0x00000108
#define EAV_DTE_P0_AV_FIFO_STAT_TYPE                                      UInt32
#define EAV_DTE_P0_AV_FIFO_STAT_RESERVED_MASK                             0x00000000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO7_EMPTY_SHIFT                      31
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO7_EMPTY_MASK                       0x80000000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO7_FULL_SHIFT                       30
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO7_FULL_MASK                        0x40000000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO6_EMPTY_SHIFT                      29
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO6_EMPTY_MASK                       0x20000000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO6_FULL_SHIFT                       28
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO6_FULL_MASK                        0x10000000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO5_EMPTY_SHIFT                      27
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO5_EMPTY_MASK                       0x08000000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO5_FULL_SHIFT                       26
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO5_FULL_MASK                        0x04000000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO4_EMPTY_SHIFT                      25
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO4_EMPTY_MASK                       0x02000000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO4_FULL_SHIFT                       24
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO4_FULL_MASK                        0x01000000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO3_EMPTY_SHIFT                      23
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO3_EMPTY_MASK                       0x00800000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO3_FULL_SHIFT                       22
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO3_FULL_MASK                        0x00400000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO2_EMPTY_SHIFT                      21
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO2_EMPTY_MASK                       0x00200000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO2_FULL_SHIFT                       20
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO2_FULL_MASK                        0x00100000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO1_EMPTY_SHIFT                      19
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO1_EMPTY_MASK                       0x00080000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO1_FULL_SHIFT                       18
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO1_FULL_MASK                        0x00040000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO0_EMPTY_SHIFT                      17
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO0_EMPTY_MASK                       0x00020000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO0_FULL_SHIFT                       16
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO0_FULL_MASK                        0x00010000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO7_OVFL_SHIFT                       15
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO7_OVFL_MASK                        0x00008000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO7_UNDFL_SHIFT                      14
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO7_UNDFL_MASK                       0x00004000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO6_OVFL_SHIFT                       13
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO6_OVFL_MASK                        0x00002000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO6_UNDFL_SHIFT                      12
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO6_UNDFL_MASK                       0x00001000
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO5_OVFL_SHIFT                       11
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO5_OVFL_MASK                        0x00000800
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO5_UNDFL_SHIFT                      10
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO5_UNDFL_MASK                       0x00000400
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO4_OVFL_SHIFT                       9
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO4_OVFL_MASK                        0x00000200
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO4_UNDFL_SHIFT                      8
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO4_UNDFL_MASK                       0x00000100
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO3_OVFL_SHIFT                       7
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO3_OVFL_MASK                        0x00000080
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO3_UNDFL_SHIFT                      6
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO3_UNDFL_MASK                       0x00000040
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO2_OVFL_SHIFT                       5
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO2_OVFL_MASK                        0x00000020
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO2_UNDFL_SHIFT                      4
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO2_UNDFL_MASK                       0x00000010
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO1_OVFL_SHIFT                       3
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO1_OVFL_MASK                        0x00000008
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO1_UNDFL_SHIFT                      2
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO1_UNDFL_MASK                       0x00000004
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO0_OVFL_SHIFT                       1
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO0_OVFL_MASK                        0x00000002
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO0_UNDFL_SHIFT                      0
#define    EAV_DTE_P0_AV_FIFO_STAT_FIFO0_UNDFL_MASK                       0x00000001

#define EAV_DTE_P0_AV_MUTE_LABEL_VAL_OFFSET                               0x0000010C
#define EAV_DTE_P0_AV_MUTE_LABEL_VAL_TYPE                                 UInt32
#define EAV_DTE_P0_AV_MUTE_LABEL_VAL_RESERVED_MASK                        0x00000000
#define    EAV_DTE_P0_AV_MUTE_LABEL_VAL_MUTE_LABEL_VAL_SHIFT              0
#define    EAV_DTE_P0_AV_MUTE_LABEL_VAL_MUTE_LABEL_VAL_MASK               0xFFFFFFFF

#define EAV_DTE_P0_AV_IO_MASK_OFFSET                                      0x00000114
#define EAV_DTE_P0_AV_IO_MASK_TYPE                                        UInt32
#define EAV_DTE_P0_AV_IO_MASK_RESERVED_MASK                               0x00000000
#define    EAV_DTE_P0_AV_IO_MASK_IO_MASK_SHIFT                            0
#define    EAV_DTE_P0_AV_IO_MASK_IO_MASK_MASK                             0xFFFFFFFF

#define EAV_DTE_P0_AV_N_REG_OFFSET                                        0x00000120
#define EAV_DTE_P0_AV_N_REG_TYPE                                          UInt32
#define EAV_DTE_P0_AV_N_REG_RESERVED_MASK                                 0xFFFFF000
#define    EAV_DTE_P0_AV_N_REG_N_REG_SHIFT                                0
#define    EAV_DTE_P0_AV_N_REG_N_REG_MASK                                 0x00000FFF

#define EAV_DTE_P0_AV_W_REG_OFFSET                                        0x00000128
#define EAV_DTE_P0_AV_W_REG_TYPE                                          UInt32
#define EAV_DTE_P0_AV_W_REG_RESERVED_MASK                                 0xFF000000
#define    EAV_DTE_P0_AV_W_REG_WH_REG_SHIFT                               12
#define    EAV_DTE_P0_AV_W_REG_WH_REG_MASK                                0x00FFF000
#define    EAV_DTE_P0_AV_W_REG_WL_REG_SHIFT                               0
#define    EAV_DTE_P0_AV_W_REG_WL_REG_MASK                                0x00000FFF

#define EAV_DTE_P0_AV_B_REG_OFFSET                                        0x00000130
#define EAV_DTE_P0_AV_B_REG_TYPE                                          UInt32
#define EAV_DTE_P0_AV_B_REG_RESERVED_MASK                                 0xFFFFF000
#define    EAV_DTE_P0_AV_B_REG_B_REG_SHIFT                                0
#define    EAV_DTE_P0_AV_B_REG_B_REG_MASK                                 0x00000FFF

#define EAV_DTE_P0_AV_T_REG_OFFSET                                        0x00000134
#define EAV_DTE_P0_AV_T_REG_TYPE                                          UInt32
#define EAV_DTE_P0_AV_T_REG_RESERVED_MASK                                 0xFF000000
#define    EAV_DTE_P0_AV_T_REG_B_REG_SHIFT                                0
#define    EAV_DTE_P0_AV_T_REG_B_REG_MASK                                 0x00FFFFFF

#define EAV_DTE_P1_AV_CSR_OFFSET                                          0x00000200
#define EAV_DTE_P1_AV_CSR_TYPE                                            UInt32
#define EAV_DTE_P1_AV_CSR_RESERVED_MASK                                   0x00020000
#define    EAV_DTE_P1_AV_CSR_SW_RST_SHIFT                                 31
#define    EAV_DTE_P1_AV_CSR_SW_RST_MASK                                  0x80000000
#define    EAV_DTE_P1_AV_CSR_IO_MODE_SHIFT                                30
#define    EAV_DTE_P1_AV_CSR_IO_MODE_MASK                                 0x40000000
#define    EAV_DTE_P1_AV_CSR_SAMP_SHFT_SHIFT                              27
#define    EAV_DTE_P1_AV_CSR_SAMP_SHFT_MASK                               0x38000000
#define    EAV_DTE_P1_AV_CSR_INV_ICLK_SHIFT                               26
#define    EAV_DTE_P1_AV_CSR_INV_ICLK_MASK                                0x04000000
#define    EAV_DTE_P1_AV_CSR_INV_WCLK_SHIFT                               25
#define    EAV_DTE_P1_AV_CSR_INV_WCLK_MASK                                0x02000000
#define    EAV_DTE_P1_AV_CSR_WCLK_SEL_SHIFT                               24
#define    EAV_DTE_P1_AV_CSR_WCLK_SEL_MASK                                0x01000000
#define    EAV_DTE_P1_AV_CSR_ICLK_SEL_SHIFT                               23
#define    EAV_DTE_P1_AV_CSR_ICLK_SEL_MASK                                0x00800000
#define    EAV_DTE_P1_AV_CSR_FB_POS_SHIFT                                 21
#define    EAV_DTE_P1_AV_CSR_FB_POS_MASK                                  0x00600000
#define    EAV_DTE_P1_AV_CSR_AV_SER_OE_SHIFT                              20
#define    EAV_DTE_P1_AV_CSR_AV_SER_OE_MASK                               0x00100000
#define    EAV_DTE_P1_AV_CSR_AV_ICLK_OE_SHIFT                             19
#define    EAV_DTE_P1_AV_CSR_AV_ICLK_OE_MASK                              0x00080000
#define    EAV_DTE_P1_AV_CSR_AV_WCLK_OE_SHIFT                             18
#define    EAV_DTE_P1_AV_CSR_AV_WCLK_OE_MASK                              0x00040000
#define    EAV_DTE_P1_AV_CSR_MODE_SHIFT                                   14
#define    EAV_DTE_P1_AV_CSR_MODE_MASK                                    0x0001C000
#define    EAV_DTE_P1_AV_CSR_MUTE_ALL_SHIFT                               13
#define    EAV_DTE_P1_AV_CSR_MUTE_ALL_MASK                                0x00002000
#define    EAV_DTE_P1_AV_CSR_LSB_MSB_SHIFT                                12
#define    EAV_DTE_P1_AV_CSR_LSB_MSB_MASK                                 0x00001000
#define    EAV_DTE_P1_AV_CSR_CH_ACT_MASK_SHIFT                            4
#define    EAV_DTE_P1_AV_CSR_CH_ACT_MASK_MASK                             0x00000FF0
#define    EAV_DTE_P1_AV_CSR_NUM_CHAN_SHIFT                               0
#define    EAV_DTE_P1_AV_CSR_NUM_CHAN_MASK                                0x0000000F

#define EAV_DTE_P1_AV_OCD_CSR_OFFSET                                      0x00000204
#define EAV_DTE_P1_AV_OCD_CSR_TYPE                                        UInt32
#define EAV_DTE_P1_AV_OCD_CSR_RESERVED_MASK                               0xFFFF0230
#define    EAV_DTE_P1_AV_OCD_CSR_ICLK_D_SEL_SHIFT                         15
#define    EAV_DTE_P1_AV_OCD_CSR_ICLK_D_SEL_MASK                          0x00008000
#define    EAV_DTE_P1_AV_OCD_CSR_WCLK_NE_SEL_SHIFT                        14
#define    EAV_DTE_P1_AV_OCD_CSR_WCLK_NE_SEL_MASK                         0x00004000
#define    EAV_DTE_P1_AV_OCD_CSR_SAMP_SHFT_DIR_SHIFT                      13
#define    EAV_DTE_P1_AV_OCD_CSR_SAMP_SHFT_DIR_MASK                       0x00002000
#define    EAV_DTE_P1_AV_OCD_CSR_LTE_SEL_SHIFT                            10
#define    EAV_DTE_P1_AV_OCD_CSR_LTE_SEL_MASK                             0x00001C00
#define    EAV_DTE_P1_AV_OCD_CSR_INV_WCLK_SHIFT                           8
#define    EAV_DTE_P1_AV_OCD_CSR_INV_WCLK_MASK                            0x00000100
#define    EAV_DTE_P1_AV_OCD_CSR_INV_ICLK_SHIFT                           7
#define    EAV_DTE_P1_AV_OCD_CSR_INV_ICLK_MASK                            0x00000080
#define    EAV_DTE_P1_AV_OCD_CSR_ICLK_SRC_SEL_SHIFT                       6
#define    EAV_DTE_P1_AV_OCD_CSR_ICLK_SRC_SEL_MASK                        0x00000040
#define    EAV_DTE_P1_AV_OCD_CSR_SYNC_SEL_SHIFT                           3
#define    EAV_DTE_P1_AV_OCD_CSR_SYNC_SEL_MASK                            0x00000008
#define    EAV_DTE_P1_AV_OCD_CSR_BCLK_SRC_SEL_SHIFT                       0
#define    EAV_DTE_P1_AV_OCD_CSR_BCLK_SRC_SEL_MASK                        0x00000007

#define EAV_DTE_P1_AV_FIFO_STAT_OFFSET                                    0x00000208
#define EAV_DTE_P1_AV_FIFO_STAT_TYPE                                      UInt32
#define EAV_DTE_P1_AV_FIFO_STAT_RESERVED_MASK                             0x00000000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO7_EMPTY_SHIFT                      31
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO7_EMPTY_MASK                       0x80000000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO7_FULL_SHIFT                       30
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO7_FULL_MASK                        0x40000000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO6_EMPTY_SHIFT                      29
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO6_EMPTY_MASK                       0x20000000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO6_FULL_SHIFT                       28
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO6_FULL_MASK                        0x10000000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO5_EMPTY_SHIFT                      27
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO5_EMPTY_MASK                       0x08000000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO5_FULL_SHIFT                       26
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO5_FULL_MASK                        0x04000000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO4_EMPTY_SHIFT                      25
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO4_EMPTY_MASK                       0x02000000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO4_FULL_SHIFT                       24
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO4_FULL_MASK                        0x01000000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO3_EMPTY_SHIFT                      23
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO3_EMPTY_MASK                       0x00800000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO3_FULL_SHIFT                       22
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO3_FULL_MASK                        0x00400000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO2_EMPTY_SHIFT                      21
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO2_EMPTY_MASK                       0x00200000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO2_FULL_SHIFT                       20
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO2_FULL_MASK                        0x00100000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO1_EMPTY_SHIFT                      19
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO1_EMPTY_MASK                       0x00080000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO1_FULL_SHIFT                       18
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO1_FULL_MASK                        0x00040000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO0_EMPTY_SHIFT                      17
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO0_EMPTY_MASK                       0x00020000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO0_FULL_SHIFT                       16
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO0_FULL_MASK                        0x00010000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO7_OVFL_SHIFT                       15
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO7_OVFL_MASK                        0x00008000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO7_UNDFL_SHIFT                      14
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO7_UNDFL_MASK                       0x00004000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO6_OVFL_SHIFT                       13
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO6_OVFL_MASK                        0x00002000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO6_UNDFL_SHIFT                      12
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO6_UNDFL_MASK                       0x00001000
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO5_OVFL_SHIFT                       11
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO5_OVFL_MASK                        0x00000800
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO5_UNDFL_SHIFT                      10
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO5_UNDFL_MASK                       0x00000400
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO4_OVFL_SHIFT                       9
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO4_OVFL_MASK                        0x00000200
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO4_UNDFL_SHIFT                      8
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO4_UNDFL_MASK                       0x00000100
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO3_OVFL_SHIFT                       7
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO3_OVFL_MASK                        0x00000080
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO3_UNDFL_SHIFT                      6
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO3_UNDFL_MASK                       0x00000040
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO2_OVFL_SHIFT                       5
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO2_OVFL_MASK                        0x00000020
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO2_UNDFL_SHIFT                      4
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO2_UNDFL_MASK                       0x00000010
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO1_OVFL_SHIFT                       3
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO1_OVFL_MASK                        0x00000008
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO1_UNDFL_SHIFT                      2
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO1_UNDFL_MASK                       0x00000004
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO0_OVFL_SHIFT                       1
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO0_OVFL_MASK                        0x00000002
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO0_UNDFL_SHIFT                      0
#define    EAV_DTE_P1_AV_FIFO_STAT_FIFO0_UNDFL_MASK                       0x00000001

#define EAV_DTE_P1_AV_MUTE_LABEL_VAL_OFFSET                               0x0000020C
#define EAV_DTE_P1_AV_MUTE_LABEL_VAL_TYPE                                 UInt32
#define EAV_DTE_P1_AV_MUTE_LABEL_VAL_RESERVED_MASK                        0x00000000
#define    EAV_DTE_P1_AV_MUTE_LABEL_VAL_MUTE_LABEL_VAL_SHIFT              0
#define    EAV_DTE_P1_AV_MUTE_LABEL_VAL_MUTE_LABEL_VAL_MASK               0xFFFFFFFF

#define EAV_DTE_P1_AV_IO_MASK_OFFSET                                      0x00000214
#define EAV_DTE_P1_AV_IO_MASK_TYPE                                        UInt32
#define EAV_DTE_P1_AV_IO_MASK_RESERVED_MASK                               0x00000000
#define    EAV_DTE_P1_AV_IO_MASK_IO_MASK_SHIFT                            0
#define    EAV_DTE_P1_AV_IO_MASK_IO_MASK_MASK                             0xFFFFFFFF

#define EAV_DTE_P1_AV_N_REG_OFFSET                                        0x00000220
#define EAV_DTE_P1_AV_N_REG_TYPE                                          UInt32
#define EAV_DTE_P1_AV_N_REG_RESERVED_MASK                                 0xFFFFF000
#define    EAV_DTE_P1_AV_N_REG_N_REG_SHIFT                                0
#define    EAV_DTE_P1_AV_N_REG_N_REG_MASK                                 0x00000FFF

#define EAV_DTE_P1_AV_W_REG_OFFSET                                        0x00000228
#define EAV_DTE_P1_AV_W_REG_TYPE                                          UInt32
#define EAV_DTE_P1_AV_W_REG_RESERVED_MASK                                 0xFF000000
#define    EAV_DTE_P1_AV_W_REG_WH_REG_SHIFT                               12
#define    EAV_DTE_P1_AV_W_REG_WH_REG_MASK                                0x00FFF000
#define    EAV_DTE_P1_AV_W_REG_WL_REG_SHIFT                               0
#define    EAV_DTE_P1_AV_W_REG_WL_REG_MASK                                0x00000FFF

#define EAV_DTE_P1_AV_B_REG_OFFSET                                        0x00000230
#define EAV_DTE_P1_AV_B_REG_TYPE                                          UInt32
#define EAV_DTE_P1_AV_B_REG_RESERVED_MASK                                 0xFFFFF000
#define    EAV_DTE_P1_AV_B_REG_B_REG_SHIFT                                0
#define    EAV_DTE_P1_AV_B_REG_B_REG_MASK                                 0x00000FFF

#define EAV_DTE_P1_AV_T_REG_OFFSET                                        0x00000234
#define EAV_DTE_P1_AV_T_REG_TYPE                                          UInt32
#define EAV_DTE_P1_AV_T_REG_RESERVED_MASK                                 0xFF000000
#define    EAV_DTE_P1_AV_T_REG_B_REG_SHIFT                                0
#define    EAV_DTE_P1_AV_T_REG_B_REG_MASK                                 0x00FFFFFF

#define EAV_DTE_DMA_CSR_OFFSET                                            0x00000300
#define EAV_DTE_DMA_CSR_TYPE                                              UInt32
#define EAV_DTE_DMA_CSR_RESERVED_MASK                                     0xFFFFFFFA
#define    EAV_DTE_DMA_CSR_ENDIAN_SEL_SHIFT                               2
#define    EAV_DTE_DMA_CSR_ENDIAN_SEL_MASK                                0x00000004
#define    EAV_DTE_DMA_CSR_GLOBAL_PAUSE_SHIFT                             0
#define    EAV_DTE_DMA_CSR_GLOBAL_PAUSE_MASK                              0x00000001

#define EAV_DTE_DMA_INT_MASK_OFFSET                                       0x00000304
#define EAV_DTE_DMA_INT_MASK_TYPE                                         UInt32
#define EAV_DTE_DMA_INT_MASK_RESERVED_MASK                                0xFFFF0000
#define    EAV_DTE_DMA_INT_MASK_INT_EN_SHIFT                              0
#define    EAV_DTE_DMA_INT_MASK_INT_EN_MASK                               0x0000FFFF

#define EAV_DTE_DMA_INT_CSR_OFFSET                                        0x00000324
#define EAV_DTE_DMA_INT_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_INT_CSR_RESERVED_MASK                                 0xFFFF0000
#define    EAV_DTE_DMA_INT_CSR_INT_SRC_SHIFT                              0
#define    EAV_DTE_DMA_INT_CSR_INT_SRC_MASK                               0x0000FFFF

#define EAV_DTE_DMA_CH0_CSR_OFFSET                                        0x00000400
#define EAV_DTE_DMA_CH0_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_CH0_CSR_RESERVED_MASK                                 0x00119480
#define    EAV_DTE_DMA_CH0_CSR_INC2_SHIFT                                 30
#define    EAV_DTE_DMA_CH0_CSR_INC2_MASK                                  0xC0000000
#define    EAV_DTE_DMA_CH0_CSR_INC7_SHIFT                                 23
#define    EAV_DTE_DMA_CH0_CSR_INC7_MASK                                  0x3F800000
#define    EAV_DTE_DMA_CH0_CSR_INT_SRC2_SHIFT                             22
#define    EAV_DTE_DMA_CH0_CSR_INT_SRC2_MASK                              0x00400000
#define    EAV_DTE_DMA_CH0_CSR_INT_SRC1_SHIFT                             21
#define    EAV_DTE_DMA_CH0_CSR_INT_SRC1_MASK                              0x00200000
#define    EAV_DTE_DMA_CH0_CSR_INT_SRC2_EN_SHIFT                          19
#define    EAV_DTE_DMA_CH0_CSR_INT_SRC2_EN_MASK                           0x00080000
#define    EAV_DTE_DMA_CH0_CSR_INT_SRC1_EN_SHIFT                          18
#define    EAV_DTE_DMA_CH0_CSR_INT_SRC1_EN_MASK                           0x00040000
#define    EAV_DTE_DMA_CH0_CSR_INT_SRC0_EN_SHIFT                          17
#define    EAV_DTE_DMA_CH0_CSR_INT_SRC0_EN_MASK                           0x00020000
#define    EAV_DTE_DMA_CH0_CSR_PRIORITY_SHIFT                             13
#define    EAV_DTE_DMA_CH0_CSR_PRIORITY_MASK                              0x00006000
#define    EAV_DTE_DMA_CH0_CSR_DONE_SHIFT                                 11
#define    EAV_DTE_DMA_CH0_CSR_DONE_MASK                                  0x00000800
#define    EAV_DTE_DMA_CH0_CSR_CBUF_EN_SHIFT                              9
#define    EAV_DTE_DMA_CH0_CSR_CBUF_EN_MASK                               0x00000200
#define    EAV_DTE_DMA_CH0_CSR_SWPTR_EN_SHIFT                             8
#define    EAV_DTE_DMA_CH0_CSR_SWPTR_EN_MASK                              0x00000100
#define    EAV_DTE_DMA_CH0_CSR_ARS_EN_SHIFT                               6
#define    EAV_DTE_DMA_CH0_CSR_ARS_EN_MASK                                0x00000040
#define    EAV_DTE_DMA_CH0_CSR_HW_HSHK_EN_SHIFT                           5
#define    EAV_DTE_DMA_CH0_CSR_HW_HSHK_EN_MASK                            0x00000020
#define    EAV_DTE_DMA_CH0_CSR_SRC_INC_SEL_SHIFT                          4
#define    EAV_DTE_DMA_CH0_CSR_SRC_INC_SEL_MASK                           0x00000010
#define    EAV_DTE_DMA_CH0_CSR_DST_INC_SEL_SHIFT                          3
#define    EAV_DTE_DMA_CH0_CSR_DST_INC_SEL_MASK                           0x00000008
#define    EAV_DTE_DMA_CH0_CSR_SRC_SEL_SHIFT                              2
#define    EAV_DTE_DMA_CH0_CSR_SRC_SEL_MASK                               0x00000004
#define    EAV_DTE_DMA_CH0_CSR_DST_SEL_SHIFT                              1
#define    EAV_DTE_DMA_CH0_CSR_DST_SEL_MASK                               0x00000002
#define    EAV_DTE_DMA_CH0_CSR_ENABLE_SHIFT                               0
#define    EAV_DTE_DMA_CH0_CSR_ENABLE_MASK                                0x00000001

#define EAV_DTE_DMA_CH0_TXSZ_OFFSET                                       0x00000404
#define EAV_DTE_DMA_CH0_TXSZ_TYPE                                         UInt32
#define EAV_DTE_DMA_CH0_TXSZ_RESERVED_MASK                                0xFE00F000
#define    EAV_DTE_DMA_CH0_TXSZ_CHK_SZ_SHIFT                              16
#define    EAV_DTE_DMA_CH0_TXSZ_CHK_SZ_MASK                               0x01FF0000
#define    EAV_DTE_DMA_CH0_TXSZ_TOT_SZ_SHIFT                              0
#define    EAV_DTE_DMA_CH0_TXSZ_TOT_SZ_MASK                               0x00000FFF

#define EAV_DTE_DMA_CH0_SRC_ADDR_OFFSET                                   0x00000408
#define EAV_DTE_DMA_CH0_SRC_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH0_SRC_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH0_SRC_ADDR_SRC_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH0_SRC_ADDR_SRC_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH0_SRC_ADDR_EM_OFFSET                                0x0000040C
#define EAV_DTE_DMA_CH0_SRC_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH0_SRC_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH0_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH0_SRC_ADDR_EM_SRC_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH0_DST_ADDR_OFFSET                                   0x00000410
#define EAV_DTE_DMA_CH0_DST_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH0_DST_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH0_DST_ADDR_DST_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH0_DST_ADDR_DST_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH0_DST_ADDR_EM_OFFSET                                0x00000414
#define EAV_DTE_DMA_CH0_DST_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH0_DST_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH0_DST_ADDR_EM_DST_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH0_DST_ADDR_EM_DST_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH0_SPT_OFFSET                                        0x0000041C
#define EAV_DTE_DMA_CH0_SPT_TYPE                                          UInt32
#define EAV_DTE_DMA_CH0_SPT_RESERVED_MASK                                 0x00000003
#define    EAV_DTE_DMA_CH0_SPT_SWPTR_SHIFT                                2
#define    EAV_DTE_DMA_CH0_SPT_SWPTR_MASK                                 0xFFFFFFFC

#define EAV_DTE_DMA_CH1_CSR_OFFSET                                        0x00000420
#define EAV_DTE_DMA_CH1_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_CH1_CSR_RESERVED_MASK                                 0x00119480
#define    EAV_DTE_DMA_CH1_CSR_INC2_SHIFT                                 30
#define    EAV_DTE_DMA_CH1_CSR_INC2_MASK                                  0xC0000000
#define    EAV_DTE_DMA_CH1_CSR_INC7_SHIFT                                 23
#define    EAV_DTE_DMA_CH1_CSR_INC7_MASK                                  0x3F800000
#define    EAV_DTE_DMA_CH1_CSR_INT_SRC2_SHIFT                             22
#define    EAV_DTE_DMA_CH1_CSR_INT_SRC2_MASK                              0x00400000
#define    EAV_DTE_DMA_CH1_CSR_INT_SRC1_SHIFT                             21
#define    EAV_DTE_DMA_CH1_CSR_INT_SRC1_MASK                              0x00200000
#define    EAV_DTE_DMA_CH1_CSR_INT_SRC2_EN_SHIFT                          19
#define    EAV_DTE_DMA_CH1_CSR_INT_SRC2_EN_MASK                           0x00080000
#define    EAV_DTE_DMA_CH1_CSR_INT_SRC1_EN_SHIFT                          18
#define    EAV_DTE_DMA_CH1_CSR_INT_SRC1_EN_MASK                           0x00040000
#define    EAV_DTE_DMA_CH1_CSR_INT_SRC0_EN_SHIFT                          17
#define    EAV_DTE_DMA_CH1_CSR_INT_SRC0_EN_MASK                           0x00020000
#define    EAV_DTE_DMA_CH1_CSR_PRIORITY_SHIFT                             13
#define    EAV_DTE_DMA_CH1_CSR_PRIORITY_MASK                              0x00006000
#define    EAV_DTE_DMA_CH1_CSR_DONE_SHIFT                                 11
#define    EAV_DTE_DMA_CH1_CSR_DONE_MASK                                  0x00000800
#define    EAV_DTE_DMA_CH1_CSR_CBUF_EN_SHIFT                              9
#define    EAV_DTE_DMA_CH1_CSR_CBUF_EN_MASK                               0x00000200
#define    EAV_DTE_DMA_CH1_CSR_SWPTR_EN_SHIFT                             8
#define    EAV_DTE_DMA_CH1_CSR_SWPTR_EN_MASK                              0x00000100
#define    EAV_DTE_DMA_CH1_CSR_ARS_EN_SHIFT                               6
#define    EAV_DTE_DMA_CH1_CSR_ARS_EN_MASK                                0x00000040
#define    EAV_DTE_DMA_CH1_CSR_HW_HSHK_EN_SHIFT                           5
#define    EAV_DTE_DMA_CH1_CSR_HW_HSHK_EN_MASK                            0x00000020
#define    EAV_DTE_DMA_CH1_CSR_SRC_INC_SEL_SHIFT                          4
#define    EAV_DTE_DMA_CH1_CSR_SRC_INC_SEL_MASK                           0x00000010
#define    EAV_DTE_DMA_CH1_CSR_DST_INC_SEL_SHIFT                          3
#define    EAV_DTE_DMA_CH1_CSR_DST_INC_SEL_MASK                           0x00000008
#define    EAV_DTE_DMA_CH1_CSR_SRC_SEL_SHIFT                              2
#define    EAV_DTE_DMA_CH1_CSR_SRC_SEL_MASK                               0x00000004
#define    EAV_DTE_DMA_CH1_CSR_DST_SEL_SHIFT                              1
#define    EAV_DTE_DMA_CH1_CSR_DST_SEL_MASK                               0x00000002
#define    EAV_DTE_DMA_CH1_CSR_ENABLE_SHIFT                               0
#define    EAV_DTE_DMA_CH1_CSR_ENABLE_MASK                                0x00000001

#define EAV_DTE_DMA_CH1_TXSZ_OFFSET                                       0x00000424
#define EAV_DTE_DMA_CH1_TXSZ_TYPE                                         UInt32
#define EAV_DTE_DMA_CH1_TXSZ_RESERVED_MASK                                0xFE00F000
#define    EAV_DTE_DMA_CH1_TXSZ_CHK_SZ_SHIFT                              16
#define    EAV_DTE_DMA_CH1_TXSZ_CHK_SZ_MASK                               0x01FF0000
#define    EAV_DTE_DMA_CH1_TXSZ_TOT_SZ_SHIFT                              0
#define    EAV_DTE_DMA_CH1_TXSZ_TOT_SZ_MASK                               0x00000FFF

#define EAV_DTE_DMA_CH1_SRC_ADDR_OFFSET                                   0x00000428
#define EAV_DTE_DMA_CH1_SRC_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH1_SRC_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH1_SRC_ADDR_SRC_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH1_SRC_ADDR_SRC_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH1_SRC_ADDR_EM_OFFSET                                0x0000042C
#define EAV_DTE_DMA_CH1_SRC_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH1_SRC_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH1_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH1_SRC_ADDR_EM_SRC_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH1_DST_ADDR_OFFSET                                   0x00000430
#define EAV_DTE_DMA_CH1_DST_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH1_DST_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH1_DST_ADDR_DST_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH1_DST_ADDR_DST_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH1_DST_ADDR_EM_OFFSET                                0x00000434
#define EAV_DTE_DMA_CH1_DST_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH1_DST_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH1_DST_ADDR_EM_DST_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH1_DST_ADDR_EM_DST_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH1_SPT_OFFSET                                        0x0000043C
#define EAV_DTE_DMA_CH1_SPT_TYPE                                          UInt32
#define EAV_DTE_DMA_CH1_SPT_RESERVED_MASK                                 0x00000003
#define    EAV_DTE_DMA_CH1_SPT_SWPTR_SHIFT                                2
#define    EAV_DTE_DMA_CH1_SPT_SWPTR_MASK                                 0xFFFFFFFC

#define EAV_DTE_DMA_CH2_CSR_OFFSET                                        0x00000440
#define EAV_DTE_DMA_CH2_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_CH2_CSR_RESERVED_MASK                                 0x00119480
#define    EAV_DTE_DMA_CH2_CSR_INC2_SHIFT                                 30
#define    EAV_DTE_DMA_CH2_CSR_INC2_MASK                                  0xC0000000
#define    EAV_DTE_DMA_CH2_CSR_INC7_SHIFT                                 23
#define    EAV_DTE_DMA_CH2_CSR_INC7_MASK                                  0x3F800000
#define    EAV_DTE_DMA_CH2_CSR_INT_SRC2_SHIFT                             22
#define    EAV_DTE_DMA_CH2_CSR_INT_SRC2_MASK                              0x00400000
#define    EAV_DTE_DMA_CH2_CSR_INT_SRC1_SHIFT                             21
#define    EAV_DTE_DMA_CH2_CSR_INT_SRC1_MASK                              0x00200000
#define    EAV_DTE_DMA_CH2_CSR_INT_SRC2_EN_SHIFT                          19
#define    EAV_DTE_DMA_CH2_CSR_INT_SRC2_EN_MASK                           0x00080000
#define    EAV_DTE_DMA_CH2_CSR_INT_SRC1_EN_SHIFT                          18
#define    EAV_DTE_DMA_CH2_CSR_INT_SRC1_EN_MASK                           0x00040000
#define    EAV_DTE_DMA_CH2_CSR_INT_SRC0_EN_SHIFT                          17
#define    EAV_DTE_DMA_CH2_CSR_INT_SRC0_EN_MASK                           0x00020000
#define    EAV_DTE_DMA_CH2_CSR_PRIORITY_SHIFT                             13
#define    EAV_DTE_DMA_CH2_CSR_PRIORITY_MASK                              0x00006000
#define    EAV_DTE_DMA_CH2_CSR_DONE_SHIFT                                 11
#define    EAV_DTE_DMA_CH2_CSR_DONE_MASK                                  0x00000800
#define    EAV_DTE_DMA_CH2_CSR_CBUF_EN_SHIFT                              9
#define    EAV_DTE_DMA_CH2_CSR_CBUF_EN_MASK                               0x00000200
#define    EAV_DTE_DMA_CH2_CSR_SWPTR_EN_SHIFT                             8
#define    EAV_DTE_DMA_CH2_CSR_SWPTR_EN_MASK                              0x00000100
#define    EAV_DTE_DMA_CH2_CSR_ARS_EN_SHIFT                               6
#define    EAV_DTE_DMA_CH2_CSR_ARS_EN_MASK                                0x00000040
#define    EAV_DTE_DMA_CH2_CSR_HW_HSHK_EN_SHIFT                           5
#define    EAV_DTE_DMA_CH2_CSR_HW_HSHK_EN_MASK                            0x00000020
#define    EAV_DTE_DMA_CH2_CSR_SRC_INC_SEL_SHIFT                          4
#define    EAV_DTE_DMA_CH2_CSR_SRC_INC_SEL_MASK                           0x00000010
#define    EAV_DTE_DMA_CH2_CSR_DST_INC_SEL_SHIFT                          3
#define    EAV_DTE_DMA_CH2_CSR_DST_INC_SEL_MASK                           0x00000008
#define    EAV_DTE_DMA_CH2_CSR_SRC_SEL_SHIFT                              2
#define    EAV_DTE_DMA_CH2_CSR_SRC_SEL_MASK                               0x00000004
#define    EAV_DTE_DMA_CH2_CSR_DST_SEL_SHIFT                              1
#define    EAV_DTE_DMA_CH2_CSR_DST_SEL_MASK                               0x00000002
#define    EAV_DTE_DMA_CH2_CSR_ENABLE_SHIFT                               0
#define    EAV_DTE_DMA_CH2_CSR_ENABLE_MASK                                0x00000001

#define EAV_DTE_DMA_CH2_TXSZ_OFFSET                                       0x00000444
#define EAV_DTE_DMA_CH2_TXSZ_TYPE                                         UInt32
#define EAV_DTE_DMA_CH2_TXSZ_RESERVED_MASK                                0xFE00F000
#define    EAV_DTE_DMA_CH2_TXSZ_CHK_SZ_SHIFT                              16
#define    EAV_DTE_DMA_CH2_TXSZ_CHK_SZ_MASK                               0x01FF0000
#define    EAV_DTE_DMA_CH2_TXSZ_TOT_SZ_SHIFT                              0
#define    EAV_DTE_DMA_CH2_TXSZ_TOT_SZ_MASK                               0x00000FFF

#define EAV_DTE_DMA_CH2_SRC_ADDR_OFFSET                                   0x00000448
#define EAV_DTE_DMA_CH2_SRC_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH2_SRC_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH2_SRC_ADDR_SRC_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH2_SRC_ADDR_SRC_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH2_SRC_ADDR_EM_OFFSET                                0x0000044C
#define EAV_DTE_DMA_CH2_SRC_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH2_SRC_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH2_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH2_SRC_ADDR_EM_SRC_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH2_DST_ADDR_OFFSET                                   0x00000450
#define EAV_DTE_DMA_CH2_DST_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH2_DST_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH2_DST_ADDR_DST_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH2_DST_ADDR_DST_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH2_DST_ADDR_EM_OFFSET                                0x00000454
#define EAV_DTE_DMA_CH2_DST_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH2_DST_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH2_DST_ADDR_EM_DST_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH2_DST_ADDR_EM_DST_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH2_SPT_OFFSET                                        0x0000045C
#define EAV_DTE_DMA_CH2_SPT_TYPE                                          UInt32
#define EAV_DTE_DMA_CH2_SPT_RESERVED_MASK                                 0x00000003
#define    EAV_DTE_DMA_CH2_SPT_SWPTR_SHIFT                                2
#define    EAV_DTE_DMA_CH2_SPT_SWPTR_MASK                                 0xFFFFFFFC

#define EAV_DTE_DMA_CH3_CSR_OFFSET                                        0x00000460
#define EAV_DTE_DMA_CH3_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_CH3_CSR_RESERVED_MASK                                 0x00119480
#define    EAV_DTE_DMA_CH3_CSR_INC2_SHIFT                                 30
#define    EAV_DTE_DMA_CH3_CSR_INC2_MASK                                  0xC0000000
#define    EAV_DTE_DMA_CH3_CSR_INC7_SHIFT                                 23
#define    EAV_DTE_DMA_CH3_CSR_INC7_MASK                                  0x3F800000
#define    EAV_DTE_DMA_CH3_CSR_INT_SRC2_SHIFT                             22
#define    EAV_DTE_DMA_CH3_CSR_INT_SRC2_MASK                              0x00400000
#define    EAV_DTE_DMA_CH3_CSR_INT_SRC1_SHIFT                             21
#define    EAV_DTE_DMA_CH3_CSR_INT_SRC1_MASK                              0x00200000
#define    EAV_DTE_DMA_CH3_CSR_INT_SRC2_EN_SHIFT                          19
#define    EAV_DTE_DMA_CH3_CSR_INT_SRC2_EN_MASK                           0x00080000
#define    EAV_DTE_DMA_CH3_CSR_INT_SRC1_EN_SHIFT                          18
#define    EAV_DTE_DMA_CH3_CSR_INT_SRC1_EN_MASK                           0x00040000
#define    EAV_DTE_DMA_CH3_CSR_INT_SRC0_EN_SHIFT                          17
#define    EAV_DTE_DMA_CH3_CSR_INT_SRC0_EN_MASK                           0x00020000
#define    EAV_DTE_DMA_CH3_CSR_PRIORITY_SHIFT                             13
#define    EAV_DTE_DMA_CH3_CSR_PRIORITY_MASK                              0x00006000
#define    EAV_DTE_DMA_CH3_CSR_DONE_SHIFT                                 11
#define    EAV_DTE_DMA_CH3_CSR_DONE_MASK                                  0x00000800
#define    EAV_DTE_DMA_CH3_CSR_CBUF_EN_SHIFT                              9
#define    EAV_DTE_DMA_CH3_CSR_CBUF_EN_MASK                               0x00000200
#define    EAV_DTE_DMA_CH3_CSR_SWPTR_EN_SHIFT                             8
#define    EAV_DTE_DMA_CH3_CSR_SWPTR_EN_MASK                              0x00000100
#define    EAV_DTE_DMA_CH3_CSR_ARS_EN_SHIFT                               6
#define    EAV_DTE_DMA_CH3_CSR_ARS_EN_MASK                                0x00000040
#define    EAV_DTE_DMA_CH3_CSR_HW_HSHK_EN_SHIFT                           5
#define    EAV_DTE_DMA_CH3_CSR_HW_HSHK_EN_MASK                            0x00000020
#define    EAV_DTE_DMA_CH3_CSR_SRC_INC_SEL_SHIFT                          4
#define    EAV_DTE_DMA_CH3_CSR_SRC_INC_SEL_MASK                           0x00000010
#define    EAV_DTE_DMA_CH3_CSR_DST_INC_SEL_SHIFT                          3
#define    EAV_DTE_DMA_CH3_CSR_DST_INC_SEL_MASK                           0x00000008
#define    EAV_DTE_DMA_CH3_CSR_SRC_SEL_SHIFT                              2
#define    EAV_DTE_DMA_CH3_CSR_SRC_SEL_MASK                               0x00000004
#define    EAV_DTE_DMA_CH3_CSR_DST_SEL_SHIFT                              1
#define    EAV_DTE_DMA_CH3_CSR_DST_SEL_MASK                               0x00000002
#define    EAV_DTE_DMA_CH3_CSR_ENABLE_SHIFT                               0
#define    EAV_DTE_DMA_CH3_CSR_ENABLE_MASK                                0x00000001

#define EAV_DTE_DMA_CH3_TXSZ_OFFSET                                       0x00000464
#define EAV_DTE_DMA_CH3_TXSZ_TYPE                                         UInt32
#define EAV_DTE_DMA_CH3_TXSZ_RESERVED_MASK                                0xFE00F000
#define    EAV_DTE_DMA_CH3_TXSZ_CHK_SZ_SHIFT                              16
#define    EAV_DTE_DMA_CH3_TXSZ_CHK_SZ_MASK                               0x01FF0000
#define    EAV_DTE_DMA_CH3_TXSZ_TOT_SZ_SHIFT                              0
#define    EAV_DTE_DMA_CH3_TXSZ_TOT_SZ_MASK                               0x00000FFF

#define EAV_DTE_DMA_CH3_SRC_ADDR_OFFSET                                   0x00000468
#define EAV_DTE_DMA_CH3_SRC_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH3_SRC_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH3_SRC_ADDR_SRC_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH3_SRC_ADDR_SRC_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH3_SRC_ADDR_EM_OFFSET                                0x0000046C
#define EAV_DTE_DMA_CH3_SRC_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH3_SRC_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH3_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH3_SRC_ADDR_EM_SRC_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH3_DST_ADDR_OFFSET                                   0x00000470
#define EAV_DTE_DMA_CH3_DST_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH3_DST_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH3_DST_ADDR_DST_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH3_DST_ADDR_DST_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH3_DST_ADDR_EM_OFFSET                                0x00000474
#define EAV_DTE_DMA_CH3_DST_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH3_DST_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH3_DST_ADDR_EM_DST_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH3_DST_ADDR_EM_DST_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH3_SPT_OFFSET                                        0x0000047C
#define EAV_DTE_DMA_CH3_SPT_TYPE                                          UInt32
#define EAV_DTE_DMA_CH3_SPT_RESERVED_MASK                                 0x00000003
#define    EAV_DTE_DMA_CH3_SPT_SWPTR_SHIFT                                2
#define    EAV_DTE_DMA_CH3_SPT_SWPTR_MASK                                 0xFFFFFFFC

#define EAV_DTE_DMA_CH4_CSR_OFFSET                                        0x00000480
#define EAV_DTE_DMA_CH4_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_CH4_CSR_RESERVED_MASK                                 0x00119480
#define    EAV_DTE_DMA_CH4_CSR_INC2_SHIFT                                 30
#define    EAV_DTE_DMA_CH4_CSR_INC2_MASK                                  0xC0000000
#define    EAV_DTE_DMA_CH4_CSR_INC7_SHIFT                                 23
#define    EAV_DTE_DMA_CH4_CSR_INC7_MASK                                  0x3F800000
#define    EAV_DTE_DMA_CH4_CSR_INT_SRC2_SHIFT                             22
#define    EAV_DTE_DMA_CH4_CSR_INT_SRC2_MASK                              0x00400000
#define    EAV_DTE_DMA_CH4_CSR_INT_SRC1_SHIFT                             21
#define    EAV_DTE_DMA_CH4_CSR_INT_SRC1_MASK                              0x00200000
#define    EAV_DTE_DMA_CH4_CSR_INT_SRC2_EN_SHIFT                          19
#define    EAV_DTE_DMA_CH4_CSR_INT_SRC2_EN_MASK                           0x00080000
#define    EAV_DTE_DMA_CH4_CSR_INT_SRC1_EN_SHIFT                          18
#define    EAV_DTE_DMA_CH4_CSR_INT_SRC1_EN_MASK                           0x00040000
#define    EAV_DTE_DMA_CH4_CSR_INT_SRC0_EN_SHIFT                          17
#define    EAV_DTE_DMA_CH4_CSR_INT_SRC0_EN_MASK                           0x00020000
#define    EAV_DTE_DMA_CH4_CSR_PRIORITY_SHIFT                             13
#define    EAV_DTE_DMA_CH4_CSR_PRIORITY_MASK                              0x00006000
#define    EAV_DTE_DMA_CH4_CSR_DONE_SHIFT                                 11
#define    EAV_DTE_DMA_CH4_CSR_DONE_MASK                                  0x00000800
#define    EAV_DTE_DMA_CH4_CSR_CBUF_EN_SHIFT                              9
#define    EAV_DTE_DMA_CH4_CSR_CBUF_EN_MASK                               0x00000200
#define    EAV_DTE_DMA_CH4_CSR_SWPTR_EN_SHIFT                             8
#define    EAV_DTE_DMA_CH4_CSR_SWPTR_EN_MASK                              0x00000100
#define    EAV_DTE_DMA_CH4_CSR_ARS_EN_SHIFT                               6
#define    EAV_DTE_DMA_CH4_CSR_ARS_EN_MASK                                0x00000040
#define    EAV_DTE_DMA_CH4_CSR_HW_HSHK_EN_SHIFT                           5
#define    EAV_DTE_DMA_CH4_CSR_HW_HSHK_EN_MASK                            0x00000020
#define    EAV_DTE_DMA_CH4_CSR_SRC_INC_SEL_SHIFT                          4
#define    EAV_DTE_DMA_CH4_CSR_SRC_INC_SEL_MASK                           0x00000010
#define    EAV_DTE_DMA_CH4_CSR_DST_INC_SEL_SHIFT                          3
#define    EAV_DTE_DMA_CH4_CSR_DST_INC_SEL_MASK                           0x00000008
#define    EAV_DTE_DMA_CH4_CSR_SRC_SEL_SHIFT                              2
#define    EAV_DTE_DMA_CH4_CSR_SRC_SEL_MASK                               0x00000004
#define    EAV_DTE_DMA_CH4_CSR_DST_SEL_SHIFT                              1
#define    EAV_DTE_DMA_CH4_CSR_DST_SEL_MASK                               0x00000002
#define    EAV_DTE_DMA_CH4_CSR_ENABLE_SHIFT                               0
#define    EAV_DTE_DMA_CH4_CSR_ENABLE_MASK                                0x00000001

#define EAV_DTE_DMA_CH4_TXSZ_OFFSET                                       0x00000484
#define EAV_DTE_DMA_CH4_TXSZ_TYPE                                         UInt32
#define EAV_DTE_DMA_CH4_TXSZ_RESERVED_MASK                                0xFE00F000
#define    EAV_DTE_DMA_CH4_TXSZ_CHK_SZ_SHIFT                              16
#define    EAV_DTE_DMA_CH4_TXSZ_CHK_SZ_MASK                               0x01FF0000
#define    EAV_DTE_DMA_CH4_TXSZ_TOT_SZ_SHIFT                              0
#define    EAV_DTE_DMA_CH4_TXSZ_TOT_SZ_MASK                               0x00000FFF

#define EAV_DTE_DMA_CH4_SRC_ADDR_OFFSET                                   0x00000488
#define EAV_DTE_DMA_CH4_SRC_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH4_SRC_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH4_SRC_ADDR_SRC_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH4_SRC_ADDR_SRC_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH4_SRC_ADDR_EM_OFFSET                                0x0000048C
#define EAV_DTE_DMA_CH4_SRC_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH4_SRC_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH4_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH4_SRC_ADDR_EM_SRC_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH4_DST_ADDR_OFFSET                                   0x00000490
#define EAV_DTE_DMA_CH4_DST_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH4_DST_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH4_DST_ADDR_DST_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH4_DST_ADDR_DST_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH4_DST_ADDR_EM_OFFSET                                0x00000494
#define EAV_DTE_DMA_CH4_DST_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH4_DST_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH4_DST_ADDR_EM_DST_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH4_DST_ADDR_EM_DST_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH4_SPT_OFFSET                                        0x0000049C
#define EAV_DTE_DMA_CH4_SPT_TYPE                                          UInt32
#define EAV_DTE_DMA_CH4_SPT_RESERVED_MASK                                 0x00000003
#define    EAV_DTE_DMA_CH4_SPT_SWPTR_SHIFT                                2
#define    EAV_DTE_DMA_CH4_SPT_SWPTR_MASK                                 0xFFFFFFFC

#define EAV_DTE_DMA_CH5_CSR_OFFSET                                        0x000004A0
#define EAV_DTE_DMA_CH5_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_CH5_CSR_RESERVED_MASK                                 0x00119480
#define    EAV_DTE_DMA_CH5_CSR_INC2_SHIFT                                 30
#define    EAV_DTE_DMA_CH5_CSR_INC2_MASK                                  0xC0000000
#define    EAV_DTE_DMA_CH5_CSR_INC7_SHIFT                                 23
#define    EAV_DTE_DMA_CH5_CSR_INC7_MASK                                  0x3F800000
#define    EAV_DTE_DMA_CH5_CSR_INT_SRC2_SHIFT                             22
#define    EAV_DTE_DMA_CH5_CSR_INT_SRC2_MASK                              0x00400000
#define    EAV_DTE_DMA_CH5_CSR_INT_SRC1_SHIFT                             21
#define    EAV_DTE_DMA_CH5_CSR_INT_SRC1_MASK                              0x00200000
#define    EAV_DTE_DMA_CH5_CSR_INT_SRC2_EN_SHIFT                          19
#define    EAV_DTE_DMA_CH5_CSR_INT_SRC2_EN_MASK                           0x00080000
#define    EAV_DTE_DMA_CH5_CSR_INT_SRC1_EN_SHIFT                          18
#define    EAV_DTE_DMA_CH5_CSR_INT_SRC1_EN_MASK                           0x00040000
#define    EAV_DTE_DMA_CH5_CSR_INT_SRC0_EN_SHIFT                          17
#define    EAV_DTE_DMA_CH5_CSR_INT_SRC0_EN_MASK                           0x00020000
#define    EAV_DTE_DMA_CH5_CSR_PRIORITY_SHIFT                             13
#define    EAV_DTE_DMA_CH5_CSR_PRIORITY_MASK                              0x00006000
#define    EAV_DTE_DMA_CH5_CSR_DONE_SHIFT                                 11
#define    EAV_DTE_DMA_CH5_CSR_DONE_MASK                                  0x00000800
#define    EAV_DTE_DMA_CH5_CSR_CBUF_EN_SHIFT                              9
#define    EAV_DTE_DMA_CH5_CSR_CBUF_EN_MASK                               0x00000200
#define    EAV_DTE_DMA_CH5_CSR_SWPTR_EN_SHIFT                             8
#define    EAV_DTE_DMA_CH5_CSR_SWPTR_EN_MASK                              0x00000100
#define    EAV_DTE_DMA_CH5_CSR_ARS_EN_SHIFT                               6
#define    EAV_DTE_DMA_CH5_CSR_ARS_EN_MASK                                0x00000040
#define    EAV_DTE_DMA_CH5_CSR_HW_HSHK_EN_SHIFT                           5
#define    EAV_DTE_DMA_CH5_CSR_HW_HSHK_EN_MASK                            0x00000020
#define    EAV_DTE_DMA_CH5_CSR_SRC_INC_SEL_SHIFT                          4
#define    EAV_DTE_DMA_CH5_CSR_SRC_INC_SEL_MASK                           0x00000010
#define    EAV_DTE_DMA_CH5_CSR_DST_INC_SEL_SHIFT                          3
#define    EAV_DTE_DMA_CH5_CSR_DST_INC_SEL_MASK                           0x00000008
#define    EAV_DTE_DMA_CH5_CSR_SRC_SEL_SHIFT                              2
#define    EAV_DTE_DMA_CH5_CSR_SRC_SEL_MASK                               0x00000004
#define    EAV_DTE_DMA_CH5_CSR_DST_SEL_SHIFT                              1
#define    EAV_DTE_DMA_CH5_CSR_DST_SEL_MASK                               0x00000002
#define    EAV_DTE_DMA_CH5_CSR_ENABLE_SHIFT                               0
#define    EAV_DTE_DMA_CH5_CSR_ENABLE_MASK                                0x00000001

#define EAV_DTE_DMA_CH5_TXSZ_OFFSET                                       0x000004A4
#define EAV_DTE_DMA_CH5_TXSZ_TYPE                                         UInt32
#define EAV_DTE_DMA_CH5_TXSZ_RESERVED_MASK                                0xFE00F000
#define    EAV_DTE_DMA_CH5_TXSZ_CHK_SZ_SHIFT                              16
#define    EAV_DTE_DMA_CH5_TXSZ_CHK_SZ_MASK                               0x01FF0000
#define    EAV_DTE_DMA_CH5_TXSZ_TOT_SZ_SHIFT                              0
#define    EAV_DTE_DMA_CH5_TXSZ_TOT_SZ_MASK                               0x00000FFF

#define EAV_DTE_DMA_CH5_SRC_ADDR_OFFSET                                   0x000004A8
#define EAV_DTE_DMA_CH5_SRC_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH5_SRC_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH5_SRC_ADDR_SRC_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH5_SRC_ADDR_SRC_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH5_SRC_ADDR_EM_OFFSET                                0x000004AC
#define EAV_DTE_DMA_CH5_SRC_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH5_SRC_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH5_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH5_SRC_ADDR_EM_SRC_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH5_DST_ADDR_OFFSET                                   0x000004B0
#define EAV_DTE_DMA_CH5_DST_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH5_DST_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH5_DST_ADDR_DST_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH5_DST_ADDR_DST_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH5_DST_ADDR_EM_OFFSET                                0x000004B4
#define EAV_DTE_DMA_CH5_DST_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH5_DST_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH5_DST_ADDR_EM_DST_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH5_DST_ADDR_EM_DST_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH5_SPT_OFFSET                                        0x000004BC
#define EAV_DTE_DMA_CH5_SPT_TYPE                                          UInt32
#define EAV_DTE_DMA_CH5_SPT_RESERVED_MASK                                 0x00000003
#define    EAV_DTE_DMA_CH5_SPT_SWPTR_SHIFT                                2
#define    EAV_DTE_DMA_CH5_SPT_SWPTR_MASK                                 0xFFFFFFFC

#define EAV_DTE_DMA_CH6_CSR_OFFSET                                        0x000004C0
#define EAV_DTE_DMA_CH6_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_CH6_CSR_RESERVED_MASK                                 0x00119480
#define    EAV_DTE_DMA_CH6_CSR_INC2_SHIFT                                 30
#define    EAV_DTE_DMA_CH6_CSR_INC2_MASK                                  0xC0000000
#define    EAV_DTE_DMA_CH6_CSR_INC7_SHIFT                                 23
#define    EAV_DTE_DMA_CH6_CSR_INC7_MASK                                  0x3F800000
#define    EAV_DTE_DMA_CH6_CSR_INT_SRC2_SHIFT                             22
#define    EAV_DTE_DMA_CH6_CSR_INT_SRC2_MASK                              0x00400000
#define    EAV_DTE_DMA_CH6_CSR_INT_SRC1_SHIFT                             21
#define    EAV_DTE_DMA_CH6_CSR_INT_SRC1_MASK                              0x00200000
#define    EAV_DTE_DMA_CH6_CSR_INT_SRC2_EN_SHIFT                          19
#define    EAV_DTE_DMA_CH6_CSR_INT_SRC2_EN_MASK                           0x00080000
#define    EAV_DTE_DMA_CH6_CSR_INT_SRC1_EN_SHIFT                          18
#define    EAV_DTE_DMA_CH6_CSR_INT_SRC1_EN_MASK                           0x00040000
#define    EAV_DTE_DMA_CH6_CSR_INT_SRC0_EN_SHIFT                          17
#define    EAV_DTE_DMA_CH6_CSR_INT_SRC0_EN_MASK                           0x00020000
#define    EAV_DTE_DMA_CH6_CSR_PRIORITY_SHIFT                             13
#define    EAV_DTE_DMA_CH6_CSR_PRIORITY_MASK                              0x00006000
#define    EAV_DTE_DMA_CH6_CSR_DONE_SHIFT                                 11
#define    EAV_DTE_DMA_CH6_CSR_DONE_MASK                                  0x00000800
#define    EAV_DTE_DMA_CH6_CSR_CBUF_EN_SHIFT                              9
#define    EAV_DTE_DMA_CH6_CSR_CBUF_EN_MASK                               0x00000200
#define    EAV_DTE_DMA_CH6_CSR_SWPTR_EN_SHIFT                             8
#define    EAV_DTE_DMA_CH6_CSR_SWPTR_EN_MASK                              0x00000100
#define    EAV_DTE_DMA_CH6_CSR_ARS_EN_SHIFT                               6
#define    EAV_DTE_DMA_CH6_CSR_ARS_EN_MASK                                0x00000040
#define    EAV_DTE_DMA_CH6_CSR_HW_HSHK_EN_SHIFT                           5
#define    EAV_DTE_DMA_CH6_CSR_HW_HSHK_EN_MASK                            0x00000020
#define    EAV_DTE_DMA_CH6_CSR_SRC_INC_SEL_SHIFT                          4
#define    EAV_DTE_DMA_CH6_CSR_SRC_INC_SEL_MASK                           0x00000010
#define    EAV_DTE_DMA_CH6_CSR_DST_INC_SEL_SHIFT                          3
#define    EAV_DTE_DMA_CH6_CSR_DST_INC_SEL_MASK                           0x00000008
#define    EAV_DTE_DMA_CH6_CSR_SRC_SEL_SHIFT                              2
#define    EAV_DTE_DMA_CH6_CSR_SRC_SEL_MASK                               0x00000004
#define    EAV_DTE_DMA_CH6_CSR_DST_SEL_SHIFT                              1
#define    EAV_DTE_DMA_CH6_CSR_DST_SEL_MASK                               0x00000002
#define    EAV_DTE_DMA_CH6_CSR_ENABLE_SHIFT                               0
#define    EAV_DTE_DMA_CH6_CSR_ENABLE_MASK                                0x00000001

#define EAV_DTE_DMA_CH6_TXSZ_OFFSET                                       0x000004C4
#define EAV_DTE_DMA_CH6_TXSZ_TYPE                                         UInt32
#define EAV_DTE_DMA_CH6_TXSZ_RESERVED_MASK                                0xFE00F000
#define    EAV_DTE_DMA_CH6_TXSZ_CHK_SZ_SHIFT                              16
#define    EAV_DTE_DMA_CH6_TXSZ_CHK_SZ_MASK                               0x01FF0000
#define    EAV_DTE_DMA_CH6_TXSZ_TOT_SZ_SHIFT                              0
#define    EAV_DTE_DMA_CH6_TXSZ_TOT_SZ_MASK                               0x00000FFF

#define EAV_DTE_DMA_CH6_SRC_ADDR_OFFSET                                   0x000004C8
#define EAV_DTE_DMA_CH6_SRC_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH6_SRC_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH6_SRC_ADDR_SRC_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH6_SRC_ADDR_SRC_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH6_SRC_ADDR_EM_OFFSET                                0x000004CC
#define EAV_DTE_DMA_CH6_SRC_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH6_SRC_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH6_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH6_SRC_ADDR_EM_SRC_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH6_DST_ADDR_OFFSET                                   0x000004D0
#define EAV_DTE_DMA_CH6_DST_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH6_DST_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH6_DST_ADDR_DST_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH6_DST_ADDR_DST_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH6_DST_ADDR_EM_OFFSET                                0x000004D4
#define EAV_DTE_DMA_CH6_DST_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH6_DST_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH6_DST_ADDR_EM_DST_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH6_DST_ADDR_EM_DST_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH6_SPT_OFFSET                                        0x000004DC
#define EAV_DTE_DMA_CH6_SPT_TYPE                                          UInt32
#define EAV_DTE_DMA_CH6_SPT_RESERVED_MASK                                 0x00000003
#define    EAV_DTE_DMA_CH6_SPT_SWPTR_SHIFT                                2
#define    EAV_DTE_DMA_CH6_SPT_SWPTR_MASK                                 0xFFFFFFFC

#define EAV_DTE_DMA_CH7_CSR_OFFSET                                        0x000004E0
#define EAV_DTE_DMA_CH7_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_CH7_CSR_RESERVED_MASK                                 0x00119480
#define    EAV_DTE_DMA_CH7_CSR_INC2_SHIFT                                 30
#define    EAV_DTE_DMA_CH7_CSR_INC2_MASK                                  0xC0000000
#define    EAV_DTE_DMA_CH7_CSR_INC7_SHIFT                                 23
#define    EAV_DTE_DMA_CH7_CSR_INC7_MASK                                  0x3F800000
#define    EAV_DTE_DMA_CH7_CSR_INT_SRC2_SHIFT                             22
#define    EAV_DTE_DMA_CH7_CSR_INT_SRC2_MASK                              0x00400000
#define    EAV_DTE_DMA_CH7_CSR_INT_SRC1_SHIFT                             21
#define    EAV_DTE_DMA_CH7_CSR_INT_SRC1_MASK                              0x00200000
#define    EAV_DTE_DMA_CH7_CSR_INT_SRC2_EN_SHIFT                          19
#define    EAV_DTE_DMA_CH7_CSR_INT_SRC2_EN_MASK                           0x00080000
#define    EAV_DTE_DMA_CH7_CSR_INT_SRC1_EN_SHIFT                          18
#define    EAV_DTE_DMA_CH7_CSR_INT_SRC1_EN_MASK                           0x00040000
#define    EAV_DTE_DMA_CH7_CSR_INT_SRC0_EN_SHIFT                          17
#define    EAV_DTE_DMA_CH7_CSR_INT_SRC0_EN_MASK                           0x00020000
#define    EAV_DTE_DMA_CH7_CSR_PRIORITY_SHIFT                             13
#define    EAV_DTE_DMA_CH7_CSR_PRIORITY_MASK                              0x00006000
#define    EAV_DTE_DMA_CH7_CSR_DONE_SHIFT                                 11
#define    EAV_DTE_DMA_CH7_CSR_DONE_MASK                                  0x00000800
#define    EAV_DTE_DMA_CH7_CSR_CBUF_EN_SHIFT                              9
#define    EAV_DTE_DMA_CH7_CSR_CBUF_EN_MASK                               0x00000200
#define    EAV_DTE_DMA_CH7_CSR_SWPTR_EN_SHIFT                             8
#define    EAV_DTE_DMA_CH7_CSR_SWPTR_EN_MASK                              0x00000100
#define    EAV_DTE_DMA_CH7_CSR_ARS_EN_SHIFT                               6
#define    EAV_DTE_DMA_CH7_CSR_ARS_EN_MASK                                0x00000040
#define    EAV_DTE_DMA_CH7_CSR_HW_HSHK_EN_SHIFT                           5
#define    EAV_DTE_DMA_CH7_CSR_HW_HSHK_EN_MASK                            0x00000020
#define    EAV_DTE_DMA_CH7_CSR_SRC_INC_SEL_SHIFT                          4
#define    EAV_DTE_DMA_CH7_CSR_SRC_INC_SEL_MASK                           0x00000010
#define    EAV_DTE_DMA_CH7_CSR_DST_INC_SEL_SHIFT                          3
#define    EAV_DTE_DMA_CH7_CSR_DST_INC_SEL_MASK                           0x00000008
#define    EAV_DTE_DMA_CH7_CSR_SRC_SEL_SHIFT                              2
#define    EAV_DTE_DMA_CH7_CSR_SRC_SEL_MASK                               0x00000004
#define    EAV_DTE_DMA_CH7_CSR_DST_SEL_SHIFT                              1
#define    EAV_DTE_DMA_CH7_CSR_DST_SEL_MASK                               0x00000002
#define    EAV_DTE_DMA_CH7_CSR_ENABLE_SHIFT                               0
#define    EAV_DTE_DMA_CH7_CSR_ENABLE_MASK                                0x00000001

#define EAV_DTE_DMA_CH7_TXSZ_OFFSET                                       0x000004E4
#define EAV_DTE_DMA_CH7_TXSZ_TYPE                                         UInt32
#define EAV_DTE_DMA_CH7_TXSZ_RESERVED_MASK                                0xFE00F000
#define    EAV_DTE_DMA_CH7_TXSZ_CHK_SZ_SHIFT                              16
#define    EAV_DTE_DMA_CH7_TXSZ_CHK_SZ_MASK                               0x01FF0000
#define    EAV_DTE_DMA_CH7_TXSZ_TOT_SZ_SHIFT                              0
#define    EAV_DTE_DMA_CH7_TXSZ_TOT_SZ_MASK                               0x00000FFF

#define EAV_DTE_DMA_CH7_SRC_ADDR_OFFSET                                   0x000004E8
#define EAV_DTE_DMA_CH7_SRC_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH7_SRC_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH7_SRC_ADDR_SRC_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH7_SRC_ADDR_SRC_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH7_SRC_ADDR_EM_OFFSET                                0x000004EC
#define EAV_DTE_DMA_CH7_SRC_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH7_SRC_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH7_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH7_SRC_ADDR_EM_SRC_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH7_DST_ADDR_OFFSET                                   0x000004F0
#define EAV_DTE_DMA_CH7_DST_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH7_DST_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH7_DST_ADDR_DST_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH7_DST_ADDR_DST_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH7_DST_ADDR_EM_OFFSET                                0x000004F4
#define EAV_DTE_DMA_CH7_DST_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH7_DST_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH7_DST_ADDR_EM_DST_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH7_DST_ADDR_EM_DST_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH7_SPT_OFFSET                                        0x000004FC
#define EAV_DTE_DMA_CH7_SPT_TYPE                                          UInt32
#define EAV_DTE_DMA_CH7_SPT_RESERVED_MASK                                 0x00000003
#define    EAV_DTE_DMA_CH7_SPT_SWPTR_SHIFT                                2
#define    EAV_DTE_DMA_CH7_SPT_SWPTR_MASK                                 0xFFFFFFFC

#define EAV_DTE_DMA_CH8_CSR_OFFSET                                        0x00000500
#define EAV_DTE_DMA_CH8_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_CH8_CSR_RESERVED_MASK                                 0x00119480
#define    EAV_DTE_DMA_CH8_CSR_INC2_SHIFT                                 30
#define    EAV_DTE_DMA_CH8_CSR_INC2_MASK                                  0xC0000000
#define    EAV_DTE_DMA_CH8_CSR_INC7_SHIFT                                 23
#define    EAV_DTE_DMA_CH8_CSR_INC7_MASK                                  0x3F800000
#define    EAV_DTE_DMA_CH8_CSR_INT_SRC2_SHIFT                             22
#define    EAV_DTE_DMA_CH8_CSR_INT_SRC2_MASK                              0x00400000
#define    EAV_DTE_DMA_CH8_CSR_INT_SRC1_SHIFT                             21
#define    EAV_DTE_DMA_CH8_CSR_INT_SRC1_MASK                              0x00200000
#define    EAV_DTE_DMA_CH8_CSR_INT_SRC2_EN_SHIFT                          19
#define    EAV_DTE_DMA_CH8_CSR_INT_SRC2_EN_MASK                           0x00080000
#define    EAV_DTE_DMA_CH8_CSR_INT_SRC1_EN_SHIFT                          18
#define    EAV_DTE_DMA_CH8_CSR_INT_SRC1_EN_MASK                           0x00040000
#define    EAV_DTE_DMA_CH8_CSR_INT_SRC0_EN_SHIFT                          17
#define    EAV_DTE_DMA_CH8_CSR_INT_SRC0_EN_MASK                           0x00020000
#define    EAV_DTE_DMA_CH8_CSR_PRIORITY_SHIFT                             13
#define    EAV_DTE_DMA_CH8_CSR_PRIORITY_MASK                              0x00006000
#define    EAV_DTE_DMA_CH8_CSR_DONE_SHIFT                                 11
#define    EAV_DTE_DMA_CH8_CSR_DONE_MASK                                  0x00000800
#define    EAV_DTE_DMA_CH8_CSR_CBUF_EN_SHIFT                              9
#define    EAV_DTE_DMA_CH8_CSR_CBUF_EN_MASK                               0x00000200
#define    EAV_DTE_DMA_CH8_CSR_SWPTR_EN_SHIFT                             8
#define    EAV_DTE_DMA_CH8_CSR_SWPTR_EN_MASK                              0x00000100
#define    EAV_DTE_DMA_CH8_CSR_ARS_EN_SHIFT                               6
#define    EAV_DTE_DMA_CH8_CSR_ARS_EN_MASK                                0x00000040
#define    EAV_DTE_DMA_CH8_CSR_HW_HSHK_EN_SHIFT                           5
#define    EAV_DTE_DMA_CH8_CSR_HW_HSHK_EN_MASK                            0x00000020
#define    EAV_DTE_DMA_CH8_CSR_SRC_INC_SEL_SHIFT                          4
#define    EAV_DTE_DMA_CH8_CSR_SRC_INC_SEL_MASK                           0x00000010
#define    EAV_DTE_DMA_CH8_CSR_DST_INC_SEL_SHIFT                          3
#define    EAV_DTE_DMA_CH8_CSR_DST_INC_SEL_MASK                           0x00000008
#define    EAV_DTE_DMA_CH8_CSR_SRC_SEL_SHIFT                              2
#define    EAV_DTE_DMA_CH8_CSR_SRC_SEL_MASK                               0x00000004
#define    EAV_DTE_DMA_CH8_CSR_DST_SEL_SHIFT                              1
#define    EAV_DTE_DMA_CH8_CSR_DST_SEL_MASK                               0x00000002
#define    EAV_DTE_DMA_CH8_CSR_ENABLE_SHIFT                               0
#define    EAV_DTE_DMA_CH8_CSR_ENABLE_MASK                                0x00000001

#define EAV_DTE_DMA_CH8_TXSZ_OFFSET                                       0x00000504
#define EAV_DTE_DMA_CH8_TXSZ_TYPE                                         UInt32
#define EAV_DTE_DMA_CH8_TXSZ_RESERVED_MASK                                0xFE00F000
#define    EAV_DTE_DMA_CH8_TXSZ_CHK_SZ_SHIFT                              16
#define    EAV_DTE_DMA_CH8_TXSZ_CHK_SZ_MASK                               0x01FF0000
#define    EAV_DTE_DMA_CH8_TXSZ_TOT_SZ_SHIFT                              0
#define    EAV_DTE_DMA_CH8_TXSZ_TOT_SZ_MASK                               0x00000FFF

#define EAV_DTE_DMA_CH8_SRC_ADDR_OFFSET                                   0x00000508
#define EAV_DTE_DMA_CH8_SRC_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH8_SRC_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH8_SRC_ADDR_SRC_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH8_SRC_ADDR_SRC_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH8_SRC_ADDR_EM_OFFSET                                0x0000050C
#define EAV_DTE_DMA_CH8_SRC_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH8_SRC_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH8_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH8_SRC_ADDR_EM_SRC_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH8_DST_ADDR_OFFSET                                   0x00000510
#define EAV_DTE_DMA_CH8_DST_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH8_DST_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH8_DST_ADDR_DST_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH8_DST_ADDR_DST_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH8_DST_ADDR_EM_OFFSET                                0x00000514
#define EAV_DTE_DMA_CH8_DST_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH8_DST_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH8_DST_ADDR_EM_DST_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH8_DST_ADDR_EM_DST_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH8_SPT_OFFSET                                        0x0000051C
#define EAV_DTE_DMA_CH8_SPT_TYPE                                          UInt32
#define EAV_DTE_DMA_CH8_SPT_RESERVED_MASK                                 0x00000003
#define    EAV_DTE_DMA_CH8_SPT_SWPTR_SHIFT                                2
#define    EAV_DTE_DMA_CH8_SPT_SWPTR_MASK                                 0xFFFFFFFC

#define EAV_DTE_DMA_CH9_CSR_OFFSET                                        0x00000520
#define EAV_DTE_DMA_CH9_CSR_TYPE                                          UInt32
#define EAV_DTE_DMA_CH9_CSR_RESERVED_MASK                                 0x00119480
#define    EAV_DTE_DMA_CH9_CSR_INC2_SHIFT                                 30
#define    EAV_DTE_DMA_CH9_CSR_INC2_MASK                                  0xC0000000
#define    EAV_DTE_DMA_CH9_CSR_INC7_SHIFT                                 23
#define    EAV_DTE_DMA_CH9_CSR_INC7_MASK                                  0x3F800000
#define    EAV_DTE_DMA_CH9_CSR_INT_SRC2_SHIFT                             22
#define    EAV_DTE_DMA_CH9_CSR_INT_SRC2_MASK                              0x00400000
#define    EAV_DTE_DMA_CH9_CSR_INT_SRC1_SHIFT                             21
#define    EAV_DTE_DMA_CH9_CSR_INT_SRC1_MASK                              0x00200000
#define    EAV_DTE_DMA_CH9_CSR_INT_SRC2_EN_SHIFT                          19
#define    EAV_DTE_DMA_CH9_CSR_INT_SRC2_EN_MASK                           0x00080000
#define    EAV_DTE_DMA_CH9_CSR_INT_SRC1_EN_SHIFT                          18
#define    EAV_DTE_DMA_CH9_CSR_INT_SRC1_EN_MASK                           0x00040000
#define    EAV_DTE_DMA_CH9_CSR_INT_SRC0_EN_SHIFT                          17
#define    EAV_DTE_DMA_CH9_CSR_INT_SRC0_EN_MASK                           0x00020000
#define    EAV_DTE_DMA_CH9_CSR_PRIORITY_SHIFT                             13
#define    EAV_DTE_DMA_CH9_CSR_PRIORITY_MASK                              0x00006000
#define    EAV_DTE_DMA_CH9_CSR_DONE_SHIFT                                 11
#define    EAV_DTE_DMA_CH9_CSR_DONE_MASK                                  0x00000800
#define    EAV_DTE_DMA_CH9_CSR_CBUF_EN_SHIFT                              9
#define    EAV_DTE_DMA_CH9_CSR_CBUF_EN_MASK                               0x00000200
#define    EAV_DTE_DMA_CH9_CSR_SWPTR_EN_SHIFT                             8
#define    EAV_DTE_DMA_CH9_CSR_SWPTR_EN_MASK                              0x00000100
#define    EAV_DTE_DMA_CH9_CSR_ARS_EN_SHIFT                               6
#define    EAV_DTE_DMA_CH9_CSR_ARS_EN_MASK                                0x00000040
#define    EAV_DTE_DMA_CH9_CSR_HW_HSHK_EN_SHIFT                           5
#define    EAV_DTE_DMA_CH9_CSR_HW_HSHK_EN_MASK                            0x00000020
#define    EAV_DTE_DMA_CH9_CSR_SRC_INC_SEL_SHIFT                          4
#define    EAV_DTE_DMA_CH9_CSR_SRC_INC_SEL_MASK                           0x00000010
#define    EAV_DTE_DMA_CH9_CSR_DST_INC_SEL_SHIFT                          3
#define    EAV_DTE_DMA_CH9_CSR_DST_INC_SEL_MASK                           0x00000008
#define    EAV_DTE_DMA_CH9_CSR_SRC_SEL_SHIFT                              2
#define    EAV_DTE_DMA_CH9_CSR_SRC_SEL_MASK                               0x00000004
#define    EAV_DTE_DMA_CH9_CSR_DST_SEL_SHIFT                              1
#define    EAV_DTE_DMA_CH9_CSR_DST_SEL_MASK                               0x00000002
#define    EAV_DTE_DMA_CH9_CSR_ENABLE_SHIFT                               0
#define    EAV_DTE_DMA_CH9_CSR_ENABLE_MASK                                0x00000001

#define EAV_DTE_DMA_CH9_TXSZ_OFFSET                                       0x00000524
#define EAV_DTE_DMA_CH9_TXSZ_TYPE                                         UInt32
#define EAV_DTE_DMA_CH9_TXSZ_RESERVED_MASK                                0xFE00F000
#define    EAV_DTE_DMA_CH9_TXSZ_CHK_SZ_SHIFT                              16
#define    EAV_DTE_DMA_CH9_TXSZ_CHK_SZ_MASK                               0x01FF0000
#define    EAV_DTE_DMA_CH9_TXSZ_TOT_SZ_SHIFT                              0
#define    EAV_DTE_DMA_CH9_TXSZ_TOT_SZ_MASK                               0x00000FFF

#define EAV_DTE_DMA_CH9_SRC_ADDR_OFFSET                                   0x00000528
#define EAV_DTE_DMA_CH9_SRC_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH9_SRC_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH9_SRC_ADDR_SRC_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH9_SRC_ADDR_SRC_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH9_SRC_ADDR_EM_OFFSET                                0x0000052C
#define EAV_DTE_DMA_CH9_SRC_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH9_SRC_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH9_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH9_SRC_ADDR_EM_SRC_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH9_DST_ADDR_OFFSET                                   0x00000530
#define EAV_DTE_DMA_CH9_DST_ADDR_TYPE                                     UInt32
#define EAV_DTE_DMA_CH9_DST_ADDR_RESERVED_MASK                            0x00000003
#define    EAV_DTE_DMA_CH9_DST_ADDR_DST_ADDR_SHIFT                        2
#define    EAV_DTE_DMA_CH9_DST_ADDR_DST_ADDR_MASK                         0xFFFFFFFC

#define EAV_DTE_DMA_CH9_DST_ADDR_EM_OFFSET                                0x00000534
#define EAV_DTE_DMA_CH9_DST_ADDR_EM_TYPE                                  UInt32
#define EAV_DTE_DMA_CH9_DST_ADDR_EM_RESERVED_MASK                         0xFFFFFFE0
#define    EAV_DTE_DMA_CH9_DST_ADDR_EM_DST_ENC_MSK_SHIFT                  0
#define    EAV_DTE_DMA_CH9_DST_ADDR_EM_DST_ENC_MSK_MASK                   0x0000001F

#define EAV_DTE_DMA_CH9_SPT_OFFSET                                        0x0000053C
#define EAV_DTE_DMA_CH9_SPT_TYPE                                          UInt32
#define EAV_DTE_DMA_CH9_SPT_RESERVED_MASK                                 0x00000003
#define    EAV_DTE_DMA_CH9_SPT_SWPTR_SHIFT                                2
#define    EAV_DTE_DMA_CH9_SPT_SWPTR_MASK                                 0xFFFFFFFC

#define EAV_DTE_DMA_CH10_CSR_OFFSET                                       0x00000540
#define EAV_DTE_DMA_CH10_CSR_TYPE                                         UInt32
#define EAV_DTE_DMA_CH10_CSR_RESERVED_MASK                                0x00119480
#define    EAV_DTE_DMA_CH10_CSR_INC2_SHIFT                                30
#define    EAV_DTE_DMA_CH10_CSR_INC2_MASK                                 0xC0000000
#define    EAV_DTE_DMA_CH10_CSR_INC7_SHIFT                                23
#define    EAV_DTE_DMA_CH10_CSR_INC7_MASK                                 0x3F800000
#define    EAV_DTE_DMA_CH10_CSR_INT_SRC2_SHIFT                            22
#define    EAV_DTE_DMA_CH10_CSR_INT_SRC2_MASK                             0x00400000
#define    EAV_DTE_DMA_CH10_CSR_INT_SRC1_SHIFT                            21
#define    EAV_DTE_DMA_CH10_CSR_INT_SRC1_MASK                             0x00200000
#define    EAV_DTE_DMA_CH10_CSR_INT_SRC2_EN_SHIFT                         19
#define    EAV_DTE_DMA_CH10_CSR_INT_SRC2_EN_MASK                          0x00080000
#define    EAV_DTE_DMA_CH10_CSR_INT_SRC1_EN_SHIFT                         18
#define    EAV_DTE_DMA_CH10_CSR_INT_SRC1_EN_MASK                          0x00040000
#define    EAV_DTE_DMA_CH10_CSR_INT_SRC0_EN_SHIFT                         17
#define    EAV_DTE_DMA_CH10_CSR_INT_SRC0_EN_MASK                          0x00020000
#define    EAV_DTE_DMA_CH10_CSR_PRIORITY_SHIFT                            13
#define    EAV_DTE_DMA_CH10_CSR_PRIORITY_MASK                             0x00006000
#define    EAV_DTE_DMA_CH10_CSR_DONE_SHIFT                                11
#define    EAV_DTE_DMA_CH10_CSR_DONE_MASK                                 0x00000800
#define    EAV_DTE_DMA_CH10_CSR_CBUF_EN_SHIFT                             9
#define    EAV_DTE_DMA_CH10_CSR_CBUF_EN_MASK                              0x00000200
#define    EAV_DTE_DMA_CH10_CSR_SWPTR_EN_SHIFT                            8
#define    EAV_DTE_DMA_CH10_CSR_SWPTR_EN_MASK                             0x00000100
#define    EAV_DTE_DMA_CH10_CSR_ARS_EN_SHIFT                              6
#define    EAV_DTE_DMA_CH10_CSR_ARS_EN_MASK                               0x00000040
#define    EAV_DTE_DMA_CH10_CSR_HW_HSHK_EN_SHIFT                          5
#define    EAV_DTE_DMA_CH10_CSR_HW_HSHK_EN_MASK                           0x00000020
#define    EAV_DTE_DMA_CH10_CSR_SRC_INC_SEL_SHIFT                         4
#define    EAV_DTE_DMA_CH10_CSR_SRC_INC_SEL_MASK                          0x00000010
#define    EAV_DTE_DMA_CH10_CSR_DST_INC_SEL_SHIFT                         3
#define    EAV_DTE_DMA_CH10_CSR_DST_INC_SEL_MASK                          0x00000008
#define    EAV_DTE_DMA_CH10_CSR_SRC_SEL_SHIFT                             2
#define    EAV_DTE_DMA_CH10_CSR_SRC_SEL_MASK                              0x00000004
#define    EAV_DTE_DMA_CH10_CSR_DST_SEL_SHIFT                             1
#define    EAV_DTE_DMA_CH10_CSR_DST_SEL_MASK                              0x00000002
#define    EAV_DTE_DMA_CH10_CSR_ENABLE_SHIFT                              0
#define    EAV_DTE_DMA_CH10_CSR_ENABLE_MASK                               0x00000001

#define EAV_DTE_DMA_CH10_TXSZ_OFFSET                                      0x00000544
#define EAV_DTE_DMA_CH10_TXSZ_TYPE                                        UInt32
#define EAV_DTE_DMA_CH10_TXSZ_RESERVED_MASK                               0xFE00F000
#define    EAV_DTE_DMA_CH10_TXSZ_CHK_SZ_SHIFT                             16
#define    EAV_DTE_DMA_CH10_TXSZ_CHK_SZ_MASK                              0x01FF0000
#define    EAV_DTE_DMA_CH10_TXSZ_TOT_SZ_SHIFT                             0
#define    EAV_DTE_DMA_CH10_TXSZ_TOT_SZ_MASK                              0x00000FFF

#define EAV_DTE_DMA_CH10_SRC_ADDR_OFFSET                                  0x00000548
#define EAV_DTE_DMA_CH10_SRC_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH10_SRC_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH10_SRC_ADDR_SRC_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH10_SRC_ADDR_SRC_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH10_SRC_ADDR_EM_OFFSET                               0x0000054C
#define EAV_DTE_DMA_CH10_SRC_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH10_SRC_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH10_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH10_SRC_ADDR_EM_SRC_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH10_DST_ADDR_OFFSET                                  0x00000550
#define EAV_DTE_DMA_CH10_DST_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH10_DST_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH10_DST_ADDR_DST_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH10_DST_ADDR_DST_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH10_DST_ADDR_EM_OFFSET                               0x00000554
#define EAV_DTE_DMA_CH10_DST_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH10_DST_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH10_DST_ADDR_EM_DST_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH10_DST_ADDR_EM_DST_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH10_SPT_OFFSET                                       0x0000055C
#define EAV_DTE_DMA_CH10_SPT_TYPE                                         UInt32
#define EAV_DTE_DMA_CH10_SPT_RESERVED_MASK                                0x00000003
#define    EAV_DTE_DMA_CH10_SPT_SWPTR_SHIFT                               2
#define    EAV_DTE_DMA_CH10_SPT_SWPTR_MASK                                0xFFFFFFFC

#define EAV_DTE_DMA_CH11_CSR_OFFSET                                       0x00000560
#define EAV_DTE_DMA_CH11_CSR_TYPE                                         UInt32
#define EAV_DTE_DMA_CH11_CSR_RESERVED_MASK                                0x00119480
#define    EAV_DTE_DMA_CH11_CSR_INC2_SHIFT                                30
#define    EAV_DTE_DMA_CH11_CSR_INC2_MASK                                 0xC0000000
#define    EAV_DTE_DMA_CH11_CSR_INC7_SHIFT                                23
#define    EAV_DTE_DMA_CH11_CSR_INC7_MASK                                 0x3F800000
#define    EAV_DTE_DMA_CH11_CSR_INT_SRC2_SHIFT                            22
#define    EAV_DTE_DMA_CH11_CSR_INT_SRC2_MASK                             0x00400000
#define    EAV_DTE_DMA_CH11_CSR_INT_SRC1_SHIFT                            21
#define    EAV_DTE_DMA_CH11_CSR_INT_SRC1_MASK                             0x00200000
#define    EAV_DTE_DMA_CH11_CSR_INT_SRC2_EN_SHIFT                         19
#define    EAV_DTE_DMA_CH11_CSR_INT_SRC2_EN_MASK                          0x00080000
#define    EAV_DTE_DMA_CH11_CSR_INT_SRC1_EN_SHIFT                         18
#define    EAV_DTE_DMA_CH11_CSR_INT_SRC1_EN_MASK                          0x00040000
#define    EAV_DTE_DMA_CH11_CSR_INT_SRC0_EN_SHIFT                         17
#define    EAV_DTE_DMA_CH11_CSR_INT_SRC0_EN_MASK                          0x00020000
#define    EAV_DTE_DMA_CH11_CSR_PRIORITY_SHIFT                            13
#define    EAV_DTE_DMA_CH11_CSR_PRIORITY_MASK                             0x00006000
#define    EAV_DTE_DMA_CH11_CSR_DONE_SHIFT                                11
#define    EAV_DTE_DMA_CH11_CSR_DONE_MASK                                 0x00000800
#define    EAV_DTE_DMA_CH11_CSR_CBUF_EN_SHIFT                             9
#define    EAV_DTE_DMA_CH11_CSR_CBUF_EN_MASK                              0x00000200
#define    EAV_DTE_DMA_CH11_CSR_SWPTR_EN_SHIFT                            8
#define    EAV_DTE_DMA_CH11_CSR_SWPTR_EN_MASK                             0x00000100
#define    EAV_DTE_DMA_CH11_CSR_ARS_EN_SHIFT                              6
#define    EAV_DTE_DMA_CH11_CSR_ARS_EN_MASK                               0x00000040
#define    EAV_DTE_DMA_CH11_CSR_HW_HSHK_EN_SHIFT                          5
#define    EAV_DTE_DMA_CH11_CSR_HW_HSHK_EN_MASK                           0x00000020
#define    EAV_DTE_DMA_CH11_CSR_SRC_INC_SEL_SHIFT                         4
#define    EAV_DTE_DMA_CH11_CSR_SRC_INC_SEL_MASK                          0x00000010
#define    EAV_DTE_DMA_CH11_CSR_DST_INC_SEL_SHIFT                         3
#define    EAV_DTE_DMA_CH11_CSR_DST_INC_SEL_MASK                          0x00000008
#define    EAV_DTE_DMA_CH11_CSR_SRC_SEL_SHIFT                             2
#define    EAV_DTE_DMA_CH11_CSR_SRC_SEL_MASK                              0x00000004
#define    EAV_DTE_DMA_CH11_CSR_DST_SEL_SHIFT                             1
#define    EAV_DTE_DMA_CH11_CSR_DST_SEL_MASK                              0x00000002
#define    EAV_DTE_DMA_CH11_CSR_ENABLE_SHIFT                              0
#define    EAV_DTE_DMA_CH11_CSR_ENABLE_MASK                               0x00000001

#define EAV_DTE_DMA_CH11_TXSZ_OFFSET                                      0x00000564
#define EAV_DTE_DMA_CH11_TXSZ_TYPE                                        UInt32
#define EAV_DTE_DMA_CH11_TXSZ_RESERVED_MASK                               0xFE00F000
#define    EAV_DTE_DMA_CH11_TXSZ_CHK_SZ_SHIFT                             16
#define    EAV_DTE_DMA_CH11_TXSZ_CHK_SZ_MASK                              0x01FF0000
#define    EAV_DTE_DMA_CH11_TXSZ_TOT_SZ_SHIFT                             0
#define    EAV_DTE_DMA_CH11_TXSZ_TOT_SZ_MASK                              0x00000FFF

#define EAV_DTE_DMA_CH11_SRC_ADDR_OFFSET                                  0x00000568
#define EAV_DTE_DMA_CH11_SRC_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH11_SRC_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH11_SRC_ADDR_SRC_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH11_SRC_ADDR_SRC_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH11_SRC_ADDR_EM_OFFSET                               0x0000056C
#define EAV_DTE_DMA_CH11_SRC_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH11_SRC_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH11_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH11_SRC_ADDR_EM_SRC_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH11_DST_ADDR_OFFSET                                  0x00000570
#define EAV_DTE_DMA_CH11_DST_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH11_DST_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH11_DST_ADDR_DST_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH11_DST_ADDR_DST_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH11_DST_ADDR_EM_OFFSET                               0x00000574
#define EAV_DTE_DMA_CH11_DST_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH11_DST_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH11_DST_ADDR_EM_DST_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH11_DST_ADDR_EM_DST_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH11_SPT_OFFSET                                       0x0000057C
#define EAV_DTE_DMA_CH11_SPT_TYPE                                         UInt32
#define EAV_DTE_DMA_CH11_SPT_RESERVED_MASK                                0x00000003
#define    EAV_DTE_DMA_CH11_SPT_SWPTR_SHIFT                               2
#define    EAV_DTE_DMA_CH11_SPT_SWPTR_MASK                                0xFFFFFFFC

#define EAV_DTE_DMA_CH12_CSR_OFFSET                                       0x00000580
#define EAV_DTE_DMA_CH12_CSR_TYPE                                         UInt32
#define EAV_DTE_DMA_CH12_CSR_RESERVED_MASK                                0x00119480
#define    EAV_DTE_DMA_CH12_CSR_INC2_SHIFT                                30
#define    EAV_DTE_DMA_CH12_CSR_INC2_MASK                                 0xC0000000
#define    EAV_DTE_DMA_CH12_CSR_INC7_SHIFT                                23
#define    EAV_DTE_DMA_CH12_CSR_INC7_MASK                                 0x3F800000
#define    EAV_DTE_DMA_CH12_CSR_INT_SRC2_SHIFT                            22
#define    EAV_DTE_DMA_CH12_CSR_INT_SRC2_MASK                             0x00400000
#define    EAV_DTE_DMA_CH12_CSR_INT_SRC1_SHIFT                            21
#define    EAV_DTE_DMA_CH12_CSR_INT_SRC1_MASK                             0x00200000
#define    EAV_DTE_DMA_CH12_CSR_INT_SRC2_EN_SHIFT                         19
#define    EAV_DTE_DMA_CH12_CSR_INT_SRC2_EN_MASK                          0x00080000
#define    EAV_DTE_DMA_CH12_CSR_INT_SRC1_EN_SHIFT                         18
#define    EAV_DTE_DMA_CH12_CSR_INT_SRC1_EN_MASK                          0x00040000
#define    EAV_DTE_DMA_CH12_CSR_INT_SRC0_EN_SHIFT                         17
#define    EAV_DTE_DMA_CH12_CSR_INT_SRC0_EN_MASK                          0x00020000
#define    EAV_DTE_DMA_CH12_CSR_PRIORITY_SHIFT                            13
#define    EAV_DTE_DMA_CH12_CSR_PRIORITY_MASK                             0x00006000
#define    EAV_DTE_DMA_CH12_CSR_DONE_SHIFT                                11
#define    EAV_DTE_DMA_CH12_CSR_DONE_MASK                                 0x00000800
#define    EAV_DTE_DMA_CH12_CSR_CBUF_EN_SHIFT                             9
#define    EAV_DTE_DMA_CH12_CSR_CBUF_EN_MASK                              0x00000200
#define    EAV_DTE_DMA_CH12_CSR_SWPTR_EN_SHIFT                            8
#define    EAV_DTE_DMA_CH12_CSR_SWPTR_EN_MASK                             0x00000100
#define    EAV_DTE_DMA_CH12_CSR_ARS_EN_SHIFT                              6
#define    EAV_DTE_DMA_CH12_CSR_ARS_EN_MASK                               0x00000040
#define    EAV_DTE_DMA_CH12_CSR_HW_HSHK_EN_SHIFT                          5
#define    EAV_DTE_DMA_CH12_CSR_HW_HSHK_EN_MASK                           0x00000020
#define    EAV_DTE_DMA_CH12_CSR_SRC_INC_SEL_SHIFT                         4
#define    EAV_DTE_DMA_CH12_CSR_SRC_INC_SEL_MASK                          0x00000010
#define    EAV_DTE_DMA_CH12_CSR_DST_INC_SEL_SHIFT                         3
#define    EAV_DTE_DMA_CH12_CSR_DST_INC_SEL_MASK                          0x00000008
#define    EAV_DTE_DMA_CH12_CSR_SRC_SEL_SHIFT                             2
#define    EAV_DTE_DMA_CH12_CSR_SRC_SEL_MASK                              0x00000004
#define    EAV_DTE_DMA_CH12_CSR_DST_SEL_SHIFT                             1
#define    EAV_DTE_DMA_CH12_CSR_DST_SEL_MASK                              0x00000002
#define    EAV_DTE_DMA_CH12_CSR_ENABLE_SHIFT                              0
#define    EAV_DTE_DMA_CH12_CSR_ENABLE_MASK                               0x00000001

#define EAV_DTE_DMA_CH12_TXSZ_OFFSET                                      0x00000584
#define EAV_DTE_DMA_CH12_TXSZ_TYPE                                        UInt32
#define EAV_DTE_DMA_CH12_TXSZ_RESERVED_MASK                               0xFE00F000
#define    EAV_DTE_DMA_CH12_TXSZ_CHK_SZ_SHIFT                             16
#define    EAV_DTE_DMA_CH12_TXSZ_CHK_SZ_MASK                              0x01FF0000
#define    EAV_DTE_DMA_CH12_TXSZ_TOT_SZ_SHIFT                             0
#define    EAV_DTE_DMA_CH12_TXSZ_TOT_SZ_MASK                              0x00000FFF

#define EAV_DTE_DMA_CH12_SRC_ADDR_OFFSET                                  0x00000588
#define EAV_DTE_DMA_CH12_SRC_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH12_SRC_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH12_SRC_ADDR_SRC_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH12_SRC_ADDR_SRC_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH12_SRC_ADDR_EM_OFFSET                               0x0000058C
#define EAV_DTE_DMA_CH12_SRC_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH12_SRC_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH12_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH12_SRC_ADDR_EM_SRC_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH12_DST_ADDR_OFFSET                                  0x00000590
#define EAV_DTE_DMA_CH12_DST_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH12_DST_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH12_DST_ADDR_DST_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH12_DST_ADDR_DST_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH12_DST_ADDR_EM_OFFSET                               0x00000594
#define EAV_DTE_DMA_CH12_DST_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH12_DST_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH12_DST_ADDR_EM_DST_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH12_DST_ADDR_EM_DST_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH12_SPT_OFFSET                                       0x0000059C
#define EAV_DTE_DMA_CH12_SPT_TYPE                                         UInt32
#define EAV_DTE_DMA_CH12_SPT_RESERVED_MASK                                0x00000003
#define    EAV_DTE_DMA_CH12_SPT_SWPTR_SHIFT                               2
#define    EAV_DTE_DMA_CH12_SPT_SWPTR_MASK                                0xFFFFFFFC

#define EAV_DTE_DMA_CH13_CSR_OFFSET                                       0x000005A0
#define EAV_DTE_DMA_CH13_CSR_TYPE                                         UInt32
#define EAV_DTE_DMA_CH13_CSR_RESERVED_MASK                                0x00119480
#define    EAV_DTE_DMA_CH13_CSR_INC2_SHIFT                                30
#define    EAV_DTE_DMA_CH13_CSR_INC2_MASK                                 0xC0000000
#define    EAV_DTE_DMA_CH13_CSR_INC7_SHIFT                                23
#define    EAV_DTE_DMA_CH13_CSR_INC7_MASK                                 0x3F800000
#define    EAV_DTE_DMA_CH13_CSR_INT_SRC2_SHIFT                            22
#define    EAV_DTE_DMA_CH13_CSR_INT_SRC2_MASK                             0x00400000
#define    EAV_DTE_DMA_CH13_CSR_INT_SRC1_SHIFT                            21
#define    EAV_DTE_DMA_CH13_CSR_INT_SRC1_MASK                             0x00200000
#define    EAV_DTE_DMA_CH13_CSR_INT_SRC2_EN_SHIFT                         19
#define    EAV_DTE_DMA_CH13_CSR_INT_SRC2_EN_MASK                          0x00080000
#define    EAV_DTE_DMA_CH13_CSR_INT_SRC1_EN_SHIFT                         18
#define    EAV_DTE_DMA_CH13_CSR_INT_SRC1_EN_MASK                          0x00040000
#define    EAV_DTE_DMA_CH13_CSR_INT_SRC0_EN_SHIFT                         17
#define    EAV_DTE_DMA_CH13_CSR_INT_SRC0_EN_MASK                          0x00020000
#define    EAV_DTE_DMA_CH13_CSR_PRIORITY_SHIFT                            13
#define    EAV_DTE_DMA_CH13_CSR_PRIORITY_MASK                             0x00006000
#define    EAV_DTE_DMA_CH13_CSR_DONE_SHIFT                                11
#define    EAV_DTE_DMA_CH13_CSR_DONE_MASK                                 0x00000800
#define    EAV_DTE_DMA_CH13_CSR_CBUF_EN_SHIFT                             9
#define    EAV_DTE_DMA_CH13_CSR_CBUF_EN_MASK                              0x00000200
#define    EAV_DTE_DMA_CH13_CSR_SWPTR_EN_SHIFT                            8
#define    EAV_DTE_DMA_CH13_CSR_SWPTR_EN_MASK                             0x00000100
#define    EAV_DTE_DMA_CH13_CSR_ARS_EN_SHIFT                              6
#define    EAV_DTE_DMA_CH13_CSR_ARS_EN_MASK                               0x00000040
#define    EAV_DTE_DMA_CH13_CSR_HW_HSHK_EN_SHIFT                          5
#define    EAV_DTE_DMA_CH13_CSR_HW_HSHK_EN_MASK                           0x00000020
#define    EAV_DTE_DMA_CH13_CSR_SRC_INC_SEL_SHIFT                         4
#define    EAV_DTE_DMA_CH13_CSR_SRC_INC_SEL_MASK                          0x00000010
#define    EAV_DTE_DMA_CH13_CSR_DST_INC_SEL_SHIFT                         3
#define    EAV_DTE_DMA_CH13_CSR_DST_INC_SEL_MASK                          0x00000008
#define    EAV_DTE_DMA_CH13_CSR_SRC_SEL_SHIFT                             2
#define    EAV_DTE_DMA_CH13_CSR_SRC_SEL_MASK                              0x00000004
#define    EAV_DTE_DMA_CH13_CSR_DST_SEL_SHIFT                             1
#define    EAV_DTE_DMA_CH13_CSR_DST_SEL_MASK                              0x00000002
#define    EAV_DTE_DMA_CH13_CSR_ENABLE_SHIFT                              0
#define    EAV_DTE_DMA_CH13_CSR_ENABLE_MASK                               0x00000001

#define EAV_DTE_DMA_CH13_TXSZ_OFFSET                                      0x000005A4
#define EAV_DTE_DMA_CH13_TXSZ_TYPE                                        UInt32
#define EAV_DTE_DMA_CH13_TXSZ_RESERVED_MASK                               0xFE00F000
#define    EAV_DTE_DMA_CH13_TXSZ_CHK_SZ_SHIFT                             16
#define    EAV_DTE_DMA_CH13_TXSZ_CHK_SZ_MASK                              0x01FF0000
#define    EAV_DTE_DMA_CH13_TXSZ_TOT_SZ_SHIFT                             0
#define    EAV_DTE_DMA_CH13_TXSZ_TOT_SZ_MASK                              0x00000FFF

#define EAV_DTE_DMA_CH13_SRC_ADDR_OFFSET                                  0x000005A8
#define EAV_DTE_DMA_CH13_SRC_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH13_SRC_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH13_SRC_ADDR_SRC_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH13_SRC_ADDR_SRC_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH13_SRC_ADDR_EM_OFFSET                               0x000005AC
#define EAV_DTE_DMA_CH13_SRC_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH13_SRC_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH13_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH13_SRC_ADDR_EM_SRC_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH13_DST_ADDR_OFFSET                                  0x000005B0
#define EAV_DTE_DMA_CH13_DST_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH13_DST_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH13_DST_ADDR_DST_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH13_DST_ADDR_DST_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH13_DST_ADDR_EM_OFFSET                               0x000005B4
#define EAV_DTE_DMA_CH13_DST_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH13_DST_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH13_DST_ADDR_EM_DST_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH13_DST_ADDR_EM_DST_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH13_SPT_OFFSET                                       0x000005BC
#define EAV_DTE_DMA_CH13_SPT_TYPE                                         UInt32
#define EAV_DTE_DMA_CH13_SPT_RESERVED_MASK                                0x00000003
#define    EAV_DTE_DMA_CH13_SPT_SWPTR_SHIFT                               2
#define    EAV_DTE_DMA_CH13_SPT_SWPTR_MASK                                0xFFFFFFFC

#define EAV_DTE_DMA_CH14_CSR_OFFSET                                       0x000005C0
#define EAV_DTE_DMA_CH14_CSR_TYPE                                         UInt32
#define EAV_DTE_DMA_CH14_CSR_RESERVED_MASK                                0x00119480
#define    EAV_DTE_DMA_CH14_CSR_INC2_SHIFT                                30
#define    EAV_DTE_DMA_CH14_CSR_INC2_MASK                                 0xC0000000
#define    EAV_DTE_DMA_CH14_CSR_INC7_SHIFT                                23
#define    EAV_DTE_DMA_CH14_CSR_INC7_MASK                                 0x3F800000
#define    EAV_DTE_DMA_CH14_CSR_INT_SRC2_SHIFT                            22
#define    EAV_DTE_DMA_CH14_CSR_INT_SRC2_MASK                             0x00400000
#define    EAV_DTE_DMA_CH14_CSR_INT_SRC1_SHIFT                            21
#define    EAV_DTE_DMA_CH14_CSR_INT_SRC1_MASK                             0x00200000
#define    EAV_DTE_DMA_CH14_CSR_INT_SRC2_EN_SHIFT                         19
#define    EAV_DTE_DMA_CH14_CSR_INT_SRC2_EN_MASK                          0x00080000
#define    EAV_DTE_DMA_CH14_CSR_INT_SRC1_EN_SHIFT                         18
#define    EAV_DTE_DMA_CH14_CSR_INT_SRC1_EN_MASK                          0x00040000
#define    EAV_DTE_DMA_CH14_CSR_INT_SRC0_EN_SHIFT                         17
#define    EAV_DTE_DMA_CH14_CSR_INT_SRC0_EN_MASK                          0x00020000
#define    EAV_DTE_DMA_CH14_CSR_PRIORITY_SHIFT                            13
#define    EAV_DTE_DMA_CH14_CSR_PRIORITY_MASK                             0x00006000
#define    EAV_DTE_DMA_CH14_CSR_DONE_SHIFT                                11
#define    EAV_DTE_DMA_CH14_CSR_DONE_MASK                                 0x00000800
#define    EAV_DTE_DMA_CH14_CSR_CBUF_EN_SHIFT                             9
#define    EAV_DTE_DMA_CH14_CSR_CBUF_EN_MASK                              0x00000200
#define    EAV_DTE_DMA_CH14_CSR_SWPTR_EN_SHIFT                            8
#define    EAV_DTE_DMA_CH14_CSR_SWPTR_EN_MASK                             0x00000100
#define    EAV_DTE_DMA_CH14_CSR_ARS_EN_SHIFT                              6
#define    EAV_DTE_DMA_CH14_CSR_ARS_EN_MASK                               0x00000040
#define    EAV_DTE_DMA_CH14_CSR_HW_HSHK_EN_SHIFT                          5
#define    EAV_DTE_DMA_CH14_CSR_HW_HSHK_EN_MASK                           0x00000020
#define    EAV_DTE_DMA_CH14_CSR_SRC_INC_SEL_SHIFT                         4
#define    EAV_DTE_DMA_CH14_CSR_SRC_INC_SEL_MASK                          0x00000010
#define    EAV_DTE_DMA_CH14_CSR_DST_INC_SEL_SHIFT                         3
#define    EAV_DTE_DMA_CH14_CSR_DST_INC_SEL_MASK                          0x00000008
#define    EAV_DTE_DMA_CH14_CSR_SRC_SEL_SHIFT                             2
#define    EAV_DTE_DMA_CH14_CSR_SRC_SEL_MASK                              0x00000004
#define    EAV_DTE_DMA_CH14_CSR_DST_SEL_SHIFT                             1
#define    EAV_DTE_DMA_CH14_CSR_DST_SEL_MASK                              0x00000002
#define    EAV_DTE_DMA_CH14_CSR_ENABLE_SHIFT                              0
#define    EAV_DTE_DMA_CH14_CSR_ENABLE_MASK                               0x00000001

#define EAV_DTE_DMA_CH14_TXSZ_OFFSET                                      0x000005C4
#define EAV_DTE_DMA_CH14_TXSZ_TYPE                                        UInt32
#define EAV_DTE_DMA_CH14_TXSZ_RESERVED_MASK                               0xFE00F000
#define    EAV_DTE_DMA_CH14_TXSZ_CHK_SZ_SHIFT                             16
#define    EAV_DTE_DMA_CH14_TXSZ_CHK_SZ_MASK                              0x01FF0000
#define    EAV_DTE_DMA_CH14_TXSZ_TOT_SZ_SHIFT                             0
#define    EAV_DTE_DMA_CH14_TXSZ_TOT_SZ_MASK                              0x00000FFF

#define EAV_DTE_DMA_CH14_SRC_ADDR_OFFSET                                  0x000005C8
#define EAV_DTE_DMA_CH14_SRC_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH14_SRC_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH14_SRC_ADDR_SRC_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH14_SRC_ADDR_SRC_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH14_SRC_ADDR_EM_OFFSET                               0x000005CC
#define EAV_DTE_DMA_CH14_SRC_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH14_SRC_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH14_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH14_SRC_ADDR_EM_SRC_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH14_DST_ADDR_OFFSET                                  0x000005D0
#define EAV_DTE_DMA_CH14_DST_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH14_DST_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH14_DST_ADDR_DST_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH14_DST_ADDR_DST_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH14_DST_ADDR_EM_OFFSET                               0x000005D4
#define EAV_DTE_DMA_CH14_DST_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH14_DST_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH14_DST_ADDR_EM_DST_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH14_DST_ADDR_EM_DST_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH14_SPT_OFFSET                                       0x000005DC
#define EAV_DTE_DMA_CH14_SPT_TYPE                                         UInt32
#define EAV_DTE_DMA_CH14_SPT_RESERVED_MASK                                0x00000003
#define    EAV_DTE_DMA_CH14_SPT_SWPTR_SHIFT                               2
#define    EAV_DTE_DMA_CH14_SPT_SWPTR_MASK                                0xFFFFFFFC

#define EAV_DTE_DMA_CH15_CSR_OFFSET                                       0x000005E0
#define EAV_DTE_DMA_CH15_CSR_TYPE                                         UInt32
#define EAV_DTE_DMA_CH15_CSR_RESERVED_MASK                                0x00119480
#define    EAV_DTE_DMA_CH15_CSR_INC2_SHIFT                                30
#define    EAV_DTE_DMA_CH15_CSR_INC2_MASK                                 0xC0000000
#define    EAV_DTE_DMA_CH15_CSR_INC7_SHIFT                                23
#define    EAV_DTE_DMA_CH15_CSR_INC7_MASK                                 0x3F800000
#define    EAV_DTE_DMA_CH15_CSR_INT_SRC2_SHIFT                            22
#define    EAV_DTE_DMA_CH15_CSR_INT_SRC2_MASK                             0x00400000
#define    EAV_DTE_DMA_CH15_CSR_INT_SRC1_SHIFT                            21
#define    EAV_DTE_DMA_CH15_CSR_INT_SRC1_MASK                             0x00200000
#define    EAV_DTE_DMA_CH15_CSR_INT_SRC2_EN_SHIFT                         19
#define    EAV_DTE_DMA_CH15_CSR_INT_SRC2_EN_MASK                          0x00080000
#define    EAV_DTE_DMA_CH15_CSR_INT_SRC1_EN_SHIFT                         18
#define    EAV_DTE_DMA_CH15_CSR_INT_SRC1_EN_MASK                          0x00040000
#define    EAV_DTE_DMA_CH15_CSR_INT_SRC0_EN_SHIFT                         17
#define    EAV_DTE_DMA_CH15_CSR_INT_SRC0_EN_MASK                          0x00020000
#define    EAV_DTE_DMA_CH15_CSR_PRIORITY_SHIFT                            13
#define    EAV_DTE_DMA_CH15_CSR_PRIORITY_MASK                             0x00006000
#define    EAV_DTE_DMA_CH15_CSR_DONE_SHIFT                                11
#define    EAV_DTE_DMA_CH15_CSR_DONE_MASK                                 0x00000800
#define    EAV_DTE_DMA_CH15_CSR_CBUF_EN_SHIFT                             9
#define    EAV_DTE_DMA_CH15_CSR_CBUF_EN_MASK                              0x00000200
#define    EAV_DTE_DMA_CH15_CSR_SWPTR_EN_SHIFT                            8
#define    EAV_DTE_DMA_CH15_CSR_SWPTR_EN_MASK                             0x00000100
#define    EAV_DTE_DMA_CH15_CSR_ARS_EN_SHIFT                              6
#define    EAV_DTE_DMA_CH15_CSR_ARS_EN_MASK                               0x00000040
#define    EAV_DTE_DMA_CH15_CSR_HW_HSHK_EN_SHIFT                          5
#define    EAV_DTE_DMA_CH15_CSR_HW_HSHK_EN_MASK                           0x00000020
#define    EAV_DTE_DMA_CH15_CSR_SRC_INC_SEL_SHIFT                         4
#define    EAV_DTE_DMA_CH15_CSR_SRC_INC_SEL_MASK                          0x00000010
#define    EAV_DTE_DMA_CH15_CSR_DST_INC_SEL_SHIFT                         3
#define    EAV_DTE_DMA_CH15_CSR_DST_INC_SEL_MASK                          0x00000008
#define    EAV_DTE_DMA_CH15_CSR_SRC_SEL_SHIFT                             2
#define    EAV_DTE_DMA_CH15_CSR_SRC_SEL_MASK                              0x00000004
#define    EAV_DTE_DMA_CH15_CSR_DST_SEL_SHIFT                             1
#define    EAV_DTE_DMA_CH15_CSR_DST_SEL_MASK                              0x00000002
#define    EAV_DTE_DMA_CH15_CSR_ENABLE_SHIFT                              0
#define    EAV_DTE_DMA_CH15_CSR_ENABLE_MASK                               0x00000001

#define EAV_DTE_DMA_CH15_TXSZ_OFFSET                                      0x000005E4
#define EAV_DTE_DMA_CH15_TXSZ_TYPE                                        UInt32
#define EAV_DTE_DMA_CH15_TXSZ_RESERVED_MASK                               0xFE00F000
#define    EAV_DTE_DMA_CH15_TXSZ_CHK_SZ_SHIFT                             16
#define    EAV_DTE_DMA_CH15_TXSZ_CHK_SZ_MASK                              0x01FF0000
#define    EAV_DTE_DMA_CH15_TXSZ_TOT_SZ_SHIFT                             0
#define    EAV_DTE_DMA_CH15_TXSZ_TOT_SZ_MASK                              0x00000FFF

#define EAV_DTE_DMA_CH15_SRC_ADDR_OFFSET                                  0x000005E8
#define EAV_DTE_DMA_CH15_SRC_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH15_SRC_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH15_SRC_ADDR_SRC_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH15_SRC_ADDR_SRC_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH15_SRC_ADDR_EM_OFFSET                               0x000005EC
#define EAV_DTE_DMA_CH15_SRC_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH15_SRC_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH15_SRC_ADDR_EM_SRC_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH15_SRC_ADDR_EM_SRC_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH15_DST_ADDR_OFFSET                                  0x000005F0
#define EAV_DTE_DMA_CH15_DST_ADDR_TYPE                                    UInt32
#define EAV_DTE_DMA_CH15_DST_ADDR_RESERVED_MASK                           0x00000003
#define    EAV_DTE_DMA_CH15_DST_ADDR_DST_ADDR_SHIFT                       2
#define    EAV_DTE_DMA_CH15_DST_ADDR_DST_ADDR_MASK                        0xFFFFFFFC

#define EAV_DTE_DMA_CH15_DST_ADDR_EM_OFFSET                               0x000005F4
#define EAV_DTE_DMA_CH15_DST_ADDR_EM_TYPE                                 UInt32
#define EAV_DTE_DMA_CH15_DST_ADDR_EM_RESERVED_MASK                        0xFFFFFFE0
#define    EAV_DTE_DMA_CH15_DST_ADDR_EM_DST_ENC_MSK_SHIFT                 0
#define    EAV_DTE_DMA_CH15_DST_ADDR_EM_DST_ENC_MSK_MASK                  0x0000001F

#define EAV_DTE_DMA_CH15_SPT_OFFSET                                       0x000005FC
#define EAV_DTE_DMA_CH15_SPT_TYPE                                         UInt32
#define EAV_DTE_DMA_CH15_SPT_RESERVED_MASK                                0x00000003
#define    EAV_DTE_DMA_CH15_SPT_SWPTR_SHIFT                               2
#define    EAV_DTE_DMA_CH15_SPT_SWPTR_MASK                                0xFFFFFFFC

#define EAV_DTE_DTE_CTRL_OFFSET                                           0x00000600
#define EAV_DTE_DTE_CTRL_TYPE                                             UInt32
#define EAV_DTE_DTE_CTRL_RESERVED_MASK                                    0xFFFCFFFF
#define    EAV_DTE_DTE_CTRL_INTERRUPT_ERROR_SHIFT                         17
#define    EAV_DTE_DTE_CTRL_INTERRUPT_ERROR_MASK                          0x00020000
#define    EAV_DTE_DTE_CTRL_INTERRUPT_SHIFT                               16
#define    EAV_DTE_DTE_CTRL_INTERRUPT_MASK                                0x00010000

#define EAV_DTE_DTE_NEXT_SOI_OFFSET                                       0x00000610
#define EAV_DTE_DTE_NEXT_SOI_TYPE                                         UInt32
#define EAV_DTE_DTE_NEXT_SOI_RESERVED_MASK                                0xC0000000
#define    EAV_DTE_DTE_NEXT_SOI_NEXT_SOI_SHIFT                            0
#define    EAV_DTE_DTE_NEXT_SOI_NEXT_SOI_MASK                             0x3FFFFFFF

#define EAV_DTE_DTE_ILEN_OFFSET                                           0x00000614
#define EAV_DTE_DTE_ILEN_TYPE                                             UInt32
#define EAV_DTE_DTE_ILEN_RESERVED_MASK                                    0xC0000000
#define    EAV_DTE_DTE_ILEN_INTV_LEN_SHIFT                                0
#define    EAV_DTE_DTE_ILEN_INTV_LEN_MASK                                 0x3FFFFFFF

#define EAV_DTE_DTE_LTS_FIFO_OFFSET                                       0x00000640
#define EAV_DTE_DTE_LTS_FIFO_TYPE                                         UInt32
#define EAV_DTE_DTE_LTS_FIFO_RESERVED_MASK                                0x00000000
#define    EAV_DTE_DTE_LTS_FIFO_MASK_AND_TIMESTAMP_SHIFT                  0
#define    EAV_DTE_DTE_LTS_FIFO_MASK_AND_TIMESTAMP_MASK                   0xFFFFFFFF

#define EAV_DTE_DTE_LTS_CSR_OFFSET                                        0x00000644
#define EAV_DTE_DTE_LTS_CSR_TYPE                                          UInt32
#define EAV_DTE_DTE_LTS_CSR_RESERVED_MASK                                 0xFFFFFFC0
#define    EAV_DTE_DTE_LTS_CSR_FIFO_FULL_SHIFT                            5
#define    EAV_DTE_DTE_LTS_CSR_FIFO_FULL_MASK                             0x00000020
#define    EAV_DTE_DTE_LTS_CSR_FIFO_EMPTY_SHIFT                           4
#define    EAV_DTE_DTE_LTS_CSR_FIFO_EMPTY_MASK                            0x00000010
#define    EAV_DTE_DTE_LTS_CSR_FIFO_OVERFLOW_SHIFT                        3
#define    EAV_DTE_DTE_LTS_CSR_FIFO_OVERFLOW_MASK                         0x00000008
#define    EAV_DTE_DTE_LTS_CSR_FIFO_UNDERFLOW_SHIFT                       2
#define    EAV_DTE_DTE_LTS_CSR_FIFO_UNDERFLOW_MASK                        0x00000004
#define    EAV_DTE_DTE_LTS_CSR_FIFO_LEVEL_SHIFT                           0
#define    EAV_DTE_DTE_LTS_CSR_FIFO_LEVEL_MASK                            0x00000003

#define EAV_DTE_DTE_NCO_LOW_TIME_OFFSET                                   0x00000650
#define EAV_DTE_DTE_NCO_LOW_TIME_TYPE                                     UInt32
#define EAV_DTE_DTE_NCO_LOW_TIME_RESERVED_MASK                            0x00000000
#define    EAV_DTE_DTE_NCO_LOW_TIME_SUM1_SHIFT                            0
#define    EAV_DTE_DTE_NCO_LOW_TIME_SUM1_MASK                             0xFFFFFFFF

#define EAV_DTE_DTE_NCO_TIME_OFFSET                                       0x00000654
#define EAV_DTE_DTE_NCO_TIME_TYPE                                         UInt32
#define EAV_DTE_DTE_NCO_TIME_RESERVED_MASK                                0x00000000
#define    EAV_DTE_DTE_NCO_TIME_SUM2_SHIFT                                0
#define    EAV_DTE_DTE_NCO_TIME_SUM2_MASK                                 0xFFFFFFFF

#define EAV_DTE_DTE_NCO_OVERFLOW_OFFSET                                   0x00000658
#define EAV_DTE_DTE_NCO_OVERFLOW_TYPE                                     UInt32
#define EAV_DTE_DTE_NCO_OVERFLOW_RESERVED_MASK                            0xFFFF0000
#define    EAV_DTE_DTE_NCO_OVERFLOW_SUM3_SHIFT                            8
#define    EAV_DTE_DTE_NCO_OVERFLOW_SUM3_MASK                             0x0000FF00
#define    EAV_DTE_DTE_NCO_OVERFLOW_SUM3_ACCUMULATOR_SHIFT                0
#define    EAV_DTE_DTE_NCO_OVERFLOW_SUM3_ACCUMULATOR_MASK                 0x000000FF

#define EAV_DTE_DTE_NCO_INC_OFFSET                                        0x0000065C
#define EAV_DTE_DTE_NCO_INC_TYPE                                          UInt32
#define EAV_DTE_DTE_NCO_INC_RESERVED_MASK                                 0x00000000
#define    EAV_DTE_DTE_NCO_INC_LT_INC_SHIFT                               0
#define    EAV_DTE_DTE_NCO_INC_LT_INC_MASK                                0xFFFFFFFF

#define EAV_DTE_DTE_LTS_DIV_54_OFFSET                                     0x00000660
#define EAV_DTE_DTE_LTS_DIV_54_TYPE                                       UInt32
#define EAV_DTE_DTE_LTS_DIV_54_RESERVED_MASK                              0x00000000
#define    EAV_DTE_DTE_LTS_DIV_54_DIV_VAL_4_SHIFT                         16
#define    EAV_DTE_DTE_LTS_DIV_54_DIV_VAL_4_MASK                          0xFFFF0000
#define    EAV_DTE_DTE_LTS_DIV_54_DIV_VAL_5_SHIFT                         0
#define    EAV_DTE_DTE_LTS_DIV_54_DIV_VAL_5_MASK                          0x0000FFFF

#define EAV_DTE_DTE_LTS_DIV_76_OFFSET                                     0x00000664
#define EAV_DTE_DTE_LTS_DIV_76_TYPE                                       UInt32
#define EAV_DTE_DTE_LTS_DIV_76_RESERVED_MASK                              0x00000000
#define    EAV_DTE_DTE_LTS_DIV_76_DIV_VAL_6_SHIFT                         16
#define    EAV_DTE_DTE_LTS_DIV_76_DIV_VAL_6_MASK                          0xFFFF0000
#define    EAV_DTE_DTE_LTS_DIV_76_DIV_VAL_7_SHIFT                         0
#define    EAV_DTE_DTE_LTS_DIV_76_DIV_VAL_7_MASK                          0x0000FFFF

#define EAV_DTE_DTE_LTS_DIV_98_OFFSET                                     0x00000668
#define EAV_DTE_DTE_LTS_DIV_98_TYPE                                       UInt32
#define EAV_DTE_DTE_LTS_DIV_98_RESERVED_MASK                              0x00000000
#define    EAV_DTE_DTE_LTS_DIV_98_DIV_VAL_8_SHIFT                         16
#define    EAV_DTE_DTE_LTS_DIV_98_DIV_VAL_8_MASK                          0xFFFF0000
#define    EAV_DTE_DTE_LTS_DIV_98_DIV_VAL_9_SHIFT                         0
#define    EAV_DTE_DTE_LTS_DIV_98_DIV_VAL_9_MASK                          0x0000FFFF

#define EAV_DTE_DTE_LTS_DIV_1110_OFFSET                                   0x0000066C
#define EAV_DTE_DTE_LTS_DIV_1110_TYPE                                     UInt32
#define EAV_DTE_DTE_LTS_DIV_1110_RESERVED_MASK                            0x00000000
#define    EAV_DTE_DTE_LTS_DIV_1110_DIV_VAL_10_SHIFT                      16
#define    EAV_DTE_DTE_LTS_DIV_1110_DIV_VAL_10_MASK                       0xFFFF0000
#define    EAV_DTE_DTE_LTS_DIV_1110_DIV_VAL_11_SHIFT                      0
#define    EAV_DTE_DTE_LTS_DIV_1110_DIV_VAL_11_MASK                       0x0000FFFF

#define EAV_DTE_DTE_LTS_DIV_1312_OFFSET                                   0x00000670
#define EAV_DTE_DTE_LTS_DIV_1312_TYPE                                     UInt32
#define EAV_DTE_DTE_LTS_DIV_1312_RESERVED_MASK                            0x00000000
#define    EAV_DTE_DTE_LTS_DIV_1312_DIV_VAL_12_SHIFT                      16
#define    EAV_DTE_DTE_LTS_DIV_1312_DIV_VAL_12_MASK                       0xFFFF0000
#define    EAV_DTE_DTE_LTS_DIV_1312_DIV_VAL_13_SHIFT                      0
#define    EAV_DTE_DTE_LTS_DIV_1312_DIV_VAL_13_MASK                       0x0000FFFF

#define EAV_DTE_DTE_LTS_DIV_14_OFFSET                                     0x00000674
#define EAV_DTE_DTE_LTS_DIV_14_TYPE                                       UInt32
#define EAV_DTE_DTE_LTS_DIV_14_RESERVED_MASK                              0xFFFF0000
#define    EAV_DTE_DTE_LTS_DIV_14_DIV_VAL_14_SHIFT                        0
#define    EAV_DTE_DTE_LTS_DIV_14_DIV_VAL_14_MASK                         0x0000FFFF

#define EAV_DTE_DTE_LTS_SRC_EN_OFFSET                                     0x00000680
#define EAV_DTE_DTE_LTS_SRC_EN_TYPE                                       UInt32
#define EAV_DTE_DTE_LTS_SRC_EN_RESERVED_MASK                              0x80000000
#define    EAV_DTE_DTE_LTS_SRC_EN_LTS_SRC_EN_SHIFT                        0
#define    EAV_DTE_DTE_LTS_SRC_EN_LTS_SRC_EN_MASK                         0x7FFFFFFF

#endif /* __BRCM_RDB_EAV_DTE_H__ */


