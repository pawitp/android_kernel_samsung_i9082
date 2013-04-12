/*******************************************************************************
* Copyright 2011 Broadcom Corporation.  All rights reserved.
*
*	@file	drivers/spi/spi_sspi_kona.c
*
* Unless you and Broadcom execute a separate written software license agreement
* governing use of this software, this software is licensed to you under the
* terms of the GNU General Public License version 2, available at
* http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*******************************************************************************/

/*
 * Broadcom KONA SSPI based SPI master controller
 *
 * TODO:(Limitations in the current implementation)
 * 1. Only Tx0 and Rx0 are used
 * 2. Only Task0 and Sequence0 and Sequence1 of SSPI is used
 * 3. Full Duplex not supported
 * 4. Jumbo SPI can be handled differently according to SSPI spec
 * 5. CLK_RATE < 12MHz leaves CS asserted(need to check this?)
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/spi/spi.h>
#include <linux/workqueue.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <plat/clock.h>

#include <plat/chal/chal_types.h>
#include <plat/chal/chal_sspi.h>
#include <plat/spi_kona.h>
#include <linux/dma-mapping.h>
#include <mach/rdb/brcm_rdb_sysmap.h>
#ifdef CONFIG_DMAC_PL330
#include <mach/dma.h>
#define MAX_DESCRIPTORS_PER_TRANSFER 400
#elif CONFIG_MAP_SDMA
#include <mach/sdma.h>
#define dma_start_transfer	sdma_start_transfer
#define dma_stop_transfer	sdma_stop_transfer
#define dma_free_chan	 sdma_free_channel
#endif

#define SSPI_MAX_TASK_LOOP	1023
#define SSPI_TASK_TIME_OUT	500000
#define SSPI_FIFO_SIZE		128

/* Timeout(ms) for wait_for_completion */
#define SSPI_WFC_TIME_OUT	4000

extern void csl_caph_ControlHWClock(Boolean eanble);
static uint8_t clk_name[3][32] = { "ssp0_clk", "ssp4_clk", "ssp3_clk" };
static uint8_t apb_clk_name[3][32] =
			{ "ssp0_apb_clk", "ssp4_apb_clk", "ssp3_apb_clk" };

#ifdef CONFIG_DMAC_PL330
static char dma_tx_chan_name[3][32] = {
	"SSP_0B_TX0",
	"SSP_1B_TX0",
	"SSP_2B_TX0"
};
static char dma_rx_chan_name[3][32] = {
	"SSP_0A_RX0",
	"SSP_1A_RX0",
	"SP_2A_RX0"
};
static unsigned int dma_fifo_base[3] = {
	SSP0_BASE_ADDR,
	SSP4_BASE_ADDR,
	SSP3_BASE_ADDR
};
#elif CONFIG_MAP_SDMA
static  DMA_Device_t dma_tx_device[6] = {
	DMA_DEVICE_SSP0B_TX0,
	DMA_DEVICE_SSP2B_TX0,
	DMA_DEVICE_SSP3B_TX0,
	DMA_DEVICE_SSP4B_TX0,
	DMA_DEVICE_SSP5B_TX0,
	DMA_DEVICE_SSP6B_TX0
};
static DMA_Device_t dma_rx_device[6] = {
	DMA_DEVICE_SSP0A_RX0,
	DMA_DEVICE_SSP2A_RX0,
	DMA_DEVICE_SSP3A_RX0,
	DMA_DEVICE_SSP4A_RX0,
	DMA_DEVICE_SSP5A_RX0,
	DMA_DEVICE_SSP6A_RX0
};
static unsigned int dma_fifo_base[6] = {
	SSP0_BASE_ADDR,
	SSP2_BASE_ADDR,
	SSP3_BASE_ADDR,
	SSP4_BASE_ADDR,
	SSP5_BASE_ADDR,
	SSP6_BASE_ADDR
};
#endif
struct spi_kona_config {
	uint32_t speed_hz;
	uint32_t bpw;
	uint32_t mode;
	int cs;
};

#define	CS_ACTIVE	1	/* normally nCS, active low */
#define	CS_INACTIVE	0

struct spi_kona_data {
	struct spi_master *master;	/* SPI framework hookup */

	CHAL_HANDLE chandle;	/* SSPI CHAL Handle */
	void __iomem *base;	/* SPI virtual base address */
	struct clk *ssp_clk;	/* SSPI bus clock */
	struct clk *ssp_apb_clk;	/* SSPI apb clock */
	unsigned long spi_clk;	/* SPI controller clock speed */

	struct clk *dma_axi_clk;	/* DMA AXI clock */

	struct workqueue_struct *workqueue;	/* Driver message queue */
	struct work_struct work;	/* Message work */
	struct completion xfer_done;	/* Used to signal completion of xfer */
	struct completion tx_dma_evt;	/* Used to signal Tx DMA completion */
	struct completion rx_dma_evt;	/* Used to signal Rx DMA completion */

	spinlock_t lock;
	struct list_head queue;
	u8 busy;
	u8 use_dma;
#ifdef CONFIG_DMAC_PL330
	u32 tx_dma_chan;
	u32 rx_dma_chan;
#elif CONFIG_MAP_SDMA
	SDMA_Handle_t  tx_dma_chan;
	SDMA_Handle_t  rx_dma_chan;
	DMA_MMAP_CFG_T tx_dma_mmap;
	DMA_MMAP_CFG_T rx_dma_mmap;
#endif
	u8 flags;		/* extra spi->mode support */
	int irq;
	int enable_dma;

	/* Current Transfer details */
	int32_t count;
	void (*tx) (struct spi_kona_data *);
	void (*rx) (struct spi_kona_data *);
	void *rx_buf;
	const void *tx_buf;
	int32_t rxpend;		/* No. of frames pending from Rx FIFO
				   corresponding to Tx done */
	uint8_t bytes_per_word;
};

#define SPI_KONA_BUF_RX(type, type2)					\
static void spi_kona_buf_rx_##type(struct spi_kona_data *d)		\
{									\
	type val = read##type2(d->base +				\
		chal_sspi_rx0_get_dma_port_addr_offset());		\
									\
	if (d->rx_buf) {						\
		*(type *)d->rx_buf = val;				\
		d->rx_buf += sizeof(type);				\
	}								\
}

#define SPI_KONA_BUF_TX(type, type2)					\
static void spi_kona_buf_tx_##type(struct spi_kona_data *d)		\
{									\
	type val = 0;							\
									\
	if (d->tx_buf) {						\
		val = *(type *)d->tx_buf;				\
		d->tx_buf += sizeof(type);				\
		d->count -= sizeof(type);				\
	}								\
									\
	write##type2(val, d->base +					\
		chal_sspi_tx0_get_dma_port_addr_offset());		\
}

SPI_KONA_BUF_RX(u8, b)
SPI_KONA_BUF_TX(u8, b)
SPI_KONA_BUF_RX(u16, w)
SPI_KONA_BUF_TX(u16, w)
SPI_KONA_BUF_RX(u32, l)
SPI_KONA_BUF_TX(u32, l)

/* DMA Burst size and Scheduler FIFO threshold */
/*#define DMA_BURST_CONFIG_64_BYTES*/
#define DMA_BURST_CONFIG_16_BYTES
#ifdef DMA_BURST_CONFIG_16_BYTES
#define FIFO_BURST_ALIGNMENT	16
#define SSPI_FIFO_THRESHOLD	    16
#else
#define FIFO_BURST_ALIGNMENT	64
#define SSPI_FIFO_THRESHOLD	    64
#endif
static void spi_kona_tx_data(struct spi_kona_data *spi_kona)
{
	while (spi_kona->rxpend < (SSPI_FIFO_SIZE / spi_kona->bytes_per_word)) {
		if (!spi_kona->count)
			break;
		spi_kona->tx(spi_kona);
		spi_kona->rxpend++;
	}
}

/*
 * The data flow is designed with the following conditions:
 *
 * 1) Every TX is gauranteed to follow by an RX
 * 2) An RX cannot start without a TX from SPI Master
 *
 * The driver assumes half duplex communication.
 */
