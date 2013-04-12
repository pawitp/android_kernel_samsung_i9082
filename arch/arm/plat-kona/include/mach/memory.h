/*****************************************************************************
*
* Copyright 2012 Broadcom Corporation.  All rights reserved.
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
#ifndef __PLAT_KONA_MEMORY_H
#define __PLAT_KONA_MEMORY_H

#include <mach/io.h>

/* APB5, APB9 and SRAM */
#define IO_APB_G1_PHYS        0x34000000
#define IO_APB_G1_VIRT        IOMEM(0xFD800000)
#define IO_APB_G1_SIZE        (SZ_512K)

/* APB6, pwr mgr and APB10 */
#define IO_APB_G2_PHYS        0x35000000
#define IO_APB_G2_VIRT        IOMEM(0xFD880000)
#define IO_APB_G2_SIZE        (SZ_256K)

/* APB14, ESUB and ESW */
#define IO_APB_G3_PHYS        0x38000000
#define IO_APB_G3_VIRT        IOMEM(0xFD900000)
#define IO_APB_G3_SIZE        (SZ_4M)

/* APB13, APB15 and WCDMA */
#define IO_APB_G4_PHYS        0x3A000000
#define IO_APB_G4_VIRT        IOMEM(0xFDD00000)
#define IO_APB_G4_SIZE        (SZ_2M)

/* ARMDSP */
#define IO_APB_G5_PHYS        0x3B400000
#define IO_APB_G5_VIRT        IOMEM(0xFDF00000)
#define IO_APB_G5_SIZE        (SZ_4M)

/* APB1, APB2, APB7, APB1 and AXI */
#define IO_APB_G6_PHYS        0x3E000000
#define IO_APB_G6_VIRT        IOMEM(0xFE300000)
#define IO_APB_G6_SIZE        (SZ_8M)

/* APB4, APB8 and AHB */
#define IO_APB_G7_PHYS        0x3F000000
#define IO_APB_G7_VIRT        IOMEM(0xFEB00000)
#define IO_APB_G7_SIZE        (SZ_2M)

/* APB0, APB11 and A9 */
#define IO_APB_G8_PHYS        0x3FE00000
#define IO_APB_G8_VIRT        IOMEM(0xFED00000)
#define IO_APB_G8_SIZE        (SZ_2M)

#define IO_START_PA        IO_APB_G1_PHYS
#define IO_START_VA        IO_APB_G1_VIRT

#define IO_TO_VIRT_BETWEEN(p, st, sz)   (((p) >= (st)) && ((p) < ((st) + (sz))))
#define IO_TO_VIRT_XLATE(p, pst, vst)   ((p) - (pst) + (vst))
#define IO_TO_VIRT(n) ( \
	IO_TO_VIRT_BETWEEN((n), IO_APB_G1_PHYS, IO_APB_G1_SIZE) ?           \
		IO_TO_VIRT_XLATE((n), IO_APB_G1_PHYS, IO_APB_G1_VIRT) :     \
	IO_TO_VIRT_BETWEEN((n), IO_APB_G2_PHYS, IO_APB_G2_SIZE) ?           \
	       IO_TO_VIRT_XLATE((n), IO_APB_G2_PHYS, IO_APB_G2_VIRT) :      \
	IO_TO_VIRT_BETWEEN((n), IO_APB_G3_PHYS, IO_APB_G3_SIZE) ?           \
	       IO_TO_VIRT_XLATE((n), IO_APB_G3_PHYS, IO_APB_G3_VIRT) :      \
	IO_TO_VIRT_BETWEEN((n), IO_APB_G4_PHYS, IO_APB_G4_SIZE) ?           \
	       IO_TO_VIRT_XLATE((n), IO_APB_G4_PHYS, IO_APB_G4_VIRT) :      \
	IO_TO_VIRT_BETWEEN((n), IO_APB_G5_PHYS, IO_APB_G5_SIZE) ?           \
	       IO_TO_VIRT_XLATE((n), IO_APB_G5_PHYS, IO_APB_G5_VIRT) :      \
	IO_TO_VIRT_BETWEEN((n), IO_APB_G6_PHYS, IO_APB_G6_SIZE) ?           \
	       IO_TO_VIRT_XLATE((n), IO_APB_G6_PHYS, IO_APB_G6_VIRT) :      \
	IO_TO_VIRT_BETWEEN((n), IO_APB_G7_PHYS, IO_APB_G7_SIZE) ?           \
	       IO_TO_VIRT_XLATE((n), IO_APB_G7_PHYS, IO_APB_G7_VIRT) :      \
	IO_TO_VIRT_BETWEEN((n), IO_APB_G8_PHYS, IO_APB_G8_SIZE) ?           \
	       IO_TO_VIRT_XLATE((n), IO_APB_G8_PHYS, IO_APB_G8_VIRT) :      \
	NULL)

#define HW_IO_PHYS_TO_VIRT(phys) (IO_TO_VIRT(phys))

#define CONSISTENT_DMA_SIZE SZ_4M

#endif /* __PLAT_KONA_MEMORY_H */
