/*
 * Copyright (c) 2010-2011 Broadcom Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/bug.h>
#include <linux/dma-mapping.h>
#include <linux/broadcom/vc_dt.h>
#include <mach/dma_mmap.h>

#ifdef CONFIG_DMAC_PL330
#include <mach/dma.h>
#define MAX_DESCRIPTORS_PER_TRANSFER 400
#elif defined(CONFIG_MAP_SDMA)
#include <mach/sdma.h>
#else
#error "No supported DMA driver configured"
#endif

#include <vc_mem.h>

#include "vchiq_arm.h"
#include "vchiq_kona_arm.h"
#include "vchiq_bi.h"

#include <linux/pm_qos_params.h>

static int use_memcpy;
module_param(use_memcpy, bool, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(use_memcpy,
	"Force use of memcpy rather than DMA for all transfers");

static DMA_MMAP_CFG_T    gVchiqDmaMmap;
static struct completion gDmaDone;
static struct mutex      g_dma_mutex;
static struct pm_qos_request_list g_dma_qos_request;

#define DMA_QOS_VAL 100

static int
ipc_dma(void *vcaddr, void *armaddr, int len, DMA_MMAP_PAGELIST_T *pagelist,
	enum dma_data_direction dir);

int __init
vchiq_platform_init(VCHIQ_STATE_T *state)
{
	mutex_init(&g_dma_mutex);

	return vchiq_kona_init(state);
}

void __exit
vchiq_platform_exit(VCHIQ_STATE_T *state)
{
	vchiq_kona_exit(state);
}

VCHIQ_STATUS_T
vchiq_prepare_bulk_data(VCHIQ_BULK_T *bulk, VCHI_MEM_HANDLE_T memhandle,
	void *offset, int size, int dir)
{
	WARN_ON(memhandle != VCHI_MEM_HANDLE_INVALID);

	/* Check the memory is supported by dma_mmap */
	if (!dma_mmap_dma_is_supported(offset)) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: buffer at %lx not supported",
			__func__, (unsigned long)offset);
		return VCHIQ_ERROR;
	}

	if (dma_mmap_mem_type(offset) == DMA_MMAP_TYPE_USER) {
		DMA_MMAP_PAGELIST_T *pagelist;
		int ret;
		ret = dma_mmap_create_pagelist(
			(char __user *)offset,
			size,
			(dir == VCHIQ_BULK_RECEIVE)
			? DMA_FROM_DEVICE
			: DMA_TO_DEVICE,
			current,
			&pagelist);
		if (ret == 0)
			/*
			 * pagelist was allocated,
			 * but no pages were actually locked.
			 * Free pagelist before returning error.
			 */
			dma_mmap_free_pagelist(pagelist);
		if (ret <= 0)
			return VCHIQ_ERROR;
		bulk->data = offset;
		bulk->handle = (VCHI_MEM_HANDLE_T)pagelist;
	} else {
		bulk->data = offset;
		bulk->handle = 0;
	}

	return VCHIQ_SUCCESS;
}

void
vchiq_complete_bulk(VCHIQ_BULK_T *bulk)
{
	if (bulk->handle != 0)
		dma_mmap_free_pagelist((DMA_MMAP_PAGELIST_T *)bulk->handle);
}

void
vchiq_transfer_bulk(VCHIQ_BULK_T *bulk)
{
	if ((bulk->size == bulk->remote_size) &&
		(ipc_dma(bulk->remote_data, bulk->data, bulk->size,
			(DMA_MMAP_PAGELIST_T *)bulk->handle,
			(bulk->dir == VCHIQ_BULK_TRANSMIT) ? DMA_TO_DEVICE :
				 DMA_FROM_DEVICE) == 0))
		bulk->actual = bulk->size;
	else
		bulk->actual = VCHIQ_BULK_ACTUAL_ABORTED;
}

void
vchiq_dump_platform_state(void *dump_context)
{
	char buf[80];
	int len;
	len = snprintf(buf, sizeof(buf),
		"  Platform: BI (ARM master)");
	vchiq_dump(dump_context, buf, len + 1);
}

/****************************************************************************
*
*   vchiq_platform_deferred_init
*
***************************************************************************/

VCHIQ_STATUS_T vchiq_platform_deferred_init(VCHIQ_STATE_T *state,
	VCHIQ_SLOT_ZERO_T *slot_zero)
{
	VCHIQ_STATUS_T status;
	int bulk_transfer_size_bytes;

	/* Initialize the local state. Note that vc04 has already started by now
	** so the slot memory is expected to be initialised. */
	status = vchiq_init_state(state, slot_zero, 1/* master */);

	if (status != VCHIQ_SUCCESS) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: vchiq_init_state failed",
			__func__);
		goto failed_init_state;
	}

	/* initialize dma_mmap for use */
	dma_mmap_init_map(&gVchiqDmaMmap);

	/* Preallocate space to manage bulk transfers, if needed */
	bulk_transfer_size_bytes = vc_dt_get_vchiq_bulk_xfer_size();
	if (bulk_transfer_size_bytes > 0)
		dma_mmap_preallocate(&gVchiqDmaMmap, 1,
			bulk_transfer_size_bytes);

failed_init_state:
	return status;
}

/****************************************************************************
*
*   Function which translates a dma direction into a printable string.
*
***************************************************************************/