static irqreturn_t spi_kona_isr(int irq, void *dev_id)
{
	struct spi_kona_data *spi_kona = dev_id;
	uint16_t fifo_level = 0;
	uint32_t status = 0, dstat = 0;

	chal_sspi_get_intr_status(spi_kona->chandle, &status, &dstat);
//	pr_info("sspi intr status %x det %x \n", status, dstat);
	if (status & SSPIL_INTERRUPT_STATUS_FIFO_OVERRUN_STATUS_MASK) {
		chal_sspi_clear_intr(spi_kona->chandle,
		SSPIL_INTERRUPT_STATUS_FIFO_OVERRUN_STATUS_MASK,
				     dstat);
	}

	if (status & SSPIL_INTERRUPT_STATUS_SCHEDULER_STATUS_MASK) {
		chal_sspi_clear_intr(spi_kona->chandle, status, dstat);
    if (!spi_kona->use_dma) {
			/* read rx fifo for the PIO case only when DMA transer
			 * is not in progress */
			chal_sspi_get_fifo_level(spi_kona->chandle,
						SSPI_FIFO_ID_RX0,
					 &fifo_level);
		while (fifo_level) {
			spi_kona->rx(spi_kona);
			spi_kona->rxpend--;
			chal_sspi_get_fifo_level(spi_kona->chandle,
						 SSPI_FIFO_ID_RX0, &fifo_level);
		}
    }

		complete(&spi_kona->xfer_done);
		/* Disable all Interrupt */
		chal_sspi_enable_intr(spi_kona->chandle, 0);
	}

	return IRQ_HANDLED;
}

static int spi_kona_config_clk(struct spi_kona_data *spi_kona,
			       uint32_t clk_rate)
{
	CHAL_HANDLE chandle = spi_kona->chandle;

	if (clk_rate < (12000))
		return -EINVAL;
	clk_disable(spi_kona->ssp_clk);
	clk_set_rate(spi_kona->ssp_clk, clk_rate * 2);

	chal_sspi_set_clk_src_select(chandle, SSPI_CLK_SRC_INTCLK);
	chal_sspi_set_clk_divider(chandle, SSPI_CLK_DIVIDER0, 0);
	chal_sspi_set_clk_divider(chandle, SSPI_CLK_REF_DIVIDER, 0);
	clk_enable(spi_kona->ssp_clk);
	return 0;
}

static void spi_kona_fifo_config(struct spi_kona_data *spi_kona, int enable_dma)
{
	CHAL_HANDLE chandle = spi_kona->chandle;
	CHAL_SSPI_FIFO_DATA_PACK_t bpw = SSPI_FIFO_DATA_PACK_NONE;
	CHAL_SSPI_FIFO_DATA_SIZE_t dsize = SSPI_FIFO_DATA_SIZE_32BIT;

	if (spi_kona->bytes_per_word == 1) {
		bpw = SSPI_FIFO_DATA_PACK_8BIT;
		dsize = SSPI_FIFO_DATA_SIZE_8BIT;
	} else if (spi_kona->bytes_per_word == 2) {
		bpw = SSPI_FIFO_DATA_PACK_16BIT;
		dsize = SSPI_FIFO_DATA_SIZE_16BIT;
	}

	/* Reset FIFO before configuring */
	chal_sspi_fifo_reset(chandle, SSPI_FIFO_ID_TX0);
	chal_sspi_fifo_reset(chandle, SSPI_FIFO_ID_RX0);

	/* config data packing */
	chal_sspi_set_fifo_pack(chandle, SSPI_FIFO_ID_TX0, bpw);
	chal_sspi_set_fifo_pack(chandle, SSPI_FIFO_ID_RX0, bpw);

	/* config data size: For DMA mode, always use 32 bit */
	if (enable_dma) {
		chal_sspi_set_fifo_data_size(chandle, SSPI_FIFO_ID_RX0,
					     SSPI_FIFO_DATA_SIZE_32BIT);
		chal_sspi_set_fifo_data_size(chandle, SSPI_FIFO_ID_TX0,
					     SSPI_FIFO_DATA_SIZE_32BIT);
	} else {
		chal_sspi_set_fifo_data_size(chandle, SSPI_FIFO_ID_RX0, dsize);
		chal_sspi_set_fifo_data_size(chandle, SSPI_FIFO_ID_TX0, dsize);
	}
	return;
}

static int spi_kona_configure(struct spi_kona_data *spi_kona,
			      struct spi_kona_config *config)
{
	CHAL_HANDLE chandle = spi_kona->chandle;
	uint32_t frame_mask = 1;
	int ret;

	/* Configure FIFO Packing and Read/Write Data Size */
	spi_kona_fifo_config(spi_kona, spi_kona->enable_dma);

	/* Configure the clock speed */
	ret = spi_kona_config_clk(spi_kona, config->speed_hz);
	if (ret < 0)
		return ret;

	/* Set idle state */
	if (chal_sspi_set_idle_state(chandle, config->mode & SPI_MODE_3))
		return -EIO;

	/* Set frame data size */
	ret = chal_sspi_set_spi_frame(chandle, &frame_mask,
				  config->mode & SPI_MODE_3, config->bpw, 0);
		
	if (ret < 0)
		return ret;

	return 0;
}

static int spi_kona_setupxfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct spi_kona_data *spi_kona = spi_master_get_devdata(spi->master);
	struct spi_kona_config config;

	config.bpw = t ? t->bits_per_word : spi->bits_per_word;
	config.speed_hz = t ? t->speed_hz : spi->max_speed_hz;
	config.mode = spi->mode;
	config.cs = spi->chip_select;

	if (!config.speed_hz)
		config.speed_hz = spi->max_speed_hz;
	if (!config.bpw)
		config.bpw = spi->bits_per_word;

	/* Initialize the functions for transfer */
	if (config.bpw <= 8) {
		config.bpw = 8;
		spi_kona->bytes_per_word = 1;
		spi_kona->rx = spi_kona_buf_rx_u8;
		spi_kona->tx = spi_kona_buf_tx_u8;
	} else if (config.bpw <= 16) {
		if ((config.mode & SPI_3WIRE) == 0)
			config.bpw = 16;
		spi_kona->bytes_per_word = 2;
		spi_kona->rx = spi_kona_buf_rx_u16;
		spi_kona->tx = spi_kona_buf_tx_u16;
	} else if (config.bpw <= 32) {
		config.bpw = 32;
		spi_kona->bytes_per_word = 4;
		spi_kona->rx = spi_kona_buf_rx_u32;
		spi_kona->tx = spi_kona_buf_tx_u32;
	} else
		BUG();

	return spi_kona_configure(spi_kona, &config);
}

static int spi_kona_config_task(struct spi_device *spi,
				struct spi_transfer *transfer)
{
	struct spi_kona_data *spi_kona = spi_master_get_devdata(spi->master);
	CHAL_HANDLE chandle = spi_kona->chandle;
	chal_sspi_seq_conf_t seq_conf;
	chal_sspi_task_conf_t task_conf;
	int rep_cnt, loop_cnt, seq_idx = 0;
	
	/* task_conf struct initialization */
	memset(&task_conf, 0, sizeof(task_conf));

	/* seq_conf struct initialization */
	memset(&seq_conf, 0, sizeof(seq_conf));

	/* disable scheduler operation */
	if (chal_sspi_enable_scheduler(chandle, 0))
		return -EIO;

	/* task_conf struct configuration */
	task_conf.chan_sel = SSPI_CHAN_SEL_CHAN0;
	task_conf.cs_sel = SSPI_CS_SEL_CS0;
	task_conf.tx_sel = SSPI_TX_SEL_TX0;
	task_conf.rx_sel =
	    (spi->
	     mode & (SPI_LOOP | SPI_3WIRE)) ? SSPI_RX_SEL_COPY_TX0 :
	    SSPI_RX_SEL_RX0;
	task_conf.div_sel = SSPI_CLK_DIVIDER0;
	task_conf.seq_ptr = 0;

	loop_cnt = transfer->len / 256 / spi_kona->bytes_per_word;
	rep_cnt = (transfer->len % (256 * spi_kona->bytes_per_word)) /
		spi_kona->bytes_per_word;
	task_conf.loop_cnt = (loop_cnt) ? loop_cnt - 1 : 0;
	
	if (task_conf.loop_cnt > SSPI_MAX_TASK_LOOP) {
		/* Care needs to be taken to stop this sequence */
		task_conf.loop_cnt = 0;
		task_conf.continuous = 1;
	} else {
		task_conf.continuous = 0;
	}

