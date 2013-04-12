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
