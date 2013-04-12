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

#ifndef __BRCM_RDB_CIR_H__
#define __BRCM_RDB_CIR_H__

#define CIR_CTRL_OFFSET                                                   0x00000000
#define CIR_CTRL_TYPE                                                     UInt32
#define CIR_CTRL_RESERVED_MASK                                            0xFFFFFF00
#define    CIR_CTRL_RX_ACTIVE_STATE_SHIFT                                 7
#define    CIR_CTRL_RX_ACTIVE_STATE_MASK                                  0x00000080
#define    CIR_CTRL_RX_DMA_ENABLE_SHIFT                                   6
#define    CIR_CTRL_RX_DMA_ENABLE_MASK                                    0x00000040
#define    CIR_CTRL_RX_ENABLE_SHIFT                                       5
#define    CIR_CTRL_RX_ENABLE_MASK                                        0x00000020
#define    CIR_CTRL_RX_RESET_SHIFT                                        4
#define    CIR_CTRL_RX_RESET_MASK                                         0x00000010
#define    CIR_CTRL_TX_MODE_SHIFT                                         3
#define    CIR_CTRL_TX_MODE_MASK                                          0x00000008
#define    CIR_CTRL_TX_DMA_ENABLE_SHIFT                                   2
#define    CIR_CTRL_TX_DMA_ENABLE_MASK                                    0x00000004
#define    CIR_CTRL_TX_ENABLE_SHIFT                                       1
#define    CIR_CTRL_TX_ENABLE_MASK                                        0x00000002
#define    CIR_CTRL_TX_RESET_SHIFT                                        0
#define    CIR_CTRL_TX_RESET_MASK                                         0x00000001

#define CIR_TX_FIFO_CTRL_OFFSET                                           0x00000004
#define CIR_TX_FIFO_CTRL_TYPE                                             UInt32
#define CIR_TX_FIFO_CTRL_RESERVED_MASK                                    0xFFFFFF8E
#define    CIR_TX_FIFO_CTRL_THRESHOLD_SHIFT                               4
#define    CIR_TX_FIFO_CTRL_THRESHOLD_MASK                                0x00000070
#define    CIR_TX_FIFO_CTRL_CLEAR_SHIFT                                   0
#define    CIR_TX_FIFO_CTRL_CLEAR_MASK                                    0x00000001

#define CIR_TX_FIFO_STATUS_OFFSET                                         0x00000008
#define CIR_TX_FIFO_STATUS_TYPE                                           UInt32
#define CIR_TX_FIFO_STATUS_RESERVED_MASK                                  0xFFFFF088
#define    CIR_TX_FIFO_STATUS_DEPTH_SHIFT                                 8
#define    CIR_TX_FIFO_STATUS_DEPTH_MASK                                  0x00000F00
#define    CIR_TX_FIFO_STATUS_RD_PTR_SHIFT                                4
#define    CIR_TX_FIFO_STATUS_RD_PTR_MASK                                 0x00000070
#define    CIR_TX_FIFO_STATUS_WR_PTR_SHIFT                                0
#define    CIR_TX_FIFO_STATUS_WR_PTR_MASK                                 0x00000007

#define CIR_TC_ON_OFFSET                                                  0x0000000C
#define CIR_TC_ON_TYPE                                                    UInt32
#define CIR_TC_ON_RESERVED_MASK                                           0xFFFF0000
#define    CIR_TC_ON_TC_ON_SHIFT                                          0
#define    CIR_TC_ON_TC_ON_MASK                                           0x0000FFFF

#define CIR_TC_OFF_OFFSET                                                 0x00000010
#define CIR_TC_OFF_TYPE                                                   UInt32
#define CIR_TC_OFF_RESERVED_MASK                                          0xFFFF0000
#define    CIR_TC_OFF_TC_OFF_SHIFT                                        0
#define    CIR_TC_OFF_TC_OFF_MASK                                         0x0000FFFF

#define CIR_TX_PERIOD_OFFSET                                              0x00000014
#define CIR_TX_PERIOD_TYPE                                                UInt32
#define CIR_TX_PERIOD_RESERVED_MASK                                       0xFFFF0000
#define    CIR_TX_PERIOD_TX_PERIOD_SHIFT                                  0
#define    CIR_TX_PERIOD_TX_PERIOD_MASK                                   0x0000FFFF