	task_conf.init_cond_mask = (transfer->tx_buf || transfer->rx_buf) ?
	    (SSPI_TASK_INIT_COND_THRESHOLD_TX0 |
	     SSPI_TASK_INIT_COND_THRESHOLD_RX0) : 0;
	task_conf.wait_before_start = 1;
	if (chal_sspi_set_task(chandle, 0, spi->mode & SPI_MODE_3, &task_conf))
		return -EIO;

	/* configure sequence */
	seq_conf.tx_enable = (transfer->tx_buf) ? TRUE : FALSE;
	seq_conf.rx_enable = (transfer->rx_buf) ? TRUE : FALSE;
	seq_conf.cs_activate = 1;
	seq_conf.cs_deactivate = 0;
	seq_conf.pattern_mode = 0;
	seq_conf.rep_cnt = (loop_cnt) ? 255 : rep_cnt - 1;
	seq_conf.opcode = SSPI_SEQ_OPCODE_COND_JUMP;
	seq_conf.rx_fifo_sel = SSPI_FIFO_ID_RX0;
	seq_conf.tx_fifo_sel = 0;	/* SSPI_FIFO_ID_TX0 */
	seq_conf.frm_sel = 0;
	seq_conf.rx_sidetone_on = 0;
	seq_conf.tx_sidetone_on = 0;
	seq_conf.next_pc = 0;
	if (chal_sspi_set_sequence(chandle, seq_idx, spi->mode & SPI_MODE_3,
				   &seq_conf))
		return -EIO;

	seq_idx++;
	if (loop_cnt && rep_cnt) {
		seq_conf.tx_enable = (transfer->tx_buf) ? TRUE : FALSE;
		seq_conf.rx_enable = (transfer->rx_buf) ? TRUE : FALSE;
		seq_conf.cs_activate = 1;
		seq_conf.cs_deactivate = 0;
		seq_conf.clk_idle = 0;
		seq_conf.pattern_mode = 0;
		seq_conf.rep_cnt = rep_cnt -1;
		seq_conf.opcode = SSPI_SEQ_OPCODE_NEXT_PC;
		seq_conf.rx_fifo_sel = SSPI_FIFO_ID_RX0;
		seq_conf.tx_fifo_sel = 0;	/* SSPI_FIFO_ID_TX0 */
		seq_conf.frm_sel = 0;
		seq_conf.rx_sidetone_on = 0;
		seq_conf.tx_sidetone_on = 0;
		seq_conf.next_pc = seq_idx + 1;
		if (chal_sspi_set_sequence(chandle, seq_idx,
			spi->mode & SPI_MODE_3, &seq_conf))
			return -EIO;
		seq_idx++;
		
	}

	seq_conf.tx_enable = FALSE;
	seq_conf.rx_enable = FALSE;
	seq_conf.cs_activate = 0;
	seq_conf.cs_deactivate = 1;
	seq_conf.clk_idle = 1;	/* Do not send clk in this sequence */
	seq_conf.pattern_mode = 0;
	seq_conf.rep_cnt = 0;
	seq_conf.opcode = SSPI_SEQ_OPCODE_STOP;
	seq_conf.rx_fifo_sel = 0;
	seq_conf.tx_fifo_sel = 0;
	seq_conf.frm_sel = 0;
	seq_conf.rx_sidetone_on = 0;
	seq_conf.tx_sidetone_on = 0;
	seq_conf.next_pc = 0;
	if (chal_sspi_set_sequence(chandle, seq_idx,
		spi->mode & SPI_MODE_3, &seq_conf))
			return -EIO;
	/* enable scheduler operation */
	if (chal_sspi_enable_scheduler(chandle, 1))
		return -EIO;

	return 0;
}

#ifdef CONFIG_DMAC_PL330
static void spi_dma_callback(void *priv, enum pl330_xfer_status status)
{
	struct completion *c = (struct completion *)priv;

	if (status == DMA_PL330_XFER_OK)
		pr_debug("DMA transfer status ok\n");
	else if (status == DMA_PL330_XFER_ERR)
		pr_err("DMA transfer error\n");
	else if (status == DMA_PL330_XFER_ABORT)
		pr_err("DMA transfer aborted\n");
	else
		pr_err("DMA transfer Invalid status!!!\n");

	/* If process waiting for completion */
	if (c)
		complete(c);
	else
		pr_err("NULL pointer passed to %s!!!!\n", __func__);
}
#elif CONFIG_MAP_SDMA
static void spi_dma_callback(DMA_Device_t dev, int reason, void *priv)
{
	struct completion *dmaDone = priv;

	if (reason & DMA_HANDLER_REASON_TRANSFER_COMPLETE)
		complete(dmaDone);

	if (reason != DMA_HANDLER_REASON_TRANSFER_COMPLETE)
		pr_err("%s: called with reason = 0x%x", __func__, reason);
}
#endif

#if defined(CONFIG_DMAC_PL330) || defined(CONFIG_MAP_SDMA)
static int spi_kona_dma_xfer_rx(struct spi_kona_data *spi_kona)
{
	CHAL_HANDLE chandle = spi_kona->chandle;
	int ret = -EIO;
	u32 rx_fifo = dma_fifo_base[spi_kona->master->bus_num] +
		    chal_sspi_rx0_get_dma_port_addr_offset();

#ifdef CONFIG_DMAC_PL300
	dma_addr_t dma_rx_buf = 0;
	u32 cfg_rx;
#ifdef DMA_BURST_CONFIG_16_BYTES
	/* bs = 4, bl = 4, 16 bytes xfer per request */
	cfg_rx = DMA_CFG_SRC_ADDR_FIXED | DMA_CFG_DST_ADDR_INCREMENT |
	    DMA_CFG_BURST_SIZE_4 | DMA_CFG_BURST_LENGTH_4;
#else /* Burst = 64 Bytes */
	cfg_rx = DMA_CFG_SRC_ADDR_FIXED | DMA_CFG_DST_ADDR_INCREMENT |
	    DMA_CFG_BURST_SIZE_4 | DMA_CFG_BURST_LENGTH_16;
#endif
	/* Get DMA'ble address */
	dma_rx_buf = dma_map_single(NULL, (void *)spi_kona->rx_buf,
				    spi_kona->count, DMA_FROM_DEVICE);
	if (!dma_rx_buf)
		goto err;

	/* Setup RX DMA */
	if (dma_setup_transfer(spi_kona->rx_dma_chan, rx_fifo,
			       dma_rx_buf, spi_kona->count,
			       DMA_DIRECTION_DEV_TO_MEM_FLOW_CTRL_PERI,
			       cfg_rx) != 0) {
		pr_err("dma_setup_transfer(RX) failed\n");
		goto err1;
	}
#elif CONFIG_MAP_SDMA
	if (!dma_mmap_dma_is_supported((void *)spi_kona->rx_buf)) {
		pr_err("%s: Unsupported buffer! buf=0x%x\n",
				__func__, (uint32_t)spi_kona->rx_buf);
		ret = -EINVAL;
		goto err;
	}

	ret = dma_mmap_init_map(&spi_kona->rx_dma_mmap);
	if (ret < 0) {
		pr_err("%s: dma_mmap_init_map failed\n", __func__);
		goto err;
	}

	ret = dma_mmap_map(&spi_kona->rx_dma_mmap, (void *)spi_kona->rx_buf,
		spi_kona->count, DMA_FROM_DEVICE);
	if (ret < 0) {
		pr_err("%s: dma_mmap_map failed\n", __func__);
		goto err;
	}
	/* Create SDMA descriptor rings */
	ret = sdma_map_create_descriptor_ring(spi_kona->rx_dma_chan,
			&spi_kona->rx_dma_mmap,
			rx_fifo,
			DMA_UPDATE_MODE_NO_INC);
	if (ret < 0) {
		pr_err("%s: sdma_map_create_descriptor_ring failed\n", __func__);
		goto err1;
	}
#endif
	/* Start RX DMA channel first */
	if (dma_start_transfer(spi_kona->rx_dma_chan) != 0) {
		pr_err("dma_start_transfer failed on RX chan\n");
		goto err1;
	}
	/* Enable Overrun interrupt */
	chal_sspi_enable_intr(chandle,
	SSPIL_INTERRUPT_ENABLE_FIFO_OVERRUN_INTERRUPT_ENB_MASK |
	SSPIL_INTERRUPT_ENABLE_SCHEDULER_INTERRUPT_ENB_MASK);

	/* Trigger RX FIFO DMA */
	chal_sspi_enable_dma(chandle, SSPI_DMA_CHAN_SEL_CHAN_RX0,
			     SSPI_FIFO_ID_RX0, 1);

	/* Wait for RX DMA completion */
	if ((wait_for_completion_interruptible_timeout
	     (&spi_kona->rx_dma_evt, msecs_to_jiffies(SSPI_WFC_TIME_OUT))) == 0) {
		pr_err("SPI Rx DMA Transfer timed out/interrupted\n");

		pr_err("Rx-Only SDMA %d bytes failed - dump sdma registers %d\n",
			spi_kona->count,
			sdma_dump_debug_info(spi_kona->rx_dma_chan));
		chal_sspi_dump_registers(chandle);
	}
	else
		ret = spi_kona->count;

	/* Disable DMA for RX/TX FIFO */
	chal_sspi_enable_dma(chandle, SSPI_DMA_CHAN_SEL_CHAN_RX0,
			     SSPI_FIFO_ID_RX0, 0);

	dma_stop_transfer(spi_kona->rx_dma_chan);
err1:
#ifdef CONFIG_DMAC_PL300
	dma_unmap_single(NULL, dma_rx_buf, spi_kona->count, DMA_FROM_DEVICE);
#elif CONFIG_MAP_SDMA
	dma_mmap_unmap(&spi_kona->rx_dma_mmap, DMA_MMAP_DIRTIED);
#endif
err:
	return ret;
}