static inline const char *dma_data_direction_as_str(enum dma_data_direction dir)
{
	 switch (dir) {
	 case DMA_BIDIRECTIONAL:     return "BIDIRECTIONAL";
	 case DMA_TO_DEVICE:         return "TO_DEVICE";
	 case DMA_FROM_DEVICE:       return "FROM_DEVICE";
	 case DMA_NONE:              return "NONE";
	 }
	 return "???";
}

#ifdef CONFIG_DMAC_PL330
/****************************************************************************
*
*   dma_callback
*
***************************************************************************/

static void dma_callback(void *userData, enum pl330_xfer_status status)
{
	if (status == DMA_PL330_XFER_OK)
		complete((struct completion *)userData);
	else
		vchiq_log_error(vchiq_arm_log_level,
			"%s: called with status %d",
			__func__, status);
}


/****************************************************************************/
/**
*   Helper routine to calculate number of descriptors. Used by
*   build_transfer_list.
*
*   @return
*       0       Descriptors memory set successfully
*       -ENODEV Invalid channel
*/
/****************************************************************************/
struct xfer_list_state {
	unsigned int burst_size;
	unsigned int burst_data_length;
	int burst_pos;
	int other_pos;
	int burst_count;
	int other_count;
	struct dma_transfer_list *xfer_storage;
};

static int
calc_desc_cnt(
	void       *data1,
	void       *data2,
	dma_addr_t  srcAddr,
	dma_addr_t  dstAddr,
	size_t      numBytes
)
{
	struct xfer_list_state *state = (struct xfer_list_state *)data1;
	unsigned int headBytes, burstBytes, tailBytes;

	(void)data2;

	/* Capri requires that the source address is aligned to burst size, so
	** use an extra transfer to bring srcAddr into alignment.
	*/
	burstBytes = numBytes;
	headBytes = ((-(unsigned int)srcAddr) & (state->burst_size - 1));
	if (headBytes) {
		if (burstBytes <= headBytes) {
			headBytes = burstBytes;
			burstBytes = 0;
		} else
			burstBytes -= headBytes;
	}

	tailBytes = burstBytes & (state->burst_data_length - 1);
	burstBytes -= tailBytes;

	/*
	** If the destination address is not aligned to the burst size then the
	** final burst is truncated at the burst-size boundary, i.e. if
	** dstaddr % burst_size == 3 then the final 3 bytes will not be
	** transferred. One could benefit from this in some circumstances, but
	** relying on a write not happening is dangerous, so instead just fill
	** in the gap.
	*/
	if (burstBytes)
		tailBytes += (((unsigned int)dstAddr + headBytes) &
			(state->burst_size - 1));
		/* N.B. burstBytes and tailBytes can now overlap... */
	else {
		/* Combine headBytes and tailBytes if no burst data */
		if (headBytes && tailBytes) {
			headBytes += tailBytes;
			tailBytes = 0;
		}
	}

	state->burst_count += (burstBytes != 0);
	state->other_count += (headBytes != 0) + (tailBytes != 0);

	return (burstBytes != 0) + (headBytes != 0) + (tailBytes != 0);
}


/****************************************************************************/
/**
*   Helper routine to add descriptors used by sdma_map_create_descriptor_ring
*
*   @return
*       0       Descriptors memory set successfully
*       -ENODEV Invalid channel
*/
/****************************************************************************/

static int
add_desc(
	void       *data1,
	void       *data2,
	dma_addr_t  srcAddr,
	dma_addr_t  dstAddr,
	size_t      numBytes
)
{
	int rc = 1;
	unsigned int headBytes, burstBytes, tailBytes;
	struct xfer_list_state *state = (struct xfer_list_state *)data1;

	(void)data2;

	vchiq_log_trace(vchiq_arm_log_level,
		"add_desc - srcAddr=%x, dstAddr=%x, numBytes=%x, "
		"burst_size=%x %x",
		(unsigned int)srcAddr, (unsigned int)dstAddr,
		(unsigned int)numBytes, state->burst_size,
		(unsigned int)state);

	/* Capri requires that the source address is aligned to burst size, so
	** use an extra transfer to bring srcAddr into alignment.
	*/
	burstBytes = numBytes;
	headBytes = ((-(unsigned int)srcAddr) & (state->burst_size - 1));
	if (headBytes) {
		if (burstBytes <= headBytes) {
			headBytes = burstBytes;
			burstBytes = 0;
		} else
			burstBytes -= headBytes;
	}

	tailBytes = burstBytes & (state->burst_data_length - 1);
	burstBytes -= tailBytes;

	/*
	** If the destination address is not aligned to the burst size then the
	** final burst is truncated at the burst-size boundary, i.e. if
	** dstaddr % burst_size == 3 then the final 3 bytes will not be
	** transferred. One could benefit from this in some circumstances, but
	** relying on a write not happening is dangerous, so instead just fill
	** in the gap.
	*/
	if (burstBytes)
		tailBytes += (((unsigned int)dstAddr + headBytes) &
			(state->burst_size - 1));
		/* N.B. burstBytes and tailBytes can now overlap... */
	else {
		/* Combine headBytes and tailBytes if no burst data */
		if (headBytes && tailBytes) {
			headBytes += tailBytes;
			tailBytes = 0;
		}
	}

	if (headBytes) {
		if (state->other_pos  < state->other_count) {
			struct dma_transfer_list *xfer =
				state->xfer_storage + state->other_pos;
			xfer->srcaddr = srcAddr;
			xfer->dstaddr = dstAddr;
			xfer->xfer_size = headBytes;
			vchiq_log_trace(vchiq_arm_log_level,
				"add_desc - other (%x, %x, %x)",
				(unsigned int)xfer->srcaddr,
				(unsigned int)xfer->dstaddr,
				(unsigned int)xfer->xfer_size);
			state->other_pos++;
			xfer++;
			srcAddr += headBytes;
			dstAddr += headBytes;
		} else
			rc = 0;
	}

	if (burstBytes) {
		if (state->burst_pos < state->burst_count) {
			struct dma_transfer_list *xfer =
				state->xfer_storage + state->burst_pos;
			xfer->srcaddr = srcAddr;
			xfer->dstaddr = dstAddr;
			xfer->xfer_size = burstBytes;
			vchiq_log_trace(vchiq_arm_log_level,
				"add_desc - burst (%x, %x, %x)",
				(unsigned int)xfer->srcaddr,
				(unsigned int)xfer->dstaddr,
				(unsigned int)xfer->xfer_size);
			state->burst_pos++;
			xfer++;
			/* Watch out for the overlap */
			burstBytes -= ((unsigned int)dstAddr &
				(state->burst_size - 1));
			srcAddr += burstBytes;
			dstAddr += burstBytes;
		} else
			rc = 0;
	}

	if (tailBytes) {
		if (state->other_pos < state->other_count) {
			struct dma_transfer_list *xfer =
				state->xfer_storage + state->other_pos;
			xfer->srcaddr = srcAddr;
			xfer->dstaddr = dstAddr;
			xfer->xfer_size = tailBytes;
			vchiq_log_trace(vchiq_arm_log_level,
				"add_desc - other (%x, %x, %x)",
				(unsigned int)xfer->srcaddr,
				(unsigned int)xfer->dstaddr,
				(unsigned int)xfer->xfer_size);
			state->other_pos++;
			xfer++;
		} else
			rc = 0;
	}

	if (!rc) {
		vchiq_log_error(vchiq_arm_log_level,
			"* Internal xfer storage exhausted - "
			"calc_desc_cnt must be wrong");
		WARN(1, "\n");
	}
	return rc;
}

