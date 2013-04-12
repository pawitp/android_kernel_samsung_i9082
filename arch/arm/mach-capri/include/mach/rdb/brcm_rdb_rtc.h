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

#ifndef __BRCM_RDB_RTC_H__
#define __BRCM_RDB_RTC_H__

#define RTC_SET_DIV_OFFSET                                                0x00000000
#define RTC_SET_DIV_TYPE                                                  UInt32
#define RTC_SET_DIV_RESERVED_MASK                                         0x00000000
#define    RTC_SET_DIV_RTC_SET_DIV_UNUSED_SHIFT                           15
#define    RTC_SET_DIV_RTC_SET_DIV_UNUSED_MASK                            0xFFFF8000
#define    RTC_SET_DIV_RTC_SET_DIV_SHIFT                                  0
#define    RTC_SET_DIV_RTC_SET_DIV_MASK                                   0x00007FFF

#define RTC_SEC_0_OFFSET                                                  0x00000004
#define RTC_SEC_0_TYPE                                                    UInt32
#define RTC_SEC_0_RESERVED_MASK                                           0x00000000
#define    RTC_SEC_0_RTC_SEC_0_SHIFT                                      0
#define    RTC_SEC_0_RTC_SEC_0_MASK                                       0xFFFFFFFF

#define RTC_CTRL_OFFSET                                                   0x00000008
#define RTC_CTRL_TYPE                                                     UInt32
#define RTC_CTRL_RESERVED_MASK                                            0x00000000
#define    RTC_CTRL_RTC_STOP_UNUSED_SHIFT                                 2
#define    RTC_CTRL_RTC_STOP_UNUSED_MASK                                  0xFFFFFFFC
#define    RTC_CTRL_RTC_LOCK_SHIFT                                        1
#define    RTC_CTRL_RTC_LOCK_MASK                                         0x00000002
#define    RTC_CTRL_RTC_STOP_SHIFT                                        0
#define    RTC_CTRL_RTC_STOP_MASK                                         0x00000001

#define RTC_PER_OFFSET                                                    0x0000000C
#define RTC_PER_TYPE                                                      UInt32
#define RTC_PER_RESERVED_MASK                                             0x00000000
#define    RTC_PER_RTC_PER_UNUSED_SHIFT                                   12
#define    RTC_PER_RTC_PER_UNUSED_MASK                                    0xFFFFF000
#define    RTC_PER_RTC_RTC_PER_SHIFT                                      0
#define    RTC_PER_RTC_RTC_PER_MASK                                       0x00000FFF

#define RTC_MATCH_OFFSET                                                  0x00000010
#define RTC_MATCH_TYPE                                                    UInt32
#define RTC_MATCH_RESERVED_MASK                                           0x00000000
#define    RTC_MATCH_RTC_MATCH_UNUSED_SHIFT                               16
#define    RTC_MATCH_RTC_MATCH_UNUSED_MASK                                0xFFFF0000
#define    RTC_MATCH_RTC_MATCH_SHIFT                                      0
#define    RTC_MATCH_RTC_MATCH_MASK                                       0x0000FFFF

#define RTC_CLR_INT_OFFSET                                                0x00000014
#define RTC_CLR_INT_TYPE                                                  UInt32
#define RTC_CLR_INT_RESERVED_MASK                                         0x00000000
#define    RTC_CLR_INT_RTC_CLR_INT_UNUSED_SHIFT                           2
#define    RTC_CLR_INT_RTC_CLR_INT_UNUSED_MASK                            0xFFFFFFFC
#define    RTC_CLR_INT_RTC_CLR_INT_MATCH_SHIFT                            1
#define    RTC_CLR_INT_RTC_CLR_INT_MATCH_MASK                             0x00000002
#define    RTC_CLR_INT_RTC_CLR_INT_PER_SHIFT                              0
#define    RTC_CLR_INT_RTC_CLR_INT_PER_MASK                               0x00000001

