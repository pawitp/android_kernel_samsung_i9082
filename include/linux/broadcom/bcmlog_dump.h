/*****************************************************************************
* Copyright 2001 - 2009 Broadcom Corporation.  All rights reserved.
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

/****************************************************************************
*
*  bcmlog_dump.h
*
*  PURPOSE:
*
* This file contains functions and macros for broadcom logging and crash dump.
*
*****************************************************************************/

#if !defined(LINUX_BROADCOM_BCMLOG_DUMP_H)
#define LINUX_BROADCOM_BCMLOG_DUMP_H

/* ---- Include Files ---------------------------------------------------- */

/* ---- Constants and Types ---------------------------------------------- */

/* ---- Variable Externs ------------------------------------------------- */
extern int dump_start_ui_on;

/* ---- Function Prototypes ---------------------------------------------- */
void display_crash_dump_start_ui(const char *mode);

#endif /* LINUX_BROADCOM_DUMP_MEM_H */