static void *
build_transfer_list(DMA_MMAP_CFG_T *memMap, dma_addr_t physAddr,
	unsigned int cfg, struct xfer_list_state *state)
{
	int rc;
	int desc_cnt;
	unsigned int burst_size, burst_length;
	unsigned int burst_data_length;

	burst_size = 1 << ((cfg & DMA_CFG_BURST_SIZE_MASK) >>
		DMA_CFG_BURST_SIZE_SHIFT);
	burst_length = 1 + ((cfg & DMA_CFG_BURST_LENGTH_MASK) >>
		DMA_CFG_BURST_LENGTH_SHIFT);
	burst_data_length = burst_size * burst_length;

	state->burst_count = 0;
	state->other_count = 0;
	state->burst_data_length = burst_data_length;
	state->burst_size = burst_size;

	dma_mmap_lock_map(memMap);

	desc_cnt = dma_mmap_calc_desc_cnt(memMap, physAddr,
		DMA_MMAP_DEV_ADDR_INC,
		(void *)state, /* user data 1 */
		NULL,          /* user data 2 */
		&calc_desc_cnt);

	if (desc_cnt < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"dma_mmap_calc_desc_cnt failed: %d",
			desc_cnt);
		goto out;
	}

	vchiq_log_trace(vchiq_arm_log_level,
		"%s: %d burst, %d other",
		__func__, state->burst_count, state->other_count);

	state->xfer_storage = kmalloc(
		sizeof(struct dma_transfer_list) * desc_cnt, GFP_KERNEL);
	if (!state->xfer_storage) {
		vchiq_log_error(vchiq_arm_log_level,
			"failed to kalloc %d bytes",
			sizeof(struct dma_transfer_list) * desc_cnt);
		goto out;
	}

	state->burst_pos = 0;
	state->other_count = desc_cnt;
	state->other_pos = state->burst_count;

	rc = dma_mmap_add_desc(memMap, physAddr, DMA_MMAP_DEV_ADDR_INC,
		(void *)state, NULL, &add_desc);
	if (rc < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"dma_mmap_add_desc failed: %d",
			rc);
		kfree(state->xfer_storage);
		state->xfer_storage = NULL;
	}

out:
	dma_mmap_unlock_map(memMap);

	return state->xfer_storage;
}

/****************************************************************************
*
*   ipc_dma
*
***************************************************************************/

static int
ipc_dma(void *vcaddr, void *armaddr, int len, DMA_MMAP_PAGELIST_T *pagelist,
	enum dma_data_direction dir)
{
	dma_addr_t vcAddrOffset;
	dma_addr_t vcPhysAddr;
	unsigned int chan;
	unsigned int dma_cfg;
	void *storage;
	struct xfer_list_state state;
	int pos;
	int rc;

