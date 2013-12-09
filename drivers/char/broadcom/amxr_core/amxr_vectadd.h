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

/**
*
*  @file    amxr_vectadd.h
*
*****************************************************************************/
#if !defined( AMXR_VECTADD_H )
#define AMXR_VECTADD_H

/* ---- Include Files ---------------------------------------------------- */
#include <linux/types.h>	/* Needed for int16_t */

/* ---- Constants and Types ---------------------------------------------- */
/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */

/***************************************************************************/
/**
*  Generic vector add with saturation
*/
void amxrCVectorAdd(int16_t *dstp,	/*<< (o) Ptr to vector sum */
		    const int16_t *src1p,	/*<< (i) Ptr to vector summand 1 */
		    const int16_t *src2p,	/*<< (i) Ptr to vector summand 2 */
		    int numsamp	/*<< (i) Number of samples to add */
    );

#endif /* AMXR_VECTADD_H */
