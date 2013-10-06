/*****************************************************************************
*  Copyright 2001 - 2012 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/crypto.h>
#include <linux/cryptohash.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <plat/clock.h>
#include <asm/byteorder.h>
#include <crypto/sha.h>
#include <crypto/md5.h>
#include <crypto/hash.h>
#include <crypto/internal/hash.h>
#include <crypto/algapi.h>
#include <crypto/scatterwalk.h>
#include <mach/dma_mmap.h>
#include <mach/sdma.h>
#include <mach/rdb/brcm_rdb_spum_apb.h>
#include <mach/rdb/brcm_rdb_spum_axi.h>
#include "brcm_spum.h"

#define KNLLOG_DEBUG               0
#if KNLLOG_DEBUG
#include <linux/broadcom/knllog.h>
#else
#define KNLLOG(x, ...)
#endif

#define SPUM_MAX_PAYLOAD_PER_CMD    (SZ_64K - SZ_64)
#define SPUM_HASH_QUEUE_LENGTH      300

#define ALIGN_BOUNDARY              64
#define ALIGN_BUF(x) (((uint32_t)x + (ALIGN_BOUNDARY-1)) & ~(ALIGN_BOUNDARY-1))

/* When using init, update, final, wait for the following
	minimum size before executing SPUM. Must be a multiple
	of block size (64-bytes). */
#define SPUM_MIN_PROC_SIZE          256

#define b_host_2_be32(__buf, __size)        \
do {                                        \
	uint32_t *b = (uint32_t *)(__buf);  \
	uint32_t i;                         \
	for (i = 0; i < __size; i++, b++) { \
		*b = cpu_to_be32(*b);       \
	}                                   \
} while (0)

enum spum_op {
	SPUM_OP_UPDATE = 0,
	SPUM_OP_FINAL,
	SPUM_OP_FINUP,
	SPUM_OP_DIGEST
};

struct spum_stats_op {
	uint32_t update;
	uint32_t final;
	uint32_t finup;
	uint32_t digest;
	uint64_t bytes;
};

struct spum_dir {
	DMA_Device_t device;
	dma_addr_t fifo_addr;
};

struct spum_dev {
	struct device *dev;
	void __iomem *io_apb_base;
	void __iomem *io_axi_base;
	struct clk *clk;
	struct spum_dir rx;
	struct spum_dir tx;
	struct semaphore proc_sem;
	struct semaphore done_sem;
	struct task_struct *proc_thread;
	struct task_struct *done_thread;
	struct ahash_request *req; /* Current request being processed */
	bool thread_exit;
	spinlock_t lock;
	struct crypto_queue queue;
	atomic_t busy;
	struct spum_stats_op *curr_stats;
	struct spum_stats_op sha1;
	struct spum_stats_op sha224;
	struct spum_stats_op sha256;
	struct spum_stats_op md5;
};

struct hash_ctx {
	struct spum_dev *spum;
};

enum spum_buf_type {
	SPUM_BUF_TYPE_KMALLOC = 0,
	SPUM_BUF_TYPE_SG,
	SPUM_BUF_TYPE_USER,
	SPUM_BUF_TYPE_USER_NEED_FREE
};

struct hash_rctx_dir {
	uint32_t size;
	SDMA_Handle_t dma_handle;
	bool dma_mmap_started;
	DMA_MMAP_CFG_T dma_mmap;
	atomic_t done;
};

struct hash_rctx {
	enum spum_op op;
	bool init;

	/* Collection buffer */
	uint8_t *collect_buf;
	uint32_t collect_size;

	/* Operation block size (64-bytes likely) */
	uint32_t block_size;

	/* Hold intermediate digest values and status */
	uint8_t digest_buf[SHA512_DIGEST_SIZE +
		SPUM_OUTPUT_STATUS_LEN + (ALIGN_BOUNDARY-1)];
	uint32_t digest_size;

	/* SPU Algorithm */
	spum_auth_algo auth_algo;

	/* Number of bytes sent */
	uint32_t num_bytes_sent;

	/* RX/TX */
	struct list_head buf_list;
	struct hash_rctx_dir tx;
	struct hash_rctx_dir rx;

	/* Hold intermediate SG location when processing
	 * large block sizes */
	struct scatterlist *sg_save;
	struct scatterlist *sg_walk;
	uint32_t sg_offset;
	uint32_t sg_len;

	/* Still need to send pad? */
	bool pad_send;
};

struct dma_buf {
	uint8_t *orig_buf;
	uint8_t *buf;
	uint32_t len;
	uint32_t use_count;
	enum spum_buf_type type;
	struct list_head list;
	struct scatterlist *sg;
};

static struct spum_dev *spum_dev;
static struct proc_dir_entry *hash_proc_entry;

static int hash_proc_read(char *buf, char **start, off_t offset, int count,
			int *eof, void *data)
{
	char *p = buf;
	struct spum_stats_op *op;

	(void)start;
	(void)count;
	(void)data;

	if (offset > 0) {
		*eof = 1;
		return 0;
	}

	op = &spum_dev->sha1;
	p += sprintf(p,
		"sha1: update=%u final=%u finup=%u digest=%u bytes=%llu\n",
		op->update, op->final, op->finup, op->digest, op->bytes);
	op = &spum_dev->sha224;
	p += sprintf(p,
		"sha224: update=%u final=%u finup=%u digest=%u bytes=%llu\n",
		op->update, op->final, op->finup, op->digest, op->bytes);
	op = &spum_dev->sha256;
	p += sprintf(p,
		"sha256: update=%u final=%u finup=%u digest=%u bytes=%llu\n",
		op->update, op->final, op->finup, op->digest, op->bytes);
	op = &spum_dev->md5;
	p += sprintf(p,
		"md5: update=%u final=%u finup=%u digest=%u bytes=%llu\n",
		op->update, op->final, op->finup, op->digest, op->bytes);

	*eof = 1;
	return p - buf;
}