	/* Double check the memory is supported by dma_mmap */
	rc = dma_mmap_dma_is_supported(armaddr);
	if (!rc) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: Buffer not supported buf=0x%lx",
			__func__, (unsigned long)armaddr);
		goto failed_dma_mmap_dma_is_supported;
	}

	if (mutex_lock_interruptible(&g_dma_mutex) != 0)
		return -1;

	/* Setup qos request for the duration of the actual
	 * transfer.
	 */
	pm_qos_add_request(&g_dma_qos_request,
		PM_QOS_CPU_DMA_LATENCY, DMA_QOS_VAL);

	vchiq_log_trace(vchiq_arm_log_level,
		"(Bulk) dir=%s vcaddr=0x%x armaddr=0x%x len=%u",
		(dir == DMA_TO_DEVICE) ? "Tx" : "Rx", (unsigned int)vcaddr,
		(unsigned int)armaddr, len);

	/* Convert the videocore pointer to a videocore address offset */
	vcAddrOffset = (dma_addr_t)(((unsigned long)vcaddr) & 0x3FFFFFFFuL);

	/* Convert the videocore physical address into an ARM physical
	** address */
	vcPhysAddr = mm_vc_mem_phys_addr + vcAddrOffset;

	dma_cfg = DMA_CFG_BURST_SIZE_8 |
		DMA_CFG_BURST_LENGTH_8 |
		DMA_CFG_SRC_ADDR_INCREMENT |
		DMA_CFG_DST_ADDR_INCREMENT;

	/* Set the pagelist (for user buffers) */
	if (pagelist)
		dma_mmap_set_pagelist(&gVchiqDmaMmap, pagelist);

	/* Map memory */
	rc = dma_mmap_map(&gVchiqDmaMmap, armaddr, len, dir);
	if (rc < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: dma_mmap_map FAILED buf=0x%lx len=0x%lx",
			__func__, (unsigned long)armaddr, (unsigned long)len);
		goto failed_dma_mmap_map;
	}

	storage = build_transfer_list(&gVchiqDmaMmap, vcPhysAddr, dma_cfg,
		&state);
	if (!storage)
		goto failed_build_transfer_list;

	rc = dma_request_chan(&chan, NULL/*memory<->memory*/);
	if (rc < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: dma_request_chan failed -> %d",
			__func__, rc);
		rc = -1;
		goto failed_dma_request_chan;
	}

	rc = dma_register_callback(chan, dma_callback, &gDmaDone);
	if (rc < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: dma_register_callback failed",
			__func__);
		goto failed_dma_register_callback;
	}

	init_completion(&gDmaDone);

	pos = 0;

	while (pos < state.burst_pos) {
		struct list_head list;
		int start_pos = pos;
		int max_pos = pos + MAX_DESCRIPTORS_PER_TRANSFER;
		if (max_pos > state.burst_pos)
			max_pos = state.burst_pos;

		INIT_LIST_HEAD(&list);

		while (pos < max_pos) {
			struct dma_transfer_list *xfer =
				state.xfer_storage + pos;
			list_add_tail(&xfer->next, &list);
			pos++;
		}

		rc = dma_setup_transfer_list(chan, &list,
			DMA_DIRECTION_MEM_TO_UNALIGNED_MEM,
			dma_cfg);
		if (rc < 0) {
			vchiq_log_error(vchiq_arm_log_level,
				"%s: dma_setup_transfer_list(burst) "
				"FAILED rc=%d",
				__func__, rc);
			vchiq_log_error(vchiq_arm_log_level,
				 "%s: vcaddr=0x%p armaddr=0x%p len=%d dir=%s",
				__func__, vcaddr, armaddr, len,
				dma_data_direction_as_str(dir));
			goto failed_dma_setup_transfer_list;
		}

		INIT_COMPLETION(gDmaDone);   /* Mark as incomplete */

		rc = dma_start_transfer(chan);
		if (rc != 0) {
			vchiq_log_error(vchiq_arm_log_level,
				"dma_start_transfer (burst) failed - %d "
				"(%d descriptors)",
				rc, pos - start_pos);
			goto failed_dma_start_transfer;
		}

		wait_for_completion(&gDmaDone);

		dma_stop_transfer(chan);
	}

	pos = state.burst_count;

	while (pos < state.other_pos) {
		struct list_head list;
		int start_pos = pos;
		int max_pos = pos + MAX_DESCRIPTORS_PER_TRANSFER;
		if (max_pos > state.other_pos)
			max_pos = state.other_pos;

		INIT_LIST_HEAD(&list);

		while (pos < max_pos) {
			struct dma_transfer_list *xfer =
				state.xfer_storage + pos;
			list_add_tail(&xfer->next, &list);
			pos++;
		}

		rc = dma_setup_transfer_list(chan, &list,
			DMA_DIRECTION_MEM_TO_MEM,
			DMA_CFG_BURST_SIZE_1 | DMA_CFG_BURST_LENGTH_1 |
			DMA_CFG_SRC_ADDR_INCREMENT |
			DMA_CFG_DST_ADDR_INCREMENT);
		if (rc < 0) {
			vchiq_log_error(vchiq_arm_log_level,
				"%s: dma_setup_transfer_list(other) "
				"FAILED rc=%d",
				__func__, rc);
			vchiq_log_error(vchiq_arm_log_level,
				"%s: vcaddr=0x%p armaddr=0x%p len=%d dir=%s",
				__func__, vcaddr, armaddr, len,
				dma_data_direction_as_str(dir));
			goto failed_dma_setup_transfer_list;
		}

		rc = dma_start_transfer(chan);
		if (rc != 0) {
			vchiq_log_error(vchiq_arm_log_level,
				"dma_start_transfer (other) failed - %d "
				"(%d descriptors)",
				rc, pos - start_pos);
			goto failed_dma_start_transfer;
		}

		wait_for_completion(&gDmaDone);

		dma_stop_transfer(chan);
	}

	rc = 0;

