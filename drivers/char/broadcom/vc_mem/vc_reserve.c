/*****************************************************************************
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <asm/cacheflush.h>

#include <linux/broadcom/vc_dt.h>
#include <linux/broadcom/vc_mem.h>

static struct resource vc_lowmem_res;
static uint32_t vc_lowmem_phys;
static uint32_t vc_lowmem_reserve_size = SZ_4K;

int __init vc_lowmem_reserve(void)
{
	uint32_t base, load, size, safety;
	if (vc_dt_get_mem_config(&base, &load, &size) < 0) {
		printk(KERN_ERR "vc-lowmem: VC memory config not found\n");
		return -EINVAL;
	}
	base = (base & 0xC0000000);

	safety = (uint32_t)virt_to_phys((void *)PAGE_OFFSET);
	if (base < safety) {
		printk(KERN_INFO
			"vc-lowmem: safety 0x%x, base 0x%x - no reservation\n",
			safety, base);
		vc_lowmem_phys = 0;
	} else {
		vc_lowmem_res.name = "vc-lowmem region";
		vc_lowmem_res.flags = IORESOURCE_MEM;
		vc_lowmem_res.start = base;
		vc_lowmem_res.end = base + vc_lowmem_reserve_size - 1;
		if (request_resource(&iomem_resource, &vc_lowmem_res)) {
			printk(KERN_ERR "vc-lowmem: could not reserve 0x%x-0x%x\n",
				base, base+vc_lowmem_reserve_size);
			return -EBUSY;
		}

		printk(KERN_INFO "vc-lowmem: reserved 0x%x-0x%x, safety 0x%x\n",
				base, base+vc_lowmem_reserve_size, safety);
		vc_lowmem_phys = base;
	}
	return 0;
}

static int __init vc_lowmem_init(void)
{
	int rc = -EINVAL;
	int fill_words = vc_lowmem_reserve_size / sizeof(uint32_t);

	if (vc_lowmem_phys) {
		uint32_t *ptr = phys_to_virt(vc_lowmem_phys);
		if (ptr) {
			int i;
			for (i = 0; i < fill_words; i++)
				writel(0xdeadbeef, ptr+i);

			dmac_flush_range(ptr, ptr+fill_words);

			outer_flush_range(
				vc_lowmem_phys,
				vc_lowmem_phys+vc_lowmem_reserve_size);

			printk(KERN_INFO "vc-lowmem: filled 0x%x-0x%x\n",
				vc_lowmem_phys,
				vc_lowmem_phys+vc_lowmem_reserve_size);
		}

		rc = 0;
	}
	return rc;
}


module_init(vc_lowmem_init);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom Corporation");