#define CIR_TX_DMA_CNT_OFFSET                                             0x00000018
#define CIR_TX_DMA_CNT_TYPE                                               UInt32
#define CIR_TX_DMA_CNT_RESERVED_MASK                                      0xFF000000
#define    CIR_TX_DMA_CNT_TX_DMA_CNT_SHIFT                                0
#define    CIR_TX_DMA_CNT_TX_DMA_CNT_MASK                                 0x00FFFFFF

#define CIR_TX_DATA_OFFSET                                                0x0000001C
#define CIR_TX_DATA_TYPE                                                  UInt32
#define CIR_TX_DATA_RESERVED_MASK                                         0x00000000
#define    CIR_TX_DATA_DATA_SHIFT                                         0
#define    CIR_TX_DATA_DATA_MASK                                          0xFFFFFFFF

#define CIR_RX_FIFO_CTRL_OFFSET                                           0x00000020
#define CIR_RX_FIFO_CTRL_TYPE                                             UInt32
#define CIR_RX_FIFO_CTRL_RESERVED_MASK                                    0xFFFFFF0E
#define    CIR_RX_FIFO_CTRL_THRESHOLD_SHIFT                               4
#define    CIR_RX_FIFO_CTRL_THRESHOLD_MASK                                0x000000F0
#define    CIR_RX_FIFO_CTRL_CLEAR_SHIFT                                   0
#define    CIR_RX_FIFO_CTRL_CLEAR_MASK                                    0x00000001

#define CIR_RX_FIFO_STATUS_OFFSET                                         0x00000024
#define CIR_RX_FIFO_STATUS_TYPE                                           UInt32
#define CIR_RX_FIFO_STATUS_RESERVED_MASK                                  0xFFFFF088
#define    CIR_RX_FIFO_STATUS_DEPTH_SHIFT                                 8
#define    CIR_RX_FIFO_STATUS_DEPTH_MASK                                  0x00000F00
#define    CIR_RX_FIFO_STATUS_RD_PTR_SHIFT                                4
#define    CIR_RX_FIFO_STATUS_RD_PTR_MASK                                 0x00000070
#define    CIR_RX_FIFO_STATUS_WR_PTR_SHIFT                                0
#define    CIR_RX_FIFO_STATUS_WR_PTR_MASK                                 0x00000007

#define CIR_PW_NOISE_OFFSET                                               0x00000028
#define CIR_PW_NOISE_TYPE                                                 UInt32
#define CIR_PW_NOISE_RESERVED_MASK                                        0xFFFF0000
#define    CIR_PW_NOISE_PW_NOISE_SHIFT                                    0
#define    CIR_PW_NOISE_PW_NOISE_MASK                                     0x0000FFFF

#define CIR_PW_WIDTH_OFFSET                                               0x0000002C
#define CIR_PW_WIDTH_TYPE                                                 UInt32
#define CIR_PW_WIDTH_RESERVED_MASK                                        0x00000000
#define    CIR_PW_WIDTH_PW_MAX_SHIFT                                      16
#define    CIR_PW_WIDTH_PW_MAX_MASK                                       0xFFFF0000
#define    CIR_PW_WIDTH_PW_MIN_SHIFT                                      0
#define    CIR_PW_WIDTH_PW_MIN_MASK                                       0x0000FFFF

#define CIR_RX_PERIOD_OFFSET                                              0x00000030
#define CIR_RX_PERIOD_TYPE                                                UInt32
#define CIR_RX_PERIOD_RESERVED_MASK                                       0xFFFF0000
#define    CIR_RX_PERIOD_RX_PERIOD_SHIFT                                  0
#define    CIR_RX_PERIOD_RX_PERIOD_MASK                                   0x0000FFFF

#define CIR_RX_DATA_OFFSET                                                0x00000038
#define CIR_RX_DATA_TYPE                                                  UInt32
#define CIR_RX_DATA_RESERVED_MASK                                         0x00000000
#define    CIR_RX_DATA_DATA_SHIFT                                         0
#define    CIR_RX_DATA_DATA_MASK                                          0xFFFFFFFF