static int spi_kona_dma_xfer_tx(struct spi_kona_data *spi_kona)
{
	CHAL_HANDLE chandle = spi_kona->chandle;
	int ret = -EIO;
	u32 tx_fifo = dma_fifo_base[spi_kona->master->bus_num] +
		    chal_sspi_tx0_get_dma_port_addr_offset();

#ifdef CONFIG_DMAC_PL300
	dma_addr_t dma_tx_buf = 0;
	u32 cfg_tx;
#ifdef DMA_BURST_CONFIG_16_BYTES
	/* bs = 4, bl = 4, 16 bytes xfer per request */
	cfg_tx = DMA_CFG_SRC_ADDR_INCREMENT | DMA_CFG_DST_ADDR_FIXED |
	    DMA_CFG_BURST_SIZE_4 | DMA_CFG_BURST_LENGTH_4;
#else /* Burst = 64 Bytes */
	cfg_tx = DMA_CFG_SRC_ADDR_INCREMENT | DMA_CFG_DST_ADDR_FIXED |
	    DMA_CFG_BURST_SIZE_4 | DMA_CFG_BURST_LENGTH_16;
#endif
	/* Get DMA'ble address */
	dma_tx_buf = dma_map_single(NULL, (void *)spi_kona->tx_buf,
				    spi_kona->count, DMA_TO_DEVICE);
	if (!dma_tx_buf)
		goto err;
	/* Setup TX DMA */
	if (dma_setup_transfer(spi_kona->tx_dma_chan, dma_tx_buf,
			       tx_fifo, spi_kona->count,
			       DMA_DIRECTION_MEM_TO_DEV_FLOW_CTRL_PERI,
			       cfg_tx) != 0) {
		pr_err("dma_setup_transfer(TX) failed\n");
		goto err1;
	}
#elif CONFIG_MAP_SDMA
	dma_addr_t dma_tx_buf = 0;
	if (!dma_mmap_dma_is_supported((void *)spi_kona->tx_buf)) {
		pr_err("%s: Unsupported buffer! buf=0x%x\n",
				__func__, (uint32_t)spi_kona->tx_buf);
		ret = -EINVAL;
		goto err;
	}

	/* Get DMA'ble address */
	dma_tx_buf = dma_map_single(NULL, (void *)spi_kona->tx_buf,
		spi_kona->count, DMA_TO_DEVICE);
	if (!dma_tx_buf)
		goto err;

#endif

	/* Start TX DMA channel */
	if (sdma_transfer(spi_kona->tx_dma_chan, dma_tx_buf, tx_fifo,
					  spi_kona->count) != 0) {
		pr_err("dma_start_transfer failed on TX chan\n");
		goto err1;
	}

	/* Enable Overrun interrupt */
	chal_sspi_enable_intr(chandle,
	SSPIL_INTERRUPT_ENABLE_FIFO_UNDERRUN_INTERRUPT_ENB_MASK |
	SSPIL_INTERRUPT_ENABLE_SCHEDULER_INTERRUPT_ENB_MASK);

	/* Trigger TX FIFO DMA */
	chal_sspi_enable_dma(chandle, SSPI_DMA_CHAN_SEL_CHAN_TX0,
			     SSPI_FIFO_ID_TX0, 1);

	/* Wait for TX DMA completion */
	if ((wait_for_completion_interruptible_timeout
	     (&spi_kona->tx_dma_evt,
	      msecs_to_jiffies(SSPI_WFC_TIME_OUT))) == 0) {
		pr_err("SPI Tx DMA Transfer timed out/interrupted\n");

		pr_err("Tx-Only SDMA %d bytes failed - dump sdma registers %d\n",
			spi_kona->count,
			sdma_dump_debug_info(spi_kona->tx_dma_chan));
		chal_sspi_dump_registers(chandle);
	} else {
		ret = spi_kona->count;
	}
	chal_sspi_enable_dma(chandle, SSPI_DMA_CHAN_SEL_CHAN_TX0,
			     SSPI_FIFO_ID_TX0, 0);

err1:
#ifdef CONFIG_DMAC_PL300
	dma_unmap_single(NULL, dma_tx_buf, spi_kona->count, DMA_TO_DEVICE);
#elif CONFIG_MAP_SDMA
	dma_unmap_single(NULL, dma_tx_buf, spi_kona->count, DMA_TO_DEVICE);
#endif
err:
	return ret;
}

