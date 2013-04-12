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

#ifndef __BRCM_RDB_LED_MATRIX_H__
#define __BRCM_RDB_LED_MATRIX_H__

#define LED_MATRIX_LEDM_CTL_OFFSET                                        0x00000000
#define LED_MATRIX_LEDM_CTL_TYPE                                          UInt32
#define LED_MATRIX_LEDM_CTL_RESERVED_MASK                                 0x7FC00080
#define    LED_MATRIX_LEDM_CTL_SET_SHIFT                                  31
#define    LED_MATRIX_LEDM_CTL_SET_MASK                                   0x80000000
#define    LED_MATRIX_LEDM_CTL_SCANMODE_SHIFT                             21
#define    LED_MATRIX_LEDM_CTL_SCANMODE_MASK                              0x00200000
#define    LED_MATRIX_LEDM_CTL_SCANCLK_SHIFT                              19
#define    LED_MATRIX_LEDM_CTL_SCANCLK_MASK                               0x00180000
#define    LED_MATRIX_LEDM_CTL_YONVAL_SHIFT                               18
#define    LED_MATRIX_LEDM_CTL_YONVAL_MASK                                0x00040000
#define    LED_MATRIX_LEDM_CTL_XONVAL_SHIFT                               17
#define    LED_MATRIX_LEDM_CTL_XONVAL_MASK                                0x00020000
#define    LED_MATRIX_LEDM_CTL_EN_SHIFT                                   16
#define    LED_MATRIX_LEDM_CTL_EN_MASK                                    0x00010000
#define    LED_MATRIX_LEDM_CTL_RESOLUTION_SHIFT                           8
#define    LED_MATRIX_LEDM_CTL_RESOLUTION_MASK                            0x0000FF00
#define    LED_MATRIX_LEDM_CTL_ROW_CNT_SHIFT                              4
#define    LED_MATRIX_LEDM_CTL_ROW_CNT_MASK                               0x00000070
#define    LED_MATRIX_LEDM_CTL_COL_CNT_SHIFT                              0
#define    LED_MATRIX_LEDM_CTL_COL_CNT_MASK                               0x0000000F

#define LED_MATRIX_LEDM_ON_OFFSET                                         0x00000004
#define LED_MATRIX_LEDM_ON_TYPE                                           UInt32
#define LED_MATRIX_LEDM_ON_RESERVED_MASK                                  0xFFFE0000
#define    LED_MATRIX_LEDM_ON_ON_SHIFT                                    0
#define    LED_MATRIX_LEDM_ON_ON_MASK                                     0x0001FFFF

#define LED_MATRIX_LEDM_GUARD_OFFSET                                      0x00000008
#define LED_MATRIX_LEDM_GUARD_TYPE                                        UInt32
#define LED_MATRIX_LEDM_GUARD_RESERVED_MASK                               0xFFFE0000
#define    LED_MATRIX_LEDM_GUARD_GUARD_SHIFT                              0
#define    LED_MATRIX_LEDM_GUARD_GUARD_MASK                               0x0001FFFF

#define LED_MATRIX_LEDM_CURPAT_0_OFFSET                                   0x0000000C
#define LED_MATRIX_LEDM_CURPAT_0_TYPE                                     UInt32
#define LED_MATRIX_LEDM_CURPAT_0_RESERVED_MASK                            0xF000F000
#define    LED_MATRIX_LEDM_CURPAT_0_CUR_PAT1_SHIFT                        16
#define    LED_MATRIX_LEDM_CURPAT_0_CUR_PAT1_MASK                         0x0FFF0000
#define    LED_MATRIX_LEDM_CURPAT_0_CUR_PAT0_SHIFT                        0
#define    LED_MATRIX_LEDM_CURPAT_0_CUR_PAT0_MASK                         0x00000FFF

