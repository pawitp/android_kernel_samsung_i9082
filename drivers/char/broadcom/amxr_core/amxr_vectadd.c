/******************************************************************************
* Copyright 2006 - 2009 Broadcom Corporation.  All rights reserved.
 *   This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2, as published by
 *  the Free Software Foundation (the "GPL").
*
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
*
 *  A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
 *  writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
******************************************************************************/

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
