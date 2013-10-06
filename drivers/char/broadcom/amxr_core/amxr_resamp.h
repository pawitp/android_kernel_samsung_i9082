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
*  @file    amxr_resamp.h
*
*****************************************************************************/
#if !defined( AMXR_RESAMP_H )
#define AMXR_RESAMP_H

/* ---- Include Files ---------------------------------------------------- */
#include <linux/types.h>	/* Needed for int16_t */
#include <linux/broadcom/amxr.h>	/* AMXR API */

/* ---- Constants and Types ---------------------------------------------- */

/**
*  Types of supported resamplers
*/
typedef enum {
	AMXR_RESAMP_TYPE_NONE = 0,	/* No resampling needed */
	AMXR_RESAMP_TYPE_6TO1,	/* Decimate 6:1 */
	AMXR_RESAMP_TYPE_5TO1,	/* Decimate 5:1 */
	AMXR_RESAMP_TYPE_4TO1,	/* Decimate 4:1 */
	AMXR_RESAMP_TYPE_3TO1,	/* Decimate 3:1 */
	AMXR_RESAMP_TYPE_2TO1,	/* Decimate 2:1 */
	AMXR_RESAMP_TYPE_3TO2,	/* Decimate 3:2 */
	AMXR_RESAMP_TYPE_6TO5,	/* Decimate 6:5 */
	AMXR_RESAMP_TYPE_1TO2,	/* Interpolate 1:2 */
	AMXR_RESAMP_TYPE_1TO3,	/* Interpolate 1:3 */
	AMXR_RESAMP_TYPE_1TO4,	/* Interpolate 1:4 */
	AMXR_RESAMP_TYPE_1TO5,	/* Interpolate 1:5 */
	AMXR_RESAMP_TYPE_1TO6,	/* Interpolate 1:6 */
	AMXR_RESAMP_TYPE_4TO5,	/* Interpolate 4:5 */
} AMXR_RESAMP_TYPE;

/* Maximum filter length (i.e. filter order) supported */
#define AMXR_FILTENLEN_MAXSZ           400

/* Maximum history size and sample frame size (240 sample = 5ms of 48kHz)
 * in 16-bit samples */
#define AMXR_RESAMP_BUFFER_BYTES       ((AMXR_FILTENLEN_MAXSZ + 240) * sizeof(int16_t))

/**
*  Resampler look-up table entry
*/
typedef struct amxr_resample_tabentry {
	AMXR_RESAMP_TYPE rtype;	/* Resampler type */
	int filterlen;		/* Filter length */
	int decim_ratio;	/* Decimation ratio. Resampling = inter/decim */
	int inter_ratio;	/* Interpolation ratio. Resampling = inter/decim */
	const int16_t *coeffs;	/* Pointer to coefficient table */
} AMXR_RESAMPLE_TABENTRY;

/* ---- Variable Externs ------------------------------------------------- */
/* ---- Function Prototypes ---------------------------------------------- */

/***************************************************************************/
/**
*  Check whether resampler rates are feasible and return the 
*  appropriate resampler type.
*
*  @return
*     0        Success, appropriate resampler exists
*     -EINVAL  No appropriate resampler can be found
*/
int amxr_check_resample_rates(int src_hz,	/*<< (i) Source sampling freq in Hz */
			      int dst_hz,	/*<< (i) Destination sampling freq in Hz */
			      AMXR_RESAMP_TYPE * rtypep	/*<< (o) Ptr to resampler type */
    );

/***************************************************************************/
/**
*  Lookup resampler information.
*
*  @return
*     0        Success, valid resampler information returned
*     -EINVAL  No appropriate resampler can be found
*/
int amxr_get_resampler(AMXR_RESAMP_TYPE rtype,	/*<< (i) Resampler type */
		       AMXR_RESAMPLE_TABENTRY ** tablep	/*<< (o) Return pointer to resampler table */
    );

/***************************************************************************/
/**
*  Generic resampler. Provided an appropriate filter is designed, 
*  this resampler can accommodate any resampling ratio of 
*  interpfac/decimfac.
*/
void amxrCResample(int16_t *insamp,	/*<< (i) Ptr to input samples */
		   int16_t *outsamp,	/*<< (o) Ptr to output samples */
		   int16_t numsamp,	/*<< (i) Number of samples to generate */
		   const int16_t *filtcoeff,	/*<< (i) Ptr to filter coefficients */
		   int filtlen,	/*<< (i) Filter length */
		   int interpfac,	/*<< (i) Interpolation factor */
		   int decimfac	/*<< (i) Decimation factor */
    );

#endif /* AMXR_RESAMP_H */
