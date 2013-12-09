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
*  @file    amxr_vectmul.h
*
*****************************************************************************/
#if !defined( AMXR_VECTMUL_H )
#define AMXR_VECTMUL_H

/* ---- Include Files ---------------------------------------------------- */
#include <linux/types.h>	/* Needed for int16_t */

/* ---- Constants and Types ---------------------------------------------- */
/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */

/***************************************************************************/
/**
*  Vector multiply with Q16 multiplican
*
*  dst[i] = (src[i] * q16) >> 16
*/
void amxrCVectorMpyQ16(int16_t *dstp,	/*<< (o) Ptr to output samples */
		       const int16_t *srcp,	/*<< (i) Ptr to input samples */
		       int numsamp,	/*<< (i) Number of samples to add */
		       uint16_t q16gain	/*<< (i) Q16 linear gain value to multiply with */
    );

/***************************************************************************/
/**
*  Vector multiply and accumulate with Q16 constant multiplicand
*
*  dst[i] += (src[i] * q16) >> 16
*/
void amxrCVectorMacQ16(int16_t *dstp,	/*<< (o) Ptr to output samples */
		       const int16_t *srcp,	/*<< (i) Ptr to input samples */
		       int numsamp,	/*<< (i) Number of samples to add */
		       uint16_t q16gain	/*<< (i) Q16 linear gain value to multiply with */
    );
#endif /* AMXR_VECTMUL_H */
