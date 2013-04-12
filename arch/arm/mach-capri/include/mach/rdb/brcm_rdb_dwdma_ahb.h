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

#ifndef __BRCM_RDB_DWDMA_AHB_H__
#define __BRCM_RDB_DWDMA_AHB_H__

#define DWDMA_AHB_SAR0_OFFSET                                             0x00000000
#define DWDMA_AHB_SAR0_TYPE                                               UInt64
#define DWDMA_AHB_SAR0_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_SAR0_SAR_SHIFT                                       0
#define    DWDMA_AHB_SAR0_SAR_MASK                                        0xFFFFFFFF

#define DWDMA_AHB_DAR0_OFFSET                                             0x00000008
#define DWDMA_AHB_DAR0_TYPE                                               UInt64
#define DWDMA_AHB_DAR0_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_DAR0_DAR_SHIFT                                       0
#define    DWDMA_AHB_DAR0_DAR_MASK                                        0xFFFFFFFF

#define DWDMA_AHB_LLP0_OFFSET                                             0x00000010
#define DWDMA_AHB_LLP0_TYPE                                               UInt64
#define DWDMA_AHB_LLP0_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_LLP0_LOC_SHIFT                                       2
#define    DWDMA_AHB_LLP0_LOC_MASK                                        0xFFFFFFFC
#define    DWDMA_AHB_LLP0_LMS_SHIFT                                       0
#define    DWDMA_AHB_LLP0_LMS_MASK                                        0x00000003

#define DWDMA_AHB_CTL0_OFFSET                                             0x00000018
#define DWDMA_AHB_CTL0_TYPE                                               UInt64
#define DWDMA_AHB_CTL0_RESERVED_MASK                                      0xFFFFE000E0080000
#define    DWDMA_AHB_CTL0_DONE_SHIFT                                      44
#define    DWDMA_AHB_CTL0_DONE_MASK                                       0x100000000000
#define    DWDMA_AHB_CTL0_BLOCK_TS_SHIFT                                  32
#define    DWDMA_AHB_CTL0_BLOCK_TS_MASK                                   0xFFF00000000
#define    DWDMA_AHB_CTL0_LLP_SRC_EN_SHIFT                                28
#define    DWDMA_AHB_CTL0_LLP_SRC_EN_MASK                                 0x10000000
#define    DWDMA_AHB_CTL0_LLP_DST_EN_SHIFT                                27
#define    DWDMA_AHB_CTL0_LLP_DST_EN_MASK                                 0x08000000
#define    DWDMA_AHB_CTL0_SMS_SHIFT                                       25
#define    DWDMA_AHB_CTL0_SMS_MASK                                        0x06000000
#define    DWDMA_AHB_CTL0_DMS_SHIFT                                       23
#define    DWDMA_AHB_CTL0_DMS_MASK                                        0x01800000
#define    DWDMA_AHB_CTL0_TT_FC_SHIFT                                     20
#define    DWDMA_AHB_CTL0_TT_FC_MASK                                      0x00700000
#define    DWDMA_AHB_CTL0_DST_SCATTER_EN_SHIFT                            18
#define    DWDMA_AHB_CTL0_DST_SCATTER_EN_MASK                             0x00040000
#define    DWDMA_AHB_CTL0_SRC_GATHER_EN_SHIFT                             17
#define    DWDMA_AHB_CTL0_SRC_GATHER_EN_MASK                              0x00020000
#define    DWDMA_AHB_CTL0_SRC_MSIZE_SHIFT                                 14
#define    DWDMA_AHB_CTL0_SRC_MSIZE_MASK                                  0x0001C000
#define    DWDMA_AHB_CTL0_DST_MSIZE_SHIFT                                 11
#define    DWDMA_AHB_CTL0_DST_MSIZE_MASK                                  0x00003800
#define    DWDMA_AHB_CTL0_SINC_SHIFT                                      9
#define    DWDMA_AHB_CTL0_SINC_MASK                                       0x00000600
#define    DWDMA_AHB_CTL0_DINC_SHIFT                                      7
#define    DWDMA_AHB_CTL0_DINC_MASK                                       0x00000180
#define    DWDMA_AHB_CTL0_SRC_TR_WIDTH_SHIFT                              4
#define    DWDMA_AHB_CTL0_SRC_TR_WIDTH_MASK                               0x00000070
#define    DWDMA_AHB_CTL0_DST_TR_WIDTH_SHIFT                              1
#define    DWDMA_AHB_CTL0_DST_TR_WIDTH_MASK                               0x0000000E
#define    DWDMA_AHB_CTL0_INT_EN_SHIFT                                    0
#define    DWDMA_AHB_CTL0_INT_EN_MASK                                     0x00000001

#define DWDMA_AHB_SSTAT0_OFFSET                                           0x00000020
#define DWDMA_AHB_SSTAT0_TYPE                                             UInt64
#define DWDMA_AHB_SSTAT0_RESERVED_MASK                                    0xFFFFFFFF00000000
#define    DWDMA_AHB_SSTAT0_SSTAT_SHIFT                                   0
#define    DWDMA_AHB_SSTAT0_SSTAT_MASK                                    0xFFFFFFFF

#define DWDMA_AHB_DSTAT0_OFFSET                                           0x00000028
#define DWDMA_AHB_DSTAT0_TYPE                                             UInt64
#define DWDMA_AHB_DSTAT0_RESERVED_MASK                                    0xFFFFFFFF00000000
#define    DWDMA_AHB_DSTAT0_DSTAT_SHIFT                                   0
#define    DWDMA_AHB_DSTAT0_DSTAT_MASK                                    0xFFFFFFFF

#define DWDMA_AHB_SSTATAR0_OFFSET                                         0x00000030
#define DWDMA_AHB_SSTATAR0_TYPE                                           UInt64
#define DWDMA_AHB_SSTATAR0_RESERVED_MASK                                  0xFFFFFFFF00000000
#define    DWDMA_AHB_SSTATAR0_SSTATAR_SHIFT                               0
#define    DWDMA_AHB_SSTATAR0_SSTATAR_MASK                                0xFFFFFFFF

#define DWDMA_AHB_DSTATAR0_OFFSET                                         0x00000038
#define DWDMA_AHB_DSTATAR0_TYPE                                           UInt64
#define DWDMA_AHB_DSTATAR0_RESERVED_MASK                                  0xFFFFFFFF00000000
#define    DWDMA_AHB_DSTATAR0_DSTATAR_SHIFT                               0
#define    DWDMA_AHB_DSTATAR0_DSTATAR_MASK                                0xFFFFFFFF

#define DWDMA_AHB_CFG0_OFFSET                                             0x00000040
#define DWDMA_AHB_CFG0_TYPE                                               UInt64
#define DWDMA_AHB_CFG0_RESERVED_MASK                                      0xFFFF80000000001F
#define    DWDMA_AHB_CFG0_DEST_PER_SHIFT                                  43
#define    DWDMA_AHB_CFG0_DEST_PER_MASK                                   0x780000000000
#define    DWDMA_AHB_CFG0_SRC_PER_SHIFT                                   39
#define    DWDMA_AHB_CFG0_SRC_PER_MASK                                    0x78000000000
#define    DWDMA_AHB_CFG0_SS_UPD_EN_SHIFT                                 38
#define    DWDMA_AHB_CFG0_SS_UPD_EN_MASK                                  0x4000000000
#define    DWDMA_AHB_CFG0_DS_UPD_EN_SHIFT                                 37
#define    DWDMA_AHB_CFG0_DS_UPD_EN_MASK                                  0x2000000000
#define    DWDMA_AHB_CFG0_PROTCTL_SHIFT                                   34
#define    DWDMA_AHB_CFG0_PROTCTL_MASK                                    0x1C00000000
#define    DWDMA_AHB_CFG0_FIFO_MODE_SHIFT                                 33
#define    DWDMA_AHB_CFG0_FIFO_MODE_MASK                                  0x200000000
#define    DWDMA_AHB_CFG0_FCMODE_SHIFT                                    32
#define    DWDMA_AHB_CFG0_FCMODE_MASK                                     0x100000000
#define    DWDMA_AHB_CFG0_RELOAD_DST_SHIFT                                31
#define    DWDMA_AHB_CFG0_RELOAD_DST_MASK                                 0x80000000
#define    DWDMA_AHB_CFG0_RELOAD_SRC_SHIFT                                30
#define    DWDMA_AHB_CFG0_RELOAD_SRC_MASK                                 0x40000000
#define    DWDMA_AHB_CFG0_MAX_ABRST_SHIFT                                 20
#define    DWDMA_AHB_CFG0_MAX_ABRST_MASK                                  0x3FF00000
#define    DWDMA_AHB_CFG0_SRC_HS_POL_SHIFT                                19
#define    DWDMA_AHB_CFG0_SRC_HS_POL_MASK                                 0x00080000
#define    DWDMA_AHB_CFG0_DST_HS_POL_SHIFT                                18
#define    DWDMA_AHB_CFG0_DST_HS_POL_MASK                                 0x00040000
#define    DWDMA_AHB_CFG0_LOCK_B_SHIFT                                    17
#define    DWDMA_AHB_CFG0_LOCK_B_MASK                                     0x00020000
#define    DWDMA_AHB_CFG0_LOCK_CH_SHIFT                                   16
#define    DWDMA_AHB_CFG0_LOCK_CH_MASK                                    0x00010000
#define    DWDMA_AHB_CFG0_LOCK_B_L_SHIFT                                  14
#define    DWDMA_AHB_CFG0_LOCK_B_L_MASK                                   0x0000C000
#define    DWDMA_AHB_CFG0_LOCK_CH_L_SHIFT                                 12
#define    DWDMA_AHB_CFG0_LOCK_CH_L_MASK                                  0x00003000
#define    DWDMA_AHB_CFG0_HS_SEL_SRC_SHIFT                                11
#define    DWDMA_AHB_CFG0_HS_SEL_SRC_MASK                                 0x00000800
#define    DWDMA_AHB_CFG0_HS_SEL_DST_SHIFT                                10
#define    DWDMA_AHB_CFG0_HS_SEL_DST_MASK                                 0x00000400
#define    DWDMA_AHB_CFG0_FIFO_EMPTY_SHIFT                                9
#define    DWDMA_AHB_CFG0_FIFO_EMPTY_MASK                                 0x00000200
#define    DWDMA_AHB_CFG0_CH_SUSP_SHIFT                                   8
#define    DWDMA_AHB_CFG0_CH_SUSP_MASK                                    0x00000100
#define    DWDMA_AHB_CFG0_CH_PRIOR_SHIFT                                  5
#define    DWDMA_AHB_CFG0_CH_PRIOR_MASK                                   0x000000E0

