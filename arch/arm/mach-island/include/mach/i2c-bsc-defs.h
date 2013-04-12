/*****************************************************************************
* Copyright 2011 Broadcom Corporation.  All rights reserved.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a
* license other than the GPL, without Broadcom's express prior written
* consent.
*****************************************************************************/
#ifndef I2C_BSC_DEFS_H
#define I2C_BSC_DEFS_H

#include <mach/rdb/brcm_rdb_i2c_mm_hs.h>

/* Mnemonic mappings to resolve inconsistencies between RDB files */

#define I2C_MM_HS_TIM_PRESCALE_CMD_DIV1		I2C_MM_HS_TIM_PRESCALE_CMD_NODIV
#define I2C_MM_HS_CS_EN_CMD_RESET_BSC		I2C_MM_HS_CS_EN_CMD_RST_BSC
#define I2C_MM_HS_CS_CMD_CMD_START_RESTART	I2C_MM_HS_CS_CMD_CMD_GEN_START
#define I2C_MM_HS_CS_CMD_CMD_STOP		I2C_MM_HS_CS_CMD_CMD_GEN_STOP
#define I2C_MM_HS_CS_CMD_CMD_READ_BYTE		I2C_MM_HS_CS_CMD_CMD_RD_A_BYTE

#endif /* I2C_BSC_DEFS_H */