static int save_sg_list(struct hash_rctx *rctx, struct scatterlist *sgl)
{
	int i;
	int nents = 0;
	struct scatterlist *sg, *sg_save;

	sg = sgl;
	while (sg) {
		nents++;
		sg = sg_next(sg);
	}

	rctx->sg_save = kmalloc(nents * sizeof(struct scatterlist), GFP_KERNEL);
	if (!rctx->sg_save) {
		printk(KERN_ERR
			"%s: Failed to allocate buffer to save SG list\n",
			__func__);
		return -ENOMEM;
	}

	sg_save = rctx->sg_save;
	sg_init_table(rctx->sg_save, nents);
	for_each_sg(sgl, sg, nents, i) {
		sg_save->length = sg->length;
		sg_save->page_link = sg->page_link;
		sg_save->offset = sg->offset;
		sg_save = sg_next(sg_save);
	}

	return 0;
}

static struct dma_buf *dma_buf_sg(
		struct spum_dev *spum,
		struct scatterlist *sg,
		uint32_t offset,
		uint32_t size_in_bytes)
{
	struct dma_buf *buf;

	buf = kzalloc(sizeof(struct dma_buf), GFP_KERNEL);
	if (buf) {
		dma_map_sg(spum->dev, sg, 1, DMA_TO_DEVICE);
		buf->sg = sg;
		buf->buf = (uint8_t *)sg_dma_address(sg);
		buf->len = size_in_bytes;
		buf->type = SPUM_BUF_TYPE_SG;
	}

	KNLLOG("dma_buf=0x%x buf=0x%x len=%u\n",
		(uint32_t)buf, (uint32_t)buf->buf, buf->len);

	return buf;
}

static struct dma_buf *dma_buf_user(
		void *buffer, uint32_t size_in_bytes, bool buf_need_free)
{
	struct dma_buf *buf;

	buf = kzalloc(sizeof(struct dma_buf), GFP_KERNEL);
	if (buf) {
		buf->orig_buf = buffer;
		buf->buf = buffer;
		buf->len = size_in_bytes;
		buf->type = buf_need_free ? SPUM_BUF_TYPE_USER_NEED_FREE :
			SPUM_BUF_TYPE_USER;
	}

	KNLLOG("dma_buf=0x%x buf=0x%x len=%u\n",
		(uint32_t)buf, (uint32_t)buf->buf, buf->len);

	return buf;
}

static struct dma_buf *dma_buf_kmalloc(uint32_t size_in_bytes)
{
	struct dma_buf *buf;

	buf = kzalloc(sizeof(struct dma_buf), GFP_KERNEL);
	if (buf) {
		buf->orig_buf =
			kzalloc(size_in_bytes + (ALIGN_BOUNDARY-1), GFP_KERNEL);
		if (buf->orig_buf == NULL) {
			printk(KERN_ERR "%s: Failed to allocate buf! size=%u\n",
					__func__, size_in_bytes);
			kfree(buf);
			return NULL;
		}
		buf->buf = (uint8_t *)ALIGN_BUF(buf->orig_buf);
		buf->len = size_in_bytes;
		buf->type = SPUM_BUF_TYPE_KMALLOC;
	}

	KNLLOG("dma_buf=0x%x orig_buf=0x%x buf=0x%x len=%u\n",
		(uint32_t)buf, (uint32_t)buf->orig_buf,
		(uint32_t)buf->buf, buf->len);

	return buf;
}

static void dma_buf_free(struct spum_dev *spum, struct dma_buf *buf)
{
	if (buf == NULL)
		return;

	KNLLOG("Free dma_buf buf=0x%x cnt=%u\n",
		(uint32_t)buf, buf->use_count);

	if (buf->use_count)
		buf->use_count--;

	if (buf->use_count)
		return;

	if (buf->type == SPUM_BUF_TYPE_KMALLOC ||
		buf->type == SPUM_BUF_TYPE_USER_NEED_FREE) {
		kfree(buf->orig_buf);
		buf->orig_buf = NULL;
		buf->buf = NULL;
	} else if (buf->type == SPUM_BUF_TYPE_SG) {
		dma_unmap_sg(spum->dev, buf->sg, 1, DMA_TO_DEVICE);
	} else if (buf->type == SPUM_BUF_TYPE_USER) {
		/* Nothing to do */
	}

	/* Free dma_buf */
	kfree(buf);
}

static int dma_enq(struct hash_rctx *rctx, struct hash_rctx_dir *dir,
		struct dma_buf *buf, enum dma_data_direction dma_dir)
{
	int rc = 0;

	/* dma_mmap buffer */
	if (buf->type != SPUM_BUF_TYPE_SG &&
		!dma_mmap_dma_is_supported(buf->buf)) {
		printk(KERN_ERR "%s: Unsupported buffer! buf=0x%x\n",
				__func__, (uint32_t)buf);
		rc = -EINVAL;
		goto exit;
	}

	buf->use_count++;

	if (!dir->dma_mmap_started) {
		KNLLOG("dma_mmap_map buf=0x%x\n", (uint32_t)buf);
		rc = dma_mmap_init_map(&dir->dma_mmap);
		if (rc < 0) {
			printk(KERN_ERR "%s: dma_mmap_init_map failed\n",
					__func__);
			goto exit;
		}

		rc = dma_mmap_map(&dir->dma_mmap, buf->buf, buf->len, dma_dir);
		if (rc < 0) {
			printk(KERN_ERR "%s: dma_mmap_map failed\n", __func__);
			goto exit;
		}
		dir->dma_mmap_started = 1;
	} else {
		KNLLOG("dma_mmap_add_region buf=0x%x\n", (uint32_t)buf);
		if (buf->type != SPUM_BUF_TYPE_SG)
			rc = dma_mmap_add_region(&dir->dma_mmap,
					buf->buf, buf->len);
		else
			rc = dma_mmap_add_phys_region(&dir->dma_mmap,
					buf->buf, buf->len);
		if (rc < 0) {
			printk(KERN_ERR "dma_mmap_add_region failed\n");
			goto exit;
		}
	}