#define RTC_INT_STS_OFFSET                                                0x00000018
#define RTC_INT_STS_TYPE                                                  UInt32
#define RTC_INT_STS_RESERVED_MASK                                         0x00000000
#define    RTC_INT_STS_RTC_INT_MASK_UNUSED_SHIFT                          2
#define    RTC_INT_STS_RTC_INT_MASK_UNUSED_MASK                           0xFFFFFFFC
#define    RTC_INT_STS_RTC_INT_MATCH_SHIFT                                1
#define    RTC_INT_STS_RTC_INT_MATCH_MASK                                 0x00000002
#define    RTC_INT_STS_RTC_INT_PER_SHIFT                                  0
#define    RTC_INT_STS_RTC_INT_PER_MASK                                   0x00000001

#define RTC_INT_ENABLE_OFFSET                                             0x0000001C
#define RTC_INT_ENABLE_TYPE                                               UInt32
#define RTC_INT_ENABLE_RESERVED_MASK                                      0x00000000
#define    RTC_INT_ENABLE_RTC_INT_ENABLE_UNUSED_SHIFT                     2
#define    RTC_INT_ENABLE_RTC_INT_ENABLE_UNUSED_MASK                      0xFFFFFFFC
#define    RTC_INT_ENABLE_RTC_MATCH_INTR_ENABLE_SHIFT                     1
#define    RTC_INT_ENABLE_RTC_MATCH_INTR_ENABLE_MASK                      0x00000002
#define    RTC_INT_ENABLE_RTC_PER_INTR_ENABLE_SHIFT                       0
#define    RTC_INT_ENABLE_RTC_PER_INTR_ENABLE_MASK                        0x00000001

#define RTC_RESET_ACCESS_OFFSET                                           0x00000020
#define RTC_RESET_ACCESS_TYPE                                             UInt32
#define RTC_RESET_ACCESS_RESERVED_MASK                                    0x00000000
#define    RTC_RESET_ACCESS_RTC_RESET_ACCESS_STATUS_SHIFT                 0
#define    RTC_RESET_ACCESS_RTC_RESET_ACCESS_STATUS_MASK                  0xFFFFFFFF

#define RTC_MTC_LSB_OFFSET                                                0x00000024
#define RTC_MTC_LSB_TYPE                                                  UInt32
#define RTC_MTC_LSB_RESERVED_MASK                                         0x00000000
#define    RTC_MTC_LSB_RTC_MT_CTR_LSB_SHIFT                               0
#define    RTC_MTC_LSB_RTC_MT_CTR_LSB_MASK                                0xFFFFFFFF

#define RTC_MTC_MSB_OFFSET                                                0x00000028
#define RTC_MTC_MSB_TYPE                                                  UInt32
#define RTC_MTC_MSB_RESERVED_MASK                                         0x00000000
#define    RTC_MTC_MSB_RTC_MT_CTR_MSB_SHIFT                               0
#define    RTC_MTC_MSB_RTC_MT_CTR_MSB_MASK                                0xFFFFFFFF

#define RTC_MTC_CTRL_OFFSET                                               0x0000002C
#define RTC_MTC_CTRL_TYPE                                                 UInt32
#define RTC_MTC_CTRL_RESERVED_MASK                                        0x00000000
#define    RTC_MTC_CTRL_RTC_MT_CTR_INCR_AMT_UNUSED_SHIFT                  24
#define    RTC_MTC_CTRL_RTC_MT_CTR_INCR_AMT_UNUSED_MASK                   0xFF000000
#define    RTC_MTC_CTRL_RTC_MT_CTR_INCR_AMT_SHIFT                         8
#define    RTC_MTC_CTRL_RTC_MT_CTR_INCR_AMT_MASK                          0x00FFFF00
#define    RTC_MTC_CTRL_RTC_MT_CTR_UNUSED_SHIFT                           2
#define    RTC_MTC_CTRL_RTC_MT_CTR_UNUSED_MASK                            0x000000FC
#define    RTC_MTC_CTRL_RTC_MT_CTR_INCR_SHIFT                             1
#define    RTC_MTC_CTRL_RTC_MT_CTR_INCR_MASK                              0x00000002
#define    RTC_MTC_CTRL_RTC_MT_CTR_LOCK_SHIFT                             0
#define    RTC_MTC_CTRL_RTC_MT_CTR_LOCK_MASK                              0x00000001

#endif /* __BRCM_RDB_RTC_H__ */