#define LED_MATRIX_LEDM_CURPAT_1_OFFSET                                   0x00000010
#define LED_MATRIX_LEDM_CURPAT_1_TYPE                                     UInt32
#define LED_MATRIX_LEDM_CURPAT_1_RESERVED_MASK                            0xF000F000
#define    LED_MATRIX_LEDM_CURPAT_1_CUR_PAT3_SHIFT                        16
#define    LED_MATRIX_LEDM_CURPAT_1_CUR_PAT3_MASK                         0x0FFF0000
#define    LED_MATRIX_LEDM_CURPAT_1_CUR_PAT2_SHIFT                        0
#define    LED_MATRIX_LEDM_CURPAT_1_CUR_PAT2_MASK                         0x00000FFF

#define LED_MATRIX_LEDM_CURPAT_2_OFFSET                                   0x00000014
#define LED_MATRIX_LEDM_CURPAT_2_TYPE                                     UInt32
#define LED_MATRIX_LEDM_CURPAT_2_RESERVED_MASK                            0xF000F000
#define    LED_MATRIX_LEDM_CURPAT_2_CUR_PAT5_SHIFT                        16
#define    LED_MATRIX_LEDM_CURPAT_2_CUR_PAT5_MASK                         0x0FFF0000
#define    LED_MATRIX_LEDM_CURPAT_2_CUR_PAT4_SHIFT                        0
#define    LED_MATRIX_LEDM_CURPAT_2_CUR_PAT4_MASK                         0x00000FFF

#define LED_MATRIX_LEDM_NEXTPAT_0_OFFSET                                  0x00000018
#define LED_MATRIX_LEDM_NEXTPAT_0_TYPE                                    UInt32
#define LED_MATRIX_LEDM_NEXTPAT_0_RESERVED_MASK                           0xF000F000
#define    LED_MATRIX_LEDM_NEXTPAT_0_NEXT_PAT1_SHIFT                      16
#define    LED_MATRIX_LEDM_NEXTPAT_0_NEXT_PAT1_MASK                       0x0FFF0000
#define    LED_MATRIX_LEDM_NEXTPAT_0_NEXT_PAT0_SHIFT                      0
#define    LED_MATRIX_LEDM_NEXTPAT_0_NEXT_PAT0_MASK                       0x00000FFF

#define LED_MATRIX_LEDM_NEXTPAT_1_OFFSET                                  0x0000001C
#define LED_MATRIX_LEDM_NEXTPAT_1_TYPE                                    UInt32
#define LED_MATRIX_LEDM_NEXTPAT_1_RESERVED_MASK                           0xF000F000
#define    LED_MATRIX_LEDM_NEXTPAT_1_NEXT_PAT3_SHIFT                      16
#define    LED_MATRIX_LEDM_NEXTPAT_1_NEXT_PAT3_MASK                       0x0FFF0000
#define    LED_MATRIX_LEDM_NEXTPAT_1_NEXT_PAT2_SHIFT                      0
#define    LED_MATRIX_LEDM_NEXTPAT_1_NEXT_PAT2_MASK                       0x00000FFF

#define LED_MATRIX_LEDM_NEXTPAT_2_OFFSET                                  0x00000020
#define LED_MATRIX_LEDM_NEXTPAT_2_TYPE                                    UInt32
#define LED_MATRIX_LEDM_NEXTPAT_2_RESERVED_MASK                           0xF000F000
#define    LED_MATRIX_LEDM_NEXTPAT_2_NEXT_PAT5_SHIFT                      16
#define    LED_MATRIX_LEDM_NEXTPAT_2_NEXT_PAT5_MASK                       0x0FFF0000
#define    LED_MATRIX_LEDM_NEXTPAT_2_NEXT_PAT4_SHIFT                      0
#define    LED_MATRIX_LEDM_NEXTPAT_2_NEXT_PAT4_MASK                       0x00000FFF

#endif /* __BRCM_RDB_LED_MATRIX_H__ */