static int spi_kona_dma_xfer(struct spi_kona_data *spi_kona)
{
	CHAL_HANDLE chandle = spi_kona->chandle;
	int ret = -EIO;
	struct spi_master *master = spi_kona->master;
	u32 tx_fifo = dma_fifo_base[master->bus_num] +
		    chal_sspi_tx0_get_dma_port_addr_offset();
	u32 rx_fifo = dma_fifo_base[master->bus_num] +
		    chal_sspi_rx0_get_dma_port_addr_offset();

#ifdef CONFIG_DMAC_PL330
	dma_addr_t dma_rx_buf = 0, dma_tx_buf = 0;
	u32 cfg_rx, cfg_tx;
#ifdef DMA_BURST_CONFIG_16_BYTES
	/* bs = 4, bl = 4, 16 bytes xfer per request */
	cfg_tx = DMA_CFG_SRC_ADDR_INCREMENT | DMA_CFG_DST_ADDR_FIXED |
	    DMA_CFG_BURST_SIZE_4 | DMA_CFG_BURST_LENGTH_4;
	cfg_rx = DMA_CFG_SRC_ADDR_FIXED | DMA_CFG_DST_ADDR_INCREMENT |
	    DMA_CFG_BURST_SIZE_4 | DMA_CFG_BURST_LENGTH_4;
#else /* Burst = 64 Bytes */
	cfg_tx = DMA_CFG_SRC_ADDR_INCREMENT | DMA_CFG_DST_ADDR_FIXED |
	    DMA_CFG_BURST_SIZE_4 | DMA_CFG_BURST_LENGTH_16;
	cfg_rx = DMA_CFG_SRC_ADDR_FIXED | DMA_CFG_DST_ADDR_INCREMENT |
	    DMA_CFG_BURST_SIZE_4 | DMA_CFG_BURST_LENGTH_16;
#endif
	/* Get DMA'ble address */
	if (spi_kona->tx_buf != NULL) {
		dma_tx_buf = dma_map_single(NULL, (void *)spi_kona->tx_buf,
					    spi_kona->count, DMA_TO_DEVICE);
		if (!dma_tx_buf)
			return ret;
	}
	if (spi_kona->rx_buf != NULL) {
		dma_rx_buf = dma_map_single(NULL, (void *)spi_kona->rx_buf,
					    spi_kona->count, DMA_FROM_DEVICE);
		if (!dma_rx_buf)
			goto err;
	}
	/* Setup TX DMA */
	if (dma_setup_transfer(spi_kona->tx_dma_chan, dma_tx_buf,
			       tx_fifo, spi_kona->count,
			       DMA_DIRECTION_MEM_TO_DEV_FLOW_CTRL_PERI,
			       cfg_tx) != 0) {
		pr_err("dma_setup_transfer(TX) failed\n");
		goto err1;
	}

	/* Setup RX DMA */
	if (dma_setup_transfer(spi_kona->rx_dma_chan, rx_fifo,
			       dma_rx_buf, spi_kona->count,
			       DMA_DIRECTION_DEV_TO_MEM_FLOW_CTRL_PERI,
			       cfg_rx) != 0) {
		pr_err("dma_setup_transfer(RX) failed\n");
		goto err1;
	}
#elif CONFIG_MAP_SDMA
	dma_addr_t dma_tx_buf = 0;
	if (!dma_mmap_dma_is_supported((void *)spi_kona->tx_buf)) {
		pr_err("%s: Unsupported buffer! buf=0x%x\n",
				__func__, (uint32_t)spi_kona->tx_buf);
		ret = -EINVAL;
		return ret;
	}

	if (!dma_mmap_dma_is_supported((void *)spi_kona->rx_buf)) {
		pr_err("%s: Unsupported buffer! buf=0x%x\n",
				__func__, (uint32_t)spi_kona->rx_buf);
		ret = -EINVAL;
		return ret;
	}

	/* Get DMA'ble address */
	if (spi_kona->tx_buf != NULL) {
		dma_tx_buf = dma_map_single(NULL, (void *)spi_kona->tx_buf,
		spi_kona->count, DMA_TO_DEVICE);
		if (!dma_tx_buf)
		return ret;
	}

	ret = dma_mmap_init_map(&spi_kona->rx_dma_mmap);
	if (ret < 0) {
		pr_err("%s: dma_mmap_init_map failed\n", __func__);
		goto err;
	}

	ret = dma_mmap_map(&spi_kona->rx_dma_mmap, (void *)spi_kona->rx_buf,
		spi_kona->count, DMA_FROM_DEVICE);
	if (ret < 0) {
		pr_err("%s: dma_mmap_map failed\n", __func__);
		goto err;
	}
	/* Create SDMA descriptor rings */
	ret = sdma_map_create_descriptor_ring(spi_kona->rx_dma_chan,
			&spi_kona->rx_dma_mmap,
			rx_fifo,
			DMA_UPDATE_MODE_NO_INC);
	if (ret < 0) {
		pr_err("%s: sdma_map_create_descriptor_ring failed\n", __func__);
		goto err1;
	}

#endif

	/* Enable Overrun interrupt */
	chal_sspi_enable_intr(chandle,
	SSPIL_INTERRUPT_ENABLE_FIFO_OVERRUN_INTERRUPT_ENB_MASK |
	SSPIL_INTERRUPT_ENABLE_SCHEDULER_INTERRUPT_ENB_MASK);

	/* Reset FIFO before configuring */
	chal_sspi_fifo_reset(chandle, SSPI_FIFO_ID_TX0);
	chal_sspi_fifo_reset(chandle, SSPI_FIFO_ID_RX0);

	/* Start TX DMA channel */
	if (sdma_transfer(spi_kona->tx_dma_chan, dma_tx_buf, tx_fifo,  spi_kona->count) != 0) {
		pr_err("dma_start_transfer failed on TX chan\n");
		goto err1;
	}

	/* Start RX DMA channel first */
	if (dma_start_transfer(spi_kona->rx_dma_chan) != 0) {
		pr_err("dma_start_transfer failed on RX chan\n");
		goto err1;
	}

	/* Trigger RX FIFO DMA */
	chal_sspi_enable_dma(chandle, SSPI_DMA_CHAN_SEL_CHAN_RX0,
			     SSPI_FIFO_ID_RX0, 1);

	/* Trigger TX FIFO DMA */
	chal_sspi_enable_dma(chandle, SSPI_DMA_CHAN_SEL_CHAN_TX0,
			     SSPI_FIFO_ID_TX0, 1);


	/* Wait for RX DMA completion */
	if ((wait_for_completion_interruptible_timeout
		  (&spi_kona->rx_dma_evt,
		   msecs_to_jiffies(SSPI_WFC_TIME_OUT))) == 0) {
		pr_err("SPI Rx DMA Transfer timed out/interrupted\n");
		pr_err("Rx SDMA %d bytes failed - dump sdma registers %d\n",
			spi_kona->count,
			sdma_dump_debug_info(spi_kona->rx_dma_chan));
		pr_err("Tx SDMA %d bytes failed - dump sdma registers %d\n",
			spi_kona->count,
			sdma_dump_debug_info(spi_kona->tx_dma_chan));

		chal_sspi_dump_registers(chandle);
	}
	/* Wait for TX DMA completion */
	else if ((wait_for_completion_interruptible_timeout
	     (&spi_kona->tx_dma_evt,
	      msecs_to_jiffies(SSPI_WFC_TIME_OUT))) == 0) {
		pr_err(" %s SPI Tx DMA Transfer timed out/interrupted\n",
		       __func__);
		pr_err("Tx SDMA %d bytes failed - dump sdma registers %d\n",
			spi_kona->count,
			sdma_dump_debug_info(spi_kona->tx_dma_chan));
		chal_sspi_dump_registers(chandle);
	}
	else
		ret = spi_kona->count;

	/* Disable DMA for RX/TX FIFO */
	chal_sspi_enable_dma(chandle, SSPI_DMA_CHAN_SEL_CHAN_RX0,
			     SSPI_FIFO_ID_RX0, 0);
	chal_sspi_enable_dma(chandle, SSPI_DMA_CHAN_SEL_CHAN_TX0,
			     SSPI_FIFO_ID_TX0, 0);

#if CONFIG_MAP_SDMA
	dma_stop_transfer(spi_kona->rx_dma_chan);
#endif
err1:
#ifdef CONFIG_DMAC_PL300
	dma_unmap_single(NULL, dma_rx_buf, spi_kona->count, DMA_FROM_DEVICE);
#elif CONFIG_MAP_SDMA
	dma_mmap_unmap(&spi_kona->rx_dma_mmap, DMA_MMAP_DIRTIED);
#endif
err:
#ifdef CONFIG_DMAC_PL300
	dma_unmap_single(NULL, dma_tx_buf, spi_kona->count, DMA_TO_DEVICE);
#elif CONFIG_MAP_SDMA
	dma_unmap_single(NULL, dma_tx_buf, spi_kona->count, DMA_TO_DEVICE);
#endif
	return ret;
}
#else

static int spi_kona_dma_xfer_tx(struct spi_kona_data *spi_kona)
{
	return 0;
}
static int spi_kona_dma_xfer_rx(struct spi_kona_data *spi_kona)
{
	return 0;
}
static int spi_kona_dma_xfer(struct spi_kona_data *spi_kona)
{
	return 0;
}
#endif

static int spi_kona_txrxfer_bufs(struct spi_device *spi,
				 struct spi_transfer *transfer)
{
	struct spi_kona_data *spi_kona = spi_master_get_devdata(spi->master);
	CHAL_HANDLE chandle = spi_kona->chandle;
	int32_t unaligned, xfer_len = 0;
	int status;
	int ret;
	unsigned temp_length;

	if (transfer->len % spi_kona->bytes_per_word)
		return -EINVAL;

	/* Set default FIFO threshold */
	if (transfer->tx_buf) {
		ret = chal_sspi_set_fifo_threshold(chandle, SSPI_FIFO_ID_TX0,
						   min((int)transfer->len,
							   SSPI_FIFO_THRESHOLD));
		if (ret < 0)
			return ret;
	}
	else {
		ret = chal_sspi_set_fifo_threshold(chandle,
					SSPI_FIFO_ID_TX0, 0);
		if (ret < 0)
			return ret;
	}

	/* Check if 8-byte unalligned address buffer was passed */
	if (transfer->rx_buf != NULL && ((int)(transfer->rx_buf) % 8) != 0) {
		pr_err("8-byte unalligned access seen for RX buffer\n");
		return -EINVAL;
	}

	if (transfer->tx_buf != NULL && ((int)(transfer->tx_buf) % 8) != 0) {
		pr_err("8-byte unalligned access seen for TX buffer\n");
		return -EINVAL;
	}

	spi_kona->rx_buf = transfer->rx_buf;
	spi_kona->tx_buf = transfer->tx_buf;