	/* Add to buffer list to free/unmap later */
	if (buf->list.next == NULL)
		list_add_tail(&buf->list, &rctx->buf_list);

exit:
	KNLLOG("rc=%d dma_buf=0x%x\n", rc, (uint32_t)buf);

	return rc;
}

static int dma_enq_rx(struct hash_rctx *rctx, struct dma_buf *buf)
{
	KNLLOG("enter buf=0x%x\n", (uint32_t)buf);

	return dma_enq(rctx, &rctx->rx, buf, DMA_FROM_DEVICE);
}

static int dma_enq_tx(struct hash_rctx *rctx, struct dma_buf *buf)
{
	KNLLOG("enter buf=0x%x\n", (uint32_t)buf);

	return dma_enq(rctx, &rctx->tx, buf, DMA_TO_DEVICE);
}

static void dma_irq(DMA_Device_t dev, int reason, void *data)
{
	struct spum_dev *spum = (struct spum_dev *)data;
	struct hash_rctx *rctx = ahash_request_ctx(spum->req);

	(void)reason;

	KNLLOG("dev=%u\n", dev);

	if (dev == spum->rx.device)
		atomic_set(&rctx->rx.done, 1);
	else
		atomic_set(&rctx->tx.done, 1);

	if (atomic_read(&rctx->rx.done) && atomic_read(&rctx->tx.done)) {
		KNLLOG("Signal done thread\n");
		up(&spum->done_sem);
	}
}

static void dma_cleanup(struct hash_rctx_dir *dir,
		enum dma_data_direction dma_dir)
{
	sdma_free_channel(dir->dma_handle);

	/* Unmap dma_mmap */
	dma_mmap_unmap(&dir->dma_mmap, (dma_dir == DMA_TO_DEVICE) ?
			DMA_MMAP_CLEAN : DMA_MMAP_DIRTIED);

	/* Terminate dma_mmap */
	dma_mmap_term_map(&dir->dma_mmap);

	atomic_set(&dir->done, 0);
	dir->dma_mmap_started = 0;
}

static void list_cleanup(struct spum_dev *spum, struct hash_rctx *rctx)
{
	struct dma_buf *buf;
	struct list_head *buf_walk, *buf_walk_next;

	/* Empty active list */
	list_for_each_safe(buf_walk, buf_walk_next, &rctx->buf_list) {
		buf = list_entry(buf_walk, struct dma_buf, list);
		list_del(&buf->list);
		dma_buf_free(spum, buf);
	}
}

static int execute_spum(struct spum_dev *spum)
{
	int rc;
	struct ahash_request *req = spum->req;
	struct hash_rctx *rctx = ahash_request_ctx(req);
	struct hash_rctx_dir *tx = &rctx->tx;
	struct hash_rctx_dir *rx = &rctx->rx;

	KNLLOG("enter\n");

	clk_enable(spum->clk);

	/* Initialize SPUM block */
	spum_init_device(spum->io_apb_base, spum->io_axi_base);

	spum_set_pkt_length(spum->io_axi_base, rctx->tx.size, rctx->rx.size);

	/* Reserve TX/RX DMA channels */
	tx->dma_handle = sdma_request_channel(spum->tx.device);
	if (tx->dma_handle < 0) {
		printk(KERN_ERR "%s: sdma_request_channel tx failed.\n",
				__func__);
		return -EBUSY;
	}
	rx->dma_handle = sdma_request_channel(spum->rx.device);
	if (rx->dma_handle < 0) {
		printk(KERN_ERR "%s: sdma_request_channel rx failed.\n",
				__func__);
		sdma_free_channel(tx->dma_handle);
		return -EBUSY;
	}
	rc = sdma_set_device_handler(spum->rx.device, dma_irq, spum);
	if (rc) {
		printk(KERN_ERR "%s: sdma_set_device_handler failed\n",
				__func__);
		goto exit;
	}
	rc = sdma_set_device_handler(spum->tx.device, dma_irq, spum);
	if (rc) {
		printk(KERN_ERR "%s: sdma_set_device_handler failed\n",
				__func__);
		goto exit;
	}

	/* Create SDMA descriptor rings */
	rc = sdma_map_create_descriptor_ring(tx->dma_handle,
			&tx->dma_mmap,
			spum->tx.fifo_addr,
			DMA_UPDATE_MODE_NO_INC);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: tx sdma_map_create_descriptor_ring failed\n",
			__func__);
		goto exit;
	}
	rc = sdma_map_create_descriptor_ring(rx->dma_handle,
			&rx->dma_mmap,
			spum->rx.fifo_addr,
			DMA_UPDATE_MODE_NO_INC);
	if (rc < 0) {
		printk(KERN_ERR
			"%s: rx sdma_map_create_descriptor_ring failed\n",
			__func__);
		goto exit;
	}

	/* tell the DMA ready for transfer */
	rc = sdma_start_transfer(tx->dma_handle);
	if (rc < 0) {
		printk(KERN_ERR "%s: tx sdma_start_transfer failed\n",
				__func__);
		goto exit;
	}
	rc = sdma_start_transfer(rx->dma_handle);
	if (rc < 0) {
		printk(KERN_ERR "%s: rx sdma_start_transfer failed\n",
				__func__);
		goto exit_stop_xfer;
	}

	/* Ignite the SPU FIFO for DMA */
	spum_dma_init(spum->io_axi_base);

	KNLLOG("exit\n");

	return 0;

exit_stop_xfer:
	sdma_stop_transfer(tx->dma_handle);
exit:
	sdma_free_channel(tx->dma_handle);
	sdma_free_channel(rx->dma_handle);

	list_cleanup(spum, rctx);

	KNLLOG("error exit\n");

	return -EIO;
}