#define DWDMA_AHB_SGR0_OFFSET                                             0x00000048
#define DWDMA_AHB_SGR0_TYPE                                               UInt64
#define DWDMA_AHB_SGR0_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_SGR0_SGR_SHIFT                                       20
#define    DWDMA_AHB_SGR0_SGR_MASK                                        0xFFF00000
#define    DWDMA_AHB_SGR0_SGI_SHIFT                                       0
#define    DWDMA_AHB_SGR0_SGI_MASK                                        0x000FFFFF

#define DWDMA_AHB_DSR0_OFFSET                                             0x00000050
#define DWDMA_AHB_DSR0_TYPE                                               UInt64
#define DWDMA_AHB_DSR0_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_DSR0_DSC_SHIFT                                       20
#define    DWDMA_AHB_DSR0_DSC_MASK                                        0xFFF00000
#define    DWDMA_AHB_DSR0_DSI_SHIFT                                       0
#define    DWDMA_AHB_DSR0_DSI_MASK                                        0x000FFFFF

#define DWDMA_AHB_SAR1_OFFSET                                             0x00000058
#define DWDMA_AHB_SAR1_TYPE                                               UInt64
#define DWDMA_AHB_SAR1_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_SAR1_SAR_SHIFT                                       0
#define    DWDMA_AHB_SAR1_SAR_MASK                                        0xFFFFFFFF

#define DWDMA_AHB_DAR1_OFFSET                                             0x00000060
#define DWDMA_AHB_DAR1_TYPE                                               UInt64
#define DWDMA_AHB_DAR1_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_DAR1_DAR_SHIFT                                       0
#define    DWDMA_AHB_DAR1_DAR_MASK                                        0xFFFFFFFF

#define DWDMA_AHB_LLP1_OFFSET                                             0x00000068
#define DWDMA_AHB_LLP1_TYPE                                               UInt64
#define DWDMA_AHB_LLP1_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_LLP1_LOC_SHIFT                                       2
#define    DWDMA_AHB_LLP1_LOC_MASK                                        0xFFFFFFFC
#define    DWDMA_AHB_LLP1_LMS_SHIFT                                       0
#define    DWDMA_AHB_LLP1_LMS_MASK                                        0x00000003

#define DWDMA_AHB_CTL1_OFFSET                                             0x00000070
#define DWDMA_AHB_CTL1_TYPE                                               UInt64
#define DWDMA_AHB_CTL1_RESERVED_MASK                                      0xFFFFE000E0080000
#define    DWDMA_AHB_CTL1_DONE_SHIFT                                      44
#define    DWDMA_AHB_CTL1_DONE_MASK                                       0x100000000000
#define    DWDMA_AHB_CTL1_BLOCK_TS_SHIFT                                  32
#define    DWDMA_AHB_CTL1_BLOCK_TS_MASK                                   0xFFF00000000
#define    DWDMA_AHB_CTL1_LLP_SRC_EN_SHIFT                                28
#define    DWDMA_AHB_CTL1_LLP_SRC_EN_MASK                                 0x10000000
#define    DWDMA_AHB_CTL1_LLP_DST_EN_SHIFT                                27
#define    DWDMA_AHB_CTL1_LLP_DST_EN_MASK                                 0x08000000
#define    DWDMA_AHB_CTL1_SMS_SHIFT                                       25
#define    DWDMA_AHB_CTL1_SMS_MASK                                        0x06000000
#define    DWDMA_AHB_CTL1_DMS_SHIFT                                       23
#define    DWDMA_AHB_CTL1_DMS_MASK                                        0x01800000
#define    DWDMA_AHB_CTL1_TT_FC_SHIFT                                     20
#define    DWDMA_AHB_CTL1_TT_FC_MASK                                      0x00700000
#define    DWDMA_AHB_CTL1_DST_SCATTER_EN_SHIFT                            18
#define    DWDMA_AHB_CTL1_DST_SCATTER_EN_MASK                             0x00040000
#define    DWDMA_AHB_CTL1_SRC_GATHER_EN_SHIFT                             17
#define    DWDMA_AHB_CTL1_SRC_GATHER_EN_MASK                              0x00020000
#define    DWDMA_AHB_CTL1_SRC_MSIZE_SHIFT                                 14
#define    DWDMA_AHB_CTL1_SRC_MSIZE_MASK                                  0x0001C000
#define    DWDMA_AHB_CTL1_DST_MSIZE_SHIFT                                 11
#define    DWDMA_AHB_CTL1_DST_MSIZE_MASK                                  0x00003800
#define    DWDMA_AHB_CTL1_SINC_SHIFT                                      9
#define    DWDMA_AHB_CTL1_SINC_MASK                                       0x00000600
#define    DWDMA_AHB_CTL1_DINC_SHIFT                                      7
#define    DWDMA_AHB_CTL1_DINC_MASK                                       0x00000180
#define    DWDMA_AHB_CTL1_SRC_TR_WIDTH_SHIFT                              4
#define    DWDMA_AHB_CTL1_SRC_TR_WIDTH_MASK                               0x00000070
#define    DWDMA_AHB_CTL1_DST_TR_WIDTH_SHIFT                              1
#define    DWDMA_AHB_CTL1_DST_TR_WIDTH_MASK                               0x0000000E
#define    DWDMA_AHB_CTL1_INT_EN_SHIFT                                    0
#define    DWDMA_AHB_CTL1_INT_EN_MASK                                     0x00000001

#define DWDMA_AHB_SSTAT1_OFFSET                                           0x00000078
#define DWDMA_AHB_SSTAT1_TYPE                                             UInt64
#define DWDMA_AHB_SSTAT1_RESERVED_MASK                                    0xFFFFFFFF00000000
#define    DWDMA_AHB_SSTAT1_SSTAT_SHIFT                                   0
#define    DWDMA_AHB_SSTAT1_SSTAT_MASK                                    0xFFFFFFFF

#define DWDMA_AHB_DSTAT1_OFFSET                                           0x00000080
#define DWDMA_AHB_DSTAT1_TYPE                                             UInt64
#define DWDMA_AHB_DSTAT1_RESERVED_MASK                                    0xFFFFFFFF00000000
#define    DWDMA_AHB_DSTAT1_DSTAT_SHIFT                                   0
#define    DWDMA_AHB_DSTAT1_DSTAT_MASK                                    0xFFFFFFFF

#define DWDMA_AHB_SSTATAR1_OFFSET                                         0x00000088
#define DWDMA_AHB_SSTATAR1_TYPE                                           UInt64
#define DWDMA_AHB_SSTATAR1_RESERVED_MASK                                  0xFFFFFFFF00000000
#define    DWDMA_AHB_SSTATAR1_SSTATAR_SHIFT                               0
#define    DWDMA_AHB_SSTATAR1_SSTATAR_MASK                                0xFFFFFFFF

#define DWDMA_AHB_DSTATAR1_OFFSET                                         0x00000090
#define DWDMA_AHB_DSTATAR1_TYPE                                           UInt64
#define DWDMA_AHB_DSTATAR1_RESERVED_MASK                                  0xFFFFFFFF00000000
#define    DWDMA_AHB_DSTATAR1_DSTATAR_SHIFT                               0
#define    DWDMA_AHB_DSTATAR1_DSTATAR_MASK                                0xFFFFFFFF