#define CIR_INT_ENABLE_OFFSET                                             0x00000040
#define CIR_INT_ENABLE_TYPE                                               UInt32
#define CIR_INT_ENABLE_RESERVED_MASK                                      0xFFFFFFC0
#define    CIR_INT_ENABLE_RX_FIFO_OVERRUN_SHIFT                           5
#define    CIR_INT_ENABLE_RX_FIFO_OVERRUN_MASK                            0x00000020
#define    CIR_INT_ENABLE_RX_FIFO_INT_SHIFT                               4
#define    CIR_INT_ENABLE_RX_FIFO_INT_MASK                                0x00000010
#define    CIR_INT_ENABLE_RX_DONE_INT_SHIFT                               3
#define    CIR_INT_ENABLE_RX_DONE_INT_MASK                                0x00000008
#define    CIR_INT_ENABLE_TX_FIFO_UNDERRUN_SHIFT                          2
#define    CIR_INT_ENABLE_TX_FIFO_UNDERRUN_MASK                           0x00000004
#define    CIR_INT_ENABLE_TX_FIFO_INT_SHIFT                               1
#define    CIR_INT_ENABLE_TX_FIFO_INT_MASK                                0x00000002
#define    CIR_INT_ENABLE_TX_DONE_INT_SHIFT                               0
#define    CIR_INT_ENABLE_TX_DONE_INT_MASK                                0x00000001

#define CIR_INT_PEND_OFFSET                                               0x00000044
#define CIR_INT_PEND_TYPE                                                 UInt32
#define CIR_INT_PEND_RESERVED_MASK                                        0xFFFFFFC0
#define    CIR_INT_PEND_RX_FIFO_OVERRUN_SHIFT                             5
#define    CIR_INT_PEND_RX_FIFO_OVERRUN_MASK                              0x00000020
#define    CIR_INT_PEND_RX_FIFO_INT_SHIFT                                 4
#define    CIR_INT_PEND_RX_FIFO_INT_MASK                                  0x00000010
#define    CIR_INT_PEND_RX_DONE_INT_SHIFT                                 3
#define    CIR_INT_PEND_RX_DONE_INT_MASK                                  0x00000008
#define    CIR_INT_PEND_TX_FIFO_UNDERRUN_SHIFT                            2
#define    CIR_INT_PEND_TX_FIFO_UNDERRUN_MASK                             0x00000004
#define    CIR_INT_PEND_TX_FIFO_INT_SHIFT                                 1
#define    CIR_INT_PEND_TX_FIFO_INT_MASK                                  0x00000002
#define    CIR_INT_PEND_TX_DONE_INT_SHIFT                                 0
#define    CIR_INT_PEND_TX_DONE_INT_MASK                                  0x00000001

#define CIR_TX_DATA0_OFFSET                                               0x00000100
#define CIR_TX_DATA0_TYPE                                                 UInt32
#define CIR_TX_DATA0_RESERVED_MASK                                        0x00000000
#define    CIR_TX_DATA0_DATA_SHIFT                                        0
#define    CIR_TX_DATA0_DATA_MASK                                         0xFFFFFFFF

#define CIR_TX_DATA1_OFFSET                                               0x00000104
#define CIR_TX_DATA1_TYPE                                                 UInt32
#define CIR_TX_DATA1_RESERVED_MASK                                        0x00000000
#define    CIR_TX_DATA1_DATA_SHIFT                                        0
#define    CIR_TX_DATA1_DATA_MASK                                         0xFFFFFFFF

#define CIR_TX_DATA2_OFFSET                                               0x00000108
#define CIR_TX_DATA2_TYPE                                                 UInt32
#define CIR_TX_DATA2_RESERVED_MASK                                        0x00000000
#define    CIR_TX_DATA2_DATA_SHIFT                                        0
#define    CIR_TX_DATA2_DATA_MASK                                         0xFFFFFFFF

#define CIR_TX_DATA3_OFFSET                                               0x0000010C
#define CIR_TX_DATA3_TYPE                                                 UInt32
#define CIR_TX_DATA3_RESERVED_MASK                                        0x00000000
#define    CIR_TX_DATA3_DATA_SHIFT                                        0
#define    CIR_TX_DATA3_DATA_MASK                                         0xFFFFFFFF

#define CIR_TX_DATA4_OFFSET                                               0x00000110
#define CIR_TX_DATA4_TYPE                                                 UInt32
#define CIR_TX_DATA4_RESERVED_MASK                                        0x00000000
#define    CIR_TX_DATA4_DATA_SHIFT                                        0
#define    CIR_TX_DATA4_DATA_MASK                                         0xFFFFFFFF