	/* bytes to be transfered in PIO */
	unaligned = transfer->len % FIFO_BURST_ALIGNMENT;
	spi_kona->rxpend = 0;
	init_completion(&spi_kona->xfer_done);
	spi_kona->count = transfer->len - unaligned;

	temp_length = transfer->len;
	transfer->len = (spi_kona->count) ? spi_kona->count : transfer->len;
	spi_kona_config_task(spi, transfer);
	transfer->len = temp_length;

	spi_kona->use_dma = 0;

	/* Use DMA mode transfer */
	if (spi_kona->enable_dma && spi_kona->count) {
		spi_kona->use_dma = 1;
		/* Setup completion events */
		init_completion(&spi_kona->tx_dma_evt);
		init_completion(&spi_kona->rx_dma_evt);

		/* Disable DMA before config */
		chal_sspi_enable_dma(chandle, SSPI_DMA_CHAN_SEL_CHAN_TX0,
				     SSPI_FIFO_ID_TX0, 0);
		chal_sspi_enable_dma(chandle, SSPI_DMA_CHAN_SEL_CHAN_RX0,
				     SSPI_FIFO_ID_RX0, 0);

		/* Disable DMA-AXI autogating */
	//	clk_disable_autogate(spi_kona->dma_axi_clk);

#ifdef DMA_BURST_CONFIG_16_BYTES
		chal_sspi_dma_set_burstsize(chandle, SSPI_DMA_CHAN_SEL_CHAN_TX0,
					    CHAL_SSPI_DMA_BURSTSIZE_16BYTES);
		chal_sspi_dma_set_burstsize(chandle, SSPI_DMA_CHAN_SEL_CHAN_RX0,
					    CHAL_SSPI_DMA_BURSTSIZE_16BYTES);
#else /* Burst = 64 Bytes */
		chal_sspi_dma_set_burstsize(chandle, SSPI_DMA_CHAN_SEL_CHAN_TX0,
					    CHAL_SSPI_DMA_BURSTSIZE_64BYTES);
		chal_sspi_dma_set_burstsize(chandle, SSPI_DMA_CHAN_SEL_CHAN_RX0,
					    CHAL_SSPI_DMA_BURSTSIZE_64BYTES);
#endif

		if (spi_kona->rx_buf != NULL && spi_kona->tx_buf != NULL) {
			xfer_len = spi_kona_dma_xfer(spi_kona);
			spi_kona->rx_buf = transfer->rx_buf + spi_kona->count;
			spi_kona->tx_buf = transfer->tx_buf + spi_kona->count;
		} else {
			if (spi_kona->rx_buf != NULL) {
				xfer_len = spi_kona_dma_xfer_rx(spi_kona);
				spi_kona->rx_buf =
				    transfer->rx_buf + spi_kona->count;
			}
			if (spi_kona->tx_buf != NULL) {
				xfer_len = spi_kona_dma_xfer_tx(spi_kona);
				spi_kona->tx_buf =
				    transfer->tx_buf + spi_kona->count;
			}
		}

		if (xfer_len == spi_kona->count)
			spi_kona->count = unaligned;	/* DMA Success */
		else
			spi_kona->count = 0;	/* DMA failed, no PIO */

		spi_kona->use_dma = 0;  /* DMA transfer is done here */
		if ((wait_for_completion_interruptible_timeout
			 (&spi_kona->xfer_done,
			  msecs_to_jiffies(SSPI_WFC_TIME_OUT))) == 0)
			xfer_len = -EIO;

		/* Re-enable DMA-AXI autogating */
//		clk_enable_autogate(spi_kona->dma_axi_clk);
	} else
		spi_kona->count = transfer->len;

	if (spi_kona->count) {	/* PIO mode with Interrupts */
		xfer_len += spi_kona->count;

		/* If the remainder bits needs to be transferred
		 * through PIO after DMA, reconfigure */
		if (spi_kona->count != transfer->len) {
			if (spi_kona->tx_buf) {
				ret =
				    chal_sspi_set_fifo_threshold(chandle,
							 SSPI_FIFO_ID_TX0,
							 spi_kona->count);
				if (ret < 0)
					return ret;
			}
			else {
				ret =
				    chal_sspi_set_fifo_threshold(chandle,
							 SSPI_FIFO_ID_TX0, 0);
				if (ret < 0)
					return ret;
			}
			/* transfer length set to the bytes
			 * to be trasferred by PIO */
			temp_length = transfer->len;
			transfer->len = spi_kona->count;
			spi_kona->use_dma = 0;
			spi_kona_config_task(spi, transfer);
			transfer->len = temp_length;
		}

		/* Reconfigure FIFO Pack and Read/Write Data Size if DMA mode */
		if (spi_kona->enable_dma)
			spi_kona_fifo_config(spi_kona, 0);

		if (spi_kona->tx_buf)
			spi_kona_tx_data(spi_kona);

		chal_sspi_enable_intr(chandle,
		SSPIL_INTERRUPT_ENABLE_SCHEDULER_INTERRUPT_ENB_MASK |
		SSPIL_INTERRUPT_ENABLE_SCHEDULER_INTERRUPT_ENB_MASK);
		if ((wait_for_completion_interruptible_timeout
		     (&spi_kona->xfer_done,
		      msecs_to_jiffies(SSPI_WFC_TIME_OUT))) == 0)
			xfer_len = -EIO;

	}

	chal_sspi_enable_scheduler(chandle, 0);
	chal_sspi_get_intr_status(chandle, &status, NULL);

	/* Check if the under-flow or over-flow interrupts are set -
	 * clear if set */
	if (status &
	    (SSPIL_INTERRUPT_ENABLE_FIFO_OVERRUN_INTERRUPT_ENB_MASK |
	     SSPIL_INTERRUPT_ENABLE_FIFO_UNDERRUN_INTERRUPT_ENB_MASK))
		chal_sspi_clear_intr(chandle, status,
		((SSPIL_DSP_DETAIL_INTERRUPT_STATUS_FIFO_OVERRUN_VECTOR_MASK &
		(0x1 <<
		SSPIL_DSP_DETAIL_INTERRUPT_STATUS_FIFO_OVERRUN_VECTOR_SHIFT)) |
		(SSPIL_DSP_DETAIL_INTERRUPT_STATUS_FIFO_UNDERRUN_VECTOR_MASK &
		(0x1 <<
		SSPIL_DSP_DETAIL_INTERRUPT_STATUS_FIFO_UNDERRUN_VECTOR_SHIFT)))
		);
	else
		chal_sspi_clear_intr(chandle, status, 0);

	/* Reset FIFO before configuring */
	chal_sspi_fifo_reset(chandle, SSPI_FIFO_ID_TX0);
	chal_sspi_fifo_reset(chandle, SSPI_FIFO_ID_RX0);

	return xfer_len;
}

static void spi_kona_chipselect(struct spi_device *spi, int is_active)
{
	/* TODO
	 * Need to do CS functionality here, can be platform specific
	 */
	return;
}

/*
 * This costs a task context per controller, running the queue by
 * performing each transfer in sequence.
 */
