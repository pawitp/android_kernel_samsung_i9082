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
*   @file   util_lswap.h
*
*   @brief  This file prototypes the utility lswap function used by the platform.
*
*****************************************************************************/
#ifndef _UTIL_LSWAP_H_
#define _UTIL_LSWAP_H_

#ifndef __BIG_ENDIAN
__inline unsigned long lswap(unsigned long l)
{
	typedef struct {
		unsigned char byte0;
		unsigned char byte1;
		unsigned char byte2;
		unsigned char byte3;
	} little_e;

	typedef struct {
		unsigned char byte3;
		unsigned char byte2;
		unsigned char byte1;
		unsigned char byte0;
	} big_e;

	union {
		little_e le;
		unsigned long l;
	} lu;

	union {
		big_e be;
		unsigned long l;
	} bu;

	bu.l = l;
	lu.le.byte0 = bu.be.byte0;
	lu.le.byte1 = bu.be.byte1;
	lu.le.byte2 = bu.be.byte2;
	lu.le.byte3 = bu.be.byte3;
	return lu.l;
}				// lswap
#endif //__BIG_ENDIAN

#endif //_UTIL_LSWAP_H_
