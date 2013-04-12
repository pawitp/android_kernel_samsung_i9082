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
#include <mach/dma_mmap.h>

#include <linux/pagemap.h>
#include <asm/pgtable.h>
#include <mach/sdma.h>

#include "vchiq_arm.h"
#include "vchiq_kona_arm.h"
#include "vchiq_bivcm.h"

#if defined(CONFIG_MAP_LITTLE_ISLAND_MODE) || defined(CONFIG_LITTLE_MODE)
#define ARM_RAM_BASE_IN_VC 0xc0000000
#else
#define ARM_RAM_BASE_IN_VC 0xe0000000
#endif
#define VIRT_TO_VC(x) PHYS_TO_VC((unsigned long)x - PAGE_OFFSET + PHYS_OFFSET)
#define PHYS_TO_VC(x) ((unsigned long)x - 0x80000000 + ARM_RAM_BASE_IN_VC)

static int          g_pagelist_count;
static FRAGMENTS_T *g_fragments_base;
static FRAGMENTS_T *g_free_fragments;
static int          g_free_fragments_count;
struct semaphore    g_free_fragments_sema;
static DEFINE_SEMAPHORE(g_free_fragments_mutex);

static int
create_pagelist(char __user *buf, size_t count, unsigned short type,
	struct task_struct *task, PAGELIST_T ** ppagelist);

static void
free_pagelist(PAGELIST_T *pagelist, int actual);

int __init
vchiq_platform_init(VCHIQ_STATE_T *state)
{
#if ((defined(CONFIG_ARCH_KONA) || \
	defined(CONFIG_ARCH_BCMHANA)) && \
	!(defined(CONFIG_MAP_LITTLE_ISLAND_MODE) || \
	defined(CONFIG_LITTLE_MODE)))

	/*
	** On Big Island, the videocore can only access the lower 512 Mb of the
	** ARM memory. So BIVCM can only work if the host is located in the
	** lower 512 Mb of physical memory.
	*/

	#define MAX_BIVCM_MEM   (512 * 1024 * 1024)

	if ((num_physpages * PAGE_SIZE) > MAX_BIVCM_MEM) {
		vchiq_loud_error_header();
		vchiq_loud_error("BIVCM can't be used when the kernel has "
			"more than 512 Mb of memory.");
		vchiq_loud_error("Either limit the amount of memory by using "
			"mem=512M on the kernel");
		vchiq_loud_error("command line, or switch to using BI "
			"instead.");
		vchiq_loud_error("");
		vchiq_loud_error("num_physpages: 0x%08lx (%4ld Mb)\n",
			num_physpages, num_physpages >> (20 - PAGE_SHIFT));
		vchiq_loud_error("max supported: 0x%08lx (%4d Mb) "
			"num_physpages can't exceed this to use BIVCM.",
			MAX_BIVCM_MEM / PAGE_SIZE, MAX_BIVCM_MEM >> 20);
		vchiq_loud_error_footer();

		BUG();
		return -ENOMEM;
	}
#endif

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
	PAGELIST_T *pagelist;
	int ret;

	WARN_ON(memhandle != VCHI_MEM_HANDLE_INVALID);

	ret = create_pagelist((char __user *)offset, size,
			(dir == VCHIQ_BULK_RECEIVE)
			? PAGELIST_READ
			: PAGELIST_WRITE,
			current,
			&pagelist);
	if (ret != 0)
		return VCHIQ_ERROR;

	bulk->handle = memhandle;
	bulk->data = (void *)VIRT_TO_VC(pagelist);

	/* Store the pagelist address in remote_data, which isn't used by the
		slave. */
	bulk->remote_data = pagelist;

	return VCHIQ_SUCCESS;
}

void
vchiq_complete_bulk(VCHIQ_BULK_T *bulk)
{
	free_pagelist((PAGELIST_T *)bulk->remote_data, bulk->actual);
}

void
vchiq_transfer_bulk(VCHIQ_BULK_T *bulk)
{
	/*
	 * This should only be called on the master (VideoCore) side, but
	 * provide an implementation to avoid the need for ifdefery.
	 */
	BUG();
}

void
vchiq_dump_platform_state(void *dump_context)
{
	char buf[80];
	int len;
	len = snprintf(buf, sizeof(buf),
		"  Platform: BI (VC master) pagelists=%d, free_fragments=%d",
		g_pagelist_count, g_free_fragments_count);
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
	int i;

	/* Initialize the local state. Note that vc04 has already started by now
	** so the slot memory is expected to be initialised. */
	status = vchiq_init_state(state, slot_zero, 0/* slave */);

	if (status != VCHIQ_SUCCESS) {
		vchiq_log_error(vchiq_arm_log_level,
			"%s: vchiq_init_state failed",
			__func__);
		goto failed_init_state;
	}

	g_pagelist_count = 0;
	g_fragments_base = (FRAGMENTS_T *)((char *)slot_zero +
		slot_zero->platform_data[
			VCHIQ_PLATFORM_FRAGMENTS_OFFSET_IDX]);
	g_free_fragments_count =
		slot_zero->platform_data[
			VCHIQ_PLATFORM_FRAGMENTS_COUNT_IDX];

	g_free_fragments = g_fragments_base;
	for (i = 0; i < (g_free_fragments_count - 1); i++) {
		*(FRAGMENTS_T **)&g_fragments_base[i] =
			&g_fragments_base[i + 1];
	}
	*(FRAGMENTS_T **)&g_fragments_base[i] = NULL;
	sema_init(&g_free_fragments_sema, g_free_fragments_count);

failed_init_state:
	return status;
}