static int proc_thread(void *data)
{
	struct crypto_async_request *async_req, *backlog;
	struct spum_dev *spum = (struct spum_dev *)data;
	struct ahash_request *req;
	int rc;
	unsigned long flags;

	KNLLOG("entry\n");

	while (1) {
		if (!down_interruptible(&spum->proc_sem)) {
			if (spum->thread_exit)
				break;

			KNLLOG("signalled\n");

			/* If SPUM is already busy, return */
			if (atomic_read(&spum->busy))
				continue;

			spin_lock_irqsave(&spum->lock, flags);
			backlog = crypto_get_backlog(&spum->queue);
			async_req = crypto_dequeue_request(&spum->queue);
			spin_unlock_irqrestore(&spum->lock, flags);

			if (!async_req)
				continue;

			atomic_set(&spum->busy, 1);

			if (backlog)
				backlog->complete(backlog, -EINPROGRESS);

			req = ahash_request_cast(async_req);
			spum->req = req;

			rc = execute_spum(spum);
			if (rc) {
				printk(KERN_ERR "%s: failed rc=%d\n",
					__func__, rc);
				continue;
			}
		}
	}

	KNLLOG("exit\n");
	return 0;
}

static void send_collect(struct hash_rctx *rctx)
{
	struct dma_buf *buf;
	uint32_t send_len;

	KNLLOG("Send collect buf size=%u\n", rctx->collect_size);

	send_len = ((rctx->collect_size + 3) / sizeof(uint32_t)) *
			sizeof(uint32_t);

	/* Send off the collection buffer */
	buf = dma_buf_user(rctx->collect_buf, send_len, 1);
	dma_enq_tx(rctx, buf);
	rctx->collect_buf = NULL; rctx->collect_size = 0;
}

static int collect(struct hash_rctx *rctx, uint8_t *src, uint32_t len)
{
	int rc = 0;
	uint32_t collect_len =
		((len + rctx->collect_size) > SPUM_MIN_PROC_SIZE) ?
		(SPUM_MIN_PROC_SIZE - rctx->collect_size) : len;

	if (rctx->collect_size == 0) {
		rctx->collect_buf = kmalloc(SPUM_MIN_PROC_SIZE, GFP_KERNEL);
		if (rctx->collect_buf == NULL) {
			printk(KERN_ERR
				"%s: Failed to allocate collection buffer.\n",
				__func__);
			return -ENOMEM;
		}
	}

	memcpy(rctx->collect_buf + rctx->collect_size, src, collect_len);

	rctx->collect_size += collect_len;

	if (rctx->collect_size == SPUM_MIN_PROC_SIZE) {
		send_collect(rctx);

		if (len - collect_len)
			rc = collect(rctx, src + collect_len,
					len - collect_len);
	}

	return rc;
}

static int walk_and_collect(struct hash_rctx *rctx, struct scatterlist *sg,
		uint32_t offset, uint32_t bytes)
{
	int rc;
	struct scatterlist *sg_walk = sg;
	uint32_t bytes_collected = 0;
	void *sg_vaddr;

	while (sg_walk) {
		int copy_len;

		copy_len = min((uint32_t)(bytes - bytes_collected),
				(sg_walk->length - offset));
		if (copy_len == 0)
			break;

		sg_vaddr = crypto_kmap(sg_page(sg_walk), 0) + sg_walk->offset;
		rc = collect(rctx, (uint8_t *)sg_vaddr + offset,
				copy_len);
		crypto_kunmap(sg_vaddr, 0);
		if (rc)
			return 0;

		bytes_collected += copy_len;
		offset = 0;
		sg_walk = sg_next(sg_walk);
	}

	return bytes_collected;
}

