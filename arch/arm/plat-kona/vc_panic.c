/*****************************************************************************
*  Copyright 2012 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/licenses/old-license/gpl-2.0.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/

#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/module.h>
#include <linux/of_fdt.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <chal/chal_ipc.h>
#include "debug_sym.h"
#include <vc_mem.h>

#define LAST_RESORT_IPC_DOORBELL   (0xE)
/* priority higher than mach-capri crash debugger
*  to ensure it gets called before reboot invoked.
*/
#define VC_PANIC_NOTIFIER_PRIO     2

struct VC_LAST_RESORT_T {
	unsigned long command;
	unsigned long result;
};

static CHAL_IPC_HANDLE vc_ipc_hdl;
static size_t vc_mem_offset;
static uint8_t *vc_io_map_addr;

int vc_panic_flush_cache(void)
{
	struct VC_LAST_RESORT_T last_resort;

	last_resort.command = 1;

	if (vc_io_map_addr != NULL) {
		memcpy(vc_io_map_addr + vc_mem_offset,
			&last_resort,
			sizeof(last_resort));

		chal_ipc_int_vcset(vc_ipc_hdl, LAST_RESORT_IPC_DOORBELL);
		/* Make sure we give enough time for the cache flushing to
		 * terminate.
		 */
		mdelay(10);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(vc_panic_flush_cache);

static int vc_panic_handler(struct notifier_block *nb,
	unsigned long l, void *buf)
{
	/* force flush of VC cache. */
	vc_panic_flush_cache();

	return NOTIFY_DONE;
}

static struct notifier_block panic_block = {
	.notifier_call = vc_panic_handler,
	.priority = VC_PANIC_NOTIFIER_PRIO,
};

static int __init vc_panic_init(void)
{
	const char *last_resort_sym_name = "last_resort_trigger_ipc";
	int rc = 0;
	unsigned long vc_phy_addr;
	VC_MEM_ACCESS_HANDLE_T vc_handle;
	VC_MEM_ADDR_T vc_addr;
	size_t vc_size;
	size_t vc_map_size;

	/* assign the ipc handle to signal vc. */
	vc_ipc_hdl = chal_ipc_config(NULL);

	/* create the mapped region to pass the information. */
	rc = OpenVideoCoreMemory(&vc_handle);
	if (rc != 0)
		return rc;

	if (LookupVideoCoreSymbol(vc_handle,
			last_resort_sym_name, &vc_addr, &vc_size) < 0) {
		CloseVideoCoreMemory(vc_handle);
		return -ENOENT;
	}

	if (vc_size != sizeof(struct VC_LAST_RESORT_T)) {
		CloseVideoCoreMemory(vc_handle);
		return -EIO;
	}


	vc_addr = ALIAS_NORMAL(vc_addr);
	vc_mem_offset = vc_addr & ~PAGE_MASK;
	vc_phy_addr = vc_addr & PAGE_MASK;
	vc_phy_addr += mm_vc_mem_phys_addr;
	vc_map_size = (vc_mem_offset + vc_size + PAGE_SIZE - 1) & PAGE_MASK;
	vc_io_map_addr = ioremap_nocache(vc_phy_addr, vc_map_size);

	CloseVideoCoreMemory(vc_handle);

	printk(KERN_INFO "[%s]: 0x%x, %d -> 0x%x -> 0x%x, %d, 0x%x\n",
		__func__,
		vc_addr,
		vc_size,
		(unsigned int)vc_phy_addr,
		vc_io_map_addr,
		vc_map_size,
		vc_mem_offset);

	/* chain the notifier. */
	atomic_notifier_chain_register(&panic_notifier_list, &panic_block);
	return 0;
}

static void vc_panic_exit(void)
{
	if (vc_io_map_addr)
		iounmap(vc_io_map_addr);
	vc_io_map_addr = NULL;

	return;
}

late_initcall(vc_panic_init);
module_exit(vc_panic_exit);