/* There is a potential problem with partial cache lines
	at the ends of the block when reading. If the CPU accessed anything in
	the same line (page?) then it may have pulled old data into the cache,
	obscuring the new data underneath. We can solve this by transferring the
	partial cache lines separately, and allowing the ARM to copy into the
	cached area.
 */

static int
create_pagelist(char __user *buf, size_t count, unsigned short type,
	struct task_struct *task, PAGELIST_T ** ppagelist)
{
	PAGELIST_T *pagelist;
	struct page **pages;
	unsigned long *addrs;
	unsigned int num_pages, offset, i;
	unsigned long addr, base_addr, next_addr;
	void *kaddr;
	size_t size;
	int run, addridx;
	int actual_pages;

	offset = (unsigned int)buf & (PAGE_SIZE - 1);
	num_pages = (count + offset + PAGE_SIZE - 1) / PAGE_SIZE;

	*ppagelist = NULL;

	/* Allocate enough storage to hold the page pointers and the
	** page list */
	pagelist = kmalloc(sizeof(PAGELIST_T) +
		(num_pages * sizeof(unsigned long)) +
		(num_pages * sizeof(pages[0])),
		GFP_KERNEL);

	vchiq_log_trace(vchiq_arm_log_level,
		"create_pagelist %x@%x - %x",
		(unsigned int)count, (unsigned int)buf, (unsigned int)pagelist);
	if (!pagelist)
		return -ENOMEM;

	addrs = pagelist->addrs;
	pages = (struct page **)(addrs + num_pages);

	down_read(&task->mm->mmap_sem);
	actual_pages = get_user_pages(task, task->mm,
				(unsigned long)buf & ~(PAGE_SIZE - 1),
				num_pages,
				(type == PAGELIST_READ)/*Write*/, 0/*Force*/,
				pages, NULL/*vmas*/);
	up_read(&task->mm->mmap_sem);

	if (actual_pages != num_pages) {
		/* This is probably due to the process being killed */
		while (actual_pages > 0) {
			actual_pages--;
			page_cache_release(pages[actual_pages]);
		}
		kfree(pagelist);
		if (actual_pages == 0)
			actual_pages = -ENOMEM;
		return actual_pages;
	}

	pagelist->length = count;
	pagelist->type = type;
	pagelist->offset = offset;

	/* Group the pages into runs of contiguous pages */

	addr = PFN_PHYS(page_to_pfn(pages[0]));
	kaddr = page_address(pages[0]);
	next_addr = addr + PAGE_SIZE;
	addr += offset;
	size = min((unsigned int)(PAGE_SIZE - offset), (unsigned int)count);
	count -= size;

	if (type == PAGELIST_READ) {
		/* Only invalidate whole cache lines */
		unsigned long start = (addr + (CACHE_LINE_SIZE - 1)) &
			~(CACHE_LINE_SIZE - 1);
		unsigned long end = (addr + size) & ~(CACHE_LINE_SIZE - 1);
		if (start < end) {
			outer_inv_range(start, end);
			if (kaddr)
				dmac_map_area(
					kaddr +	(start & (PAGE_SIZE - 1)),
					end - start,
					DMA_FROM_DEVICE);
		}
		count &= ~(CACHE_LINE_SIZE - 1);
	} else {
		if (kaddr)
			dmac_map_area(kaddr + offset, size, DMA_TO_DEVICE);
		outer_clean_range(addr, addr + size);
	}

	addridx = 0;
	run = 0;
	base_addr = addr;

	for (i = 1; i < num_pages; i++) {
		int page_bytes = min((int)PAGE_SIZE, (int)count);
		addr = PFN_PHYS(page_to_pfn(pages[i]));
		kaddr = page_address(pages[i]);

		if (type == PAGELIST_READ) {
			outer_inv_range(addr, addr + page_bytes);
			if (kaddr)
				dmac_map_area(kaddr, page_bytes,
					DMA_FROM_DEVICE);
		} else {
			if (kaddr)
				dmac_map_area(kaddr, page_bytes,
					DMA_TO_DEVICE);
			outer_clean_range(addr, addr + page_bytes);
		}

		if ((addr == next_addr) && (run < (PAGE_SIZE - 1))) {
			size += page_bytes;
			run++;
		} else {
			addrs[addridx] = PHYS_TO_VC((base_addr &
				~(PAGE_SIZE - 1)) + run);
			addridx++;
			base_addr = addr;
			size = page_bytes;
			run = 0;
		}
		count -= page_bytes;
		next_addr = addr + PAGE_SIZE;
	}

	addrs[addridx] = PHYS_TO_VC((base_addr & ~(PAGE_SIZE - 1)) + run);
	addridx++;