static int process_data(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct hash_ctx *ctx = crypto_ahash_ctx(tfm);
	struct hash_rctx *rctx = ahash_request_ctx(req);
	int rc;
	struct spum_hw_context spu_ctx;
	struct dma_buf *buf;
	int total_bytes = 0, partial_bytes = 0, bytes_to_send = 0;
	int sg_offset = 0;
	struct scatterlist *sg_walk = NULL;
	bool large_payload = 0;
	int cmd_bytes;
	uint8_t *cmd_buf;
	int pad_size = 0;
	unsigned long flags;

	KNLLOG("entry\n");

	if (rctx->sg_len) {
		total_bytes = rctx->sg_len;
		sg_offset = rctx->sg_offset;
		sg_walk = rctx->sg_walk;

		rctx->sg_len = 0;
		rctx->sg_offset = 0;
		rctx->sg_walk = NULL;
	} else {
		total_bytes = (rctx->op == SPUM_OP_FINAL || rctx->pad_send) ?
			rctx->collect_size : (rctx->collect_size + req->nbytes);
		sg_offset = 0;

		if (rctx->op != SPUM_OP_FINAL) {
			/* Save the SG list in case the SG list is not
			 * persistant from the source (e.g. allocated from
			 * stack). The SG list may need to be accessed later
			 * for large payloads * and not in the context of
			 * the caller. */
			save_sg_list(rctx, req->src);
			sg_walk = rctx->sg_save;
		}
	}

	if (total_bytes > SPUM_MAX_PAYLOAD_PER_CMD) {
		large_payload = 1;
		bytes_to_send = SPUM_MAX_PAYLOAD_PER_CMD;
		partial_bytes = total_bytes - bytes_to_send;
	} else if (rctx->op == SPUM_OP_FINAL ||
		rctx->op == SPUM_OP_FINUP ||
		rctx->op == SPUM_OP_DIGEST) {
		partial_bytes = 0;
		bytes_to_send = total_bytes;
	} else {
		partial_bytes = total_bytes % rctx->block_size;
		bytes_to_send = total_bytes - partial_bytes;
	}

	/* Log number of bytes sent */
	rctx->num_bytes_sent += bytes_to_send;

	KNLLOG("partial_bytes=%u bytes_to_send=%u total_bytes=%u large=%u\n",
		partial_bytes, bytes_to_send, total_bytes, large_payload);

	memset(&spu_ctx, 0, sizeof(spu_ctx));
	spu_ctx.operation   = SPUM_CRYPTO_ENCRYPTION;
	spu_ctx.crypto_algo = SPUM_CRYPTO_ALGO_NULL;
	spu_ctx.auth_mode   = SPUM_AUTH_MODE_HASH;
	spu_ctx.auth_algo   = rctx->auth_algo;

	if (large_payload || rctx->op == SPUM_OP_UPDATE) {
		/* UPDATE or the need for UPDATES due to large payload */
		spu_ctx.auth_type = rctx->init ? SPUM_AUTH_TYPE_SHA1_UPDATE
				: SPUM_AUTH_TYPE_SHA1_INIT;
	} else if (rctx->init) {
		/* FINAL operation after UPDATES.  Padding is required */
		spu_ctx.auth_type = SPUM_AUTH_TYPE_SHA1_UPDATE;
		if (SPUM_MAX_PAYLOAD_PER_CMD - (bytes_to_send % 64) >= 8) {
			/* There is enough room to pad the length */
			pad_size = (bytes_to_send % 64) < 56 ?
					(64 - (bytes_to_send % 64)) :
					((64 + 64) - (bytes_to_send % 64));
		} else {
			/* Need another update to pad */
			pad_size = bytes_to_send % 64;
			rctx->pad_send = 1;
		}
	} else {
		/* Single FULL operation is sufficient */
		spu_ctx.auth_type = SPUM_AUTH_TYPE_SHA1_FULL;
	}

	spu_ctx.auth_order                 = SPUM_CMD_AUTH_FIRST;
	spu_ctx.key_type                   = SPUM_KEY_OPEN;
	spu_ctx.icv_len                    =
		(rctx->digest_size / sizeof(uint32_t));
	if ((rctx->digest_size == SHA224_DIGEST_SIZE) &&
			(spu_ctx.auth_type != SPUM_AUTH_TYPE_SHA1_FULL))
		/* SHA224 intermediate digest size is 256 bit */
		spu_ctx.icv_len += 1;
	spu_ctx.data_attribute.mac_offset  = 0;
	spu_ctx.data_attribute.mac_length  = bytes_to_send + pad_size;
	spu_ctx.data_attribute.data_length =
		((bytes_to_send + pad_size + 3) / sizeof(uint32_t)) *
		sizeof(uint32_t);

	if (rctx->init) {
		spu_ctx.auth_key     = (void *)ALIGN_BUF(rctx->digest_buf);
		spu_ctx.auth_key_len = (rctx->digest_size / sizeof(uint32_t));
		if (rctx->digest_size == SHA224_DIGEST_SIZE)
			/* SHA224 intermediate digest size is 256 bit */
			spu_ctx.auth_key_len += 1;
	}

	KNLLOG("algo=%u auth_type=%u auth_key=0x%x auth_key_len=%u length=%u\n",
			spu_ctx.auth_algo,
			spu_ctx.auth_type,
			(uint32_t)spu_ctx.auth_key,
			spu_ctx.auth_key_len,
			spu_ctx.data_attribute.mac_length);

	/* TX: Command */
	cmd_buf = kmalloc(512, GFP_KERNEL);
	cmd_bytes = spum_format_command(&spu_ctx, cmd_buf);
	buf = dma_buf_user(cmd_buf, cmd_bytes, 1);
	dma_enq_tx(rctx, buf);

	/* TX: Walk SG */
	bytes_to_send -= rctx->collect_size;
	while (sg_walk && bytes_to_send) {
		int copy_len, sg_offset_new = 0;

		copy_len = min(req->nbytes, (sg_walk->length - sg_offset));
		if (copy_len > bytes_to_send) {
			copy_len = bytes_to_send;
			sg_offset_new = sg_offset + copy_len;
		}
		if (copy_len == 0)
			break;

		KNLLOG("bytes_to_send=%u copy_len=%u collect_size=%u\n",
			bytes_to_send, copy_len, rctx->collect_size);
		KNLLOG("sg_virt=0x%x sg_offset=%u sg_offset_new=%u sg_len=%u\n",
			(int)sg_virt(sg_walk), sg_offset, sg_offset_new,
			sg_walk->length);

		/* If collection buffer is not aligned, we have to add to it. */
		if (((rctx->collect_size % (4 * 4)) != 0) ||
			/* Or if SG buffer cannot be directly DMA'ed */
			(((uint32_t)((uint8_t *)sg_walk->offset +
			sg_offset) % 4) != 0) ||
			((copy_len % (4 * 4)) != 0)) {
			uint8_t *sg_vaddr;

			KNLLOG("Collect\n");

			sg_vaddr = crypto_kmap(sg_page(sg_walk), 0) +
				sg_walk->offset;
			collect(rctx, (uint8_t *)sg_vaddr + sg_offset,
					copy_len);
			crypto_kunmap(sg_vaddr, 0);
		} else {
			KNLLOG("Send SG\n");
			/* Queue Collection buffer if needed. */
			if (rctx->collect_size)
				send_collect(rctx);

			/* Queue SG */
			buf = dma_buf_sg(ctx->spum, sg_walk,
					sg_offset, copy_len);
			dma_enq_tx(rctx, buf);
		}
		bytes_to_send -= copy_len;

		sg_offset = sg_offset_new;
		if (sg_offset == 0)
			sg_walk = sg_next(sg_walk);
	}

	/* TX: Padding */
	if (pad_size) {
		uint64_t bits_le, bits_be;
		uint8_t pad[128];
		memset(pad, 0, sizeof(pad));
		pad[0] = 0x80;

		collect(rctx, pad, pad_size < 8 ? pad_size : pad_size - 8);
		if (pad_size >= 8) {
			bits_le = (uint64_t)rctx->num_bytes_sent << 3;
			bits_be = cpu_to_be64(bits_le);

			if (rctx->digest_size == MD5_DIGEST_SIZE)
				collect(rctx, (uint8_t *)&bits_le, 8);
			else
				collect(rctx, (uint8_t *)&bits_be, 8);
		}
	}

	/* TX: Collection buffer */
	if (rctx->collect_size)
		send_collect(rctx);

	/* TX: Status */
	buf = dma_buf_kmalloc(SPUM_OUTPUT_STATUS_LEN);
	memset(buf->buf, 0, SPUM_OUTPUT_STATUS_LEN);
	dma_enq_tx(rctx, buf);

	/* RX: Header + Data */
	buf = dma_buf_kmalloc(SPUM_OUTPUT_HEADER_LEN +
			spu_ctx.data_attribute.data_length);
	dma_enq_rx(rctx, buf);

	/* RX: Digest buffer + Status */
	buf = dma_buf_user((void *)ALIGN_BUF(rctx->digest_buf),
			(spu_ctx.icv_len * sizeof(uint32_t)) +
			SPUM_OUTPUT_STATUS_LEN, 0);
	dma_enq_rx(rctx, buf);

	/* Save left over data */
	if (large_payload) {
		rctx->sg_walk = sg_walk;
		rctx->sg_offset = sg_offset;
		rctx->sg_len = partial_bytes;
	} else if (partial_bytes) {
		walk_and_collect(rctx, sg_walk, sg_offset, partial_bytes);
	}

	/* Log sizes */
	rctx->tx.size = cmd_bytes + spu_ctx.data_attribute.data_length;
	rctx->rx.size = SPUM_OUTPUT_HEADER_LEN +
			 spu_ctx.data_attribute.data_length +
			(spu_ctx.icv_len * sizeof(uint32_t)) +
			 SPUM_OUTPUT_STATUS_LEN;

	/* Remember INIT is done */
	if (spu_ctx.auth_type == SPUM_AUTH_TYPE_SHA1_INIT)
		rctx->init = 1;

	/* Submit to SPUM */
	spin_lock_irqsave(&ctx->spum->lock, flags);
	rc = ahash_enqueue_request(&ctx->spum->queue, req);
	spin_unlock_irqrestore(&ctx->spum->lock, flags);

	/* Signal processing thread */
	if (!atomic_read(&ctx->spum->busy))
		up(&ctx->spum->proc_sem);

	return rc;
}

