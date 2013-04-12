/*****************************************************************************
*  Copyright 2002 - 2012 Broadcom Corporation.  All rights reserved.
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
/*****************************************************************************
 * $brcm_Workfile: $
 * $brcm_Revision: $
 * $brcm_Date: $
 *
 * Module Description:
 *
 * Revision History:
 *
 * $brcm_Log: $
 ***************************************************************************/
#ifndef BCM_REG_MEM__
#define BCM_REG_MEM__

#ifdef __cplusplus
extern "C" {
#endif

/* handle is not used for now */
#define BCM_REG_WRITE64( handle, addr, wdata ) (*((volatile unsigned long long *)(addr)) = (wdata))
#define BCM_REG_WRITE32( handle, addr, wdata ) (*((volatile unsigned long      *)(addr)) = (wdata))
#define BCM_REG_WRITE16( handle, addr, wdata ) (*((volatile unsigned short     *)(addr)) = (wdata))
#define BCM_REG_WRITE8( handle, addr, wdata ) (*((volatile unsigned char      *)(addr)) = (wdata))
#define BCM_REG_READ64( handle, addr )         (*((volatile unsigned long long * )(addr)))
#define BCM_REG_READ32( handle, addr )         (*((volatile unsigned long      * )(addr)))
#define BCM_REG_READ16( handle, addr )         (*((volatile unsigned short     * )(addr)))
#define BCM_REG_READ8( handle, addr )         (*((volatile unsigned char      * )(addr)))

#ifdef __cplusplus
}
#endif
#endif				/* BCM_REG_MEM__ */