#define DWDMA_AHB_CFG1_OFFSET                                             0x00000098
#define DWDMA_AHB_CFG1_TYPE                                               UInt64
#define DWDMA_AHB_CFG1_RESERVED_MASK                                      0xFFFF80000000001F
#define    DWDMA_AHB_CFG1_DEST_PER_SHIFT                                  43
#define    DWDMA_AHB_CFG1_DEST_PER_MASK                                   0x780000000000
#define    DWDMA_AHB_CFG1_SRC_PER_SHIFT                                   39
#define    DWDMA_AHB_CFG1_SRC_PER_MASK                                    0x78000000000
#define    DWDMA_AHB_CFG1_SS_UPD_EN_SHIFT                                 38
#define    DWDMA_AHB_CFG1_SS_UPD_EN_MASK                                  0x4000000000
#define    DWDMA_AHB_CFG1_DS_UPD_EN_SHIFT                                 37
#define    DWDMA_AHB_CFG1_DS_UPD_EN_MASK                                  0x2000000000
#define    DWDMA_AHB_CFG1_PROTCTL_SHIFT                                   34
#define    DWDMA_AHB_CFG1_PROTCTL_MASK                                    0x1C00000000
#define    DWDMA_AHB_CFG1_FIFO_MODE_SHIFT                                 33
#define    DWDMA_AHB_CFG1_FIFO_MODE_MASK                                  0x200000000
#define    DWDMA_AHB_CFG1_FCMODE_SHIFT                                    32
#define    DWDMA_AHB_CFG1_FCMODE_MASK                                     0x100000000
#define    DWDMA_AHB_CFG1_RELOAD_DST_SHIFT                                31
#define    DWDMA_AHB_CFG1_RELOAD_DST_MASK                                 0x80000000
#define    DWDMA_AHB_CFG1_RELOAD_SRC_SHIFT                                30
#define    DWDMA_AHB_CFG1_RELOAD_SRC_MASK                                 0x40000000
#define    DWDMA_AHB_CFG1_MAX_ABRST_SHIFT                                 20
#define    DWDMA_AHB_CFG1_MAX_ABRST_MASK                                  0x3FF00000
#define    DWDMA_AHB_CFG1_SRC_HS_POL_SHIFT                                19
#define    DWDMA_AHB_CFG1_SRC_HS_POL_MASK                                 0x00080000
#define    DWDMA_AHB_CFG1_DST_HS_POL_SHIFT                                18
#define    DWDMA_AHB_CFG1_DST_HS_POL_MASK                                 0x00040000
#define    DWDMA_AHB_CFG1_LOCK_B_SHIFT                                    17
#define    DWDMA_AHB_CFG1_LOCK_B_MASK                                     0x00020000
#define    DWDMA_AHB_CFG1_LOCK_CH_SHIFT                                   16
#define    DWDMA_AHB_CFG1_LOCK_CH_MASK                                    0x00010000
#define    DWDMA_AHB_CFG1_LOCK_B_L_SHIFT                                  14
#define    DWDMA_AHB_CFG1_LOCK_B_L_MASK                                   0x0000C000
#define    DWDMA_AHB_CFG1_LOCK_CH_L_SHIFT                                 12
#define    DWDMA_AHB_CFG1_LOCK_CH_L_MASK                                  0x00003000
#define    DWDMA_AHB_CFG1_HS_SEL_SRC_SHIFT                                11
#define    DWDMA_AHB_CFG1_HS_SEL_SRC_MASK                                 0x00000800
#define    DWDMA_AHB_CFG1_HS_SEL_DST_SHIFT                                10
#define    DWDMA_AHB_CFG1_HS_SEL_DST_MASK                                 0x00000400
#define    DWDMA_AHB_CFG1_FIFO_EMPTY_SHIFT                                9
#define    DWDMA_AHB_CFG1_FIFO_EMPTY_MASK                                 0x00000200
#define    DWDMA_AHB_CFG1_CH_SUSP_SHIFT                                   8
#define    DWDMA_AHB_CFG1_CH_SUSP_MASK                                    0x00000100
#define    DWDMA_AHB_CFG1_CH_PRIOR_SHIFT                                  5
#define    DWDMA_AHB_CFG1_CH_PRIOR_MASK                                   0x000000E0

#define DWDMA_AHB_SGR1_OFFSET                                             0x000000A0
#define DWDMA_AHB_SGR1_TYPE                                               UInt64
#define DWDMA_AHB_SGR1_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_SGR1_SGR_SHIFT                                       20
#define    DWDMA_AHB_SGR1_SGR_MASK                                        0xFFF00000
#define    DWDMA_AHB_SGR1_SGI_SHIFT                                       0
#define    DWDMA_AHB_SGR1_SGI_MASK                                        0x000FFFFF

#define DWDMA_AHB_DSR1_OFFSET                                             0x000000A8
#define DWDMA_AHB_DSR1_TYPE                                               UInt64
#define DWDMA_AHB_DSR1_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_DSR1_DSC_SHIFT                                       20
#define    DWDMA_AHB_DSR1_DSC_MASK                                        0xFFF00000
#define    DWDMA_AHB_DSR1_DSI_SHIFT                                       0
#define    DWDMA_AHB_DSR1_DSI_MASK                                        0x000FFFFF

#define DWDMA_AHB_SAR2_OFFSET                                             0x000000B0
#define DWDMA_AHB_SAR2_TYPE                                               UInt64
#define DWDMA_AHB_SAR2_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_SAR2_SAR_SHIFT                                       0
#define    DWDMA_AHB_SAR2_SAR_MASK                                        0xFFFFFFFF

#define DWDMA_AHB_DAR2_OFFSET                                             0x000000B8
#define DWDMA_AHB_DAR2_TYPE                                               UInt64
#define DWDMA_AHB_DAR2_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_DAR2_DAR_SHIFT                                       0
#define    DWDMA_AHB_DAR2_DAR_MASK                                        0xFFFFFFFF

#define DWDMA_AHB_LLP2_OFFSET                                             0x000000C0
#define DWDMA_AHB_LLP2_TYPE                                               UInt64
#define DWDMA_AHB_LLP2_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_LLP2_LOC_SHIFT                                       2
#define    DWDMA_AHB_LLP2_LOC_MASK                                        0xFFFFFFFC
#define    DWDMA_AHB_LLP2_LMS_SHIFT                                       0
#define    DWDMA_AHB_LLP2_LMS_MASK                                        0x00000003

#define DWDMA_AHB_CTL2_OFFSET                                             0x000000C8
#define DWDMA_AHB_CTL2_TYPE                                               UInt64
#define DWDMA_AHB_CTL2_RESERVED_MASK                                      0xFFFFE000E0080000
#define    DWDMA_AHB_CTL2_DONE_SHIFT                                      44
#define    DWDMA_AHB_CTL2_DONE_MASK                                       0x100000000000
#define    DWDMA_AHB_CTL2_BLOCK_TS_SHIFT                                  32
#define    DWDMA_AHB_CTL2_BLOCK_TS_MASK                                   0xFFF00000000
#define    DWDMA_AHB_CTL2_LLP_SRC_EN_SHIFT                                28
#define    DWDMA_AHB_CTL2_LLP_SRC_EN_MASK                                 0x10000000
#define    DWDMA_AHB_CTL2_LLP_DST_EN_SHIFT                                27
#define    DWDMA_AHB_CTL2_LLP_DST_EN_MASK                                 0x08000000
#define    DWDMA_AHB_CTL2_SMS_SHIFT                                       25
#define    DWDMA_AHB_CTL2_SMS_MASK                                        0x06000000
#define    DWDMA_AHB_CTL2_DMS_SHIFT                                       23
#define    DWDMA_AHB_CTL2_DMS_MASK                                        0x01800000
#define    DWDMA_AHB_CTL2_TT_FC_SHIFT                                     20
#define    DWDMA_AHB_CTL2_TT_FC_MASK                                      0x00700000
#define    DWDMA_AHB_CTL2_DST_SCATTER_EN_SHIFT                            18
#define    DWDMA_AHB_CTL2_DST_SCATTER_EN_MASK                             0x00040000
#define    DWDMA_AHB_CTL2_SRC_GATHER_EN_SHIFT                             17
#define    DWDMA_AHB_CTL2_SRC_GATHER_EN_MASK                              0x00020000
#define    DWDMA_AHB_CTL2_SRC_MSIZE_SHIFT                                 14
#define    DWDMA_AHB_CTL2_SRC_MSIZE_MASK                                  0x0001C000
#define    DWDMA_AHB_CTL2_DST_MSIZE_SHIFT                                 11
#define    DWDMA_AHB_CTL2_DST_MSIZE_MASK                                  0x00003800
#define    DWDMA_AHB_CTL2_SINC_SHIFT                                      9
#define    DWDMA_AHB_CTL2_SINC_MASK                                       0x00000600
#define    DWDMA_AHB_CTL2_DINC_SHIFT                                      7
#define    DWDMA_AHB_CTL2_DINC_MASK                                       0x00000180
#define    DWDMA_AHB_CTL2_SRC_TR_WIDTH_SHIFT                              4
#define    DWDMA_AHB_CTL2_SRC_TR_WIDTH_MASK                               0x00000070
#define    DWDMA_AHB_CTL2_DST_TR_WIDTH_SHIFT                              1
#define    DWDMA_AHB_CTL2_DST_TR_WIDTH_MASK                               0x0000000E
#define    DWDMA_AHB_CTL2_INT_EN_SHIFT                                    0
#define    DWDMA_AHB_CTL2_INT_EN_MASK                                     0x00000001

#define DWDMA_AHB_SSTAT2_OFFSET                                           0x000000D0
#define DWDMA_AHB_SSTAT2_TYPE                                             UInt64
#define DWDMA_AHB_SSTAT2_RESERVED_MASK                                    0xFFFFFFFF00000000
#define    DWDMA_AHB_SSTAT2_SSTAT_SHIFT                                   0
#define    DWDMA_AHB_SSTAT2_SSTAT_MASK                                    0xFFFFFFFF

#define DWDMA_AHB_DSTAT2_OFFSET                                           0x000000D8
#define DWDMA_AHB_DSTAT2_TYPE                                             UInt64
#define DWDMA_AHB_DSTAT2_RESERVED_MASK                                    0xFFFFFFFF00000000
#define    DWDMA_AHB_DSTAT2_DSTAT_SHIFT                                   0
#define    DWDMA_AHB_DSTAT2_DSTAT_MASK                                    0xFFFFFFFF