static int done_thread(void *data)
{
	struct spum_dev *spum = (struct spum_dev *)data;
	struct hash_rctx *rctx;

	while (1) {
		if (!down_interruptible(&spum->done_sem)) {
			if (spum->thread_exit)
				break;

			KNLLOG("signalled\n");

			rctx = ahash_request_ctx(spum->req);

			/* Cleanup */
			dma_cleanup(&rctx->rx, DMA_FROM_DEVICE);
			dma_cleanup(&rctx->tx, DMA_TO_DEVICE);

			list_cleanup(spum, rctx);

			clk_disable(spum->clk);

			/* Process additional data or send padding */
			if (rctx->pad_send || rctx->sg_len) {
				process_data(spum->req);
				continue;
			}

			/* Delete saved SG list */
			kfree(rctx->sg_save);
			rctx->sg_save = NULL;

			/* Copy result if there is a buffer in the request */
			if (spum->req->result) {
				memcpy(spum->req->result,
					(void *)ALIGN_BUF(rctx->digest_buf),
					rctx->digest_size);

				if (rctx->digest_size == MD5_DIGEST_SIZE
					&& rctx->init)
					b_host_2_be32(spum->req->result,
						rctx->digest_size /
						sizeof(uint32_t));
			}

			/* Indicate completion */
			if (spum->req->base.complete) {
				KNLLOG("Completing Request...\n");
				spum->req->base.complete(&spum->req->base, 0);
			}

			atomic_set(&spum->busy, 0);

			/* Check if there is more work */
			up(&spum->proc_sem);
		}
	}

	KNLLOG("exit\n");
	return 0;
}

static int hash_init(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct hash_ctx *ctx = crypto_ahash_ctx(tfm);
	struct hash_rctx *rctx = ahash_request_ctx(req);

	KNLLOG("enter nbytes=%u\n", req->nbytes);

	/* Assign the only SPUM device to the context */
	ctx->spum = spum_dev;

	memset(rctx, 0, sizeof(struct hash_rctx));

	/* SPUM only supports hash operations with block
		size up to 64 bytes */
	rctx->block_size = SHA256_BLOCK_SIZE;
	rctx->digest_size = crypto_ahash_digestsize(tfm);

	switch (rctx->digest_size) {
	case SHA1_DIGEST_SIZE:
		rctx->auth_algo = SPUM_AUTH_ALGO_SHA1;
		ctx->spum->curr_stats = &ctx->spum->sha1;
		break;

	case SHA224_DIGEST_SIZE:
		rctx->auth_algo = SPUM_AUTH_ALGO_SHA224;
		ctx->spum->curr_stats = &ctx->spum->sha224;
		break;

	case SHA256_DIGEST_SIZE:
		rctx->auth_algo = SPUM_AUTH_ALGO_SHA256;
		ctx->spum->curr_stats = &ctx->spum->sha256;
		break;

	case MD5_DIGEST_SIZE:
		rctx->auth_algo = SPUM_AUTH_ALGO_MD5;
		ctx->spum->curr_stats = &ctx->spum->md5;
		break;

	default:
		break;
	}

	/* Initialize lists */
	INIT_LIST_HEAD(&rctx->buf_list);

	KNLLOG("exit\n");

	return 0;
}

static int hash_update(struct ahash_request *req)
{
	struct hash_rctx *rctx = ahash_request_ctx(req);
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct hash_ctx *ctx = crypto_ahash_ctx(tfm);

	KNLLOG("enter collect_size=%u nbytes=%u\n",
			rctx->collect_size, req->nbytes);

	if (req->nbytes == 0)
		return 0;

	rctx->op = SPUM_OP_UPDATE;

	ctx->spum->curr_stats->update++;
	ctx->spum->curr_stats->bytes += req->nbytes;

	/* Need to collect 1 block worth of data.  We copy it out since I cannot
	 * gaurantee the supplied buffers persist after this update call. */
	if (rctx->collect_size + req->nbytes < SPUM_MIN_PROC_SIZE) {
		walk_and_collect(rctx, req->src, 0, req->nbytes);
		return 0;
	}

	/* There is enough data to start processing */
	return process_data(req);
}