failed_dma_start_transfer:
failed_dma_setup_transfer_list:
failed_dma_register_callback:
	dma_free_chan(chan);
	kfree(storage);
failed_build_transfer_list:
	dma_mmap_unmap(&gVchiqDmaMmap,
		(dir == DMA_FROM_DEVICE) ? DMA_MMAP_DIRTIED : DMA_MMAP_CLEAN);
failed_dma_mmap_map:
failed_dma_request_chan:
	pm_qos_remove_request(&g_dma_qos_request);
	mutex_unlock(&g_dma_mutex);

failed_dma_mmap_dma_is_supported:
	return rc;
}

#elif defined(CONFIG_MAP_SDMA)

/****************************************************************************
*
*   sdma_device_handler
*
***************************************************************************/

static void sdma_device_handler(DMA_Device_t dev, int reason, void *userData)
{
	 (void)dev;
	 (void)userData;

	 struct completion *dmaDone = userData;

	 if (reason & DMA_HANDLER_REASON_TRANSFER_COMPLETE)
		complete(dmaDone);

	 if (reason != DMA_HANDLER_REASON_TRANSFER_COMPLETE)
		vchiq_log_error(vchiq_arm_log_level,
			"%s: called with reason = 0x%x",
			__func__, reason);
}

static void vchiq_dev_to_cpu(dma_addr_t vcPhysAddr, void *vcVirtAddr,
			     size_t len,
			     enum dma_data_direction dir)
{
	dmac_unmap_area(vcVirtAddr, len, dir);
	if (dir == DMA_FROM_DEVICE) {
		/* just about to memcpy from VC to the ARM */
		outer_inv_range(vcPhysAddr, vcPhysAddr + len);
		vchiq_log_trace(vchiq_arm_log_level,
				"%s:invalidate: vc[0] = 0x%08x vcphys 0x%x"
				"len 0x%x\n", __func__,
				((uint32_t *)vcVirtAddr)[0],
				vcPhysAddr, len);
	} else {
		/* just about to memcpy from the ARM to VC */
	}
}

static void vchiq_cpu_to_dev(dma_addr_t vcPhysAddr, void *vcVirtAddr,
			     size_t len, enum dma_data_direction dir)
{
	dmac_map_area(vcVirtAddr, len, dir);
	if (dir == DMA_FROM_DEVICE) {
		/* have just copied from VC to ARM */
	} else {
		/* DMA_TO_DEVICE: have just copied from ARM to VC */
		outer_flush_range(vcPhysAddr, vcPhysAddr + len);
		vchiq_log_trace(vchiq_arm_log_level,
				"%s:clean: vc[0] = 0x%08x\n",
				__func__,
				((uint32_t *)vcVirtAddr)[0]);
	}

}

/*
 * Copy using memcpy where the VC memory has been mapped into HIGHMEM. This has
 * to be done just one page at a time.
 */
static int
ipc_copy_highmem(dma_addr_t vcPhysAddr, uint8_t *armaddr, int len,
	DMA_MMAP_PAGELIST_T *pagelist,
	enum dma_data_direction dir, struct page *vc_page, int pagelist_start)
{
	int rc;
	DMA_MMAP_DIRTIED_T dirty = (dir == DMA_FROM_DEVICE) ?
		DMA_MMAP_DIRTIED : DMA_MMAP_CLEAN;

	size_t vcFirstPageOffset = vcPhysAddr & (PAGE_SIZE-1);
	size_t vcFirstPageSize = PAGE_SIZE - vcFirstPageOffset;
	uint8_t *baseArmAddr = (uint8_t *)((uintptr_t)armaddr & ~(PAGE_SIZE-1));
	if (vcFirstPageSize > len)
		vcFirstPageSize = len;

	if (pagelist) {
		dma_mmap_set_pagelist(&gVchiqDmaMmap, pagelist);
		rc = dma_mmap_set_pagelist_start(&gVchiqDmaMmap,
			pagelist_start);
		if (rc < 0)
			return -1;
	}

	vchiq_log_trace(vchiq_arm_log_level,
			"%s: start: len = %d armaddr %p, vcphys 0x%x dir %d\n",
			__func__,
			len, armaddr, vcPhysAddr, dir);

	rc = dma_mmap_map(&gVchiqDmaMmap, armaddr, vcFirstPageSize, dir);
	if (rc < 0) {
		printk(KERN_ERR "%s: failed to add %d @ 0x%p to dma mmap: %d\n",
		       __func__, vcFirstPageSize, armaddr, rc);
		goto fail;
	}

	/* Do the first VC page, which may be partial. */

	uint8_t *vcVirtAddrBase = kmap_atomic(vc_page);
	uint8_t *vcVirtAddr = vcVirtAddrBase + vcFirstPageOffset;

	vchiq_dev_to_cpu(vcPhysAddr, vcVirtAddr, vcFirstPageSize, dir);

	if (pagelist)
		dma_mmap_set_pagelist_start(&gVchiqDmaMmap, pagelist_start);

	dma_mmap_memcpy(&gVchiqDmaMmap, vcVirtAddr);

	if (pagelist)
		dma_mmap_set_pagelist_start(&gVchiqDmaMmap, 0);

	vchiq_cpu_to_dev(vcPhysAddr, vcVirtAddr, vcFirstPageSize, dir);

	kunmap_atomic(vcVirtAddrBase);

	dma_mmap_unmap(&gVchiqDmaMmap, dirty);