#define DWDMA_AHB_SSTATAR2_OFFSET                                         0x000000E0
#define DWDMA_AHB_SSTATAR2_TYPE                                           UInt64
#define DWDMA_AHB_SSTATAR2_RESERVED_MASK                                  0xFFFFFFFF00000000
#define    DWDMA_AHB_SSTATAR2_SSTATAR_SHIFT                               0
#define    DWDMA_AHB_SSTATAR2_SSTATAR_MASK                                0xFFFFFFFF

#define DWDMA_AHB_DSTATAR2_OFFSET                                         0x000000E8
#define DWDMA_AHB_DSTATAR2_TYPE                                           UInt64
#define DWDMA_AHB_DSTATAR2_RESERVED_MASK                                  0xFFFFFFFF00000000
#define    DWDMA_AHB_DSTATAR2_DSTATAR_SHIFT                               0
#define    DWDMA_AHB_DSTATAR2_DSTATAR_MASK                                0xFFFFFFFF

#define DWDMA_AHB_CFG2_OFFSET                                             0x000000F0
#define DWDMA_AHB_CFG2_TYPE                                               UInt64
#define DWDMA_AHB_CFG2_RESERVED_MASK                                      0xFFFF80000000001F
#define    DWDMA_AHB_CFG2_DEST_PER_SHIFT                                  43
#define    DWDMA_AHB_CFG2_DEST_PER_MASK                                   0x780000000000
#define    DWDMA_AHB_CFG2_SRC_PER_SHIFT                                   39
#define    DWDMA_AHB_CFG2_SRC_PER_MASK                                    0x78000000000
#define    DWDMA_AHB_CFG2_SS_UPD_EN_SHIFT                                 38
#define    DWDMA_AHB_CFG2_SS_UPD_EN_MASK                                  0x4000000000
#define    DWDMA_AHB_CFG2_DS_UPD_EN_SHIFT                                 37
#define    DWDMA_AHB_CFG2_DS_UPD_EN_MASK                                  0x2000000000
#define    DWDMA_AHB_CFG2_PROTCTL_SHIFT                                   34
#define    DWDMA_AHB_CFG2_PROTCTL_MASK                                    0x1C00000000
#define    DWDMA_AHB_CFG2_FIFO_MODE_SHIFT                                 33
#define    DWDMA_AHB_CFG2_FIFO_MODE_MASK                                  0x200000000
#define    DWDMA_AHB_CFG2_FCMODE_SHIFT                                    32
#define    DWDMA_AHB_CFG2_FCMODE_MASK                                     0x100000000
#define    DWDMA_AHB_CFG2_RELOAD_DST_SHIFT                                31
#define    DWDMA_AHB_CFG2_RELOAD_DST_MASK                                 0x80000000
#define    DWDMA_AHB_CFG2_RELOAD_SRC_SHIFT                                30
#define    DWDMA_AHB_CFG2_RELOAD_SRC_MASK                                 0x40000000
#define    DWDMA_AHB_CFG2_MAX_ABRST_SHIFT                                 20
#define    DWDMA_AHB_CFG2_MAX_ABRST_MASK                                  0x3FF00000
#define    DWDMA_AHB_CFG2_SRC_HS_POL_SHIFT                                19
#define    DWDMA_AHB_CFG2_SRC_HS_POL_MASK                                 0x00080000
#define    DWDMA_AHB_CFG2_DST_HS_POL_SHIFT                                18
#define    DWDMA_AHB_CFG2_DST_HS_POL_MASK                                 0x00040000
#define    DWDMA_AHB_CFG2_LOCK_B_SHIFT                                    17
#define    DWDMA_AHB_CFG2_LOCK_B_MASK                                     0x00020000
#define    DWDMA_AHB_CFG2_LOCK_CH_SHIFT                                   16
#define    DWDMA_AHB_CFG2_LOCK_CH_MASK                                    0x00010000
#define    DWDMA_AHB_CFG2_LOCK_B_L_SHIFT                                  14
#define    DWDMA_AHB_CFG2_LOCK_B_L_MASK                                   0x0000C000
#define    DWDMA_AHB_CFG2_LOCK_CH_L_SHIFT                                 12
#define    DWDMA_AHB_CFG2_LOCK_CH_L_MASK                                  0x00003000
#define    DWDMA_AHB_CFG2_HS_SEL_SRC_SHIFT                                11
#define    DWDMA_AHB_CFG2_HS_SEL_SRC_MASK                                 0x00000800
#define    DWDMA_AHB_CFG2_HS_SEL_DST_SHIFT                                10
#define    DWDMA_AHB_CFG2_HS_SEL_DST_MASK                                 0x00000400
#define    DWDMA_AHB_CFG2_FIFO_EMPTY_SHIFT                                9
#define    DWDMA_AHB_CFG2_FIFO_EMPTY_MASK                                 0x00000200
#define    DWDMA_AHB_CFG2_CH_SUSP_SHIFT                                   8
#define    DWDMA_AHB_CFG2_CH_SUSP_MASK                                    0x00000100
#define    DWDMA_AHB_CFG2_CH_PRIOR_SHIFT                                  5
#define    DWDMA_AHB_CFG2_CH_PRIOR_MASK                                   0x000000E0

#define DWDMA_AHB_SGR2_OFFSET                                             0x000000F8
#define DWDMA_AHB_SGR2_TYPE                                               UInt64
#define DWDMA_AHB_SGR2_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_SGR2_SGR_SHIFT                                       20
#define    DWDMA_AHB_SGR2_SGR_MASK                                        0xFFF00000
#define    DWDMA_AHB_SGR2_SGI_SHIFT                                       0
#define    DWDMA_AHB_SGR2_SGI_MASK                                        0x000FFFFF

#define DWDMA_AHB_DSR2_OFFSET                                             0x00000100
#define DWDMA_AHB_DSR2_TYPE                                               UInt64
#define DWDMA_AHB_DSR2_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_DSR2_DSC_SHIFT                                       20
#define    DWDMA_AHB_DSR2_DSC_MASK                                        0xFFF00000
#define    DWDMA_AHB_DSR2_DSI_SHIFT                                       0
#define    DWDMA_AHB_DSR2_DSI_MASK                                        0x000FFFFF

#define DWDMA_AHB_SAR3_OFFSET                                             0x00000108
#define DWDMA_AHB_SAR3_TYPE                                               UInt64
#define DWDMA_AHB_SAR3_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_SAR3_SAR_SHIFT                                       0
#define    DWDMA_AHB_SAR3_SAR_MASK                                        0xFFFFFFFF

#define DWDMA_AHB_DAR3_OFFSET                                             0x00000110
#define DWDMA_AHB_DAR3_TYPE                                               UInt64
#define DWDMA_AHB_DAR3_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_DAR3_DAR_SHIFT                                       0
#define    DWDMA_AHB_DAR3_DAR_MASK                                        0xFFFFFFFF

#define DWDMA_AHB_LLP3_OFFSET                                             0x00000118
#define DWDMA_AHB_LLP3_TYPE                                               UInt64
#define DWDMA_AHB_LLP3_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_LLP3_LOC_SHIFT                                       2
#define    DWDMA_AHB_LLP3_LOC_MASK                                        0xFFFFFFFC
#define    DWDMA_AHB_LLP3_LMS_SHIFT                                       0
#define    DWDMA_AHB_LLP3_LMS_MASK                                        0x00000003

#define DWDMA_AHB_CTL3_OFFSET                                             0x00000120
#define DWDMA_AHB_CTL3_TYPE                                               UInt64
#define DWDMA_AHB_CTL3_RESERVED_MASK                                      0xFFFFE000E0080000
#define    DWDMA_AHB_CTL3_DONE_SHIFT                                      44
#define    DWDMA_AHB_CTL3_DONE_MASK                                       0x100000000000
#define    DWDMA_AHB_CTL3_BLOCK_TS_SHIFT                                  32
#define    DWDMA_AHB_CTL3_BLOCK_TS_MASK                                   0xFFF00000000
#define    DWDMA_AHB_CTL3_LLP_SRC_EN_SHIFT                                28
#define    DWDMA_AHB_CTL3_LLP_SRC_EN_MASK                                 0x10000000
#define    DWDMA_AHB_CTL3_LLP_DST_EN_SHIFT                                27
#define    DWDMA_AHB_CTL3_LLP_DST_EN_MASK                                 0x08000000
#define    DWDMA_AHB_CTL3_SMS_SHIFT                                       25
#define    DWDMA_AHB_CTL3_SMS_MASK                                        0x06000000
#define    DWDMA_AHB_CTL3_DMS_SHIFT                                       23
#define    DWDMA_AHB_CTL3_DMS_MASK                                        0x01800000
#define    DWDMA_AHB_CTL3_TT_FC_SHIFT                                     20
#define    DWDMA_AHB_CTL3_TT_FC_MASK                                      0x00700000
#define    DWDMA_AHB_CTL3_DST_SCATTER_EN_SHIFT                            18
#define    DWDMA_AHB_CTL3_DST_SCATTER_EN_MASK                             0x00040000
#define    DWDMA_AHB_CTL3_SRC_GATHER_EN_SHIFT                             17
#define    DWDMA_AHB_CTL3_SRC_GATHER_EN_MASK                              0x00020000
#define    DWDMA_AHB_CTL3_SRC_MSIZE_SHIFT                                 14
#define    DWDMA_AHB_CTL3_SRC_MSIZE_MASK                                  0x0001C000
#define    DWDMA_AHB_CTL3_DST_MSIZE_SHIFT                                 11
#define    DWDMA_AHB_CTL3_DST_MSIZE_MASK                                  0x00003800
#define    DWDMA_AHB_CTL3_SINC_SHIFT                                      9
#define    DWDMA_AHB_CTL3_SINC_MASK                                       0x00000600
#define    DWDMA_AHB_CTL3_DINC_SHIFT                                      7
#define    DWDMA_AHB_CTL3_DINC_MASK                                       0x00000180
#define    DWDMA_AHB_CTL3_SRC_TR_WIDTH_SHIFT                              4
#define    DWDMA_AHB_CTL3_SRC_TR_WIDTH_MASK                               0x00000070
#define    DWDMA_AHB_CTL3_DST_TR_WIDTH_SHIFT                              1
#define    DWDMA_AHB_CTL3_DST_TR_WIDTH_MASK                               0x0000000E
#define    DWDMA_AHB_CTL3_INT_EN_SHIFT                                    0
#define    DWDMA_AHB_CTL3_INT_EN_MASK                                     0x00000001

