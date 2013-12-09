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
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/crypto.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <plat/clock.h>
#include <asm/byteorder.h>
#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <crypto/scatterwalk.h>
#include <mach/dma_mmap.h>
#include <mach/sdma.h>
#include <mach/rdb/brcm_rdb_spum_apb.h>
#include <mach/rdb/brcm_rdb_spum_axi.h>
#include "brcm_spum.h"

#define KNLLOG_DEBUG 0
#if KNLLOG_DEBUG
#include <linux/broadcom/knllog.h>
#else
#define KNLLOG(x, ...)
#endif

#define SPUM_AES_QUEUE_LENGTH   300
#define AES_XTS_MIN_KEY_SIZE    32
#define AES_XTS_MAX_KEY_SIZE    64
#define AES_KEYSIZE_512	        64

struct spum_dir {
	DMA_Device_t device;
	dma_addr_t fifo_addr;
};

struct spum_stats_op {
	uint32_t enc_count;
	uint64_t enc_bytes;
	uint32_t dec_count;
	uint64_t dec_bytes;
	uint32_t last_key_len;
};

struct spum_stats {
	struct spum_stats_op cbc;
	struct spum_stats_op ctr;
	struct spum_stats_op ecb;
	struct spum_stats_op xts;
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
	struct ablkcipher_request *req; /* Current request being processed */
	bool thread_exit;
	spinlock_t lock;
	struct crypto_queue queue;
	atomic_t busy;
	struct spum_stats stats;
};

struct aes_ctx {
	struct spum_dev *spum;
	uint32_t key_enc[AES_MAX_KEYLENGTH_U32];
	uint32_t key_len;
};

struct aes_rctx_dir {
	uint32_t size;
	SDMA_Handle_t dma_handle;
	bool dma_mmap_started;
	DMA_MMAP_CFG_T dma_mmap;
	atomic_t done;
};

struct aes_rctx {
	/* RX/TX */
	struct aes_rctx_dir tx;
	struct aes_rctx_dir rx;

	/* Command */
	uint32_t cmd[512/4];
	uint32_t out_hdr[SPUM_OUTPUT_HEADER_LEN/4];

	/* XTS */
	uint32_t xts_info[4];

	/* Status */
	uint32_t status[SPUM_OUTPUT_HEADER_LEN/4];
};

static struct spum_dev *spum_dev;
static struct proc_dir_entry *aes_proc_entry;

static int aes_proc_read(char *buf, char **start, off_t offset, int count,
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

	op = &spum_dev->stats.cbc;
	p += sprintf(p, "cbc: enc=%u encbytes=%llu "
			"dec=%u decbytes=%llu last_key_len=%u\n",
		op->enc_count, op->enc_bytes, op->dec_count, op->dec_bytes,
		op->last_key_len);
	op = &spum_dev->stats.ecb;
	p += sprintf(p, "ecb: enc=%u encbytes=%llu "
			"dec=%u decbytes=%llu last_key_len=%u\n",
		op->enc_count, op->enc_bytes, op->dec_count, op->dec_bytes,
		op->last_key_len);
	op = &spum_dev->stats.ctr;
	p += sprintf(p, "ctr: enc=%u encbytes=%llu "
			"dec=%u decbytes=%llu last_key_len=%u\n",
		op->enc_count, op->enc_bytes, op->dec_count, op->dec_bytes,
		op->last_key_len);
	op = &spum_dev->stats.xts;
	p += sprintf(p, "xts: enc=%u encbytes=%llu "
			"dec=%u decbytes=%llu last_key_len=%u\n",
		op->enc_count, op->enc_bytes, op->dec_count, op->dec_bytes,
		op->last_key_len);

	*eof = 1;
	return p - buf;
}

static void map_sgl(struct spum_dev *spum,
		struct scatterlist *sgl, enum dma_data_direction dma_dir)
{
	struct scatterlist *sg_walk = sgl;
	while (sg_walk) {
		dma_map_sg(spum->dev, sg_walk, 1, dma_dir);
		sg_walk = sg_next(sg_walk);
	}
}

