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
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/ioport.h>

#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/mach/map.h>
#include <asm/pgalloc.h>

#include <mach/aram_layout.h>
#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/io_map.h>
#include "mach.h"

#if defined(CONFIG_SEC_DEBUG)
#include <mach/sec_debug.h>
#endif

static struct map_desc io_desc[] __initdata = {
	{
	 .virtual = (unsigned long)IO_APB_G1_VIRT,
	 .pfn = __phys_to_pfn(IO_APB_G1_PHYS),
	 .length = IO_APB_G1_SIZE,
	 .type = MT_DEVICE,
	 },
	{
	 .virtual = (unsigned long)IO_APB_G2_VIRT,
	 .pfn = __phys_to_pfn(IO_APB_G2_PHYS),
	 .length = IO_APB_G2_SIZE,
	 .type = MT_DEVICE,
	 },
	{
	 .virtual = (unsigned long)IO_APB_G3_VIRT,
	 .pfn = __phys_to_pfn(IO_APB_G3_PHYS),
	 .length = IO_APB_G3_SIZE,
	 .type = MT_DEVICE,
	 },
	{
	 .virtual = (unsigned long)IO_APB_G4_VIRT,
	 .pfn = __phys_to_pfn(IO_APB_G4_PHYS),
	 .length = IO_APB_G4_SIZE,
	 .type = MT_DEVICE,
	 },
	{
	 .virtual = (unsigned long)IO_APB_G5_VIRT,
	 .pfn = __phys_to_pfn(IO_APB_G5_PHYS),
	 .length = IO_APB_G5_SIZE,
	 .type = MT_DEVICE,
	 },
	{
	 .virtual = (unsigned long)IO_APB_G6_VIRT,
	 .pfn = __phys_to_pfn(IO_APB_G6_PHYS),
	 .length = IO_APB_G6_SIZE,
	 .type = MT_DEVICE,
	 },
	{
	 .virtual = (unsigned long)IO_APB_G7_VIRT,
	 .pfn = __phys_to_pfn(IO_APB_G7_PHYS),
	 .length = IO_APB_G7_SIZE,
	 .type = MT_DEVICE,
	 },
	{
	 .virtual = (unsigned long)IO_APB_G8_VIRT,
	 .pfn = __phys_to_pfn(IO_APB_G8_PHYS),
	 .length = IO_APB_G8_SIZE,
	 .type = MT_DEVICE,
	 },
	{
	/* CAUTION: Please do NOT remove this section.
	 *          It's needed by DDR3 code to run off SRAM
	 *
	 * Length allocated: 80K.
	 * Warning: Do not unlock all region to be
	 * executable for Security resaon.
	 */
	 .virtual = (unsigned long)KONA_SRAM_VA,
	 .pfn = __phys_to_pfn(INT_SRAM_BASE),
	 .length = (SZ_64K + SZ_16K),
	 .type = MT_MEMORY_NONCACHED,
	},
};

void __init capri_map_io(void)
{
	iotable_init(io_desc, ARRAY_SIZE(io_desc));
#if defined(CONFIG_SEC_DEBUG)
	sec_debug_init();
#endif
}
