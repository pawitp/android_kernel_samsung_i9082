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

#ifndef __BRCM_RDB_RTC_APB_H__
#define __BRCM_RDB_RTC_APB_H__

#define RTC_APB_ADDR_OFFSET                                               0x00000000
#define RTC_APB_ADDR_TYPE                                                 UInt32
#define RTC_APB_ADDR_RESERVED_MASK                                        0x00000000
#define    RTC_APB_ADDR_ADDR_UNUSED_SHIFT                                 16
#define    RTC_APB_ADDR_ADDR_UNUSED_MASK                                  0xFFFF0000
#define    RTC_APB_ADDR_APB_ADDR_SHIFT                                    0
#define    RTC_APB_ADDR_APB_ADDR_MASK                                     0x0000FFFF

#define RTC_APB_WR_DATA_OFFSET                                            0x00000004
#define RTC_APB_WR_DATA_TYPE                                              UInt32
#define RTC_APB_WR_DATA_RESERVED_MASK                                     0x00000000
#define    RTC_APB_WR_DATA_APB_WR_DATA_SHIFT                              0
#define    RTC_APB_WR_DATA_APB_WR_DATA_MASK                               0xFFFFFFFF

#define RTC_APB_RD_DATA_OFFSET                                            0x00000008
#define RTC_APB_RD_DATA_TYPE                                              UInt32
#define RTC_APB_RD_DATA_RESERVED_MASK                                     0x00000000
#define    RTC_APB_RD_DATA_APB_RD_DATA_SHIFT                              0
#define    RTC_APB_RD_DATA_APB_RD_DATA_MASK                               0xFFFFFFFF

#define RTC_APB_CTRL_STS_OFFSET                                           0x0000000C
#define RTC_APB_CTRL_STS_TYPE                                             UInt32
#define RTC_APB_CTRL_STS_RESERVED_MASK                                    0xFFFFFFF0
#define    RTC_APB_CTRL_STS_APB_CMD_DONE_INT_EN_SHIFT                     3
#define    RTC_APB_CTRL_STS_APB_CMD_DONE_INT_EN_MASK                      0x00000008
#define    RTC_APB_CTRL_STS_RTC_CMD_DONE_SHIFT                            2
#define    RTC_APB_CTRL_STS_RTC_CMD_DONE_MASK                             0x00000004
#define    RTC_APB_CTRL_STS_RTC_CMD_START_SHIFT                           1
#define    RTC_APB_CTRL_STS_RTC_CMD_START_MASK                            0x00000002
#define    RTC_APB_CTRL_STS_RTC_APB_CMD_SHIFT                             0
#define    RTC_APB_CTRL_STS_RTC_APB_CMD_MASK                              0x00000001

#define RTC_APB_INT_STS_OFFSET                                            0x00000010
#define RTC_APB_INT_STS_TYPE                                              UInt32
#define RTC_APB_INT_STS_RESERVED_MASK                                     0xFFFFFFF8
#define    RTC_APB_INT_STS_RTC_MATCH_INT_STS_SHIFT                        2
#define    RTC_APB_INT_STS_RTC_MATCH_INT_STS_MASK                         0x00000004
#define    RTC_APB_INT_STS_RTC_PER_INT_STS_SHIFT                          1
#define    RTC_APB_INT_STS_RTC_PER_INT_STS_MASK                           0x00000002
#define    RTC_APB_INT_STS_APB_CMD_DONE_INT_STS_SHIFT                     0
#define    RTC_APB_INT_STS_APB_CMD_DONE_INT_STS_MASK                      0x00000001

#endif /* __BRCM_RDB_RTC_APB_H__ */