static void unmap_sgl(struct spum_dev *spum,
		struct scatterlist *sgl, enum dma_data_direction dma_dir)
{
	struct scatterlist *sg_walk = sgl;
	while (sg_walk) {
		dma_unmap_sg(spum->dev, sg_walk, 1, dma_dir);
		sg_walk = sg_next(sg_walk);
	}
}

static int dma_enq(struct aes_rctx *rctx, struct aes_rctx_dir *dir,
	void *buf, uint32_t len, enum dma_data_direction dma_dir, bool phys)
{
	int rc = 0;

	/* dma_mmap buffer */
	if (!phys && !dma_mmap_dma_is_supported(buf)) {
		printk(KERN_ERR "%s: Unsupported buffer! buf=0x%x\n",
				__func__, (uint32_t)buf);
		rc = -EINVAL;
		goto exit;
	}

	if (!dir->dma_mmap_started) {
		KNLLOG("dma_mmap_map buf=0x%x len=%u\n",
				(uint32_t)buf, len);
		rc = dma_mmap_init_map(&dir->dma_mmap);
		if (rc < 0) {
			printk(KERN_ERR "%s: dma_mmap_init_map failed\n",
					__func__);
			goto exit;
		}

		rc = dma_mmap_map(&dir->dma_mmap, buf, len, dma_dir);
		if (rc < 0) {
			printk(KERN_ERR "%s: dma_mmap_map failed\n",
					__func__);
			goto exit;
		}
		dir->dma_mmap_started = 1;
	} else {
		KNLLOG("dma_mmap_add_region buf=0x%x len=%u\n",
				(uint32_t)buf, len);
		if (!phys)
			rc = dma_mmap_add_region(&dir->dma_mmap, buf, len);
		else
			rc = dma_mmap_add_phys_region(&dir->dma_mmap, buf, len);
		if (rc < 0) {
			printk(KERN_ERR "dma_mmap_add_region failed\n");
			goto exit;
		}
	}

exit:
	KNLLOG("rc=%d buf=0x%x\n", rc, (uint32_t)buf);

	return rc;
}

static int dma_enq_rx(struct aes_rctx *rctx, void *buf, uint32_t len)
{
	KNLLOG("enter buf=0x%x\n", (uint32_t)buf);

	return dma_enq(rctx, &rctx->rx, buf, len, DMA_FROM_DEVICE, 0);
}

static int dma_enq_tx(struct aes_rctx *rctx, void *buf, uint32_t len)
{
	KNLLOG("enter buf=0x%x\n", (uint32_t)buf);

	return dma_enq(rctx, &rctx->tx, buf, len, DMA_TO_DEVICE, 0);
}

static void dma_irq(DMA_Device_t dev, int reason, void *data)
{
	struct spum_dev *spum = (struct spum_dev *)data;
	struct aes_rctx *rctx = ablkcipher_request_ctx(spum->req);

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

static void dma_cleanup(struct aes_rctx_dir *dir,
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

static int execute_spum(struct spum_dev *spum)
{
	int rc;
	struct ablkcipher_request *req = spum->req;
	struct aes_rctx *rctx = ablkcipher_request_ctx(req);
	struct aes_rctx_dir *tx = &rctx->tx;
	struct aes_rctx_dir *rx = &rctx->rx;

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
		printk(KERN_ERR "%s: sdma_map_create_descriptor_ring failed\n",
				__func__);
		goto exit;
	}
	rc = sdma_map_create_descriptor_ring(rx->dma_handle,
			&rx->dma_mmap,
			spum->rx.fifo_addr,
			DMA_UPDATE_MODE_NO_INC);
	if (rc < 0) {
		printk(KERN_ERR "%s: sdma_map_create_descriptor_ring failed\n",
				__func__);
		goto exit;
	}

	/* tell the DMA ready for transfer */
	rc = sdma_start_transfer(tx->dma_handle);
	if (rc < 0) {
		printk(KERN_ERR "%s: sdma_start_transfer failed\n", __func__);
		goto exit;
	}
	rc = sdma_start_transfer(rx->dma_handle);
	if (rc < 0) {
		printk(KERN_ERR "%s: sdma_start_transfer failed\n", __func__);
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

	KNLLOG("error exit\n");

	return -EIO;
}

static int proc_thread(void *data)
{
	struct crypto_async_request *async_req, *backlog;
	struct spum_dev *spum = (struct spum_dev *)data;
	struct ablkcipher_request *req;
	unsigned long flags;
	int rc;

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

			req = ablkcipher_request_cast(async_req);
			spum->req = req;

			rc = execute_spum(spum);
			if (rc) {
				printk(KERN_ERR "%s: execute_spum failed rc=%d\n",
						__func__, rc);
				continue;
			}
		}
	}

	KNLLOG("exit\n");
	return 0;
}

