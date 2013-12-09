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
*  @file    amxr_vectadd.c
*
*  @brief   This file implements the C model for the vector add.
*
****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */
#include "amxr_vectadd.h"

/* ---- Private Constants and Types -------------------------------------- */
/* ---- Public Variables ------------------------------------------------- */
/* ---- Private Variables ------------------------------------------------ */
/* ---- Private Function Prototypes -------------------------------------- */
/* ---- Functions -------------------------------------------------------- */

/***************************************************************************/
/**
*  16-bit saturation
*
*  @return  saturated signed 16-bit value
*/
static int16_t saturate16(int32_t num	/*<< (i) signed 32-bit number */
    )
{
	int16_t result = (int16_t)num;
	if (num > 0x07fff) {
		result = 0x7fff;
	} else if (num < -32768) {
		result = 0x8000;
	}
	return result;
}

/***************************************************************************/
/**
*  Generic vector add with saturation
*/
void amxrCVectorAdd(int16_t *dstp,	/*<< (o) Ptr to vector sum */
		    const int16_t *src1p,	/*<< (i) Ptr to vector summand 1 */
		    const int16_t *src2p,	/*<< (i) Ptr to vector summand 2 */
		    int numsamp	/*<< (i) Number of samples to add */
    ) {
	int k;
	for (k = 0; k < numsamp; k++) {
		dstp[k] = saturate16((int32_t)src1p[k] + (int32_t)src2p[k]);
	}
}