	vcPhysAddr += vcFirstPageSize;
	armaddr += vcFirstPageSize;
	len -= vcFirstPageSize;

	/* Now the remaining pages. */

	while (len) {
		int vcpfn = __phys_to_pfn(vcPhysAddr);
		size_t bytesThisPage = len > PAGE_SIZE ? PAGE_SIZE : len;
		int start = 0;
		BUG_ON(vcPhysAddr & (PAGE_SIZE-1));
		vc_page = pfn_to_page(vcpfn);
		if (pagelist) {
			start = pagelist_start
				+ ((armaddr - baseArmAddr) >> PAGE_SHIFT);

			dma_mmap_set_pagelist(&gVchiqDmaMmap, pagelist);
			rc = dma_mmap_set_pagelist_start(&gVchiqDmaMmap,
							 start);
			if (rc < 0)
				break;
		}
		rc = dma_mmap_map(&gVchiqDmaMmap, armaddr, bytesThisPage, dir);
		if (rc < 0) {
			printk(KERN_ERR "%s: could not dma_mmap %d @ %p\n",
				__func__, bytesThisPage, armaddr);
			break;
		}
		vcVirtAddr = kmap_atomic(vc_page);

		vchiq_dev_to_cpu(vcPhysAddr, vcVirtAddr, bytesThisPage, dir);

		if (pagelist)
			dma_mmap_set_pagelist_start(&gVchiqDmaMmap, start);

		dma_mmap_memcpy(&gVchiqDmaMmap, vcVirtAddr);

		if (pagelist)
			dma_mmap_set_pagelist_start(&gVchiqDmaMmap, 0);

		vchiq_cpu_to_dev(vcPhysAddr, vcVirtAddr, bytesThisPage, dir);

		kunmap_atomic(vcVirtAddr);
		dma_mmap_unmap(&gVchiqDmaMmap, dirty);

		vcPhysAddr += bytesThisPage;
		armaddr += bytesThisPage;
		len -= bytesThisPage;
	}
fail:
	return rc;
}

/*
 * Verify that the full region of the required copy can be handled by using
 * the existing kernel/vc memory mapping.
 * */
static bool
check_vc_mapping(dma_addr_t vcPhysAddr, int len)
{
	bool valid = false;
	int start_pfn = __phys_to_pfn(vcPhysAddr);
	int end_pfn   = __phys_to_pfn(vcPhysAddr + len);

	if (unlikely(!pfn_valid(start_pfn)) || unlikely(!pfn_valid(end_pfn)))
		goto out;

#ifdef CONFIG_HIGHMEM
	/* If we're using highmem, check if the start and end addresses straddle
	 * high and lowmem.  If they do, don't trust that the whole region is
	 * mapped */
	if (unlikely(PageHighMem(pfn_to_page(end_pfn))) &&
				likely(!PageHighMem(pfn_to_page(start_pfn)))) {
		goto out;
	}

#endif

	valid = true;
out:
	return valid;
}

/*
 *  __dma_memcpy
 *
 *  NOTE: caller must grab g_dma_mutex lock!!!
 */
static inline int __dma_memcpy(void *vcaddr, void *armaddr, int len,
		DMA_MMAP_PAGELIST_T *pagelist,
		enum dma_data_direction dir,
		int pagelist_start)
{
	int rc;
	dma_addr_t vcAddrOffset;
	dma_addr_t vcPhysAddr;
	int vcpfn;
	struct resource *res = NULL;
	uint8_t *vcVirtAddr = NULL;

	vchiq_log_trace(vchiq_arm_log_level,
		"%s: (Bulk) dir=%s vcaddr=0x%x armaddr=0x%x len=%u",
		__func__, (dir == DMA_TO_DEVICE) ? "Tx" : "Rx",
		(unsigned int)vcaddr,
		(unsigned int)armaddr, len);

	/* Convert the videocore pointer to a videocore address offset */
	vcAddrOffset = (dma_addr_t)(((unsigned long)vcaddr) & 0x3FFFFFFFuL);

	/* Convert the videocore physical address into an ARM physical
	** address */
	vcPhysAddr = mm_vc_mem_phys_addr + vcAddrOffset;

	/* Double check the memory is supported by dma_mmap */
	rc = dma_mmap_dma_is_supported(armaddr);
	if (!rc) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: Buffer not supported buf=0x%lx",
			__func__, (unsigned long)armaddr);
		goto failed_dma_mmap_dma_is_supported;
	}

	/* Set the pagelist (for user buffers) */
	if (pagelist)
		dma_mmap_set_pagelist(&gVchiqDmaMmap, pagelist);

	/* How is the VC memory mapped? Might be mapped in already via CMA,
	 * as either a normal page or a high memory page.
	 */
	vcpfn = __phys_to_pfn(vcPhysAddr);
	if (pfn_valid(vcpfn)) {
		struct page *page = pfn_to_page(vcpfn);
		if (PageHighMem(page)) {
			rc = ipc_copy_highmem(vcPhysAddr, armaddr, len,
					      pagelist, dir, page,
					      pagelist_start);
			if (rc != 0)
				vchiq_log_error(vchiq_arm_log_level,
					"%s: ipc_copy_highmem FAILED",
					__func__);

			goto finish_highmem;
		}
	}

	/* Map memory */
	rc = dma_mmap_map(&gVchiqDmaMmap, armaddr, len, dir);
	if (rc < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: dma_mmap_map FAILED buf=0x%lx len=0x%lx",
				__func__, (unsigned long)armaddr,
				(unsigned long)len);
		goto failed_dma_mmap_map;
	}

	if (check_vc_mapping(vcPhysAddr, len)) {
		/* This memory is shared between Linux and VC */
		vcVirtAddr = (void *)__phys_to_virt(vcPhysAddr);

		/* N.B. If this logic seems backwards compared to what you are
		 * used to, that's because it is. Here the "device" memory is
		 * also host memory, hence the need for host cache maintenance.
		 */
		vchiq_dev_to_cpu(vcPhysAddr, vcVirtAddr, len, dir);
	} else {
		/* Request an I/O memory region for remapping */
		res = request_mem_region(vcPhysAddr, len, "vchiq");
		if (res == NULL) {
			vchiq_log_error(vchiq_arm_log_level,
				"%s: failed to request I/O memory region 0x%x, "
				"len %d", __func__, vcPhysAddr, len);
			rc = -1;
			goto failed_request_mem_region;
		} else {
			/* I/O remap the videocore memory */
			vcVirtAddr = ioremap_nocache(vcPhysAddr, len);
			if (vcVirtAddr == NULL) {
				vchiq_log_error(vchiq_arm_log_level,
					"%s: failed to I/O remap "
					"videocore bulk buffer", __func__);
				release_mem_region(res->start,
					resource_size(res));
				rc = -1;
				goto failed_ioremap;
			}
		}
	}

	/* need to point to the correct page before memory transfer */
	if (pagelist)
		dma_mmap_set_pagelist_start(&gVchiqDmaMmap, pagelist_start);

	dma_mmap_memcpy(&gVchiqDmaMmap, vcVirtAddr);

	/* reset back to the first page so it can be unmapped properly */
	if (pagelist)
		dma_mmap_set_pagelist_start(&gVchiqDmaMmap, 0);

	if (res) {
		iounmap(vcVirtAddr);
		release_mem_region(res->start, resource_size(res));
	} else
		vchiq_cpu_to_dev(vcPhysAddr, vcVirtAddr, len, dir);
	rc = 0;