	/* Partial cache lines (fragments) require special measures */
	if ((type == PAGELIST_READ) &&
		((pagelist->offset & (CACHE_LINE_SIZE - 1)) ||
		((pagelist->offset + pagelist->length) &
			(CACHE_LINE_SIZE - 1)))) {
		FRAGMENTS_T *fragments;

		if (down_interruptible(&g_free_fragments_sema) != 0) {
			kfree(pagelist);
			return -EINTR;
		}

		WARN_ON(g_free_fragments == NULL);

		down(&g_free_fragments_mutex);
		fragments = (FRAGMENTS_T *) g_free_fragments;
		WARN_ON(fragments == NULL);
		g_free_fragments = *(FRAGMENTS_T **) g_free_fragments;
		g_free_fragments_count--;
		up(&g_free_fragments_mutex);
		pagelist->type = PAGELIST_READ_WITH_FRAGMENTS +
			(fragments - g_fragments_base);
	}

	dmac_map_area(pagelist, (int)(addrs + addridx) - (int)pagelist,
		DMA_TO_DEVICE);
	outer_clean_range(__virt_to_phys((unsigned long)pagelist),
		__virt_to_phys((unsigned long)(addrs + addridx)));

	*ppagelist = pagelist;

	down(&g_free_fragments_mutex);
	g_pagelist_count++;
	up(&g_free_fragments_mutex);

	return 0;
}

static void
memcpy_fragment_to_page(struct page *page, int offset,
	void *fragbuf, int bytes)
{
	char *kaddr;
	kaddr = page_address(page);
	if (kaddr)
		memcpy(kaddr + offset, fragbuf, bytes);
	else {
		dma_addr_t paddr;

		kaddr = kmap(page) + offset;
		paddr = PFN_PHYS(page_to_pfn(page)) + offset;
		/* Flush the cache */
		dmac_map_area(kaddr, bytes, DMA_FROM_DEVICE);
		outer_clean_range(paddr, paddr + bytes);
		memcpy(kaddr, fragbuf, bytes);
		/* Invalidate the cache */
		outer_inv_range(paddr, paddr + bytes);
		dmac_unmap_area(kaddr, bytes, DMA_FROM_DEVICE);
		kunmap(page);
	}
}

static void
free_pagelist(PAGELIST_T *pagelist, int actual)
{
	struct page **pages;
	unsigned int num_pages, i;
	int len, offset;

	vchiq_log_trace(vchiq_arm_log_level,
		"free_pagelist - %x, %d",
		(unsigned int)pagelist, actual);

	len = pagelist->length;
	offset = pagelist->offset;

	num_pages =
		 (len + offset + PAGE_SIZE - 1) / PAGE_SIZE;

	pages = (struct page **)(pagelist->addrs + num_pages);

	/* Deal with any partial cache lines (fragments) */
	if (pagelist->type >= PAGELIST_READ_WITH_FRAGMENTS) {
		FRAGMENTS_T *fragments =
			 g_fragments_base + (pagelist->type -
					PAGELIST_READ_WITH_FRAGMENTS);
		int head_bytes, tail_bytes;

		if (actual >= 0) {
			head_bytes = (CACHE_LINE_SIZE - pagelist->offset) &
				(CACHE_LINE_SIZE - 1);
			tail_bytes = (pagelist->offset + actual) &
				(CACHE_LINE_SIZE - 1);
			if (head_bytes != 0) {
				if (head_bytes > actual)
					head_bytes = actual;
				memcpy_fragment_to_page(pages[0], offset,
					fragments->headbuf, head_bytes);
				offset += head_bytes;
				len -= head_bytes;
			}
			if ((head_bytes < actual) && (tail_bytes != 0)) {
				memcpy_fragment_to_page(pages[num_pages - 1],
					((pagelist->offset + actual) &
					(PAGE_SIZE - 1) &
					~(CACHE_LINE_SIZE - 1)),
					fragments->tailbuf, tail_bytes);
				len -= tail_bytes;
			}
		}

		down(&g_free_fragments_mutex);
		*(FRAGMENTS_T **) fragments = g_free_fragments;
		g_free_fragments = fragments;
		g_free_fragments_count++;
		up(&g_free_fragments_mutex);
		up(&g_free_fragments_sema);
	}

	for (i = 0; i < num_pages; i++) {
		if (pagelist->type != PAGELIST_WRITE) {
			int page_bytes = PAGE_SIZE - offset;
			if (page_bytes > len)
				page_bytes = len;
			if (page_bytes) {
				void *kaddr = page_address(pages[i]);
				unsigned long addr =
					PFN_PHYS(page_to_pfn(pages[i])) +
					offset;
				if (kaddr)
					dmac_unmap_area((char *)kaddr + offset,
						page_bytes, DMA_FROM_DEVICE);
				outer_inv_range(addr, addr + page_bytes);
				len -= page_bytes;
			}
			set_page_dirty(pages[i]);
			offset = 0;
		}
		page_cache_release(pages[i]);
	}

	kfree(pagelist);

	down(&g_free_fragments_mutex);
	g_pagelist_count--;
	up(&g_free_fragments_mutex);
}