#define DWDMA_AHB_SSTAT3_OFFSET                                           0x00000128
#define DWDMA_AHB_SSTAT3_TYPE                                             UInt64
#define DWDMA_AHB_SSTAT3_RESERVED_MASK                                    0xFFFFFFFF00000000
#define    DWDMA_AHB_SSTAT3_SSTAT_SHIFT                                   0
#define    DWDMA_AHB_SSTAT3_SSTAT_MASK                                    0xFFFFFFFF

#define DWDMA_AHB_DSTAT3_OFFSET                                           0x00000130
#define DWDMA_AHB_DSTAT3_TYPE                                             UInt64
#define DWDMA_AHB_DSTAT3_RESERVED_MASK                                    0xFFFFFFFF00000000
#define    DWDMA_AHB_DSTAT3_DSTAT_SHIFT                                   0
#define    DWDMA_AHB_DSTAT3_DSTAT_MASK                                    0xFFFFFFFF

#define DWDMA_AHB_SSTATAR3_OFFSET                                         0x00000138
#define DWDMA_AHB_SSTATAR3_TYPE                                           UInt64
#define DWDMA_AHB_SSTATAR3_RESERVED_MASK                                  0xFFFFFFFF00000000
#define    DWDMA_AHB_SSTATAR3_SSTATAR_SHIFT                               0
#define    DWDMA_AHB_SSTATAR3_SSTATAR_MASK                                0xFFFFFFFF

#define DWDMA_AHB_DSTATAR3_OFFSET                                         0x00000140
#define DWDMA_AHB_DSTATAR3_TYPE                                           UInt64
#define DWDMA_AHB_DSTATAR3_RESERVED_MASK                                  0xFFFFFFFF00000000
#define    DWDMA_AHB_DSTATAR3_DSTATAR_SHIFT                               0
#define    DWDMA_AHB_DSTATAR3_DSTATAR_MASK                                0xFFFFFFFF

#define DWDMA_AHB_CFG3_OFFSET                                             0x00000148
#define DWDMA_AHB_CFG3_TYPE                                               UInt64
#define DWDMA_AHB_CFG3_RESERVED_MASK                                      0xFFFF80000000001F
#define    DWDMA_AHB_CFG3_DEST_PER_SHIFT                                  43
#define    DWDMA_AHB_CFG3_DEST_PER_MASK                                   0x780000000000
#define    DWDMA_AHB_CFG3_SRC_PER_SHIFT                                   39
#define    DWDMA_AHB_CFG3_SRC_PER_MASK                                    0x78000000000
#define    DWDMA_AHB_CFG3_SS_UPD_EN_SHIFT                                 38
#define    DWDMA_AHB_CFG3_SS_UPD_EN_MASK                                  0x4000000000
#define    DWDMA_AHB_CFG3_DS_UPD_EN_SHIFT                                 37
#define    DWDMA_AHB_CFG3_DS_UPD_EN_MASK                                  0x2000000000
#define    DWDMA_AHB_CFG3_PROTCTL_SHIFT                                   34
#define    DWDMA_AHB_CFG3_PROTCTL_MASK                                    0x1C00000000
#define    DWDMA_AHB_CFG3_FIFO_MODE_SHIFT                                 33
#define    DWDMA_AHB_CFG3_FIFO_MODE_MASK                                  0x200000000
#define    DWDMA_AHB_CFG3_FCMODE_SHIFT                                    32
#define    DWDMA_AHB_CFG3_FCMODE_MASK                                     0x100000000
#define    DWDMA_AHB_CFG3_RELOAD_DST_SHIFT                                31
#define    DWDMA_AHB_CFG3_RELOAD_DST_MASK                                 0x80000000
#define    DWDMA_AHB_CFG3_RELOAD_SRC_SHIFT                                30
#define    DWDMA_AHB_CFG3_RELOAD_SRC_MASK                                 0x40000000
#define    DWDMA_AHB_CFG3_MAX_ABRST_SHIFT                                 20
#define    DWDMA_AHB_CFG3_MAX_ABRST_MASK                                  0x3FF00000
#define    DWDMA_AHB_CFG3_SRC_HS_POL_SHIFT                                19
#define    DWDMA_AHB_CFG3_SRC_HS_POL_MASK                                 0x00080000
#define    DWDMA_AHB_CFG3_DST_HS_POL_SHIFT                                18
#define    DWDMA_AHB_CFG3_DST_HS_POL_MASK                                 0x00040000
#define    DWDMA_AHB_CFG3_LOCK_B_SHIFT                                    17
#define    DWDMA_AHB_CFG3_LOCK_B_MASK                                     0x00020000
#define    DWDMA_AHB_CFG3_LOCK_CH_SHIFT                                   16
#define    DWDMA_AHB_CFG3_LOCK_CH_MASK                                    0x00010000
#define    DWDMA_AHB_CFG3_LOCK_B_L_SHIFT                                  14
#define    DWDMA_AHB_CFG3_LOCK_B_L_MASK                                   0x0000C000
#define    DWDMA_AHB_CFG3_LOCK_CH_L_SHIFT                                 12
#define    DWDMA_AHB_CFG3_LOCK_CH_L_MASK                                  0x00003000
#define    DWDMA_AHB_CFG3_HS_SEL_SRC_SHIFT                                11
#define    DWDMA_AHB_CFG3_HS_SEL_SRC_MASK                                 0x00000800
#define    DWDMA_AHB_CFG3_HS_SEL_DST_SHIFT                                10
#define    DWDMA_AHB_CFG3_HS_SEL_DST_MASK                                 0x00000400
#define    DWDMA_AHB_CFG3_FIFO_EMPTY_SHIFT                                9
#define    DWDMA_AHB_CFG3_FIFO_EMPTY_MASK                                 0x00000200
#define    DWDMA_AHB_CFG3_CH_SUSP_SHIFT                                   8
#define    DWDMA_AHB_CFG3_CH_SUSP_MASK                                    0x00000100
#define    DWDMA_AHB_CFG3_CH_PRIOR_SHIFT                                  5
#define    DWDMA_AHB_CFG3_CH_PRIOR_MASK                                   0x000000E0

#define DWDMA_AHB_SGR3_OFFSET                                             0x00000150
#define DWDMA_AHB_SGR3_TYPE                                               UInt64
#define DWDMA_AHB_SGR3_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_SGR3_SGR_SHIFT                                       20
#define    DWDMA_AHB_SGR3_SGR_MASK                                        0xFFF00000
#define    DWDMA_AHB_SGR3_SGI_SHIFT                                       0
#define    DWDMA_AHB_SGR3_SGI_MASK                                        0x000FFFFF

#define DWDMA_AHB_DSR3_OFFSET                                             0x00000158
#define DWDMA_AHB_DSR3_TYPE                                               UInt64
#define DWDMA_AHB_DSR3_RESERVED_MASK                                      0xFFFFFFFF00000000
#define    DWDMA_AHB_DSR3_DSC_SHIFT                                       20
#define    DWDMA_AHB_DSR3_DSC_MASK                                        0xFFF00000
#define    DWDMA_AHB_DSR3_DSI_SHIFT                                       0
#define    DWDMA_AHB_DSR3_DSI_MASK                                        0x000FFFFF

#define DWDMA_AHB_RAWTFR_OFFSET                                           0x000002C0
#define DWDMA_AHB_RAWTFR_TYPE                                             UInt64
#define DWDMA_AHB_RAWTFR_RESERVED_MASK                                    0x00000000
#define    DWDMA_AHB_RAWTFR_RAWTFR_SHIFT                                  0
#define    DWDMA_AHB_RAWTFR_RAWTFR_MASK                                   0x0000000F

#define DWDMA_AHB_RAWBLOCK_OFFSET                                         0x000002C8
#define DWDMA_AHB_RAWBLOCK_TYPE                                           UInt64
#define DWDMA_AHB_RAWBLOCK_RESERVED_MASK                                  0x00000000
#define    DWDMA_AHB_RAWBLOCK_RAWBLOCK_SHIFT                              0
#define    DWDMA_AHB_RAWBLOCK_RAWBLOCK_MASK                               0x0000000F

#define DWDMA_AHB_RAWSRCTRAN_OFFSET                                       0x000002D0
#define DWDMA_AHB_RAWSRCTRAN_TYPE                                         UInt64
#define DWDMA_AHB_RAWSRCTRAN_RESERVED_MASK                                0x00000000
#define    DWDMA_AHB_RAWSRCTRAN_RAWSRCTRAN_SHIFT                          0
#define    DWDMA_AHB_RAWSRCTRAN_RAWSRCTRAN_MASK                           0x0000000F