failed_request_mem_region:
failed_ioremap:
	dma_mmap_unmap(&gVchiqDmaMmap, (dir == DMA_FROM_DEVICE) ?
		DMA_MMAP_DIRTIED : DMA_MMAP_CLEAN);
failed_dma_mmap_map:
failed_dma_mmap_dma_is_supported:
finish_highmem:

	return rc;
}

/****************************************************************************
*
*   ipc_dma_memcpy
*
***************************************************************************/
static int
ipc_dma_memcpy(void *vcaddr, void *armaddr, int len,
	DMA_MMAP_PAGELIST_T *pagelist, enum dma_data_direction dir)
{
	int rc;

	if (mutex_lock_interruptible(&g_dma_mutex) != 0)
		return -1;

	/* Setup qos request for the duration of the actual
	 * transfer.
	 */
	pm_qos_add_request(&g_dma_qos_request,
		PM_QOS_CPU_DMA_LATENCY, DMA_QOS_VAL);

	vchiq_log_trace(vchiq_arm_log_level,
		"%s: (Bulk) dir=%s vcaddr=0x%x armaddr=0x%x len=%u",
		__func__, (dir == DMA_TO_DEVICE) ? "Tx" : "Rx",
		(unsigned int)vcaddr,
		(unsigned int)armaddr, len);

	rc = __dma_memcpy(vcaddr, armaddr, len, pagelist, dir, 0);
	if (rc < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: __dma_memcpy failed rc=%d", __func__, rc);
	}

	pm_qos_remove_request(&g_dma_qos_request);
	mutex_unlock(&g_dma_mutex);

	return rc;
}

/****************************************************************************
*
*   do_dma
*
***************************************************************************/
static int do_dma(DMA_Device_t dmaDev, void *armaddr, dma_addr_t vcPhysAddr,
	unsigned long len, DMA_MMAP_PAGELIST_T *pagelist,
	enum dma_data_direction dir)
{
	SDMA_Handle_t dmaHndl = SDMA_INVALID_HANDLE;
	int rc;

	dmaHndl = sdma_request_channel(dmaDev);

	if (dmaHndl < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: sdma_request_channel failed", __func__);
		rc = -1;
		goto failed_sdma_request_channel;
	}

	rc = sdma_set_device_handler(dmaDev, sdma_device_handler, &gDmaDone);
	if (rc < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: sdma_set_device_handler failed", __func__);
		goto failed_sdma_set_device_handler;
	}

	INIT_COMPLETION(gDmaDone);   /* Mark as incomplete */