static int done_thread(void *data)
{
	struct spum_dev *spum = (struct spum_dev *)data;
	struct aes_rctx *rctx;

	while (1) {
		if (!down_interruptible(&spum->done_sem)) {
			if (spum->thread_exit)
				break;

			KNLLOG("signalled\n");

			rctx = ablkcipher_request_ctx(spum->req);

			/* Cleanup */
			dma_cleanup(&rctx->tx, DMA_TO_DEVICE);
			dma_cleanup(&rctx->rx, DMA_FROM_DEVICE);

			/* Unmap scatterlists */
			unmap_sgl(spum, spum->req->src, DMA_TO_DEVICE);
			unmap_sgl(spum, spum->req->dst, DMA_FROM_DEVICE);

			clk_disable(spum->clk);

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

	KNLLOG("%s: exit\n");
	return 0;
}

static int setkey(struct crypto_ablkcipher *tfm, const u8 *in_key,
		unsigned int key_len, bool xts)
{
	struct aes_ctx *ctx = crypto_ablkcipher_ctx(tfm);
	struct ablkcipher_alg *cipher = crypto_ablkcipher_alg(tfm);

	KNLLOG("enter len=%u\n", key_len);

	if ((key_len < cipher->min_keysize) ||
			(key_len > cipher->max_keysize)) {
		crypto_ablkcipher_set_flags(tfm,
					CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	if (xts) {
		memcpy(ctx->key_enc, in_key + key_len/2, key_len/2);
		memcpy((uint8_t *)ctx->key_enc + key_len/2, in_key, key_len/2);
	} else {
		memcpy(ctx->key_enc, in_key, key_len);
	}

	ctx->key_len = key_len;

	KNLLOG("exit\n");
	return 0;
}

static int aes_xts_setkey(struct crypto_ablkcipher *tfm, const u8 *in_key,
		unsigned int key_len)
{
	return setkey(tfm, in_key, key_len, 1);
}

static int aes_setkey(struct crypto_ablkcipher *tfm, const u8 *in_key,
		unsigned int key_len)
{
	return setkey(tfm, in_key, key_len, 0);
}

static void walk_sg_list(struct aes_rctx *rctx, struct scatterlist *sgl,
		int total_bytes, enum dma_data_direction dma_dir)
{
	struct scatterlist *sg_walk;
	int byte_count = 0;

	sg_walk = sgl;
	while (sg_walk) {
		uint32_t copy_len = min((uint32_t)(total_bytes - byte_count),
					sg_walk->length);
		if (copy_len == 0)
			break;

		dma_enq(rctx,
			(dma_dir == DMA_TO_DEVICE) ? &rctx->tx : &rctx->rx,
			(void *)sg_dma_address(sg_walk), copy_len, dma_dir, 1);
		sg_walk = sg_next(sg_walk);
		byte_count += copy_len;
	}
}

static int aes_crypt(struct ablkcipher_request *req, spum_crypto_algo algo,
		spum_crypto_mode mode, spum_crypto_op op)
{
	struct aes_ctx *ctx = crypto_tfm_ctx(req->base.tfm);
	struct aes_rctx *rctx = ablkcipher_request_ctx(req);
	struct spum_hw_context spu_ctx;
	int rc;
	int cmd_bytes;
	unsigned long flags;
	char *iv = "\x00\x00\x00\x00\x00\x00\x00\x00"
			"\x00\x00\x00\x00\x00\x00\x00\x00";
	uint32_t key_len = ctx->key_len;

	KNLLOG("nbytes=%u\n", req->nbytes);

	memset(rctx, 0, sizeof(struct aes_rctx));

	memset((void *)&spu_ctx, 0, sizeof(spu_ctx));
	spu_ctx.crypto_algo     = algo;
	spu_ctx.crypto_mode     = mode;
	spu_ctx.auth_algo       = SPUM_AUTH_ALGO_NULL;
	spu_ctx.init_vector_len =
		crypto_ablkcipher_ivsize(crypto_ablkcipher_reqtfm(req))/
		sizeof(uint32_t);
	spu_ctx.data_attribute.crypto_offset = 0;
	spu_ctx.data_attribute.crypto_length = req->nbytes;
	spu_ctx.data_attribute.data_length =
		((req->nbytes + 3) / sizeof(uint32_t)) * sizeof(uint32_t);

	/* Set operation to encrypt */
	spu_ctx.operation      = op;
	spu_ctx.key_type       = SPUM_KEY_OPEN;
	spu_ctx.crypto_key     = (void *)ctx->key_enc;
	spu_ctx.crypto_key_len = (key_len / sizeof(uint32_t));

	if (mode == SPUM_CRYPTO_MODE_XTS) {
		spu_ctx.init_vector = iv;
		spu_ctx.data_attribute.crypto_length +=
		    spu_ctx.init_vector_len * sizeof(uint32_t);
		spu_ctx.data_attribute.data_length +=
		    spu_ctx.init_vector_len * sizeof(uint32_t);
		key_len /= 2;
	} else {
		spu_ctx.init_vector = req->info;
	}

	switch (key_len) {
	case AES_KEYSIZE_128:
		spu_ctx.crypto_type = SPUM_CRYPTO_TYPE_AES_K128;
		break;
	case AES_KEYSIZE_192:
		spu_ctx.crypto_type = SPUM_CRYPTO_TYPE_AES_K192;
		break;
	case AES_KEYSIZE_256:
		spu_ctx.crypto_type = SPUM_CRYPTO_TYPE_AES_K256;
		break;
	default:
		return -EPERM;
	}

	/* Create Command */
	cmd_bytes = spum_format_command(&spu_ctx, rctx->cmd);

	/* Calculate tx/rx lengths */
	rctx->tx.size = cmd_bytes + spu_ctx.data_attribute.data_length;
	rctx->rx.size = SPUM_OUTPUT_HEADER_LEN +
			spu_ctx.data_attribute.data_length +
			SPUM_OUTPUT_STATUS_LEN;

	/* TX: Command */
	dma_enq_tx(rctx, rctx->cmd, cmd_bytes);
	if (mode == SPUM_CRYPTO_MODE_XTS) {
		int len = crypto_ablkcipher_ivsize(
				crypto_ablkcipher_reqtfm(req));
		memcpy(rctx->xts_info, req->info, len);
		dma_enq_tx(rctx, rctx->xts_info, len);
	}
	/* TX: Source */
	map_sgl(ctx->spum, req->src, DMA_TO_DEVICE);
	walk_sg_list(rctx, req->src, req->nbytes, DMA_TO_DEVICE);
	/* TX: Status */
	dma_enq_tx(rctx, rctx->status, SPUM_OUTPUT_STATUS_LEN);

	/* RX: Header */
	dma_enq_rx(rctx, rctx->out_hdr, SPUM_OUTPUT_HEADER_LEN);
	/* RX: Destination */
	map_sgl(ctx->spum, req->dst, DMA_FROM_DEVICE);
	if (mode == SPUM_CRYPTO_MODE_XTS) {
		int len = crypto_ablkcipher_ivsize(
				crypto_ablkcipher_reqtfm(req));
		dma_enq_rx(rctx, rctx->xts_info, len);
	}
	walk_sg_list(rctx, req->dst, req->nbytes, DMA_FROM_DEVICE);
	/* RX: Status */
	dma_enq_rx(rctx, rctx->status, SPUM_OUTPUT_STATUS_LEN);

	/* Submit to SPUM */
	spin_lock_irqsave(&ctx->spum->lock, flags);
	rc = ablkcipher_enqueue_request(&ctx->spum->queue, req);
	spin_unlock_irqrestore(&ctx->spum->lock, flags);

	/* Signal processing thread */
	if (!atomic_read(&ctx->spum->busy))
		up(&ctx->spum->proc_sem);

	KNLLOG("exit rc=%d\n", rc);
	return rc;
}

static int aes_ecb_encrypt(struct ablkcipher_request *req)
{
	struct spum_stats_op *op = &spum_dev->stats.ecb;
	struct aes_ctx *ctx = crypto_tfm_ctx(req->base.tfm);

	KNLLOG("entry\n");

	op->enc_count++;
	op->enc_bytes += req->nbytes;
	op->last_key_len = ctx->key_len;

	return aes_crypt(req, SPUM_CRYPTO_ALGO_AES,
				SPUM_CRYPTO_MODE_ECB,
				SPUM_CRYPTO_ENCRYPTION);
}

static int aes_ecb_decrypt(struct ablkcipher_request *req)
{
	struct spum_stats_op *op = &spum_dev->stats.ecb;
	struct aes_ctx *ctx = crypto_tfm_ctx(req->base.tfm);

	KNLLOG("entry\n");

	op->dec_count++;
	op->dec_bytes += req->nbytes;
	op->last_key_len = ctx->key_len;

	return aes_crypt(req, SPUM_CRYPTO_ALGO_AES,
				SPUM_CRYPTO_MODE_ECB,
				SPUM_CRYPTO_DECRYPTION);
}

static int aes_cbc_encrypt(struct ablkcipher_request *req)
{
	struct spum_stats_op *op = &spum_dev->stats.cbc;
	struct aes_ctx *ctx = crypto_tfm_ctx(req->base.tfm);

	KNLLOG("entry\n");

	op->enc_count++;
	op->enc_bytes += req->nbytes;
	op->last_key_len = ctx->key_len;

	return aes_crypt(req, SPUM_CRYPTO_ALGO_AES,
				SPUM_CRYPTO_MODE_CBC,
				SPUM_CRYPTO_ENCRYPTION);
}

static int aes_cbc_decrypt(struct ablkcipher_request *req)
{
	struct spum_stats_op *op = &spum_dev->stats.cbc;
	struct aes_ctx *ctx = crypto_tfm_ctx(req->base.tfm);

	KNLLOG("entry\n");

	op->dec_count++;
	op->dec_bytes += req->nbytes;
	op->last_key_len = ctx->key_len;

	return aes_crypt(req, SPUM_CRYPTO_ALGO_AES,
				SPUM_CRYPTO_MODE_CBC,
				SPUM_CRYPTO_DECRYPTION);
}

static int aes_ctr_encrypt(struct ablkcipher_request *req)
{
	struct spum_stats_op *op = &spum_dev->stats.ctr;
	struct aes_ctx *ctx = crypto_tfm_ctx(req->base.tfm);

	KNLLOG("entry\n");

	op->enc_count++;
	op->enc_bytes += req->nbytes;
	op->last_key_len = ctx->key_len;

	return aes_crypt(req, SPUM_CRYPTO_ALGO_AES,
				SPUM_CRYPTO_MODE_CTR,
				SPUM_CRYPTO_ENCRYPTION);
}

static int aes_ctr_decrypt(struct ablkcipher_request *req)
{
	struct spum_stats_op *op = &spum_dev->stats.ctr;
	struct aes_ctx *ctx = crypto_tfm_ctx(req->base.tfm);

	KNLLOG("entry\n");

	op->dec_count++;
	op->dec_bytes += req->nbytes;
	op->last_key_len = ctx->key_len;

	return aes_crypt(req, SPUM_CRYPTO_ALGO_AES,
				SPUM_CRYPTO_MODE_CTR,
				SPUM_CRYPTO_DECRYPTION);
}

static int aes_xts_encrypt(struct ablkcipher_request *req)
{
	struct spum_stats_op *op = &spum_dev->stats.xts;
	struct aes_ctx *ctx = crypto_tfm_ctx(req->base.tfm);

	KNLLOG("entry\n");

	op->enc_count++;
	op->enc_bytes += req->nbytes;
	op->last_key_len = ctx->key_len;

	return aes_crypt(req, SPUM_CRYPTO_ALGO_AES,
				SPUM_CRYPTO_MODE_XTS,
				SPUM_CRYPTO_ENCRYPTION);
}

static int aes_xts_decrypt(struct ablkcipher_request *req)
{
	struct spum_stats_op *op = &spum_dev->stats.xts;
	struct aes_ctx *ctx = crypto_tfm_ctx(req->base.tfm);

	KNLLOG("entry\n");

	op->dec_count++;
	op->dec_bytes += req->nbytes;
	op->last_key_len = ctx->key_len;

	return aes_crypt(req, SPUM_CRYPTO_ALGO_AES,
				SPUM_CRYPTO_MODE_XTS,
				SPUM_CRYPTO_DECRYPTION);
}

static int aes_cra_init(struct crypto_tfm *tfm)
{
	struct aes_ctx *ctx = crypto_tfm_ctx(tfm);

	KNLLOG("entry\n");

	/* Assign only spum device to context */
	ctx->spum = spum_dev;

	tfm->crt_ablkcipher.reqsize = sizeof(struct aes_rctx);

	KNLLOG("exit\n");

	return 0;
}

static void aes_cra_exit(struct crypto_tfm *tfm)
{
}

static struct crypto_alg algs[] = {
{
	.cra_name          = "ecb(aes)",
	.cra_driver_name   = "spum-ecb-aes",
	.cra_priority      = 300,
	.cra_flags         = CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC |
				CRYPTO_TFM_REQ_MAY_BACKLOG,
	.cra_blocksize     = AES_BLOCK_SIZE,
	.cra_ctxsize       = sizeof(struct aes_ctx),
	.cra_alignmask     = 0,
	.cra_type          = &crypto_ablkcipher_type,
	.cra_module        = THIS_MODULE,
	.cra_init          = aes_cra_init,
	.cra_exit          = aes_cra_exit,
	.cra_u = {
		.ablkcipher = {
			.min_keysize   = AES_MIN_KEY_SIZE,
			.max_keysize   = AES_MAX_KEY_SIZE,
			.setkey        = aes_setkey,
			.encrypt       = aes_ecb_encrypt,
			.decrypt       = aes_ecb_decrypt
		}
	}
},
{
	.cra_name          = "cbc(aes)",
	.cra_driver_name   = "spum-cbc-aes",
	.cra_priority      = 300,
	.cra_flags         = CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC |
				CRYPTO_TFM_REQ_MAY_BACKLOG,
	.cra_blocksize     = AES_BLOCK_SIZE,
	.cra_ctxsize       = sizeof(struct aes_ctx),
	.cra_alignmask     = 0,
	.cra_type          = &crypto_ablkcipher_type,
	.cra_module        = THIS_MODULE,
	.cra_init          = aes_cra_init,
	.cra_exit          = aes_cra_exit,
	.cra_u = {
		.ablkcipher = {
			.min_keysize   = AES_MIN_KEY_SIZE,
			.max_keysize   = AES_MAX_KEY_SIZE,
			.ivsize        = AES_BLOCK_SIZE,
			.setkey        = aes_setkey,
			.encrypt       = aes_cbc_encrypt,
			.decrypt       = aes_cbc_decrypt
		}
	}
},
{
	.cra_name          = "ctr(aes)",
	.cra_driver_name   = "spum-ctr-aes",
	.cra_priority      = 300,
	.cra_flags         = CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC |
				CRYPTO_TFM_REQ_MAY_BACKLOG,
	.cra_blocksize     = AES_BLOCK_SIZE,
	.cra_ctxsize       = sizeof(struct aes_ctx),
	.cra_alignmask     = 0,
	.cra_type          = &crypto_ablkcipher_type,
	.cra_module        = THIS_MODULE,
	.cra_init          = aes_cra_init,
	.cra_exit          = aes_cra_exit,
	.cra_u = {
		.ablkcipher = {
			.min_keysize   = AES_MIN_KEY_SIZE,
			.max_keysize   = AES_MAX_KEY_SIZE,
			.ivsize        = AES_BLOCK_SIZE,
			.setkey        = aes_setkey,
			.encrypt       = aes_ctr_encrypt,
			.decrypt       = aes_ctr_decrypt
		}
	}
},
{
	.cra_name          = "xts(aes)",
	.cra_driver_name   = "spum-xts-aes",
	.cra_priority      = 300,
	.cra_flags         = CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC |
				CRYPTO_TFM_REQ_MAY_BACKLOG,
	.cra_blocksize     = AES_BLOCK_SIZE,
	.cra_ctxsize       = sizeof(struct aes_ctx),
	.cra_alignmask     = 0,
	.cra_type          = &crypto_ablkcipher_type,
	.cra_module        = THIS_MODULE,
	.cra_init          = aes_cra_init,
	.cra_exit          = aes_cra_exit,
	.cra_u = {
		.ablkcipher = {
			.min_keysize   = AES_XTS_MIN_KEY_SIZE,
			.max_keysize   = AES_XTS_MAX_KEY_SIZE,
			.ivsize        = AES_BLOCK_SIZE,
			.setkey        = aes_xts_setkey,
			.encrypt       = aes_xts_encrypt,
			.decrypt       = aes_xts_decrypt
		}
	}
}
};

static int __devinit aes_probe(struct platform_device *pdev)
{
	int i, j, rc;
	uint32_t test_clock;
	struct resource *res;

	printk(KERN_INFO "BRCM SPUM AES Driver...\n");

	spum_dev = kzalloc(sizeof(struct spum_dev), GFP_KERNEL);
	if (spum_dev == NULL) {
		printk(KERN_ERR "%s: Failed to allocate instance memory\n",
				__func__);
		return -ENOMEM;
	}

	spum_dev->dev = &pdev->dev;
	platform_set_drvdata(pdev, spum_dev);

	crypto_init_queue(&spum_dev->queue, SPUM_AES_QUEUE_LENGTH);

	atomic_set(&spum_dev->busy, 0);

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
	printk(KERN_ERR "SPU Clock set to %u Hz\n", test_clock);

	spum_dev->proc_thread = kthread_run(proc_thread, spum_dev,
						"SPUM proc thread");
	if (IS_ERR(spum_dev->proc_thread)) {
		printk(KERN_ERR "%s: Failed to start proc thread\n", __func__);
		goto exit_clk;
	}

	spum_dev->done_thread = kthread_run(done_thread, spum_dev,
						"SPUM done thread");
	if (IS_ERR(spum_dev->done_thread)) {
		printk(KERN_ERR "%s: Failed to start done thread\n", __func__);
		goto exit_clk;
	}

	for (i = 0; i < ARRAY_SIZE(algs); i++) {
		rc = crypto_register_alg(&algs[i]);
		if (rc) {
			printk(KERN_ERR
				"%s: Failed to register alg=%s rc=%d\n",
				__func__, algs[i].cra_name, rc);
			goto exit_unload;
		} else {
			printk(KERN_INFO
				"%s: Registered alg=%s\n",
				__func__, algs[i].cra_name);
		}
	}

	aes_proc_entry = create_proc_entry("spum-aes", 0660, NULL);
	if (aes_proc_entry == NULL)
		printk(KERN_ERR "%s: Proc-entry create failed\n", __func__);
	else {
		aes_proc_entry->read_proc = aes_proc_read;
		aes_proc_entry->write_proc = NULL;
	}

	return 0;

exit_unload:
	for (j = 0; j < i; j++)
		crypto_unregister_alg(&algs[j]);
exit_clk:
	clk_disable(spum_dev->clk);
	clk_put(spum_dev->clk);
exit_free:
	kfree(spum_dev);

	return rc;
}

static int __devexit aes_remove(struct platform_device *pdev)
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
		crypto_unregister_alg(&algs[i]);

	clk_put(spum_dev->clk);

	kfree(spum_dev);

	if (aes_proc_entry)
		remove_proc_entry(aes_proc_entry->name, NULL);

	return 0;
}

static struct platform_driver aes_driver = {
	.probe  = aes_probe,
	.remove = aes_remove,
	.driver = {
		.name   = "brcm-spum-aes",
		.owner  = THIS_MODULE,
	},
};

static int __init aes_module_init(void)
{
	printk(KERN_INFO "SPUM driver init.\n");
	return platform_driver_register(&aes_driver);
}
static void __exit aes_module_exit(void)
{
	platform_driver_unregister(&aes_driver);
}

module_init(aes_module_init);
module_exit(aes_module_exit);

MODULE_DESCRIPTION("BRCM SPUM AES block cipher algorithm");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS("spum_aes");