#define DWDMA_AHB_RAWDSTTRAN_OFFSET                                       0x000002D8
#define DWDMA_AHB_RAWDSTTRAN_TYPE                                         UInt64
#define DWDMA_AHB_RAWDSTTRAN_RESERVED_MASK                                0x00000000
#define    DWDMA_AHB_RAWDSTTRAN_RAWDSTTRAN_SHIFT                          0
#define    DWDMA_AHB_RAWDSTTRAN_RAWDSTTRAN_MASK                           0x0000000F

#define DWDMA_AHB_RAWERR_OFFSET                                           0x000002E0
#define DWDMA_AHB_RAWERR_TYPE                                             UInt64
#define DWDMA_AHB_RAWERR_RESERVED_MASK                                    0x00000000
#define    DWDMA_AHB_RAWERR_RAWERR_SHIFT                                  0
#define    DWDMA_AHB_RAWERR_RAWERR_MASK                                   0x0000000F

#define DWDMA_AHB_STATUSTFR_OFFSET                                        0x000002E8
#define DWDMA_AHB_STATUSTFR_TYPE                                          UInt64
#define DWDMA_AHB_STATUSTFR_RESERVED_MASK                                 0x00000000
#define    DWDMA_AHB_STATUSTFR_STATUSTFR_SHIFT                            0
#define    DWDMA_AHB_STATUSTFR_STATUSTFR_MASK                             0x0000000F

#define DWDMA_AHB_STATUSBLOCK_OFFSET                                      0x000002F0
#define DWDMA_AHB_STATUSBLOCK_TYPE                                        UInt64
#define DWDMA_AHB_STATUSBLOCK_RESERVED_MASK                               0x00000000
#define    DWDMA_AHB_STATUSBLOCK_STATUSBLOCK_SHIFT                        0
#define    DWDMA_AHB_STATUSBLOCK_STATUSBLOCK_MASK                         0x0000000F

#define DWDMA_AHB_STATUSSRCTRAN_OFFSET                                    0x000002F8
#define DWDMA_AHB_STATUSSRCTRAN_TYPE                                      UInt64
#define DWDMA_AHB_STATUSSRCTRAN_RESERVED_MASK                             0x00000000
#define    DWDMA_AHB_STATUSSRCTRAN_STATUSSRCTRAN_SHIFT                    0
#define    DWDMA_AHB_STATUSSRCTRAN_STATUSSRCTRAN_MASK                     0x0000000F

#define DWDMA_AHB_STATUSDSTTRAN_OFFSET                                    0x00000300
#define DWDMA_AHB_STATUSDSTTRAN_TYPE                                      UInt64
#define DWDMA_AHB_STATUSDSTTRAN_RESERVED_MASK                             0x00000000
#define    DWDMA_AHB_STATUSDSTTRAN_STATUSDSTTRAN_SHIFT                    0
#define    DWDMA_AHB_STATUSDSTTRAN_STATUSDSTTRAN_MASK                     0x0000000F

#define DWDMA_AHB_STATUSERR_OFFSET                                        0x00000308
#define DWDMA_AHB_STATUSERR_TYPE                                          UInt64
#define DWDMA_AHB_STATUSERR_RESERVED_MASK                                 0x00000000
#define    DWDMA_AHB_STATUSERR_STATUSERR_SHIFT                            0
#define    DWDMA_AHB_STATUSERR_STATUSERR_MASK                             0x0000000F

#define DWDMA_AHB_MASKTFR_OFFSET                                          0x00000310
#define DWDMA_AHB_MASKTFR_TYPE                                            UInt64
#define DWDMA_AHB_MASKTFR_RESERVED_MASK                                   0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_MASKTFR_INT_MASK_WE_SHIFT                            8
#define    DWDMA_AHB_MASKTFR_INT_MASK_WE_MASK                             0x00000F00
#define    DWDMA_AHB_MASKTFR_INT_MASK_SHIFT                               0
#define    DWDMA_AHB_MASKTFR_INT_MASK_MASK                                0x0000000F

#define DWDMA_AHB_MASKBLOCK_OFFSET                                        0x00000318
#define DWDMA_AHB_MASKBLOCK_TYPE                                          UInt64
#define DWDMA_AHB_MASKBLOCK_RESERVED_MASK                                 0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_MASKBLOCK_INT_MASK_WE_SHIFT                          8
#define    DWDMA_AHB_MASKBLOCK_INT_MASK_WE_MASK                           0x00000F00
#define    DWDMA_AHB_MASKBLOCK_INT_MASK_SHIFT                             0
#define    DWDMA_AHB_MASKBLOCK_INT_MASK_MASK                              0x0000000F

#define DWDMA_AHB_MASKSRCTRAN_OFFSET                                      0x00000320
#define DWDMA_AHB_MASKSRCTRAN_TYPE                                        UInt64
#define DWDMA_AHB_MASKSRCTRAN_RESERVED_MASK                               0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_MASKSRCTRAN_INT_MASK_WE_SHIFT                        8
#define    DWDMA_AHB_MASKSRCTRAN_INT_MASK_WE_MASK                         0x00000F00
#define    DWDMA_AHB_MASKSRCTRAN_INT_MASK_SHIFT                           0
#define    DWDMA_AHB_MASKSRCTRAN_INT_MASK_MASK                            0x0000000F

#define DWDMA_AHB_MASKDSTTRAN_OFFSET                                      0x00000328
#define DWDMA_AHB_MASKDSTTRAN_TYPE                                        UInt64
#define DWDMA_AHB_MASKDSTTRAN_RESERVED_MASK                               0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_MASKDSTTRAN_INT_MASK_WE_SHIFT                        8
#define    DWDMA_AHB_MASKDSTTRAN_INT_MASK_WE_MASK                         0x00000F00
#define    DWDMA_AHB_MASKDSTTRAN_INT_MASK_SHIFT                           0
#define    DWDMA_AHB_MASKDSTTRAN_INT_MASK_MASK                            0x0000000F

#define DWDMA_AHB_MASKERR_OFFSET                                          0x00000330
#define DWDMA_AHB_MASKERR_TYPE                                            UInt64
#define DWDMA_AHB_MASKERR_RESERVED_MASK                                   0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_MASKERR_INT_MASK_WE_SHIFT                            8
#define    DWDMA_AHB_MASKERR_INT_MASK_WE_MASK                             0x00000F00
#define    DWDMA_AHB_MASKERR_INT_MASK_SHIFT                               0
#define    DWDMA_AHB_MASKERR_INT_MASK_MASK                                0x0000000F

#define DWDMA_AHB_CLEARTFR_OFFSET                                         0x00000338
#define DWDMA_AHB_CLEARTFR_TYPE                                           UInt64
#define DWDMA_AHB_CLEARTFR_RESERVED_MASK                                  0x00000000
#define    DWDMA_AHB_CLEARTFR_CLEAR_SHIFT                                 0
#define    DWDMA_AHB_CLEARTFR_CLEAR_MASK                                  0x0000000F

#define DWDMA_AHB_CLEARBLOCK_OFFSET                                       0x00000340
#define DWDMA_AHB_CLEARBLOCK_TYPE                                         UInt64
#define DWDMA_AHB_CLEARBLOCK_RESERVED_MASK                                0x00000000
#define    DWDMA_AHB_CLEARBLOCK_CLEAR_SHIFT                               0
#define    DWDMA_AHB_CLEARBLOCK_CLEAR_MASK                                0x0000000F

#define DWDMA_AHB_CLEARSRCTRAN_OFFSET                                     0x00000348
#define DWDMA_AHB_CLEARSRCTRAN_TYPE                                       UInt64
#define DWDMA_AHB_CLEARSRCTRAN_RESERVED_MASK                              0x00000000
#define    DWDMA_AHB_CLEARSRCTRAN_CLEAR_SHIFT                             0
#define    DWDMA_AHB_CLEARSRCTRAN_CLEAR_MASK                              0x0000000F

#define DWDMA_AHB_CLEARDSTTRAN_OFFSET                                     0x00000350
#define DWDMA_AHB_CLEARDSTTRAN_TYPE                                       UInt64
#define DWDMA_AHB_CLEARDSTTRAN_RESERVED_MASK                              0x00000000
#define    DWDMA_AHB_CLEARDSTTRAN_CLEAR_SHIFT                             0
#define    DWDMA_AHB_CLEARDSTTRAN_CLEAR_MASK                              0x0000000F

#define DWDMA_AHB_CLEARERR_OFFSET                                         0x00000358
#define DWDMA_AHB_CLEARERR_TYPE                                           UInt64
#define DWDMA_AHB_CLEARERR_RESERVED_MASK                                  0x00000000
#define    DWDMA_AHB_CLEARERR_CLEAR_SHIFT                                 0
#define    DWDMA_AHB_CLEARERR_CLEAR_MASK                                  0x0000000F

#define DWDMA_AHB_STATUSINT_OFFSET                                        0x00000360
#define DWDMA_AHB_STATUSINT_TYPE                                          UInt64
#define DWDMA_AHB_STATUSINT_RESERVED_MASK                                 0x00000000
#define    DWDMA_AHB_STATUSINT_CLEAR_SHIFT                                4
#define    DWDMA_AHB_STATUSINT_CLEAR_MASK                                 0x00000010
#define    DWDMA_AHB_STATUSINT_DSTT_SHIFT                                 3
#define    DWDMA_AHB_STATUSINT_DSTT_MASK                                  0x00000008
#define    DWDMA_AHB_STATUSINT_SRCT_SHIFT                                 2
#define    DWDMA_AHB_STATUSINT_SRCT_MASK                                  0x00000004
#define    DWDMA_AHB_STATUSINT_BLOCK_SHIFT                                1
#define    DWDMA_AHB_STATUSINT_BLOCK_MASK                                 0x00000002
#define    DWDMA_AHB_STATUSINT_TFR_SHIFT                                  0
#define    DWDMA_AHB_STATUSINT_TFR_MASK                                   0x00000001