#define CIR_TX_DATA5_OFFSET                                               0x00000114
#define CIR_TX_DATA5_TYPE                                                 UInt32
#define CIR_TX_DATA5_RESERVED_MASK                                        0x00000000
#define    CIR_TX_DATA5_DATA_SHIFT                                        0
#define    CIR_TX_DATA5_DATA_MASK                                         0xFFFFFFFF

#define CIR_TX_DATA6_OFFSET                                               0x00000118
#define CIR_TX_DATA6_TYPE                                                 UInt32
#define CIR_TX_DATA6_RESERVED_MASK                                        0x00000000
#define    CIR_TX_DATA6_DATA_SHIFT                                        0
#define    CIR_TX_DATA6_DATA_MASK                                         0xFFFFFFFF

#define CIR_TX_DATA7_OFFSET                                               0x0000011C
#define CIR_TX_DATA7_TYPE                                                 UInt32
#define CIR_TX_DATA7_RESERVED_MASK                                        0x00000000
#define    CIR_TX_DATA7_DATA_SHIFT                                        0
#define    CIR_TX_DATA7_DATA_MASK                                         0xFFFFFFFF

#define CIR_RX_DATA0_OFFSET                                               0x00000140
#define CIR_RX_DATA0_TYPE                                                 UInt32
#define CIR_RX_DATA0_RESERVED_MASK                                        0x00000000
#define    CIR_RX_DATA0_DATA_SHIFT                                        0
#define    CIR_RX_DATA0_DATA_MASK                                         0xFFFFFFFF

#define CIR_RX_DATA1_OFFSET                                               0x00000144
#define CIR_RX_DATA1_TYPE                                                 UInt32
#define CIR_RX_DATA1_RESERVED_MASK                                        0x00000000
#define    CIR_RX_DATA1_DATA_SHIFT                                        0
#define    CIR_RX_DATA1_DATA_MASK                                         0xFFFFFFFF

#define CIR_RX_DATA2_OFFSET                                               0x00000148
#define CIR_RX_DATA2_TYPE                                                 UInt32
#define CIR_RX_DATA2_RESERVED_MASK                                        0x00000000
#define    CIR_RX_DATA2_DATA_SHIFT                                        0
#define    CIR_RX_DATA2_DATA_MASK                                         0xFFFFFFFF

#define CIR_RX_DATA3_OFFSET                                               0x0000014C
#define CIR_RX_DATA3_TYPE                                                 UInt32
#define CIR_RX_DATA3_RESERVED_MASK                                        0x00000000
#define    CIR_RX_DATA3_DATA_SHIFT                                        0
#define    CIR_RX_DATA3_DATA_MASK                                         0xFFFFFFFF

#define CIR_RX_DATA4_OFFSET                                               0x00000150
#define CIR_RX_DATA4_TYPE                                                 UInt32
#define CIR_RX_DATA4_RESERVED_MASK                                        0x00000000
#define    CIR_RX_DATA4_DATA_SHIFT                                        0
#define    CIR_RX_DATA4_DATA_MASK                                         0xFFFFFFFF

#define CIR_RX_DATA5_OFFSET                                               0x00000154
#define CIR_RX_DATA5_TYPE                                                 UInt32
#define CIR_RX_DATA5_RESERVED_MASK                                        0x00000000
#define    CIR_RX_DATA5_DATA_SHIFT                                        0
#define    CIR_RX_DATA5_DATA_MASK                                         0xFFFFFFFF

#define CIR_RX_DATA6_OFFSET                                               0x00000158
#define CIR_RX_DATA6_TYPE                                                 UInt32
#define CIR_RX_DATA6_RESERVED_MASK                                        0x00000000
#define    CIR_RX_DATA6_DATA_SHIFT                                        0
#define    CIR_RX_DATA6_DATA_MASK                                         0xFFFFFFFF

#define CIR_RX_DATA7_OFFSET                                               0x0000015C
#define CIR_RX_DATA7_TYPE                                                 UInt32
#define CIR_RX_DATA7_RESERVED_MASK                                        0x00000000
#define    CIR_RX_DATA7_DATA_SHIFT                                        0
#define    CIR_RX_DATA7_DATA_MASK                                         0xFFFFFFFF

#endif /* __BRCM_RDB_CIR_H__ */