static void spi_kona_work(struct work_struct *work)
{
	struct spi_kona_data *spi_kona =
	    container_of(work, struct spi_kona_data, work);
	unsigned long flags;
	int do_setup = -1;
	struct spi_master *master = spi_kona->master;
	u16 dummy_buffer[8];

	spin_lock(&spi_kona->lock);
	spi_kona->busy = 1;
	spin_unlock(&spi_kona->lock);
	while (!list_empty(&spi_kona->queue)) {
		struct spi_message *m;
		struct spi_device *spi;
		struct spi_transfer *t = NULL;
		unsigned cs_change;
		int status;

		m = container_of(spi_kona->queue.next, struct spi_message,
				 queue);

		spin_lock_irqsave(&spi_kona->lock, flags);
		list_del_init(&m->queue);
		spin_unlock_irqrestore(&spi_kona->lock, flags);

		spi = m->spi;
		cs_change = 1;
		status = 0;

		if (master->bus_num != 0) {
			/*turn on caph clock for ssp1 and ssp2 */
			csl_caph_ControlHWClock(TRUE);
		}
		clk_enable(spi_kona->ssp_clk);
		clk_enable(spi_kona->ssp_apb_clk);
		list_for_each_entry(t, &m->transfers, transfer_list) {

			/* override speed or wordsize? */
			if (t->speed_hz || t->bits_per_word)
				do_setup = 1;

			/* init (-1) or override (1) transfer params */
			if (do_setup != 0) {
				status = spi_kona_setupxfer(spi, t);
				if (status < 0)
					break;
			}

			if (cs_change)
				spi_kona_chipselect(spi, CS_ACTIVE);
			cs_change = t->cs_change;

			if (!t->tx_buf && !t->rx_buf && t->len) {
				status = -EINVAL;
				break;
			}
			if (spi->mode & SPI_3WIRE) {
				if (t->tx_buf)
					t->rx_buf = (u16 *) (((int)
					dummy_buffer + 8) & 0xFFFFFFF8);
		/* Dummy buffer for Full duplex transfer and 8 byte aligned*/
				else
					t->tx_buf = (u16 *) (((int)
					dummy_buffer + 8) & 0xFFFFFFF8);
			}

			if (t->len) {
				if (!m->is_dma_mapped)
					t->rx_dma = t->tx_dma = 0;
				status = spi_kona_txrxfer_bufs(spi, t);
			}
			if (status > 0)
				m->actual_length += status;
			if (status != t->len) {
				/* always report some kind of error */
				if (status >= 0)
					status = -EREMOTEIO;
				break;
			}
			status = 0;

			/* protocol tweaks before next transfer */
			if (t->delay_usecs)
				udelay(t->delay_usecs);

			if (!cs_change)
				continue;
			if (t->transfer_list.next == &m->transfers)
				break;

			/* sometimes a short mid-message deselect of the chip
			 * may be needed to terminate a mode or command
			 */
			spi_kona_chipselect(spi, CS_INACTIVE);
		}

		m->status = status;
		m->complete(m->context);

		/* restore speed and wordsize if it was overridden */
		if (do_setup == 1) {
			status = spi_kona_setupxfer(spi, NULL);
			if (status < 0)
				break;
		}
		do_setup = 0;

		/* normally deactivate chipselect ... unless no error and
		 * cs_change has hinted that the next message will probably
		 * be for this chip too.
		 */
		if (!(status == 0 && cs_change))
			spi_kona_chipselect(spi, CS_INACTIVE);

		clk_disable(spi_kona->ssp_clk);
		clk_disable(spi_kona->ssp_apb_clk);
		if (master->bus_num != 0) {
			/*turn on caph clock for ssp1 and ssp2 */
			csl_caph_ControlHWClock(FALSE);
		}
	}
	spin_lock(&spi_kona->lock);
	spi_kona->busy = 0;
	spin_unlock(&spi_kona->lock);
}

static int spi_kona_transfer(struct spi_device *spi, struct spi_message *m)
{
	struct spi_kona_data *spi_kona;
	unsigned long flags;
	int status = 0;

	m->actual_length = 0;
	m->status = -EINPROGRESS;

	spi_kona = spi_master_get_devdata(spi->master);

	spin_lock_irqsave(&spi_kona->lock, flags);
	if (!spi->max_speed_hz) {
		status = -ENETDOWN;
		pr_info("spi_kona_transfer error %d\n", status);
	}
	else {
		list_add_tail(&m->queue, &spi_kona->queue);
		queue_work(spi_kona->workqueue, &spi_kona->work);
	}
	spin_unlock_irqrestore(&spi_kona->lock, flags);
	return status;
}

static int spi_kona_setup(struct spi_device *spi)
{
	dev_dbg(&spi->dev, "%s: mode %d, %u bpw, %d hz\n", __func__,
		spi->mode, spi->bits_per_word, spi->max_speed_hz);

	spi_kona_chipselect(spi, CS_INACTIVE);

	return 0;
}

static void spi_kona_cleanup(struct spi_device *spi)
{
	/* Any SPI device cleanup needs to be done here */
	return;
}

static int spi_kona_config_spi_hw(struct spi_kona_data *spi_kona)
{
	CHAL_HANDLE chandle;

	chandle = chal_sspi_init((uint32_t) spi_kona->base);
	if (!chandle) {
		pr_err("%s: invalid CHAL handler\n", __func__);
		return -ENXIO;
	}
	/* Soft Reset SSPI */
	chal_sspi_soft_reset(chandle);
	/* Driver supports only Master Mode */
	chal_sspi_set_mode(chandle, SSPI_MODE_MASTER);
	/*
	 * Set SSPI IDLE State in Mode 0 SPI
	 * Currently only Mode 0 SPI supported by the driver
	 */
	if (chal_sspi_set_idle_state(chandle, SSPI_PROT_SPI_MODE0))
		return -EIO;

	/* Set SSPI FIFO Size: Rx0/Tx0 - Full, other Rx/Tx to zero */
	chal_sspi_set_fifo_size(chandle, SSPI_FIFO_ID_RX0, SSPI_FIFO_SIZE_FULL);
	chal_sspi_set_fifo_size(chandle, SSPI_FIFO_ID_RX1, SSPI_FIFO_SIZE_NONE);
	chal_sspi_set_fifo_size(chandle, SSPI_FIFO_ID_RX2, SSPI_FIFO_SIZE_NONE);
	chal_sspi_set_fifo_size(chandle, SSPI_FIFO_ID_RX3, SSPI_FIFO_SIZE_NONE);
	chal_sspi_set_fifo_size(chandle, SSPI_FIFO_ID_TX0, SSPI_FIFO_SIZE_FULL);
	chal_sspi_set_fifo_size(chandle, SSPI_FIFO_ID_TX1, SSPI_FIFO_SIZE_NONE);
	chal_sspi_set_fifo_size(chandle, SSPI_FIFO_ID_TX2, SSPI_FIFO_SIZE_NONE);
	chal_sspi_set_fifo_size(chandle, SSPI_FIFO_ID_TX3, SSPI_FIFO_SIZE_NONE);

	chal_sspi_enable_intr(chandle, 0);
	chal_sspi_enable_error_intr(chandle,
	~SSPIL_INTERRUPT_ERROR_ENABLE_RESERVED_MASK);
	chal_sspi_enable(chandle, 1);

	spi_kona->chandle = chandle;

	return 0;
}