#define DWDMA_AHB_REQSRC_OFFSET                                           0x00000368
#define DWDMA_AHB_REQSRC_TYPE                                             UInt64
#define DWDMA_AHB_REQSRC_RESERVED_MASK                                    0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_REQSRC_SRC_REQ_WE_SHIFT                              8
#define    DWDMA_AHB_REQSRC_SRC_REQ_WE_MASK                               0x00000F00
#define    DWDMA_AHB_REQSRC_SRC_REQ_SHIFT                                 0
#define    DWDMA_AHB_REQSRC_SRC_REQ_MASK                                  0x0000000F

#define DWDMA_AHB_REQDST_OFFSET                                           0x00000370
#define DWDMA_AHB_REQDST_TYPE                                             UInt64
#define DWDMA_AHB_REQDST_RESERVED_MASK                                    0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_REQDST_DST_REQ_WE_SHIFT                              8
#define    DWDMA_AHB_REQDST_DST_REQ_WE_MASK                               0x00000F00
#define    DWDMA_AHB_REQDST_DST_REQ_SHIFT                                 0
#define    DWDMA_AHB_REQDST_DST_REQ_MASK                                  0x0000000F

#define DWDMA_AHB_SGLREQSRC_OFFSET                                        0x00000378
#define DWDMA_AHB_SGLREQSRC_TYPE                                          UInt64
#define DWDMA_AHB_SGLREQSRC_RESERVED_MASK                                 0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_SGLREQSRC_SRC_SGLREQ_WE_SHIFT                        8
#define    DWDMA_AHB_SGLREQSRC_SRC_SGLREQ_WE_MASK                         0x00000F00
#define    DWDMA_AHB_SGLREQSRC_SRC_SGLREQ_SHIFT                           0
#define    DWDMA_AHB_SGLREQSRC_SRC_SGLREQ_MASK                            0x0000000F

#define DWDMA_AHB_SGLREQDST_OFFSET                                        0x00000380
#define DWDMA_AHB_SGLREQDST_TYPE                                          UInt64
#define DWDMA_AHB_SGLREQDST_RESERVED_MASK                                 0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_SGLREQDST_DST_REQ_WE_SHIFT                           8
#define    DWDMA_AHB_SGLREQDST_DST_REQ_WE_MASK                            0x00000F00
#define    DWDMA_AHB_SGLREQDST_DST_SGLREQ_SHIFT                           0
#define    DWDMA_AHB_SGLREQDST_DST_SGLREQ_MASK                            0x0000000F

#define DWDMA_AHB_LSTSRC_OFFSET                                           0x00000388
#define DWDMA_AHB_LSTSRC_TYPE                                             UInt64
#define DWDMA_AHB_LSTSRC_RESERVED_MASK                                    0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_LSTSRC_LSTSRC_WE_SHIFT                               8
#define    DWDMA_AHB_LSTSRC_LSTSRC_WE_MASK                                0x00000F00
#define    DWDMA_AHB_LSTSRC_LSTSRC_SHIFT                                  0
#define    DWDMA_AHB_LSTSRC_LSTSRC_MASK                                   0x0000000F

#define DWDMA_AHB_LSTDST_OFFSET                                           0x00000390
#define DWDMA_AHB_LSTDST_TYPE                                             UInt64
#define DWDMA_AHB_LSTDST_RESERVED_MASK                                    0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_LSTDST_LSTDST_WE_SHIFT                               8
#define    DWDMA_AHB_LSTDST_LSTDST_WE_MASK                                0x00000F00
#define    DWDMA_AHB_LSTDST_LSTDST_SHIFT                                  0
#define    DWDMA_AHB_LSTDST_LSTDST_MASK                                   0x0000000F

#define DWDMA_AHB_DMACFG_OFFSET                                           0x00000398
#define DWDMA_AHB_DMACFG_TYPE                                             UInt64
#define DWDMA_AHB_DMACFG_RESERVED_MASK                                    0x00000000
#define    DWDMA_AHB_DMACFG_DMA_EN_SHIFT                                  0
#define    DWDMA_AHB_DMACFG_DMA_EN_MASK                                   0x00000001

#define DWDMA_AHB_CHEN_OFFSET                                             0x000003A0
#define DWDMA_AHB_CHEN_TYPE                                               UInt64
#define DWDMA_AHB_CHEN_RESERVED_MASK                                      0xFFFFFFFFFFFFF0F0
#define    DWDMA_AHB_CHEN_CH_EN_WE_SHIFT                                  8
#define    DWDMA_AHB_CHEN_CH_EN_WE_MASK                                   0x00000F00
#define    DWDMA_AHB_CHEN_CH_EN_SHIFT                                     0
#define    DWDMA_AHB_CHEN_CH_EN_MASK                                      0x0000000F

#define DWDMA_AHB_DMAID_OFFSET                                            0x000003A8
#define DWDMA_AHB_DMAID_TYPE                                              UInt64
#define DWDMA_AHB_DMAID_RESERVED_MASK                                     0xFFFFFFFF00000000
#define    DWDMA_AHB_DMAID_DMA_ID_SHIFT                                   0
#define    DWDMA_AHB_DMAID_DMA_ID_MASK                                    0xFFFFFFFF

#define DWDMA_AHB_DMATEST_OFFSET                                          0x000003B0
#define DWDMA_AHB_DMATEST_TYPE                                            UInt64
#define DWDMA_AHB_DMATEST_RESERVED_MASK                                   0x00000000
#define    DWDMA_AHB_DMATEST_TEST_SLV_IF_SHIFT                            0
#define    DWDMA_AHB_DMATEST_TEST_SLV_IF_MASK                             0x00000001

#define DWDMA_AHB_COMP_PARAMS_4_OFFSET                                    0x000003D8
#define DWDMA_AHB_COMP_PARAMS_4_TYPE                                      UInt64
#define DWDMA_AHB_COMP_PARAMS_4_RESERVED_MASK                             0x80000000FFFFFFFF
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_FIFO_DEPTH_SHIFT                   60
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_FIFO_DEPTH_MASK                    0x7000000000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_SMS_SHIFT                          57
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_SMS_MASK                           0xE00000000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_LMS_SHIFT                          54
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_LMS_MASK                           0x1C0000000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_DMS_SHIFT                          51
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_DMS_MASK                           0x38000000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_MAX_MULT_SIZE_SHIFT                48
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_MAX_MULT_SIZE_MASK                 0x7000000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_FC_SHIFT                           46
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_FC_MASK                            0xC00000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_HC_LLP_SHIFT                       45
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_HC_LLP_MASK                        0x200000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_CTL_WB_EN_SHIFT                    44
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_CTL_WB_EN_MASK                     0x100000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_MULTI_BLK_EN_SHIFT                 43
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_MULTI_BLK_EN_MASK                  0x80000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_LOCK_EN_SHIFT                      42
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_LOCK_EN_MASK                       0x40000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_SRC_GAT_EN_SHIFT                   41
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_SRC_GAT_EN_MASK                    0x20000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_DST_SCA_EN_SHIFT                   40
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_DST_SCA_EN_MASK                    0x10000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_STAT_SRC_SHIFT                     39
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_STAT_SRC_MASK                      0x8000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_STAT_DST_SHIFT                     38
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_STAT_DST_MASK                      0x4000000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_STW_SHIFT                          35
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_STW_MASK                           0x3800000000
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_DTW_SHIFT                          32
#define    DWDMA_AHB_COMP_PARAMS_4_CH3_DTW_MASK                           0x700000000

#define DWDMA_AHB_COMP_PARAMS_3_OFFSET                                    0x000003E0
#define DWDMA_AHB_COMP_PARAMS_3_TYPE                                      UInt64
#define DWDMA_AHB_COMP_PARAMS_3_RESERVED_MASK                             0x8000000080000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_FIFO_DEPTH_SHIFT                   60
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_FIFO_DEPTH_MASK                    0x7000000000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_SMS_SHIFT                          57
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_SMS_MASK                           0xE00000000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_LMS_SHIFT                          54
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_LMS_MASK                           0x1C0000000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_DMS_SHIFT                          51
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_DMS_MASK                           0x38000000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_MAX_MULT_SIZE_SHIFT                48
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_MAX_MULT_SIZE_MASK                 0x7000000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_FC_SHIFT                           46
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_FC_MASK                            0xC00000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_HC_LLP_SHIFT                       45
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_HC_LLP_MASK                        0x200000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_CTL_WB_EN_SHIFT                    44
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_CTL_WB_EN_MASK                     0x100000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_MULTI_BLK_EN_SHIFT                 43
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_MULTI_BLK_EN_MASK                  0x80000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_LOCK_EN_SHIFT                      42
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_LOCK_EN_MASK                       0x40000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_SRC_GAT_EN_SHIFT                   41
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_SRC_GAT_EN_MASK                    0x20000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_DST_SCA_EN_SHIFT                   40
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_DST_SCA_EN_MASK                    0x10000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_STAT_SRC_SHIFT                     39
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_STAT_SRC_MASK                      0x8000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_STAT_DST_SHIFT                     38
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_STAT_DST_MASK                      0x4000000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_STW_SHIFT                          35
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_STW_MASK                           0x3800000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_DTW_SHIFT                          32
#define    DWDMA_AHB_COMP_PARAMS_3_CH1_DTW_MASK                           0x700000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_FIFO_DEPTH_SHIFT                   28
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_FIFO_DEPTH_MASK                    0x70000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_SMS_SHIFT                          25
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_SMS_MASK                           0x0E000000
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_LMS_SHIFT                          22
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_LMS_MASK                           0x01C00000
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_DMS_SHIFT                          19
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_DMS_MASK                           0x00380000
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_MAX_MULT_SIZE_SHIFT                16
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_MAX_MULT_SIZE_MASK                 0x00070000
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_FC_SHIFT                           14
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_FC_MASK                            0x0000C000
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_HC_LLP_SHIFT                       13
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_HC_LLP_MASK                        0x00002000
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_CTL_WB_EN_SHIFT                    12
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_CTL_WB_EN_MASK                     0x00001000
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_MULTI_BLK_EN_SHIFT                 11
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_MULTI_BLK_EN_MASK                  0x00000800
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_LOCK_EN_SHIFT                      10
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_LOCK_EN_MASK                       0x00000400
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_SRC_GAT_EN_SHIFT                   9
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_SRC_GAT_EN_MASK                    0x00000200
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_DST_SCA_EN_SHIFT                   8
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_DST_SCA_EN_MASK                    0x00000100
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_STAT_SRC_SHIFT                     7
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_STAT_SRC_MASK                      0x00000080
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_STAT_DST_SHIFT                     6
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_STAT_DST_MASK                      0x00000040
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_STW_SHIFT                          3
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_STW_MASK                           0x00000038
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_DTW_SHIFT                          0
#define    DWDMA_AHB_COMP_PARAMS_3_CH2_DTW_MASK                           0x00000007

