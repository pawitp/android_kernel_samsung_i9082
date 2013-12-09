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
*  @file    amxr_vectmul.c
*
*  @brief   This file implements the C model for the vector multiply 
*           operations.
*
****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */
#include "amxr_vectmul.h"

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
*  Vector multiply with Q16 multiplican
*
*  dst[i] = (src[i] * q16) >> 16
*/
void amxrCVectorMpyQ16(int16_t *dstp,	/*<< (o) Ptr to output samples */
		       const int16_t *srcp,	/*<< (i) Ptr to input samples */
		       int numsamp,	/*<< (i) Number of samples to add */
		       uint16_t q16gain	/*<< (i) Q16 linear gain value to multiply with */
    ) {
	int k;
	for (k = 0; k < numsamp; k++) {
		dstp[k] =
		    (int32_t)(((int32_t)(srcp[k]) * (int32_t)q16gain) >> 16);
	}
}

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
    ) {
	int k;
	int16_t a;
	for (k = 0; k < numsamp; k++) {
		a = (int32_t)(((int32_t)(srcp[k]) * (int32_t)q16gain) >> 16);
		dstp[k] = saturate16((int32_t)dstp[k] + (int32_t)a);
	}
}