#ifdef CONFIG_DMAC_PL330
static int spi_kona_setup_dma(struct spi_kona_data *spi_kona)
{
	struct spi_master *master = spi_kona->master;

	/* Aquire DMA channels */
	if (dma_request_chan(&spi_kona->tx_dma_chan,
			     dma_tx_chan_name[master->bus_num]) != 0) {
		pr_err("%s: Tx dma_request_chan failed\n", __func__);
		return -EIO;
	}
	if (dma_request_chan(&spi_kona->rx_dma_chan,
			     dma_rx_chan_name[master->bus_num]) != 0) {
		pr_err("%s: Rx dma_request_chan failed\n", __func__);
		goto err;
	}
	/* Register DMA callback */
	if (dma_register_callback(spi_kona->tx_dma_chan, spi_dma_callback,
				  &spi_kona->tx_dma_evt) != 0) {
		pr_err("%s: Tx dma_register_callback failed\n", __func__);
		goto err1;
	}
	if (dma_register_callback(spi_kona->rx_dma_chan, spi_dma_callback,
				  &spi_kona->rx_dma_evt) != 0) {
		pr_err("%s: Rx dma_register_callback failed\n", __func__);
		goto err2;
	}
	return 0;
err2:
	dma_free_callback(spi_kona->tx_dma_chan);
err1:
	dma_free_chan(spi_kona->rx_dma_chan);
err:
	dma_free_chan(spi_kona->tx_dma_chan);
	return -EIO;
}
#elif CONFIG_MAP_SDMA
static int spi_kona_setup_dma(struct spi_kona_data *spi_kona)
{
	spi_kona->tx_dma_chan = sdma_request_channel(
		dma_tx_device[spi_kona->master->bus_num]);
	if (spi_kona->tx_dma_chan < 0) {
		pr_debug("%s: Tx sdma_request_chan failed\n", __func__);
		return -EIO;
	}
	if (sdma_set_device_handler(dma_tx_device[spi_kona->master->bus_num],
		spi_dma_callback, &spi_kona->tx_dma_evt) < 0) {
		pr_debug("%s: Tx sdma_register_callback failed\n", __func__);
		goto err;
	}
	
	spi_kona->rx_dma_chan = sdma_request_channel(
		dma_rx_device[spi_kona->master->bus_num]);
	if (spi_kona->rx_dma_chan < 0) {
		pr_debug("%s: Rx sdma_request_chan failed\n", __func__);
		goto err;
	}
	if (sdma_set_device_handler(dma_rx_device[spi_kona->master->bus_num],
		spi_dma_callback, &spi_kona->rx_dma_evt) < 0) {
		pr_debug("%s: Rx sdma_register_callback failed\n", __func__);
		goto err1;
	}
	return 0;
err1:
	dma_free_chan(spi_kona->rx_dma_chan);
err:
	dma_free_chan(spi_kona->tx_dma_chan);
	return -EIO;
}
#endif
static int spi_kona_probe(struct platform_device *pdev)
{
	struct spi_kona_platform_data *platform_info;
	struct resource *res;
	struct spi_master *master;
	struct spi_kona_data *spi_kona = 0;
	/*uint8_t clk_name[32]; */
	int status = 0;

	platform_info = dev_get_platdata(&pdev->dev);
	if (!platform_info) {
		dev_err(&pdev->dev, "can't get the platform data\n");
		return -EINVAL;
	}

	/* Allocate master with space for spi_kona and null dma buffer */
	master = spi_alloc_master(&pdev->dev, sizeof(struct spi_kona_data));
	if (!master) {
		dev_err(&pdev->dev, "can not alloc spi_master\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, master);

	master->bus_num = pdev->id;
	master->num_chipselect = platform_info->cs_line;
	master->mode_bits = platform_info->mode;

	spi_kona = spi_master_get_devdata(master);
	spi_kona->master = spi_master_get(master);
	spi_kona->enable_dma = platform_info->enable_dma;

	master->setup = spi_kona_setup;
	master->cleanup = spi_kona_cleanup;
	master->transfer = spi_kona_transfer;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("%s: SPI: No resource for memory\n", __func__);
		status = -ENXIO;
		goto out_master_put;
	}

	if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
		dev_err(&pdev->dev, "request_mem_region failed\n");
		status = -EBUSY;
		goto out_master_put;
	}

	spi_kona->base = ioremap_nocache(res->start, resource_size(res));
	if (!spi_kona->base) {
		printk(KERN_ERR "%s: Ioremap failed.\n", __func__);
		status = -ENOMEM;
		goto out_release_mem;
	}

	spi_kona->irq = platform_get_irq(pdev, 0);
	if (!spi_kona->irq) {
		pr_err("%s: No resource for IRQ\n", __func__);
		status = -ENXIO;
		goto out_release_mem;
	}

	status = request_irq(spi_kona->irq, spi_kona_isr, IRQF_SHARED,
			     "spi_irq", spi_kona);
	if (status) {
		pr_err("%s:Error registering spi irq %d %d\n",
		       __func__, status, spi_kona->irq);
		goto out_release_mem;
	}

	spi_kona->ssp_clk = clk_get(NULL, clk_name[master->bus_num]);
	if (IS_ERR_OR_NULL(spi_kona->ssp_clk)) {
		dev_err(&pdev->dev, "unable to get %s clock\n",
			clk_name[master->bus_num]);
		status = PTR_ERR(spi_kona->ssp_clk);
		goto out_free_irq;
	}
	spi_kona->ssp_apb_clk = clk_get(NULL, apb_clk_name[master->bus_num]);
	if (IS_ERR_OR_NULL(spi_kona->ssp_apb_clk)) {
		dev_err(&pdev->dev, "unable to get %s clock\n",
			apb_clk_name[master->bus_num]);
		status = PTR_ERR(spi_kona->ssp_apb_clk);
		goto out_free_irq;
	}
	clk_enable(spi_kona->ssp_clk);
	clk_enable(spi_kona->ssp_apb_clk);
//	spi_kona->dma_axi_clk = clk_get(NULL, "dma_axi");

	status = spi_kona_config_spi_hw(spi_kona);
	if (status) {
		pr_err("Error configuring SPI hardware\n");
		goto out_clk_put;
	}

	INIT_WORK(&spi_kona->work, spi_kona_work);
	spin_lock_init(&spi_kona->lock);
	INIT_LIST_HEAD(&spi_kona->queue);

	/* this task is the only thing to touch the SPI bits */
	spi_kona->busy = 0;
	spi_kona->workqueue =
	    create_singlethread_workqueue(dev_name
					  (spi_kona->master->dev.parent));
	if (spi_kona->workqueue == NULL) {
		status = -EBUSY;
		goto out_iounmap;
	}

	/* Register with the SPI framework */
	status = spi_register_master(master);
	if (status != 0) {
		dev_err(&pdev->dev, "problem registering spi master\n");
		goto out_work_queue;
	}

	if (spi_kona->enable_dma && spi_kona_setup_dma(spi_kona)) {
		pr_err("%s: Failed to setup DMA, using PIO\n", __func__);
		spi_kona->enable_dma = 0;
	}

	clk_disable(spi_kona->ssp_clk);
	clk_disable(spi_kona->ssp_apb_clk);
	pr_info("%s: SSP %d setup done\n", __func__, master->bus_num);
	return status;
out_work_queue:
	destroy_workqueue(spi_kona->workqueue);
out_iounmap:
	/* iounmap(spi_kona->base); */
	chal_sspi_deinit(spi_kona->chandle);
out_clk_put:
//	clk_put(spi_kona->dma_axi_clk);
	clk_disable(spi_kona->ssp_clk);
	clk_put(spi_kona->ssp_clk);
	clk_disable(spi_kona->ssp_apb_clk);
	clk_put(spi_kona->ssp_apb_clk);
out_free_irq:
	free_irq(spi_kona->irq, spi_kona);
out_release_mem:
	release_mem_region(res->start, resource_size(res));
out_master_put:
	spi_master_put(master);
	platform_set_drvdata(pdev, NULL);
	return status;
}

static int spi_kona_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	struct spi_kona_data *spi_kona = spi_master_get_devdata(master);
	int status = 0;

	if (!spi_kona)
		return 0;

	spi_unregister_master(master);
	WARN_ON(!list_empty(&spi_kona->queue));
	destroy_workqueue(spi_kona->workqueue);

	status = chal_sspi_deinit(spi_kona->chandle);
	if (status != CHAL_SSPI_STATUS_SUCCESS)
		status = -EBUSY;

//	clk_put(spi_kona->dma_axi_clk);
	clk_disable(spi_kona->ssp_clk);
	clk_put(spi_kona->ssp_clk);
	clk_disable(spi_kona->ssp_apb_clk);
	clk_put(spi_kona->ssp_apb_clk);
	free_irq(spi_kona->irq, spi_kona);

	spi_master_put(master);

	release_mem_region(res->start, resource_size(res));

	platform_set_drvdata(pdev, NULL);

	return status;
}

static void spi_kona_shutdown(struct platform_device *pdev)
{
	int status = spi_kona_remove(pdev);

	if (status != 0)
		dev_err(&pdev->dev, "shutdown failed with %d\n", status);

}

#ifdef CONFIG_PM
static int spi_kona_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct spi_kona_data *spi_kona;

	spi_kona = spi_master_get_devdata(master);
	/* flush the workqueue to make sure all outstanding work items
	 * are done
	 */
	flush_workqueue(spi_kona->workqueue);

	/*
	 * Don't need to disable SPI (SSP) clocks here since they are now only
	 * turned on for each transaction
	 */

	return 0;
}

static int spi_kona_resume(struct platform_device *pdev)
{
	/*
	 * Don't need to enable SPI (SSP) clocks here since they are now only
	 * turned on before each transaction
	 */
	return 0;
}
#else
#define spi_kona_suspend     NULL
#define spi_kona_resume      NULL
#endif

static struct platform_driver spi_kona_sspi_driver = {
	.driver = {
		   .name = "kona_sspi_spi",
		   .owner = THIS_MODULE,
		   },
	.probe = spi_kona_probe,
	.remove = spi_kona_remove,
	.shutdown = spi_kona_shutdown,
	.suspend = spi_kona_suspend,
	.resume = spi_kona_resume
};

static int __init kona_sspi_spi_init(void)
{
	return platform_driver_register(&spi_kona_sspi_driver);
}

module_init(kona_sspi_spi_init);

static void __exit kona_sspi_spi_exit(void)
{
	platform_driver_unregister(&spi_kona_sspi_driver);
}

module_exit(kona_sspi_spi_exit);

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("KONA SSPI based SPI Contoller");
MODULE_LICENSE("GPL");
