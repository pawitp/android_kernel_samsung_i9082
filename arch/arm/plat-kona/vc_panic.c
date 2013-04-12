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

#define LAST_RESORT_IPC_DOORBELL   (0xE)
/* priority higher than mach-capri crash debugger
*  to ensure it gets called before hand.
*/
#define VC_PANIC_NOTIFIER_PRIO     2

struct VC_LAST_RESORT_T {
	unsigned long command;
	unsigned long result;
};

static CHAL_IPC_HANDLE ipcHandle;
static VC_MEM_ACCESS_HANDLE_T vc_handle;
static VC_MEM_ADDR_T vc_addr;
static size_t vc_size;
static struct VC_LAST_RESORT_T vc_last_resort;

int vc_panic_flush_cache(void)
{
	int rc = 0;
	const char *last_resort_sym_name = "last_resort_trigger_ipc";

	ipcHandle = chal_ipc_config(NULL);
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

	vc_last_resort.command = 1; /* flush cache. */
	if (!WriteVideoCoreMemory(
			vc_handle,
			&vc_last_resort,
			vc_addr,
			sizeof(struct VC_LAST_RESORT_T))) {
		return -EIO;
	}

	chal_ipc_int_vcset(ipcHandle, LAST_RESORT_IPC_DOORBELL);
	/* Make sure we give enough time for the cache flushing to
	 * terminate.
	 */
	mdelay(2);

	/* Could read result back here... */
	CloseVideoCoreMemory(vc_handle);
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
	atomic_notifier_chain_register(&panic_notifier_list, &panic_block);
	return 0;
}

static void vc_panic_exit(void)
{
	return;
}

device_initcall(vc_panic_init);
module_exit(vc_panic_exit);
