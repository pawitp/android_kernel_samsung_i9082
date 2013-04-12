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

#ifndef __BRCM_RDB_RTC_CONFIG_H__
#define __BRCM_RDB_RTC_CONFIG_H__

#define RTC_CONFIG_CLK_DISABLE_OFFSET                                     0x00000000
#define RTC_CONFIG_CLK_DISABLE_TYPE                                       UInt32
#define RTC_CONFIG_CLK_DISABLE_RESERVED_MASK                              0x0000001E
#define    RTC_CONFIG_CLK_DISABLE_CLK_DISABLE_UNUSED_SHIFT                5
#define    RTC_CONFIG_CLK_DISABLE_CLK_DISABLE_UNUSED_MASK                 0xFFFFFFE0
#define    RTC_CONFIG_CLK_DISABLE_RTC_CLK_DIS_SHIFT                       0
#define    RTC_CONFIG_CLK_DISABLE_RTC_CLK_DIS_MASK                        0x00000001

#define RTC_CONFIG_SOFT_RST_OFFSET                                        0x00000004
#define RTC_CONFIG_SOFT_RST_TYPE                                          UInt32
#define RTC_CONFIG_SOFT_RST_RESERVED_MASK                                 0xFFFFFFFE
#define    RTC_CONFIG_SOFT_RST_RTC_SOFT_RST_SHIFT                         0
#define    RTC_CONFIG_SOFT_RST_RTC_SOFT_RST_MASK                          0x00000001

#define RTC_CONFIG_RTC_GP_REG1_OFFSET                                     0x00000008
#define RTC_CONFIG_RTC_GP_REG1_TYPE                                       UInt32
#define RTC_CONFIG_RTC_GP_REG1_RESERVED_MASK                              0x00000000
#define    RTC_CONFIG_RTC_GP_REG1_RTC_GP_REG1_DATA_SHIFT                  0
#define    RTC_CONFIG_RTC_GP_REG1_RTC_GP_REG1_DATA_MASK                   0xFFFFFFFF

#define RTC_CONFIG_RTC_GP_REG2_OFFSET                                     0x0000000C
#define RTC_CONFIG_RTC_GP_REG2_TYPE                                       UInt32
#define RTC_CONFIG_RTC_GP_REG2_RESERVED_MASK                              0x00000000
#define    RTC_CONFIG_RTC_GP_REG2_RTC_GP_REG2_DATA_SHIFT                  0
#define    RTC_CONFIG_RTC_GP_REG2_RTC_GP_REG2_DATA_MASK                   0xFFFFFFFF

#define RTC_CONFIG_RTC_GP_REG3_OFFSET                                     0x00000010
#define RTC_CONFIG_RTC_GP_REG3_TYPE                                       UInt32
#define RTC_CONFIG_RTC_GP_REG3_RESERVED_MASK                              0x00000000
#define    RTC_CONFIG_RTC_GP_REG3_RTC_GP_REG3_DATA_SHIFT                  0
#define    RTC_CONFIG_RTC_GP_REG3_RTC_GP_REG3_DATA_MASK                   0xFFFFFFFF

#define RTC_CONFIG_RTC_GP_REG4_OFFSET                                     0x00000014
#define RTC_CONFIG_RTC_GP_REG4_TYPE                                       UInt32
#define RTC_CONFIG_RTC_GP_REG4_RESERVED_MASK                              0x00000000
#define    RTC_CONFIG_RTC_GP_REG4_RTC_GP_REG4_DATA_SHIFT                  0
#define    RTC_CONFIG_RTC_GP_REG4_RTC_GP_REG4_DATA_MASK                   0xFFFFFFFF

#endif /* __BRCM_RDB_RTC_CONFIG_H__ */