	/* Double check the memory is supported by dma_mmap */
	rc = dma_mmap_dma_is_supported(armaddr);
	if (!rc) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: Buffer not supported buf=0x%lx",
			__func__, (unsigned long)armaddr);
		goto failed_dma_mmap_dma_is_supported;
	}

	/* Set the pagelist (for user buffers) */
	if (pagelist)
		dma_mmap_set_pagelist(&gVchiqDmaMmap, pagelist);

	/* Map memory */
	rc = dma_mmap_map(&gVchiqDmaMmap, armaddr, len, dir);
	if (rc < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: dma_mmap_map FAILED buf=0x%lx len=0x%lx",
				__func__, (unsigned long)armaddr, len);
		goto failed_dma_mmap_map;
	}

	rc = sdma_map_create_descriptor_ring(dmaHndl, &gVchiqDmaMmap,
		vcPhysAddr, DMA_UPDATE_MODE_INC);
	if (rc < 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: sdma_map_create_descriptor_ring FAILED rc=%u",
			__func__, rc);
		vchiq_log_error(vchiq_arm_log_level,
			"%s: vcaddr=0x%x armaddr=0x%p len=%ld dir=%s",
			__func__, vcPhysAddr, armaddr, len,
			dma_data_direction_as_str(dir));
		goto failed_sdma_map_create_descriptor_ring;
	}

	rc = sdma_start_transfer(dmaHndl);
	if (rc != 0) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: DMA failed %d", __func__, rc);
		goto failed_sdma_start_transfer;
	}

	wait_for_completion(&gDmaDone);


failed_sdma_start_transfer:
failed_sdma_map_create_descriptor_ring:
	dma_mmap_unmap(&gVchiqDmaMmap, (dir == DMA_FROM_DEVICE) ?
		DMA_MMAP_DIRTIED : DMA_MMAP_CLEAN);
failed_dma_mmap_map:
failed_dma_mmap_dma_is_supported:
failed_sdma_set_device_handler:
	if (dmaHndl != SDMA_INVALID_HANDLE)
		sdma_free_channel(dmaHndl);
failed_sdma_request_channel:
	return rc;
}

#define DUAL_TXFER_THRESH 0x800
/****************************************************************************
*
*   ipc_dma
*
***************************************************************************/
static int
ipc_dma(void *vcaddr, void *armaddr, int len, DMA_MMAP_PAGELIST_T *pagelist,
	enum dma_data_direction dir)
{
	int rc;
	dma_addr_t vcAddrOffset;
	dma_addr_t vcPhysAddr;
	DMA_Device_t dmaDev;
	int pagelist_start;
	uint8_t *baseArmAddr = (uint8_t *)((uintptr_t)armaddr &
			~(PAGE_SIZE-1));
	unsigned long aligned_len = len;
	unsigned long vcaddr_val = (unsigned long)vcaddr;
	unsigned long armaddr_val = (unsigned long)armaddr;
	unsigned long alck;
	int i;
	static const DMA_Device_t dmaDevList[] = {
		DMA_DEVICE_MEM_TO_MEM,
		DMA_DEVICE_MEM_TO_MEM_32,
		DMA_DEVICE_MEM_TO_MEM_16BIT,
		DMA_DEVICE_MEM_TO_MEM_BYTE
	};

	if (use_memcpy)
		return ipc_dma_memcpy(vcaddr, armaddr, len, pagelist, dir);

	if (mutex_lock_interruptible(&g_dma_mutex) != 0)
		return -1;

	/* Setup qos request for the duration of the actual
	 * transfer.
	 */
	pm_qos_add_request(&g_dma_qos_request,
		PM_QOS_CPU_DMA_LATENCY, DMA_QOS_VAL);

	vchiq_log_trace(vchiq_arm_log_level,
		"%s: (Bulk) dir=%s vcaddr=0x%x armaddr=0x%x len=%u",
		__func__, (dir == DMA_TO_DEVICE) ? "Tx" : "Rx",
		(unsigned int)vcaddr,
		(unsigned int)armaddr, len);

	/* Convert the videocore pointer to a videocore address offset */
	vcAddrOffset = (dma_addr_t)(((unsigned long)vcaddr) & 0x3FFFFFFFuL);

	/* Convert the videocore physical address into an ARM physical
	** address */
	vcPhysAddr = mm_vc_mem_phys_addr + vcAddrOffset;

	init_completion(&gDmaDone);

	dmaDev = DMA_DEVICE_MEM_TO_MEM_BYTE;
	/* Use the largest transfer size we can.  We check the src/dest
	 * alignment and that the size is a multiple of the transfer size.
	 * For large transfers with the correct alignment but bad size, do most
	 * of the transfer using a large transfer size, and fixup the last few
	 * bytes using a smaller transfer. */
	for (alck = 0x7, i = 0; alck; alck >>= 1, i++) {
		if (((vcaddr_val & alck) == 0) && ((armaddr_val & alck) == 0) &&
			(((aligned_len & alck) == 0) ||
				(aligned_len > DUAL_TXFER_THRESH))) {
			dmaDev = dmaDevList[i];
			aligned_len &= ~alck;
			break;
		}
	}

	rc = do_dma(dmaDev, armaddr, vcPhysAddr, aligned_len, pagelist, dir);

	if (rc != 0)
		goto out;

	len -= aligned_len;
	if (len == 0)
		goto out;

	armaddr = (char *)armaddr + aligned_len;
	vcaddr += aligned_len;

	/*
	 * Copy trailing unaligned bytes. Don't need to DMA here since the
	 * trailing bytes aren't that many. The overhead of descriptor setup
	 * will be too much!
	 *
	 * Memory copy is good enough.
	 */
	dmaDev = DMA_DEVICE_NONE;
	/* figure out which page to use */
	pagelist_start = ((uint8_t *)armaddr - baseArmAddr) >> PAGE_SHIFT;
	rc = __dma_memcpy(vcaddr, armaddr, len, pagelist, dir, pagelist_start);

out:
	pm_qos_remove_request(&g_dma_qos_request);
	mutex_unlock(&g_dma_mutex);

	return rc;
}

#endif /* CONFIG_DMAC_PL330 */