static int hash_final(struct ahash_request *req)
{
	struct hash_rctx *rctx = ahash_request_ctx(req);
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct hash_ctx *ctx = crypto_ahash_ctx(tfm);

	KNLLOG("enter bytes=%u\n", req->nbytes);

	rctx->op = SPUM_OP_FINAL;

	ctx->spum->curr_stats->final++;

	/* Process remaining data (stored in collection buffer) */
	return process_data(req);
}

static int hash_finup(struct ahash_request *req)
{
	struct hash_rctx *rctx = ahash_request_ctx(req);
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct hash_ctx *ctx = crypto_ahash_ctx(tfm);

	KNLLOG("enter bytes=%u\n", req->nbytes);

	rctx->op = SPUM_OP_FINUP;

	ctx->spum->curr_stats->finup++;
	ctx->spum->curr_stats->bytes += req->nbytes;

	/* Process the data */
	return process_data(req);
}

static int hash_digest(struct ahash_request *req)
{
	struct hash_rctx *rctx = ahash_request_ctx(req);
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct hash_ctx *ctx = crypto_ahash_ctx(tfm);

	KNLLOG("enter bytes=%u\n", req->nbytes);

	/* Perform initialization first */
	hash_init(req);

	rctx->op = SPUM_OP_DIGEST;

	ctx->spum->curr_stats->digest++;
	ctx->spum->curr_stats->bytes += req->nbytes;

	/* Process the data */
	return process_data(req);
}

static int hash_cra_init(struct crypto_tfm *tfm)
{
	KNLLOG("entry\n");

	crypto_ahash_set_reqsize(__crypto_ahash_cast(tfm),
				 sizeof(struct hash_rctx));

	KNLLOG("exit\n");

	return 0;
}

static void hash_cra_exit(struct crypto_tfm *tfm)
{
}

static struct ahash_alg algs[] = {
{
	.init   = hash_init,
	.update = hash_update,
	.final  = hash_final,
	.finup  = hash_finup,
	.digest = hash_digest,
	.halg.digestsize = SHA1_DIGEST_SIZE,
	.halg.base = {
		.cra_name         = "sha1",
		.cra_driver_name  = "spum-sha1",
		.cra_priority     = 300,
		.cra_flags        = CRYPTO_ALG_TYPE_AHASH | CRYPTO_ALG_ASYNC |
					CRYPTO_TFM_REQ_MAY_BACKLOG,
		.cra_blocksize    = SHA1_BLOCK_SIZE,
		.cra_ctxsize      = sizeof(struct hash_ctx),
		.cra_module       = THIS_MODULE,
		.cra_alignmask    = 0,
		.cra_init         = hash_cra_init,
		.cra_exit         = hash_cra_exit,
	}
},
{
	.init   = hash_init,
	.update = hash_update,
	.final  = hash_final,
	.finup  = hash_finup,
	.digest = hash_digest,
	.halg.digestsize = SHA224_DIGEST_SIZE,
	.halg.base = {
		.cra_name        = "sha224",
		.cra_driver_name = "spum-sha224",
		.cra_priority    = 300,
		.cra_flags       = CRYPTO_ALG_TYPE_AHASH | CRYPTO_ALG_ASYNC |
					CRYPTO_TFM_REQ_MAY_BACKLOG,
		.cra_blocksize   = SHA224_BLOCK_SIZE,
		.cra_ctxsize     = sizeof(struct hash_ctx),
		.cra_alignmask   = 0,
		.cra_module      = THIS_MODULE,
		.cra_init        = hash_cra_init,
		.cra_exit        = hash_cra_exit,
	 }
},
{
	.init   = hash_init,
	.update = hash_update,
	.final  = hash_final,
	.finup  = hash_finup,
	.digest = hash_digest,
	.halg.digestsize = SHA256_DIGEST_SIZE,
	.halg.base = {
		.cra_name        = "sha256",
		.cra_driver_name = "spum-sha256",
		.cra_priority    = 300,
		.cra_flags       = CRYPTO_ALG_TYPE_AHASH | CRYPTO_ALG_ASYNC |
					CRYPTO_TFM_REQ_MAY_BACKLOG,
		.cra_blocksize   = SHA256_BLOCK_SIZE,
		.cra_ctxsize     = sizeof(struct hash_ctx),
		.cra_alignmask   = 0,
		.cra_module      = THIS_MODULE,
		.cra_init        = hash_cra_init,
		.cra_exit        = hash_cra_exit,
	}
},
{
	.init   = hash_init,
	.update = hash_update,
	.final  = hash_final,
	.finup  = hash_finup,
	.digest = hash_digest,
	.halg.digestsize = MD5_DIGEST_SIZE,
	.halg.base = {
		.cra_name        = "md5",
		.cra_driver_name = "spum-md5",
		.cra_priority    = 300,
		.cra_flags       = CRYPTO_ALG_TYPE_AHASH | CRYPTO_ALG_ASYNC |
					CRYPTO_TFM_REQ_MAY_BACKLOG,
		.cra_blocksize   = MD5_BLOCK_WORDS * sizeof(uint32_t),
		.cra_ctxsize     = sizeof(struct hash_ctx),
		.cra_alignmask   = 0,
		.cra_module      = THIS_MODULE,
		.cra_init        = hash_cra_init,
		.cra_exit        = hash_cra_exit,
	}
}
};