#define DWDMA_AHB_COMP_PARAMS_2_OFFSET                                    0x000003E8
#define DWDMA_AHB_COMP_PARAMS_2_TYPE                                      UInt64
#define DWDMA_AHB_COMP_PARAMS_2_RESERVED_MASK                             0x80000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH7_MULTI_BLK_TYPE_SHIFT               60
#define    DWDMA_AHB_COMP_PARAMS_2_CH7_MULTI_BLK_TYPE_MASK                0xF000000000000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH6_MULTI_BLK_TYPE_SHIFT               56
#define    DWDMA_AHB_COMP_PARAMS_2_CH6_MULTI_BLK_TYPE_MASK                0xF00000000000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH5_MULTI_BLK_TYPE_SHIFT               52
#define    DWDMA_AHB_COMP_PARAMS_2_CH5_MULTI_BLK_TYPE_MASK                0xF0000000000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH4_MULTI_BLK_TYPE_SHIFT               48
#define    DWDMA_AHB_COMP_PARAMS_2_CH4_MULTI_BLK_TYPE_MASK                0xF000000000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH3_MULTI_BLK_TYPE_SHIFT               44
#define    DWDMA_AHB_COMP_PARAMS_2_CH3_MULTI_BLK_TYPE_MASK                0xF00000000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH2_MULTI_BLK_TYPE_SHIFT               40
#define    DWDMA_AHB_COMP_PARAMS_2_CH2_MULTI_BLK_TYPE_MASK                0xF0000000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH1_MULTI_BLK_TYPE_SHIFT               36
#define    DWDMA_AHB_COMP_PARAMS_2_CH1_MULTI_BLK_TYPE_MASK                0xF000000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_MULTI_BLK_TYPE_SHIFT               32
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_MULTI_BLK_TYPE_MASK                0xF00000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_FIFO_DEPTH_SHIFT                   28
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_FIFO_DEPTH_MASK                    0x70000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_SMS_SHIFT                          25
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_SMS_MASK                           0x0E000000
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_LMS_SHIFT                          22
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_LMS_MASK                           0x01C00000
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_DMS_SHIFT                          19
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_DMS_MASK                           0x00380000
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_MAX_MULT_SIZE_SHIFT                16
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_MAX_MULT_SIZE_MASK                 0x00070000
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_FC_SHIFT                           14
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_FC_MASK                            0x0000C000
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_HC_LLP_SHIFT                       13
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_HC_LLP_MASK                        0x00002000
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_CTL_WB_EN_SHIFT                    12
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_CTL_WB_EN_MASK                     0x00001000
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_MULTI_BLK_EN_SHIFT                 11
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_MULTI_BLK_EN_MASK                  0x00000800
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_LOCK_EN_SHIFT                      10
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_LOCK_EN_MASK                       0x00000400
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_SRC_GAT_EN_SHIFT                   9
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_SRC_GAT_EN_MASK                    0x00000200
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_DST_SCA_EN_SHIFT                   8
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_DST_SCA_EN_MASK                    0x00000100
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_STAT_SRC_SHIFT                     7
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_STAT_SRC_MASK                      0x00000080
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_STAT_DST_SHIFT                     6
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_STAT_DST_MASK                      0x00000040
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_STW_SHIFT                          3
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_STW_MASK                           0x00000038
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_DTW_SHIFT                          0
#define    DWDMA_AHB_COMP_PARAMS_2_CH0_DTW_MASK                           0x00000007

#define DWDMA_AHB_COMP_PARAMS_1_OFFSET                                    0x000003F0
#define DWDMA_AHB_COMP_PARAMS_1_TYPE                                      UInt64
#define DWDMA_AHB_COMP_PARAMS_1_RESERVED_MASK                             0xC00000F0FFFF0000
#define    DWDMA_AHB_COMP_PARAMS_1_STATIC_ENDIAN_SELECT_SHIFT             61
#define    DWDMA_AHB_COMP_PARAMS_1_STATIC_ENDIAN_SELECT_MASK              0x2000000000000000
#define    DWDMA_AHB_COMP_PARAMS_1_ADD_ENCODED_PARAMS_SHIFT               60
#define    DWDMA_AHB_COMP_PARAMS_1_ADD_ENCODED_PARAMS_MASK                0x1000000000000000
#define    DWDMA_AHB_COMP_PARAMS_1_NUM_HS_INT_SHIFT                       55
#define    DWDMA_AHB_COMP_PARAMS_1_NUM_HS_INT_MASK                        0xF80000000000000
#define    DWDMA_AHB_COMP_PARAMS_1_M4_HDATA_WIDTH_SHIFT                   53
#define    DWDMA_AHB_COMP_PARAMS_1_M4_HDATA_WIDTH_MASK                    0x60000000000000
#define    DWDMA_AHB_COMP_PARAMS_1_M3_HDATA_WIDTH_SHIFT                   51
#define    DWDMA_AHB_COMP_PARAMS_1_M3_HDATA_WIDTH_MASK                    0x18000000000000
#define    DWDMA_AHB_COMP_PARAMS_1_M2_HDATA_WIDTH_SHIFT                   49
#define    DWDMA_AHB_COMP_PARAMS_1_M2_HDATA_WIDTH_MASK                    0x6000000000000
#define    DWDMA_AHB_COMP_PARAMS_1_M1_HDATA_WIDTH_SHIFT                   47
#define    DWDMA_AHB_COMP_PARAMS_1_M1_HDATA_WIDTH_MASK                    0x1800000000000
#define    DWDMA_AHB_COMP_PARAMS_1_S_HDATA_WIDTH_SHIFT                    45
#define    DWDMA_AHB_COMP_PARAMS_1_S_HDATA_WIDTH_MASK                     0x600000000000
#define    DWDMA_AHB_COMP_PARAMS_1_NUM_MASTER_INT_SHIFT                   43
#define    DWDMA_AHB_COMP_PARAMS_1_NUM_MASTER_INT_MASK                    0x180000000000
#define    DWDMA_AHB_COMP_PARAMS_1_NUM_CHANNELS_SHIFT                     40
#define    DWDMA_AHB_COMP_PARAMS_1_NUM_CHANNELS_MASK                      0x70000000000
#define    DWDMA_AHB_COMP_PARAMS_1_MABRST_SHIFT                           35
#define    DWDMA_AHB_COMP_PARAMS_1_MABRST_MASK                            0x800000000
#define    DWDMA_AHB_COMP_PARAMS_1_INTR_IO_SHIFT                          33
#define    DWDMA_AHB_COMP_PARAMS_1_INTR_IO_MASK                           0x600000000
#define    DWDMA_AHB_COMP_PARAMS_1_BIG_ENDIAN_SHIFT                       32
#define    DWDMA_AHB_COMP_PARAMS_1_BIG_ENDIAN_MASK                        0x100000000
#define    DWDMA_AHB_COMP_PARAMS_1_CH3_MAX_BLK_SIZE_SHIFT                 12
#define    DWDMA_AHB_COMP_PARAMS_1_CH3_MAX_BLK_SIZE_MASK                  0x0000F000
#define    DWDMA_AHB_COMP_PARAMS_1_CH2_MAX_BLK_SIZE_SHIFT                 8
#define    DWDMA_AHB_COMP_PARAMS_1_CH2_MAX_BLK_SIZE_MASK                  0x00000F00
#define    DWDMA_AHB_COMP_PARAMS_1_CH1_MAX_BLK_SIZE_SHIFT                 4
#define    DWDMA_AHB_COMP_PARAMS_1_CH1_MAX_BLK_SIZE_MASK                  0x000000F0
#define    DWDMA_AHB_COMP_PARAMS_1_CH0_MAX_BLK_SIZE_SHIFT                 0
#define    DWDMA_AHB_COMP_PARAMS_1_CH0_MAX_BLK_SIZE_MASK                  0x0000000F

#endif /* __BRCM_RDB_DWDMA_AHB_H__ */


