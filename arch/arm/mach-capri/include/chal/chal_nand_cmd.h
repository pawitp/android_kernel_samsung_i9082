/*****************************************************************************
*  Copyright 2007 - 2012 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/licenses/old-license/gpl-2.0.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/
/**
 * @file    chal_nand_cmd.h
 * @brief   Generic NAND commands.
 */

#ifndef __CHAL_NAND_CMD_H__
#define __CHAL_NAND_CMD_H__

/* NAND commands */
#define NAND_CMD_READ_1ST        (0x00)
#define NAND_CMD_READ_2ND        (0x30)
#define NAND_CMD_READ_RAND_1ST   (0x05)
#define NAND_CMD_READ_RAND_2ND   (0xE0)
#define NAND_CMD_READCP_2ND      (0x35)
#define NAND_CMD_ID              (0x90)
#define NAND_CMD_RESET           (0xFF)

#define NAND_CMD_READ_PARAM      (0xEC)

#define NAND_CMD_STATUS          (0x70)
#define NAND_CMD_EDC_STATUS      (0x7B)
#define NAND_CMD_STATUS_1        (0xF1)
#define NAND_CMD_STATUS_2        (0xF2)

#define NAND_CMD_BERASE_1ST      (0x60)
#define NAND_CMD_BERASE_2ND      (0xD0)

#define NAND_CMD_PROG_1ST        (0x80)
#define NAND_CMD_PROG_2ND        (0x10)
#define NAND_CMD_PROG_RAND       (0x85)

/* NAND status result masks */
#define NAND_STATUS_FAIL         (0x01)	/* command failed */
#define NAND_STATUS_READY        (0x40)	/* chip is ready */
#define NAND_STATUS_NPRO         (0x80)	/* not protected */

#endif