static int __devinit hash_probe(struct platform_device *pdev)
{
	int i, j, rc;
	uint32_t test_clock;
	struct resource *res;

	printk(KERN_INFO "BRCM SPUM Hash Driver...\n");

	spum_dev = kzalloc(sizeof(struct spum_dev), GFP_KERNEL);
	if (spum_dev == NULL) {
		printk(KERN_ERR "%s: Failed to allocate instance memory\n",
				__func__);
		return -ENOMEM;
	}

	spum_dev->dev = &pdev->dev;
	platform_set_drvdata(pdev, spum_dev);

	crypto_init_queue(&spum_dev->queue, SPUM_HASH_QUEUE_LENGTH);

	/* Get the base address APB space. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		printk(KERN_ERR "%s: Invalid resource type.\n", __func__);
		rc = -EINVAL;
		goto exit_free;
	}

	spum_dev->io_apb_base = ioremap_nocache(res->start, resource_size(res));
	if (!spum_dev->io_apb_base) {
		printk(KERN_ERR "%s: Ioremap failed.\n", __func__);
		rc = -ENOMEM;
		goto exit_free;
	}

	/* Get the base address AXI space. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		printk(KERN_ERR "%s: Invalid resource type.\n", __func__);
		rc = -EINVAL;
		goto exit_free;
	}

	spum_dev->io_axi_base = ioremap_nocache(res->start, resource_size(res));
	if (!spum_dev->io_axi_base) {
		printk(KERN_ERR "%s: Ioremap failed.\n", __func__);
		rc = -ENOMEM;
		goto exit_free;
	}

	spum_dev->tx.device = DMA_DEVICE_SPUM_MEM_TO_DEV;
	spum_dev->tx.fifo_addr =
			(dma_addr_t)res->start +
			SPUM_AXI_FIFO_IN_OFFSET;
	spum_dev->rx.device = DMA_DEVICE_SPUM_DEV_TO_MEM;
	spum_dev->rx.fifo_addr =
			(dma_addr_t)res->start +
			SPUM_AXI_FIFO_OUT_OFFSET;

	KNLLOG("tx dev=%u fifo=0x%x rx dev=%u fifo=0x%x\n",
			spum_dev->tx.device, (uint32_t)spum_dev->tx.fifo_addr,
			spum_dev->rx.device, (uint32_t)spum_dev->rx.fifo_addr);

	sema_init(&spum_dev->proc_sem, 0);
	sema_init(&spum_dev->done_sem, 0);
	spin_lock_init(&spum_dev->lock);

	/* Initializing the clock. */
	spum_dev->clk = clk_get(NULL, "spum_open");
	if (IS_ERR(spum_dev->clk)) {
		printk(KERN_ERR "%s: Clock intialization failed.\n", __func__);
		rc = PTR_ERR(spum_dev->clk);
		goto exit_free;
	}
	rc = clk_set_rate(spum_dev->clk, FREQ_MHZ(208));
	if (rc)
		pr_err("%s: Clock set_rate failed - %d.\n", __func__, rc);

	test_clock = clk_get_rate(spum_dev->clk);
	printk(KERN_INFO "SPU Clock set to %u Hz\n", test_clock);

	spum_dev->proc_thread =
		kthread_run(proc_thread, spum_dev, "SPUM proc thread");
	if (IS_ERR(spum_dev->proc_thread)) {
		printk(KERN_ERR "%s: Failed to start proc thread\n", __func__);
		goto exit_clk;
	}

	spum_dev->done_thread =
		kthread_run(done_thread, spum_dev, "SPUM done thread");
	if (IS_ERR(spum_dev->done_thread)) {
		printk(KERN_ERR "%s: Failed to start done thread\n", __func__);
		goto exit_clk;
	}

	for (i = 0; i < ARRAY_SIZE(algs); i++) {
		rc = crypto_register_ahash(&algs[i]);
		if (rc) {
			printk(KERN_ERR
				"%s: Failed to register ahash=%s rc=%d\n",
				__func__, algs[i].halg.base.cra_name, rc);
			goto exit_unload;
		} else {
			printk(KERN_INFO "%s: Registered ahash=%s\n",
				__func__, algs[i].halg.base.cra_name);
		}
	}

	hash_proc_entry = create_proc_entry("spum-hash", 0660, NULL);
	if (hash_proc_entry == NULL)
		printk(KERN_ERR "%s: Proc-entry create failed\n", __func__);
	else {
		hash_proc_entry->read_proc = hash_proc_read;
		hash_proc_entry->write_proc = NULL;
	}

	return 0;

exit_unload:
	for (j = 0; j < i; j++)
		crypto_unregister_ahash(&algs[j]);
exit_clk:
	clk_disable(spum_dev->clk);
	clk_put(spum_dev->clk);
exit_free:
	kfree(spum_dev);

	return rc;
}

static int __devexit hash_remove(struct platform_device *pdev)
{
	struct spum_dev *spum;
	int i;

	KNLLOG("entry\n");

	spum = platform_get_drvdata(pdev);
	if (!spum)
		return -ENODEV;

	spum_dev->thread_exit = 1;

	up(&spum_dev->proc_sem);
	up(&spum_dev->done_sem);

	kthread_stop(spum_dev->proc_thread);
	kthread_stop(spum_dev->done_thread);

	for (i = 0; i < ARRAY_SIZE(algs); i++)
		crypto_unregister_ahash(&algs[i]);

	clk_put(spum_dev->clk);

	kfree(spum_dev);

	if (hash_proc_entry)
		remove_proc_entry(hash_proc_entry->name, NULL);

	return 0;
}

static struct platform_driver hash_driver = {
	.probe  = hash_probe,
	.remove = hash_remove,
	.driver = {
		.name   = "brcm-spum",
		.owner  = THIS_MODULE,
	},
};

static int __init hash_module_init(void)
{
	KNLLOG("SPUM driver init.\n");
	return platform_driver_register(&hash_driver);
}
static void __exit hash_module_exit(void)
{
	KNLLOG("SPUM driver exit.\n");
	platform_driver_unregister(&hash_driver);
}

module_init(hash_module_init);
module_exit(hash_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Broadcom SPU-M Accelerated Secure Hash Driver");
MODULE_ALIAS("spum_hash");
