/*****************************************************************************
* Copyright 2004 - 2008 Broadcom Corporation.  All rights reserved.
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

/****************************************************************************/
/**
*   @file   sdma.c
*
*   @brief  Implements the SDMA interface.
*/
/****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/proc_fs.h>
#include <linux/hugetlb.h>
#include <linux/version.h>
#include <linux/sched.h>

#include <linux/mm.h>
#include <linux/pfn.h>
#include <linux/atomic.h>
#include <mach/sdma.h>
#include <chal/chal_dma.h>
#include <chal/chal_dmux.h>

#ifdef CONFIG_BCM_KNLLOG_IRQ
#include <linux/broadcom/knllog.h>
#endif

/* ---- Public Variables ------------------------------------------------- */

/* ---- Private Constants and Types -------------------------------------- */
#define MAKE_HANDLE(controllerIdx, channelIdx) \
			(((controllerIdx) << 4) | (channelIdx))

#define CONTROLLER_FROM_HANDLE(handle)    (((handle) >> 4) & 0x0f)
#define CHANNEL_FROM_HANDLE(handle)       ((handle) & 0x0f)

#ifdef CONFIG_MAP_SDMA_SECURE_MODE
#define SECURE_FLAG (DMA_DEVICE_FLAG_SECURE)
#else
#define SECURE_FLAG 0
#endif

/* ---- Private Variables ------------------------------------------------ */

static SDMA_Global_t gSDMA;
static struct proc_dir_entry *gDmaDir;
static CHAL_HANDLE gSecDmaHandle;
static CHAL_HANDLE gOpenDmaHandle;
spinlock_t gHwDmaLock;		/* acquired when starting DMA channel */
spinlock_t gDmaDevLock;

#define DEVICE_MEM_TO_MEM(n) \
{ \
	.flags = SECURE_FLAG, \
	.name = n, \
	.config = \
	{ \
		.dstBurstLen     = CHAL_DMA_BURST_LEN_8, \
		.dstBurstSize    = CHAL_DMA_BURST_SIZE_8_BYTES, \
		.dstEndpoint     = CHAL_DMA_ENDPOINT_MEMORY, \
		.srcBurstLen     = CHAL_DMA_BURST_LEN_8, \
		.srcBurstSize    = CHAL_DMA_BURST_SIZE_8_BYTES, \
		.srcEndpoint     = CHAL_DMA_ENDPOINT_MEMORY, \
		.descType        = CHAL_DMA_DESC_LIST, \
		.alwaysBurst     = FALSE \
	}, \
	.peripheralId = 0, \
}

#define DEVICE_MEM_TO_MEM_32(n) \
{ \
	.flags = SECURE_FLAG, \
	.name = n, \
	.config = \
	{ \
		.dstBurstLen     = CHAL_DMA_BURST_LEN_8, \
		.dstBurstSize    = CHAL_DMA_BURST_SIZE_4_BYTES, \
		.dstEndpoint     = CHAL_DMA_ENDPOINT_MEMORY, \
		.srcBurstLen     = CHAL_DMA_BURST_LEN_8, \
		.srcBurstSize    = CHAL_DMA_BURST_SIZE_4_BYTES, \
		.srcEndpoint     = CHAL_DMA_ENDPOINT_MEMORY, \
		.descType        = CHAL_DMA_DESC_LIST, \
		.alwaysBurst     = FALSE \
	}, \
	.peripheralId = 0, \
}

#define DEVICE_SSPI_DEV_TO_MEM(n, periph_id) \
{ \
	.flags = SECURE_FLAG, \
	.name = n, \
	.config = \
	{ \
		.dstBurstLen     = CHAL_DMA_BURST_LEN_4, \
		.dstBurstSize    = CHAL_DMA_BURST_SIZE_4_BYTES, \
		.dstEndpoint     = CHAL_DMA_ENDPOINT_MEMORY, \
		.srcBurstLen     = CHAL_DMA_BURST_LEN_4, \
		.srcBurstSize    = CHAL_DMA_BURST_SIZE_4_BYTES, \
		.srcEndpoint     = CHAL_DMA_ENDPOINT_PERIPHERAL, \
		.descType        = CHAL_DMA_DESC_RING, \
		.alwaysBurst     = FALSE, \
	}, \
	.peripheralId = periph_id, \
}

#define DEVICE_SSPI_MEM_TO_DEV(n, periph_id) \
{ \
	.flags = SECURE_FLAG, \
	.name = n, \
	.config = \
	{ \
		.dstBurstLen     = CHAL_DMA_BURST_LEN_4, \
		.dstBurstSize    = CHAL_DMA_BURST_SIZE_4_BYTES, \
		.dstEndpoint     = CHAL_DMA_ENDPOINT_PERIPHERAL, \
		.srcBurstLen     = CHAL_DMA_BURST_LEN_4, \
		.srcBurstSize    = CHAL_DMA_BURST_SIZE_4_BYTES, \
		.srcEndpoint     = CHAL_DMA_ENDPOINT_MEMORY, \
		.descType        = CHAL_DMA_DESC_RING, \
		.alwaysBurst     = FALSE, \
	}, \
	.peripheralId = periph_id, \
}

SDMA_DeviceAttribute_t SDMA_gDeviceAttribute[DMA_NUM_DEVICE_ENTRIES] = {
	[DMA_DEVICE_MEM_TO_MEM] = DEVICE_MEM_TO_MEM("mem-to-mem"),
	[DMA_DEVICE_MEM_TO_MEM_1] = DEVICE_MEM_TO_MEM("mem-to-mem1"),
	[DMA_DEVICE_MEM_TO_MEM_2] = DEVICE_MEM_TO_MEM("mem-to-mem2"),
	[DMA_DEVICE_MEM_TO_MEM_3] = DEVICE_MEM_TO_MEM("mem-to-mem3"),
	[DMA_DEVICE_MEM_TO_MEM_4] = DEVICE_MEM_TO_MEM("mem-to-mem4"),
	[DMA_DEVICE_MEM_TO_MEM_5] = DEVICE_MEM_TO_MEM("mem-to-mem5"),
	[DMA_DEVICE_MEM_TO_MEM_6] = DEVICE_MEM_TO_MEM("mem-to-mem6"),
	[DMA_DEVICE_MEM_TO_MEM_7] = DEVICE_MEM_TO_MEM("mem-to-mem7"),
	[DMA_DEVICE_MEM_TO_MEM_32] = DEVICE_MEM_TO_MEM_32("mem-to-mem32"),
	[DMA_DEVICE_SPUM_MEM_TO_DEV] = {
					.flags = SECURE_FLAG,
					.name = "spu mem-to-dev",
					.config = {
						   .dstBurstLen =
						   CHAL_DMA_BURST_LEN_16,
						   .dstBurstSize =
						   CHAL_DMA_BURST_SIZE_4_BYTES,
						   .dstEndpoint =
						   CHAL_DMA_ENDPOINT_PERIPHERAL,
						   .srcBurstLen =
						   CHAL_DMA_BURST_LEN_16,
						   .srcBurstSize =
						   CHAL_DMA_BURST_SIZE_4_BYTES,
						   .srcEndpoint =
						   CHAL_DMA_ENDPOINT_MEMORY,
						   .descType =
						   CHAL_DMA_DESC_LIST,
						   .flushMode =
						   CHAL_DMA_FLUSH_LAST,
						   .alwaysBurst = TRUE,
						   },
					.peripheralId =
					CHAL_DMA_PERIPHERAL_SPUM_SecureW,
					},
	[DMA_DEVICE_SPUM_DEV_TO_MEM] = {
					.flags = SECURE_FLAG,
					.name = "spu dev-to-mem",
					.config = {
						   .dstBurstLen =
						   CHAL_DMA_BURST_LEN_16,
						   .dstBurstSize =
						   CHAL_DMA_BURST_SIZE_4_BYTES,
						   .dstEndpoint =
						   CHAL_DMA_ENDPOINT_MEMORY,
						   .srcBurstLen =
						   CHAL_DMA_BURST_LEN_16,
						   .srcBurstSize =
						   CHAL_DMA_BURST_SIZE_4_BYTES,
						   .srcEndpoint =
						   CHAL_DMA_ENDPOINT_PERIPHERAL,
						   .descType =
						   CHAL_DMA_DESC_LIST,
						   .flushMode =
						   CHAL_DMA_FLUSH_LAST,
						   .alwaysBurst = TRUE,
						   },
					.peripheralId =
					CHAL_DMA_PERIPHERAL_SPUM_SecureR,
					},

	[DMA_DEVICE_MPHI_MEM_TO_DEV] = {
					.flags = SECURE_FLAG,
					.name = "mphi mem-to-dev",
					.config = {
						   .dstBurstLen =
						   CHAL_DMA_BURST_LEN_8,
						   .dstBurstSize =
						   CHAL_DMA_BURST_SIZE_4_BYTES,
						   .dstEndpoint =
						   CHAL_DMA_ENDPOINT_PERIPHERAL,
						   .srcBurstLen =
						   CHAL_DMA_BURST_LEN_8,
						   .srcBurstSize =
						   CHAL_DMA_BURST_SIZE_4_BYTES,
						   .srcEndpoint =
						   CHAL_DMA_ENDPOINT_MEMORY,
						   .descType =
						   CHAL_DMA_DESC_LIST,
						   .flushMode =
						   CHAL_DMA_FLUSH_FIRST,
						   .alwaysBurst = TRUE,
						   },
					.peripheralId =
					CHAL_DMA_PERIPHERAL_MPHI,
					},
	[DMA_DEVICE_MPHI_DEV_TO_MEM] = {
					.flags = SECURE_FLAG,
					.name = "mphi dev-to-mem",
					.config = {
						   .dstBurstLen =
						   CHAL_DMA_BURST_LEN_8,
						   .dstBurstSize =
						   CHAL_DMA_BURST_SIZE_4_BYTES,
						   .dstEndpoint =
						   CHAL_DMA_ENDPOINT_MEMORY,
						   .srcBurstLen =
						   CHAL_DMA_BURST_LEN_8,
						   .srcBurstSize =
						   CHAL_DMA_BURST_SIZE_4_BYTES,
						   .srcEndpoint =
						   CHAL_DMA_ENDPOINT_PERIPHERAL,
						   .descType =
						   CHAL_DMA_DESC_LIST,
						   .flushMode =
						   CHAL_DMA_FLUSH_ALWAYS,
						   .alwaysBurst = TRUE,
						   },
					.peripheralId =
					CHAL_DMA_PERIPHERAL_MPHI,
					},

	/*
	 * Note: ping-pong mode not supported with these tables. To support, we
	 * need an accessor function to change the descType from
	 * CHAL_DMA_DESC_LIST to CHAL_DMA_DESC_RING.
	 */
	[DMA_DEVICE_SSP0A_RX0] =
	    DEVICE_SSPI_DEV_TO_MEM("ssp0a_rx0", CHAL_DMA_PERIPHERAL_SSP_0A_RX0),
	[DMA_DEVICE_SSP0B_TX0] =
	    DEVICE_SSPI_MEM_TO_DEV("ssp0b_tx0", CHAL_DMA_PERIPHERAL_SSP_0B_TX0),
	[DMA_DEVICE_SSP0C_RX1] =
	    DEVICE_SSPI_DEV_TO_MEM("ssp0c_rx0", CHAL_DMA_PERIPHERAL_SSP_0C_RX1),
	[DMA_DEVICE_SSP0D_TX1] =
	    DEVICE_SSPI_MEM_TO_DEV("ssp0d_tx1", CHAL_DMA_PERIPHERAL_SSP_0D_TX1),

	[DMA_DEVICE_SSP1A_RX0] =
	    DEVICE_SSPI_DEV_TO_MEM("ssp1a_rx0", CHAL_DMA_PERIPHERAL_SSP_1A_RX0),
	[DMA_DEVICE_SSP1B_TX0] =
	    DEVICE_SSPI_MEM_TO_DEV("ssp1b_tx0", CHAL_DMA_PERIPHERAL_SSP_1B_TX0),
	[DMA_DEVICE_SSP1C_RX1] =
	    DEVICE_SSPI_DEV_TO_MEM("ssp1c_rx0", CHAL_DMA_PERIPHERAL_SSP_1C_RX1),
	[DMA_DEVICE_SSP1D_TX1] =
	    DEVICE_SSPI_MEM_TO_DEV("ssp1d_tx1", CHAL_DMA_PERIPHERAL_SSP_1D_TX1),

	[DMA_DEVICE_SSP2A_RX0] =
	    DEVICE_SSPI_DEV_TO_MEM("ssp2a_rx0", CHAL_DMA_PERIPHERAL_SSP_2A_RX0),
	[DMA_DEVICE_SSP2B_TX0] =
	    DEVICE_SSPI_MEM_TO_DEV("ssp2b_tx0", CHAL_DMA_PERIPHERAL_SSP_2B_TX0),
	[DMA_DEVICE_SSP2C_RX1] =
	    DEVICE_SSPI_DEV_TO_MEM("ssp2c_rx0", CHAL_DMA_PERIPHERAL_SSP_2C_RX1),
	[DMA_DEVICE_SSP2D_TX1] =
	    DEVICE_SSPI_MEM_TO_DEV("ssp2d_tx1", CHAL_DMA_PERIPHERAL_SSP_2D_TX1),

	[DMA_DEVICE_SSP3A_RX0] =
	    DEVICE_SSPI_DEV_TO_MEM("ssp3a_rx0", CHAL_DMA_PERIPHERAL_SSP_3A_RX0),
	[DMA_DEVICE_SSP3B_TX0] =
	    DEVICE_SSPI_MEM_TO_DEV("ssp3b_tx0", CHAL_DMA_PERIPHERAL_SSP_3B_TX0),
	[DMA_DEVICE_SSP3C_RX1] =
	    DEVICE_SSPI_DEV_TO_MEM("ssp3c_rx0", CHAL_DMA_PERIPHERAL_SSP_3C_RX1),
	[DMA_DEVICE_SSP3D_TX1] =
	    DEVICE_SSPI_MEM_TO_DEV("ssp3d_tx1", CHAL_DMA_PERIPHERAL_SSP_3D_TX1),

	[DMA_DEVICE_SSP4A_RX0] =
	    DEVICE_SSPI_DEV_TO_MEM("ssp4a_rx0", CHAL_DMA_PERIPHERAL_SSP_4A_RX0),
	[DMA_DEVICE_SSP4B_TX0] =
	    DEVICE_SSPI_MEM_TO_DEV("ssp4b_tx0", CHAL_DMA_PERIPHERAL_SSP_4B_TX0),
	[DMA_DEVICE_SSP4C_RX1] =
	    DEVICE_SSPI_DEV_TO_MEM("ssp4c_rx0", CHAL_DMA_PERIPHERAL_SSP_4C_RX1),
	[DMA_DEVICE_SSP4D_TX1] =
	    DEVICE_SSPI_MEM_TO_DEV("ssp4d_tx1", CHAL_DMA_PERIPHERAL_SSP_4D_TX1),

	/* AudioH devices */
	[DMA_DEVICE_AUDIOH_VIN_TO_MEM] = {
		.flags = SECURE_FLAG,
		.name = "audioh vin-to-mem",
		.config = {
			.dstEndpoint = CHAL_DMA_ENDPOINT_MEMORY,
			.srcEndpoint = CHAL_DMA_ENDPOINT_PERIPHERAL,
			.descType = CHAL_DMA_DESC_RING,
			.dstBurstLen = CHAL_DMA_BURST_LEN_4,
			.srcBurstLen = CHAL_DMA_BURST_LEN_4,
			.dstBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			.srcBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			},
		.peripheralId = CHAL_DMA_PERIPHERAL_VIN,
		},
	[DMA_DEVICE_AUDIOH_NVIN_TO_MEM] = {
		.flags = SECURE_FLAG,
		.name = "audioh nvin-to-mem",
		.config = {
			.dstEndpoint = CHAL_DMA_ENDPOINT_MEMORY,
			.srcEndpoint = CHAL_DMA_ENDPOINT_PERIPHERAL,
			.descType = CHAL_DMA_DESC_RING,
			.dstBurstLen = CHAL_DMA_BURST_LEN_4,
			.srcBurstLen = CHAL_DMA_BURST_LEN_4,
			.dstBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			.srcBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			},
		.peripheralId = CHAL_DMA_PERIPHERAL_NVIN,
		},
	[DMA_DEVICE_AUDIOH_MEM_TO_EARPIECE] = {
		.flags = SECURE_FLAG,
		.name = "audioh mem-to-earpiece",
		.config = {
			.dstEndpoint = CHAL_DMA_ENDPOINT_PERIPHERAL,
			.srcEndpoint = CHAL_DMA_ENDPOINT_MEMORY,
			.descType = CHAL_DMA_DESC_LIST,
			.dstBurstLen = CHAL_DMA_BURST_LEN_4,
			.srcBurstLen = CHAL_DMA_BURST_LEN_4,
			.dstBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			.srcBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			},
		.peripheralId = CHAL_DMA_PERIPHERAL_VOUT,
		},
	[DMA_DEVICE_AUDIOH_MEM_TO_HEADSET] = {
		.flags = SECURE_FLAG,
		.name = "audioh mem-to-headset",
		.config = {
			.dstEndpoint = CHAL_DMA_ENDPOINT_PERIPHERAL,
			.srcEndpoint = CHAL_DMA_ENDPOINT_MEMORY,
			.descType = CHAL_DMA_DESC_LIST,
			.dstBurstLen = CHAL_DMA_BURST_LEN_4,
			.srcBurstLen = CHAL_DMA_BURST_LEN_4,
			.dstBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			.srcBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			},
		.peripheralId = CHAL_DMA_PERIPHERAL_STEREO,
		},
	[DMA_DEVICE_AUDIOH_MEM_TO_HANDSFREE] = {
		.flags = SECURE_FLAG,
		.name =
		"audioh mem-to-handsfree",
		.config = {
			.dstEndpoint = CHAL_DMA_ENDPOINT_PERIPHERAL,
			.srcEndpoint = CHAL_DMA_ENDPOINT_MEMORY,
			.descType = CHAL_DMA_DESC_LIST,
			.dstBurstLen = CHAL_DMA_BURST_LEN_4,
			.srcBurstLen = CHAL_DMA_BURST_LEN_4,
			.dstBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			.srcBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			},
		.peripheralId = CHAL_DMA_PERIPHERAL_IHF_0,
		},
	[DMA_DEVICE_AUDIOH_MEM_TO_VIBRA] = {
		.flags = SECURE_FLAG,
		.name = "audioh mem-to-vibra",
		.config = {
			.dstEndpoint = CHAL_DMA_ENDPOINT_PERIPHERAL,
			.srcEndpoint = CHAL_DMA_ENDPOINT_MEMORY,
			.descType = CHAL_DMA_DESC_RING,
			.dstBurstLen = CHAL_DMA_BURST_LEN_4,
			.srcBurstLen = CHAL_DMA_BURST_LEN_4,
			.dstBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			.srcBurstSize = CHAL_DMA_BURST_SIZE_4_BYTES,
			},
		.peripheralId = CHAL_DMA_PERIPHERAL_VIBRA,
		},
};
EXPORT_SYMBOL(SDMA_gDeviceAttribute);

/* ---- Private Function Prototypes -------------------------------------- */

static int sdma_prealloc_descriptors_by_count(SDMA_Handle_t handle,
						/* DMA Handle */
					      int numDescriptors
						/* Number of descriptors to
						 * allocate
						 */
	);

/* ---- Functions  ------------------------------------------------------- */

/****************************************************************************/
/**
*   Displays information for /proc/sdma/channels
*/
/****************************************************************************/

static int sdma_proc_read_channels(char *buf, char **start, off_t offset,
				   int count, int *eof, void *data)
{
	int channelIdx;
	int limit = count - 200;
	int len = 0;
	SDMA_Channel_t *channel;

	down(&gSDMA.lock);

	for (channelIdx = 0; channelIdx < SDMA_NUM_CHANNELS; channelIdx++) {
		if (len >= limit)
			break;

		channel = &gSDMA.channel[channelIdx];

		len += sprintf(buf + len, "%d ", channelIdx);

		if ((channel->flags & DMA_CHANNEL_FLAG_IS_DEDICATED) != 0) {
			len +=
			    sprintf(buf + len, "Dedicated for %s ",
				    SDMA_gDeviceAttribute[channel->devType].
				    name);
		} else {
			len += sprintf(buf + len, "Shared ");
		}

		if ((channel->flags & DMA_CHANNEL_FLAG_NO_ISR) != 0)
			len += sprintf(buf + len, "No ISR ");

		if ((channel->flags & DMA_CHANNEL_FLAG_LARGE_FIFO) != 0)
			len += sprintf(buf + len, "Fifo: 128 ");
		else
			len += sprintf(buf + len, "Fifo: 64  ");

		if ((channel->flags & DMA_CHANNEL_FLAG_IN_USE) != 0) {
			len +=
			    sprintf(buf + len, "InUse by %s",
				    SDMA_gDeviceAttribute[channel->devType].
				    name);
#if (SDMA_DEBUG_TRACK_RESERVATION)
			len +=
			    sprintf(buf + len, " (%s:%d)", channel->fileName,
				    channel->lineNum);
#endif
		} else {
			len += sprintf(buf + len, "Avail ");
		}

		if (channel->lastDevType != DMA_DEVICE_NONE) {
			len +=
			    sprintf(buf + len, "Last use: %s ",
				    SDMA_gDeviceAttribute[channel->lastDevType].
				    name);
		}

		len += sprintf(buf + len, "\n");
	}
	up(&gSDMA.lock);
	*eof = 1;

	return len;
}

/****************************************************************************/
/**
*   Displays information for /proc/sdma/devices
*/
/****************************************************************************/

static int sdma_proc_read_devices(char *buf, char **start, off_t offset,
				  int count, int *eof, void *data)
{
	int limit = count - 200;
	int len = 0;
	int devIdx;

	down(&gSDMA.lock);

	for (devIdx = 0; devIdx < DMA_NUM_DEVICE_ENTRIES; devIdx++) {
		SDMA_DeviceAttribute_t *devAttr =
		    &SDMA_gDeviceAttribute[devIdx];

		if (devAttr->name == NULL)
			continue;

		if (len >= limit)
			break;

		len += sprintf(buf + len, "%-12s ", devAttr->name);

		len += sprintf(buf + len, "Shared DMA:");
		if ((devAttr->flags & DMA_DEVICE_FLAG_ON_DMA0) != 0)
			len += sprintf(buf + len, "0");
		if ((devAttr->flags & DMA_DEVICE_FLAG_ON_DMA1) != 0)
			len += sprintf(buf + len, "1");
		len += sprintf(buf + len, " ");

		if ((devAttr->flags & DMA_DEVICE_FLAG_NO_ISR) != 0)
			len += sprintf(buf + len, "NoISR ");
		if ((devAttr->flags & DMA_DEVICE_FLAG_ALLOW_LARGE_FIFO) != 0)
			len += sprintf(buf + len, "Allow-128 ");

		len += sprintf(buf + len,
			    "Xfer #: %llu Ticks: %llu Bytes: %llu "
			    "DescLen: %u initDesc: %i descUse: %i\n",
			    devAttr->numTransfers, devAttr->transferTicks,
			    devAttr->transferBytes,
			    devAttr->ring.bytesAllocated, devAttr->initDesc,
			    devAttr->descUse);

	}

	up(&gSDMA.lock);
	*eof = 1;

	return len;
}

/****************************************************************************/
/**
*   Processes writes to /proc/dma/clear_device_counters
*/
/****************************************************************************/

static ssize_t sdma_proc_write_clear_device_counters(struct file *file,
						     const char __user *buf,
						     unsigned long count,
						     void *data)
{
	char lbuf[12];
	long value;
	SDMA_DeviceAttribute_t *devAttr;

	if (count >= sizeof(lbuf))
		return -EINVAL;

	if (copy_from_user(lbuf, buf, count))
		return -EFAULT;
	lbuf[count] = '\0';

	if (strict_strtol(lbuf, 10, &value))
		return -EINVAL;

	if (value > DMA_NUM_DEVICE_ENTRIES)
		return -EINVAL;

	if (value < 0)
		return -EINVAL;

	down(&gSDMA.lock);

	devAttr = &SDMA_gDeviceAttribute[(int)value];

	devAttr->numTransfers = 0;
	devAttr->transferTicks = 0;
	devAttr->transferBytes = 0;

	up(&gSDMA.lock);

	return count;
}

/****************************************************************************/
/**
*   Registers /proc/sdma/clear_device_counters
*/
/****************************************************************************/

static int sdma_register_proc_clear_device_counters(void)
{
	struct proc_dir_entry *entry;

	entry = create_proc_entry("clear_device_counters", S_IWUGO, gDmaDir);

	if (entry == NULL) {
		pr_err("Unable to create /proc/sdma/clear_device_counters\n");
		return -ENOMEM;
	}

	entry->write_proc = sdma_proc_write_clear_device_counters;

	return 0;
}

/****************************************************************************/
/**
*   Determines if a DMA_Device_t is "valid".
*
*   @return
*       TRUE        - dma device is valid
*       FALSE       - dma device isn't valid
*/
/****************************************************************************/

static inline int IsDeviceValid(DMA_Device_t device)
{
	return (device >= 0) && (device < DMA_NUM_DEVICE_ENTRIES);
}

/****************************************************************************/
/**
*   Translates a DMA handle into a pointer to a channel.
*
*   @return
*       non-NULL    - pointer to SDMA_Channel_t
*       NULL        - DMA Handle was invalid
*/
/****************************************************************************/

static inline SDMA_Channel_t *HandleToChannel(SDMA_Handle_t handle)
{
	int channelIdx = CHANNEL_FROM_HANDLE(handle);
	if (channelIdx > SDMA_NUM_CHANNELS)
		return NULL;
	return &gSDMA.channel[channelIdx];
}

/****************************************************************************/
/**
*   Interrupt handler which is called to process DMA interrupts.
*/
/****************************************************************************/

static irqreturn_t sdma_interrupt_handler(int irq, void *dev_id)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	CHAL_HANDLE chal_hdl;
	DMA_Status_t dma_status;
	volatile uint32_t status;
	int i;
	int rc;
	int desc_idx;

	/* Obtain secure/open DMA handle */
	chal_hdl = *(CHAL_HANDLE *)dev_id;

	while (1) {
		status = chal_dma_get_int_status(chal_hdl);
		if (status == 0)
			break;

		channel = &gSDMA.channel[0];
		for (i = 0; i < CHAL_TOTAL_DMA_CHANNELS; i++, channel++) {
			if (status & 0x1) {
				int reason =
				    DMA_HANDLER_REASON_TRANSFER_COMPLETE;

				/* Clear interrupt status */
				chal_dma_clear_int_status(channel->sdmacHandle);

				/* Get channel attribute */
				devAttr =
				    &SDMA_gDeviceAttribute[channel->devType];

#ifdef CONFIG_BCM_KNLLOG_IRQ
				if (gKnllogIrqSchedEnable & KNLLOG_DMA) {
					KNLLOG
					    ("tstop [%s devType=%u bytes=%u]\n",
					     devAttr->name, channel->devType,
					     devAttr->numBytes);
				}
#endif
				/* Update stats */
				devAttr->numTransfers++;
				devAttr->transferBytes += devAttr->numBytes;
				devAttr->transferTicks +=
				    (timer_get_tick_count() -
				     devAttr->transferStartTime);

				/* Get current descriptor index */
				rc =
				chal_dma_get_current_channel_descriptor_index(
						channel->sdmacHandle,
						&desc_idx);
				if (rc)
					printk(KERN_ERR
					       "%s: Unable to determine "
					       "current descriptor index\n",
					       __func__);

				/* Fill in status struct */
				dma_status.reason = reason;
				dma_status.desc_idx = desc_idx;

				/* Call installed handler.  If 'extended device
				 * handler' defined, ignore the defined 'device
				 * handler'
				 */

				if (devAttr->devHandlerExt) {
					devAttr->devHandlerExt(channel->devType,
							       &dma_status,
							       devAttr->
							       userData);
				} else if (devAttr->devHandler) {
					devAttr->devHandler(channel->devType,
							    reason,
							    devAttr->userData);
				}
			}
			status >>= 1;
		}
	}

	return IRQ_HANDLED;
}

/****************************************************************************/
/**
*   Allocates memory to hold a descriptor. The descriptor then
*   needs to be populated by making one or more calls to
*   dma_add_descriptors.
*
*   The returned descriptor will be automatically initialized.
*
*   @return
*       0           Descriptor was allocated successfully
*       -EINVAL     Invalid parameters passed in
*       -ENOMEM     Unable to allocate memory for the desired number of
*                   descriptors.
*/
/****************************************************************************/

static int alloc_descriptor_ring(SDMA_DescriptorRing_t *ring,
					/* Descriptor to populate */
				 size_t bytesToAlloc,
					 /* Number of descriptors that need to
					  * be allocated.
					 */
				 int numDescriptors)
{
	if ((ring == NULL) || (bytesToAlloc <= 0))
		return -EINVAL;

	ring->physAddr = 0;
	ring->descriptorsAllocated = 0;
	ring->bytesAllocated = 0;

	ring->virtAddr = dma_alloc_coherent(NULL,
					    bytesToAlloc,
					    &ring->physAddr,
					    GFP_KERNEL);
	if (ring->virtAddr == NULL)
		return -ENOMEM;

	ring->bytesAllocated = bytesToAlloc;
	ring->descriptorsAllocated = numDescriptors;

	return 0;
}

/****************************************************************************/
/**
*   Releases the memory which was previously allocated for a descriptor ring.
*/
/****************************************************************************/

static void free_descriptor_ring(SDMA_DescriptorRing_t *ring
					/* Descriptor to release */
	)
{
	if (ring->virtAddr != NULL)
		dma_free_coherent(NULL,
				  ring->bytesAllocated,
				  ring->virtAddr, ring->physAddr);

	ring->bytesAllocated = 0;
	ring->descriptorsAllocated = 0;
	ring->virtAddr = NULL;
	ring->physAddr = 0;
}

/****************************************************************************/
/**
*   Determines the number of descriptors which would be required for a
*   transfer.
*
*   This function also needs to know which DMA device this transfer will
*   be destined for, so that the appropriate DMA configuration can be retrieved.
*   DMA parameters such as transfer width, and whether this is a
*   memory-to-memory or memory-to-peripheral, etc can all affect the actual
*   number of descriptors required.
*
*   @return
*       > 0     Returns the number of descriptors required for the indicated
*               transfer
*       -ENODEV - Device handed in is invalid.
*/
/****************************************************************************/

static int calculate_descriptor_count(SDMA_Handle_t handle,	/* DMA Handle */
				      size_t numBytes
					      /* Number of bytes to transfer
					       * to the device
					       */
	)
{
	int numDescriptors = 0;
	SDMA_Channel_t *channel;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	/* Check if the data is burst aligned */
	if (numBytes % chal_dma_get_burst_length(channel->sdmacHandle)) {
		/* At least allocate 2 descriptors, in case data length is not
		 * multiple of burst length
		*/
		numDescriptors =
		    2 +
		    (numBytes /
		     chal_dma_calculate_max_data_per_descriptor(channel->
								sdmacHandle));
	} else {
		numDescriptors =
		    1 +
		    (numBytes /
		     chal_dma_calculate_max_data_per_descriptor(channel->
								sdmacHandle));
	}

	return numDescriptors;
}

/****************************************************************************/
/**
*   Initializes descriptors and setup channel memory
*
*   @return
*       0       Descriptors memory set successfully
*       -ENODEV Invalid device
*/
/****************************************************************************/

static int set_descriptors(SDMA_Handle_t handle,	/* DMA Handle */
			   int numDescriptors	/* Number of descriptors */
	)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	/* Configure channel memory to hold micro-code and descriptors */
	chal_dma_config_channel_memory(channel->sdmacHandle,
				       (uint32_t)devAttr->ring.virtAddr,
				       devAttr->ring.physAddr, numDescriptors);

	/* Indicate that it is ready to add descriptors from the top of the list
	 */
	devAttr->initDesc = 1;

	return 0;
}

/****************************************************************************/
/**
*   Adds a region of memory to the descriptor. Note that it may take
*   multiple descriptors for each region of memory. It is the callers
*   responsibility to allocate a sufficiently large descriptor.
*
*   @return
*       >= 0    Number of descriptors added successfully
*       -ENODEV Device handed in is invalid.
*       -EINVAL Invalid parameters
*       -ENOMEM Memory exhausted
*/
/****************************************************************************/

static int add_descriptors(SDMA_Handle_t handle,	/* DMA Handle */
			   dma_addr_t srcData,
				   /* Place to get data (memory or device) */
			   dma_addr_t dstData,
				   /* Place to put data (memory or device) */
			   size_t numBytes
				   /* Number of bytes to transfer to the device
				    */
	)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	int start = 0;
	uint32_t unburst_data;
	uint32_t burst_data;
	uint32_t dataLength;
	uint32_t numBytesPerDesc;
	CHAL_DMA_STATUS_t chalstatus;
	int numDescs = 0;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	/* Find the portion of data that cannot use burst */
	unburst_data =
	    numBytes % chal_dma_get_burst_length(channel->sdmacHandle);
	burst_data = numBytes - unburst_data;
	numBytesPerDesc =
	    chal_dma_calculate_max_data_per_descriptor(channel->sdmacHandle);

	/* Set up descriptors */
	start = devAttr->initDesc;	/* FIXME: determine what the constraints
					 * are for adding descriptors
					 */
	if (devAttr->initDesc)
		devAttr->descUse = 0;

	/* Add descriptor information for burst data */
	while (burst_data) {
		if (devAttr->descUse >= devAttr->ring.descriptorsAllocated) {
			printk(KERN_ERR
			       "%s: insufficient memory to add descriptor\n",
			       __func__);
			return -ENOMEM;
		}

		devAttr->descUse++;

		if (burst_data > numBytesPerDesc)
			dataLength = numBytesPerDesc;
		else
			dataLength = burst_data;

		chalstatus = chal_dma_add_descriptor(channel->sdmacHandle,
						     start,
						     srcData,
						     dstData, dataLength);
		if (chalstatus != CHAL_DMA_STATUS_SUCCESS) {
			printk(KERN_ERR "%s: failed add descriptor, err=%i\n",
			       __func__, chalstatus);
			return -EINVAL;
		}

		numDescs++;

		burst_data -= dataLength;
		/* Adjust source  address */
		if (devAttr->config.srcEndpoint == CHAL_DMA_ENDPOINT_MEMORY)
			srcData += dataLength;
		/* Adjust destination address */
		if (devAttr->config.dstEndpoint == CHAL_DMA_ENDPOINT_MEMORY)
			dstData += dataLength;

		start = 0;	/* FIXME: */
	}

	/* Add descriptor information for un-burst data */
	if (unburst_data) {
		if (devAttr->descUse >= devAttr->ring.descriptorsAllocated) {
			printk(KERN_ERR
			       "%s: insufficient memory to add descriptor "
			       "for unburst data\n",
			       __func__);
			return -ENOMEM;
		}

		devAttr->descUse++;

		chalstatus = chal_dma_add_descriptor(channel->sdmacHandle,
						     0 /* burst off */ ,
						     srcData,
						     dstData, unburst_data);
		if (chalstatus != CHAL_DMA_STATUS_SUCCESS) {
			printk(KERN_ERR
			       "%s: failed add unburst descriptor, err=%i\n",
			       __func__, chalstatus);
			printk(KERN_ERR
			       "%s: srcData=0x%08x dstData=0x%08x numBytes=%d "
			       "unburst_data=%d\n",
			       __func__, srcData, dstData, numBytes,
			       unburst_data);
			return -EINVAL;
		}
		numDescs++;
	}

	/* Descriptor is added */
	devAttr->initDesc = 0;

	return numDescs;
}

/****************************************************************************/
/**
*   Adds a a command of length 4 bytes to the descriptor.
*
*   @return
*       >= 0    Number of descriptors added successfully
*       -ENODEV Device handed in is invalid.
*       -EINVAL Invalid parameters
*       -ENOMEM Memory exhausted
*/
/****************************************************************************/

static int add_device_command(SDMA_Handle_t handle,	/* DMA Handle */
			      dma_addr_t srcData,
				      /* Place to get the command from memory */
			      dma_addr_t dstData
				      /* Place to put the command to a device */
	)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	int start = 0;
	CHAL_DMA_STATUS_t chalstatus;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	/* Set up descriptors */
	start = devAttr->initDesc;
	if (devAttr->initDesc)
		devAttr->descUse = 0;

	/* Add descriptor information for burst data */
	if (devAttr->descUse >= devAttr->ring.descriptorsAllocated) {
		printk(KERN_ERR "%s: insufficient memory to add descriptor\n",
		       __func__);
		return -ENOMEM;
	}

	devAttr->descUse++;

	chalstatus =
	    chal_dma_add_device_command_to_descriptor(channel->sdmacHandle,
						      start, srcData, dstData);

	if (chalstatus != CHAL_DMA_STATUS_SUCCESS) {
		printk(KERN_ERR "%s: failed add descriptor, err=%i\n", __func__,
		       chalstatus);
		return -EINVAL;
	}

	/* Descriptor is added */
	devAttr->initDesc = 0;

	return 1;
}

/****************************************************************************/
/**
*   Intializes all of the data structures associated with the DMA.
*   @return
*       >= 0    - Initialization was successfull.
*
*       -EBUSY  - Device is currently being used.
*       -ENODEV - Device handed in is invalid.
*/
/****************************************************************************/

int sdma_init(void)
{
	int rc = 0;
	int channelIdx;
	SDMA_Channel_t *channel;

	printk(KERN_INFO "%s: initializing SDMA\n", __func__);

	memset(&gSDMA, 0, sizeof(gSDMA));

	sema_init(&gSDMA.lock, 0);
	spin_lock_init(&gHwDmaLock);
	spin_lock_init(&gDmaDevLock);
	init_waitqueue_head(&gSDMA.freeChannelQ);

	/* Initialze OPEN DMA  */
	gOpenDmaHandle = chal_dma_init(CHAL_DMA_STATE_OPEN);
	/* Initialze Secure DMA */
	gSecDmaHandle = chal_dma_init(CHAL_DMA_STATE_SECURE);

	/* Start off by marking all of the DMA channels as shared. */
	for (channelIdx = 0; channelIdx < SDMA_NUM_CHANNELS; channelIdx++) {
		channel = &gSDMA.channel[channelIdx];

		channel->flags = 0;
		channel->devType = DMA_DEVICE_NONE;
		channel->lastDevType = DMA_DEVICE_NONE;

#if (SDMA_DEBUG_TRACK_RESERVATION)
		channel->fileName = "";
		channel->lineNum = 0;
#endif
		channel->sdmacHandle = 0;
	}

	/* Install Open DMA Interrupt handler */
	rc = request_irq(INTV_DMAC_OPEN, sdma_interrupt_handler,
			      IRQF_DISABLED, "Open DMA Handler",
			      &gOpenDmaHandle);
	if (rc != 0)
		printk(KERN_ERR "request_irq for open DMA failed\n");

	/* Install Secure DMA Interrupt handler */
	rc = request_irq(INTV_DMAC_SECURE, sdma_interrupt_handler,
			      IRQF_DISABLED, "Secure DMA Handler",
			      &gSecDmaHandle);
	if (rc != 0)
		printk(KERN_ERR "request_irq for secure DMA  failed\n");

	/* Create /sproc/dma/channels, /proc/sdma/devices and
	 * /proc/sdma/clear_device_counters
	 */
	gDmaDir = create_proc_entry("sdma", S_IFDIR | S_IRUGO | S_IXUGO, NULL);

	if (gDmaDir == NULL) {
		printk(KERN_ERR "Unable to create /proc/sdma\n");
	} else {
		create_proc_read_entry("channels", 0, gDmaDir,
				       sdma_proc_read_channels, NULL);
		create_proc_read_entry("devices", 0, gDmaDir,
				       sdma_proc_read_devices, NULL);
		rc = sdma_register_proc_clear_device_counters();
	}

	up(&gSDMA.lock);

	/*
	 * Preallocate some descriptors for the mem-to-mem and mem-to-mem-32
	 * channels. 4096 descriptors allows for worst case fragmentation of a
	 * 16Mb memory reqion.  The descriptor memory allocated is around 232K
	 * since each descriptor requires approx 58 bytes..
	 */

	if (rc == 0) {
		SDMA_Handle_t dmaHndl =
		    sdma_request_channel(DMA_DEVICE_MEM_TO_MEM);
		if (dmaHndl >= 0) {
			rc = sdma_prealloc_descriptors_by_count(dmaHndl, 4096);
			sdma_free_channel(dmaHndl);
		} else {
			rc = -ENODEV;
		}
	}
	if (rc == 0) {
		SDMA_Handle_t dmaHndl =
		    sdma_request_channel(DMA_DEVICE_MEM_TO_MEM_32);
		if (dmaHndl >= 0) {
			rc = sdma_prealloc_descriptors_by_count(dmaHndl, 4096);
			sdma_free_channel(dmaHndl);
		} else {
			rc = -ENODEV;
		}
	}

	return rc;
}

/****************************************************************************/
/**
*   Reserves a channel for use with @a dev. If the device is setup to use
*   a shared channel, then this function will block until a free channel
*   becomes available.
*
*   @return
*       >= 0    - A valid DMA Handle.
*       -EBUSY  - Device is currently being used.
*       -ENODEV - Device handed in is invalid.
*/
/****************************************************************************/

#if (SDMA_DEBUG_TRACK_RESERVATION)
SDMA_Handle_t sdma_request_channel_dbg(DMA_Device_t dev,
				       const char *fileName,
				       int lineNum
	)
#else
SDMA_Handle_t sdma_request_channel(DMA_Device_t dev)
#endif
{
	SDMA_Handle_t handle;
	SDMA_DeviceAttribute_t *devAttr;
	SDMA_Channel_t *channel;
	CHAL_DMA_CHANNEL_t channelIdx;
	CHAL_HANDLE dmaHandle;

	down(&gSDMA.lock);

	/* Get device attribute */
	devAttr = &SDMA_gDeviceAttribute[dev];

	if ((dev < 0) || (dev >= DMA_NUM_DEVICE_ENTRIES)
	    || devAttr->name == NULL) {
		handle = -ENODEV;
		goto out;
	}
#if (SDMA_DEBUG_TRACK_RESERVATION)
	{
		char *s;

		s = strrchr(fileName, '/');
		if (s != NULL)
			fileName = s + 1;
	}
#endif

	if ((devAttr->flags & DMA_DEVICE_FLAG_IN_USE) != 0) {
		/* This device has already been requested and not been freed */

		printk(KERN_ERR "%s: device %s is already requested\n",
		       __func__, devAttr->name);
		handle = -EBUSY;
		goto out;
	}

	/* Select DMA interface */
	if ((devAttr->flags & DMA_DEVICE_FLAG_SECURE) != 0)
		dmaHandle = gSecDmaHandle;
	else
		dmaHandle = gOpenDmaHandle;

	/* Obtain a free DMA channel */
	handle = SDMA_INVALID_HANDLE;
	while (handle == SDMA_INVALID_HANDLE) {
		if (chal_dma_get_channel(dmaHandle, &channelIdx) ==
		    CHAL_DMA_STATUS_SUCCESS) {
			channel = &gSDMA.channel[channelIdx];

#if 0
			if (channel->flags & DMA_CHANNEL_FLAG_IN_USE) {
				/* Channel already in use, search again? */
				continue;
			}
#endif

			channel->flags |= DMA_CHANNEL_FLAG_IN_USE;
			channel->devType = dev;
			devAttr->flags |= DMA_DEVICE_FLAG_IN_USE;

#if (SDMA_DEBUG_TRACK_RESERVATION)
			channel->fileName = fileName;
			channel->lineNum = lineNum;
#endif

			handle = MAKE_HANDLE(0, channelIdx);

			/* Configure the channel */
			channel->sdmacHandle =
			    chal_dma_config_channel(dmaHandle, channelIdx,
						    &devAttr->config);

			/* Connect peripheral if requested */
			if ((devAttr->config.dstEndpoint !=
			     CHAL_DMA_ENDPOINT_MEMORY)
			    || (devAttr->config.srcEndpoint !=
				CHAL_DMA_ENDPOINT_MEMORY)) {
				if (chal_dma_connect_peripheral
				    (channel->sdmacHandle,
				     devAttr->peripheralId) !=
				    CHAL_DMA_STATUS_SUCCESS) {
					/* release the channel, since peripheral
					 * is not ready
					 */
					chal_dma_release_channel(channel->
								 sdmacHandle);
					printk(KERN_ERR
					       "sdma_request_channel: failed "
					       "to connect peripheral %d\n",
					       devAttr->peripheralId);
					/* Handle is now invalid */
					handle = -ENODEV;
					goto out;
				}
			}
			goto out;
		}

		/* No channels are currently available. Let's wait for one to
		 * free up.
		 */

		{
			DEFINE_WAIT(wait);

			prepare_to_wait(&gSDMA.freeChannelQ, &wait,
					TASK_INTERRUPTIBLE);
			up(&gSDMA.lock);
			schedule();
			finish_wait(&gSDMA.freeChannelQ, &wait);
		}
		down(&gSDMA.lock);
	}

out:
	up(&gSDMA.lock);

	return handle;
}

/* Create both _dbg and non _dbg functions for modules. */

#if (SDMA_DEBUG_TRACK_RESERVATION)
#undef sdma_request_channel
SDMA_Handle_t sdma_request_channel(DMA_Device_t dev)
{
	return sdma_request_channel_dbg(dev, __FILE__, __LINE__);
}
EXPORT_SYMBOL(sdma_request_channel_dbg);
#endif
EXPORT_SYMBOL(sdma_request_channel);

/****************************************************************************/
/**
*   Frees a previously allocated DMA Handle.
*/
/****************************************************************************/

int sdma_free_channel(SDMA_Handle_t handle	/* DMA handle. */
	)
{
	int rc = 0;
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;

	down(&gSDMA.lock);

	channel = HandleToChannel(handle);
	if (channel == NULL) {
		rc = -EINVAL;
		goto out;
	}

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	channel->flags &= ~DMA_CHANNEL_FLAG_IN_USE;
	devAttr->flags &= ~DMA_DEVICE_FLAG_IN_USE;

	if ((devAttr->config.dstEndpoint != CHAL_DMA_ENDPOINT_MEMORY)
	    || (devAttr->config.srcEndpoint != CHAL_DMA_ENDPOINT_MEMORY)) {
		chal_dma_disconnect_peripheral(channel->sdmacHandle);
	}

	chal_dma_release_channel(channel->sdmacHandle);

out:
	up(&gSDMA.lock);
	/* Notify others that is waiting for a channel */
	wake_up_interruptible(&gSDMA.freeChannelQ);

	return rc;
}
EXPORT_SYMBOL(sdma_free_channel);

/****************************************************************************/
/**
*   Pre-allocates buffers for the descriptors. This is normally done
*   automatically, but can be done ahead of time if want to allow for a
*   large worst case but don't want to wait until it actually happens.
*
*   @return
*       0       Descriptors were allocated successfully
*       -EINVAL Invalid device type for this kind of transfer
*               (i.e. the device is _MEM_TO_DEV and not _DEV_TO_MEM)
*       -ENOMEM Memory exhausted
*/
/****************************************************************************/

static int sdma_prealloc_descriptors_by_count(SDMA_Handle_t handle,
						/* DMA Handle */
					      int numDescriptors
						/* Number of descriptors to
						 * allocate
						 */
	)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	size_t ringBytesRequired;
	int rc;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	/*
	 * Check to see if we can reuse the existing descriptor ring, or if we
	 * need to allocate a new one.
	 */

	ringBytesRequired =
	    chal_dma_calculate_channel_memory(channel->sdmacHandle,
					      numDescriptors);
	if (ringBytesRequired <= 0) {
		printk(KERN_ERR
		       "%s: chal_dma_calculate_channel_memory (%d) failed\n",
		       __func__, numDescriptors);
		return -EINVAL;
	}

	/*printk( "ringBytesRequired: %d\n", ringBytesRequired ); */

	if (ringBytesRequired > devAttr->ring.bytesAllocated) {
		/*
		 * Make sure that this code path is never taken from interrupt
		 * context. It's OK for an interrupt to initiate a DMA transfer,
		 * but the descriptor allocation needs to have already been
		 * done.
		 */

		might_sleep();

		/* Free the old descriptor ring and allocate a new one. */

		free_descriptor_ring(&devAttr->ring);

		/* And allocate a new one. */

		rc = alloc_descriptor_ring(&devAttr->ring, ringBytesRequired,
					   numDescriptors);
		if (rc < 0) {
			printk(KERN_ERR
			       "%s: dma_alloc_descriptor_ring( %d ) failed\n",
			       __func__, numDescriptors);
			return rc;
		}
	}

	return 0;
}

/****************************************************************************/
/**
*   Pre-allocates buffers for the descriptors. This is normally done
*   automatically, but can be done ahead of time if want to allow for a
*   large worst case but don't want to wait until it actually happens.
*
*   @return
*       0       Descriptors were allocated successfully
*       -EINVAL Invalid device type for this kind of transfer
*               (i.e. the device is _MEM_TO_DEV and not _DEV_TO_MEM)
*       -ENOMEM Memory exhausted
*/
/****************************************************************************/

static int sdma_prealloc_descriptors_by_size(SDMA_Handle_t handle,
						/* DMA Handle */
					     unsigned int numBuffers,
						/* Number of buffers of numBytes
						 * required
						 */
					     size_t numBytes,
						/* Number of bytes to transfer
						 * to the device
						 */
					     int *outNumDescriptors
						/* Number of descriptors
						 * actually allocated
						 */
	)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	int numDescriptors;
	size_t ringBytesRequired;
	int rc;

	if (outNumDescriptors != NULL)
		*outNumDescriptors = 0;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	/* Figure out how many descriptors we need. */

	/*printk( "srcData: 0x%08x dstData: 0x%08x, numBytes: %d\n", */
	/*        srcData, dstData, numBytes ); */

	numDescriptors = calculate_descriptor_count(handle, numBytes);
	if (numDescriptors < 0) {
		printk(KERN_ERR
		       "%s: calculate_descriptor_count failed, err=%i\n",
		       __func__, numDescriptors);
		return numDescriptors;
	}
	numDescriptors *= numBuffers;

	rc = sdma_prealloc_descriptors_by_count(handle, numDescriptors);
	if (rc != 0) {
		printk(KERN_ERR
		       "%s: sdma_prealloc_descriptors_by_count for "
		       "%d descriptors failed: %d\n",
		       __func__, numDescriptors, rc);
		return rc;
	}

	/*
	 * Check to see if we can reuse the existing descriptor ring, or if we
	 * need to allocate a new one.
	 */

	ringBytesRequired =
	    chal_dma_calculate_channel_memory(channel->sdmacHandle,
					      numDescriptors);
	if (ringBytesRequired <= 0) {
		printk(KERN_ERR
		       "%s: chal_dma_calculate_channel_memory (%d) failed\n",
		       __func__, numDescriptors);
		return -EINVAL;
	}

	/*printk( "ringBytesRequired: %d\n", ringBytesRequired ); */

	if (ringBytesRequired > devAttr->ring.bytesAllocated) {
		/*
		 * Make sure that this code path is never taken from interrupt
		 * context.  It's OK for an interrupt to initiate a DMA
		 * transfer, but the descriptor allocation needs to have already
		 * been done.
		 */

		might_sleep();

		/* Free the old descriptor ring and allocate a new one. */

		free_descriptor_ring(&devAttr->ring);

		/* And allocate a new one. */

		rc = alloc_descriptor_ring(&devAttr->ring, ringBytesRequired,
					   numDescriptors);
		if (rc < 0) {
			printk(KERN_ERR
			       "%s: dma_alloc_descriptor_ring( %d ) failed\n",
			       __func__, numDescriptors);
			return rc;
		}
	}

	if (outNumDescriptors != NULL)
		*outNumDescriptors = numDescriptors;
	return 0;
}

/****************************************************************************/
/**
*   Allocates buffers for the descriptors. This is normally done automatically
*   but needs to be done explicitly when initiating a dma from interrupt
*   context.
*
*   @return
*       0       Descriptors were allocated successfully
*       -EINVAL Invalid device type for this kind of transfer
*               (i.e. the device is _MEM_TO_DEV and not _DEV_TO_MEM)
*       -ENOMEM Memory exhausted
*/
/****************************************************************************/

int sdma_alloc_descriptors(SDMA_Handle_t handle,	/* DMA Handle */
			   dma_addr_t srcData,
				/* Place to get data to write to device */
			   dma_addr_t dstData,
				/* Pointer to device data address */
			   size_t numBytes
				/* Number of bytes to transfer to the device */
	)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	int numDescriptors;
	int rc;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	/* Figure out how many descriptors we need. */

	rc = sdma_prealloc_descriptors_by_size(handle, 1, numBytes,
					       &numDescriptors);
	if (rc != 0) {
		printk(KERN_ERR
		       "%s: sdma_prealloc_descriptors for "
		       "%d bytes failed: %d\n",
		       __func__, numBytes, rc);
		return rc;
	}

	/* Set descriptors to channel */
	set_descriptors(handle, numDescriptors);

	rc = add_descriptors(handle, srcData, dstData, numBytes);
	if (rc < 0) {
		printk(KERN_ERR "%s: add_descriptors failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(sdma_alloc_descriptors);

/****************************************************************************/
/**
*   Allocates and sets up descriptors for a double buffered circular buffer
*   for destination data.
*
*   This is primarily intended to be used for things like the ingress samples
*   from a microphone.
*
*   @return
*       > 0     Number of descriptors actually allocated.
*       -EINVAL Invalid device type for this kind of transfer
*               (i.e. the device is _MEM_TO_DEV and not _DEV_TO_MEM)
*       -ENOMEM Memory exhausted
*/
/****************************************************************************/

int sdma_alloc_double_dst_descriptors(SDMA_Handle_t handle,	/* DMA Handle */
				      dma_addr_t srcData,
						/* Physical address of source
						 * data
						 */
				      dma_addr_t dstData1,
						/* Physical address of first
						 * destination buffer
						 */
				      dma_addr_t dstData2,
						/* Physical address of second
						 * destination buffer
						 */
				      size_t numBytes
						/* Number of bytes in each
						 * destination buffer
						 */
	)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	int numDescriptors;
	int rc;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	/* Figure out how many descriptors we need. */

	rc = sdma_prealloc_descriptors_by_size(handle, 2, numBytes,
					       &numDescriptors);
	if (rc != 0) {
		printk(KERN_ERR
		       "%s: sdma_prealloc_descriptors for "
		       "%d bytes failed: %d\n",
		       __func__, numBytes, rc);
		return rc;
	}

	/* Set descriptors to channel */
	set_descriptors(handle, numDescriptors);

	rc = add_descriptors(handle, srcData, dstData1, numBytes);
	if (rc != 1) {
		printk(KERN_ERR "%s: 1 add_descriptors failed, rc=%i\n",
		       __func__, rc);
		return -EINVAL;
	}

	rc = add_descriptors(handle, srcData, dstData2, numBytes);
	if (rc != 1) {
		printk(KERN_ERR "%s: 2 add_descriptors failed, rc=%i\n",
		       __func__, rc);
		return -EINVAL;
	}

	return numDescriptors;
}
EXPORT_SYMBOL(sdma_alloc_double_dst_descriptors);

/****************************************************************************/
/**
*   Allocates and sets up descriptors for a double buffered circular buffer
*   for source data.
*
*   This is primarily intended to be used for things like the egress samples
*   to a speaker.
*
*   @return
*       > 0     Number of descriptors actually allocated.
*       -EINVAL Invalid device type for this kind of transfer
*               (i.e. the device is _MEM_TO_DEV and not _DEV_TO_MEM)
*       -ENOMEM Memory exhausted
*/
/****************************************************************************/

int sdma_alloc_double_src_descriptors(DMA_Handle_t handle,	/* DMA Handle */
				      dma_addr_t srcData1,
				/* Physical address of first source buffer */
				      dma_addr_t srcData2,
				/* Physical address of second source buffer */
				      dma_addr_t dstData,
				/* Physical address of destination data */
				      size_t numBytes
				/* Number of bytes in each source buffer */
	)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	int numDescriptors;
	int rc;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	/* Figure out how many descriptors we need. */

	rc = sdma_prealloc_descriptors_by_size(handle, 2, numBytes,
					       &numDescriptors);
	if (rc != 0) {
		printk(KERN_ERR
		       "%s: sdma_prealloc_descriptors for "
		       "%d bytes failed: %d\n",
		       __func__, numBytes, rc);
		return rc;
	}

	/* Set descriptors to channel */
	set_descriptors(handle, numDescriptors);

	rc = add_descriptors(handle, srcData1, dstData, numBytes);
	if (rc != 1) {
		printk(KERN_ERR "%s: add_descriptors 1 failed, rc=%i\n",
		       __func__, rc);
		return -EINVAL;
	}

	rc = add_descriptors(handle, srcData2, dstData, numBytes);
	if (rc != 1) {
		printk(KERN_ERR "%s: add_descriptors 2 failede, rc=%i\n",
		       __func__, rc);
		return -EINVAL;
	}

	return numDescriptors;
}
EXPORT_SYMBOL(sdma_alloc_double_src_descriptors);

/****************************************************************************/
/**
*   Initiates a DMA, allocating the descriptors as required.
*
*   @return
*       0       Transfer was started successfully
*       -EINVAL Invalid device type for this kind of transfer
*               (i.e. the device is _DEV_TO_MEM and not _MEM_TO_DEV)
*/
/****************************************************************************/

int sdma_start_transfer(SDMA_Handle_t handle	/* DMA Handle */
	)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	unsigned long flags;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	devAttr->numBytes = chal_dma_prepare_transfer(channel->sdmacHandle);
	if (devAttr->numBytes <= 0) {
		devAttr->numBytes = 0;
		printk(KERN_ERR
		       "sdma_process_descriptors: failed to process "
		       "descriptor on device %d\n",
		       channel->devType);
		return -EINVAL;
	}

	if (devAttr->config.descType == CHAL_DMA_DESC_RING) {
		/* Adjust transfer size for ring transfers */
		devAttr->numBytes = devAttr->numBytes / devAttr->descUse;
	}

	spin_lock_irqsave(&gHwDmaLock, flags);

	/* And kick off the transfer */
	devAttr->transferStartTime = timer_get_tick_count();

#ifdef CONFIG_BCM_KNLLOG_IRQ
	if (gKnllogIrqSchedEnable & KNLLOG_DMA) {
		KNLLOG("tstart [%s devType=%u bytes=%u]\n",
		       devAttr->name, channel->devType, devAttr->numBytes);
	}
#endif

	chal_dma_start_transfer(channel->sdmacHandle);

	spin_unlock_irqrestore(&gHwDmaLock, flags);

	return 0;
}
EXPORT_SYMBOL(sdma_start_transfer);

/****************************************************************************/
/**
*   Stops a previously started DMA transfer.
*
*   @return
*       0       Transfer was stopped successfully
*       -ENODEV Invalid handle
*/
/****************************************************************************/

int sdma_stop_transfer(DMA_Handle_t handle)
{
	SDMA_Channel_t *channel;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	chal_dma_shutdown_channel(channel->sdmacHandle);

	return 0;
}
EXPORT_SYMBOL(sdma_stop_transfer);

/****************************************************************************/
/**
*   Waits for a DMA to complete by polling. This function is only intended
*   to be used for testing. Interrupts should be used for most DMA operations.
*/
/****************************************************************************/

int sdma_wait_transfer_done(SDMA_Handle_t handle)
{
	SDMA_Channel_t *channel;
	CHAL_DMA_STATUS_t status;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	while (1) {
		status = chal_dma_transfer_complete(channel->sdmacHandle);
		if (status == CHAL_DMA_STATUS_SUCCESS
		    || status == CHAL_DMA_STATUS_FAULT)
			break;
	}

	if (status != CHAL_DMA_STATUS_SUCCESS) {
		/* Get channel attribute */
		SDMA_DeviceAttribute_t *devAttr =
		    &SDMA_gDeviceAttribute[channel->devType];

		printk(KERN_ERR
		       "%s: DMA transfer failed on %s [status 0x%08X]\n",
		       __func__, devAttr->name, status);

		chal_dma_dump_register(channel->sdmacHandle, printk);

		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(sdma_wait_transfer_done);

/****************************************************************************/
/**
*   Initiates a DMA, allocating the descriptors as required.
*
*   @return
*       0       Transfer was started successfully
*       -EINVAL Invalid device type for this kind of transfer
*               (i.e. the device is _DEV_TO_MEM and not _MEM_TO_DEV)
*/
/****************************************************************************/

int sdma_transfer(SDMA_Handle_t handle,	/* DMA Handle */
		  dma_addr_t srcData,
				/* Place to get data to write to device */
		  dma_addr_t dstData,
				/* Pointer to device data address */
		  size_t numBytes
				/* Number of bytes to transfer to the device */
	)
{
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	int rc = 0;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	/*
	 * We keep track of the information about the previous request for this
	 * device, and if the attributes match, then we can use the descriptors
	 * we setup the last time, and not have to reinitialize everything.
	 */

	rc = sdma_alloc_descriptors(handle, srcData, dstData, numBytes);
	if (rc != 0)
		return rc;

	/* And kick off the transfer */

	return sdma_start_transfer(handle);
}
EXPORT_SYMBOL(sdma_transfer);

/****************************************************************************/
/**
*   Set the callback function which will be called when a transfer completes.
*   If a NULL callback function is set, then no callback will occur.
*
*   @note   @a devHandler will be called from IRQ context.
*
*   @return
*       0       - Success
*       -ENODEV - Device handed in is invalid.
*/
/****************************************************************************/

int sdma_set_device_handler(DMA_Device_t dev,
				/* Device to set the callback for. */
			    DMA_DeviceHandler_t devHandler,
				/* Function to call when the DMA completes */
			    void *userData
				/* Pointer which will be passed to devHandler.
				 */
	)
{
	SDMA_DeviceAttribute_t *devAttr;
	unsigned long flags;

	if (!IsDeviceValid(dev))
		return -ENODEV;
	devAttr = &SDMA_gDeviceAttribute[dev];

	spin_lock_irqsave(&gDmaDevLock, flags);

	devAttr->userData = userData;
	devAttr->devHandler = devHandler;

	if (devHandler == NULL) {
		/* No handler defined, so assume user wants to disable
		 * interrupts and poll for transfer completion
		 */
		/* TODO:  Add call to disable interrupts at block level.  API
		 *        in chal driver does not currently exist
		 */
	}

	spin_unlock_irqrestore(&gDmaDevLock, flags);

	return 0;
}
EXPORT_SYMBOL(sdma_set_device_handler);

/****************************************************************************/
/**
*   Set the callback function which will be called when a transfer completes.
*   If a NULL callback function is set, then no callback will occur.
*
*   @note   @a devHandler will be called from IRQ context.
*
*   @return
*       0       - Success
*       -ENODEV - Device handed in is invalid.
*/
/****************************************************************************/

int sdma_set_device_handler_extended(DMA_Device_t dev,
					/* Device to set the callback for. */
				     DMA_DeviceHandlerExt_t devHandlerExt,
					/* Function to call when the DMA
					 * completes
					 */
				     void *userData
					/* Pointer which will be passed to
					 * devHandlerExt.
					 */
	)
{
	SDMA_DeviceAttribute_t *devAttr;
	unsigned long flags;

	if (!IsDeviceValid(dev))
		return -ENODEV;
	devAttr = &SDMA_gDeviceAttribute[dev];

	spin_lock_irqsave(&gDmaDevLock, flags);

	devAttr->userData = userData;
	devAttr->devHandlerExt = devHandlerExt;

	if (devHandlerExt == NULL) {
		/* No handler defined, so assume user wants to disable
		 * interrupts and poll for transfer completion
		 */
		/* TODO:  Add call to disable interrupts at block level.  API
		 *        in chal driver does not currently exist
		 */
	}

	spin_unlock_irqrestore(&gDmaDevLock, flags);

	return 0;
}
EXPORT_SYMBOL(sdma_set_device_handler_extended);

/****************************************************************************/
/**
*   Debug a DMA channel
*
*   @return
*       0       Descriptors memory set successfully
*       -ENODEV Invalid channel
*/
/****************************************************************************/

int sdma_dump_debug_info(SDMA_Handle_t handle	/* DMA Handle */
	)
{
	SDMA_Channel_t *channel;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;
	chal_dma_dump_register(channel->sdmacHandle, printk);

	return 0;
}
EXPORT_SYMBOL(sdma_dump_debug_info);

/****************************************************************************/
/**
*   Helper routine to calculate number of descriptors. Used by
*   sdma_map_create_descriptor_ring.
*
*   @return
*       0       Descriptors memory set successfully
*       -ENODEV Invalid channel
*/
/****************************************************************************/

static int calc_desc_cnt(void *data1,
			 void *data2,
			 dma_addr_t srcAddr,
			 dma_addr_t dstAddr, size_t numBytes)
{
	(void)data2;
	(void)srcAddr;
	(void)dstAddr;

	return calculate_descriptor_count((SDMA_Handle_t) data1, numBytes);
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

static int add_desc(void *data1,
		    void *data2,
		    dma_addr_t srcAddr, dma_addr_t dstAddr, size_t numBytes)
{
	int rc;
	(void)data2;

	rc = add_descriptors((SDMA_Handle_t) data1, srcAddr, dstAddr, numBytes);
	return (rc >= 0);
}

/****************************************************************************/
/**
*   Setup a descriptor ring for a given memory map.
*
*   It is assumed that the descriptor ring has already been initialized, and
*   this routine will only reallocate a new descriptor ring if the existing
*   one is too small.
*
*   @return     0 on success, error code otherwise.
*/
/****************************************************************************/
int sdma_map_create_descriptor_ring(SDMA_Handle_t handle,
				    DMA_MMAP_CFG_T *memMap,
					/* Memory map that will be used */
				    dma_addr_t devPhysAddr,
					/* Physical address of device */
				    DMA_UpdateMode_t updateMode) {
	int rc;
	int numDescriptors;
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	DMA_MMAP_DEV_ADDR_MODE_T addrMode;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	if (updateMode == DMA_UPDATE_MODE_INC)
		addrMode = DMA_MMAP_DEV_ADDR_INC;
	else
		addrMode = DMA_MMAP_DEV_ADDR_NOINC;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	dma_mmap_lock_map(memMap);

	/* Figure out how many descriptors we need */
	numDescriptors = dma_mmap_calc_desc_cnt(memMap,
						devPhysAddr,
						addrMode,
						(void *)handle,
							/* user data 1 */
						NULL,	/* user data 2 */
						&calc_desc_cnt);

	if (numDescriptors < 0) {
		rc = numDescriptors;
		printk(KERN_ERR "%s: dma_mmap_calc_desc_cnt failed: %d\n",
		       __func__, rc);
		goto out;
	}

	rc = sdma_prealloc_descriptors_by_count(handle, numDescriptors);
	if (rc != 0) {
		printk(KERN_ERR
		       "%s: sdma_prealloc_descriptors_by_count for "
		       "%d descriptors failed: %d\n",
		       __func__, numDescriptors, rc);
		goto out;
	}

	/* Set descriptors to channel */
	set_descriptors(handle, numDescriptors);

	/* Populate the descriptors */
	rc = dma_mmap_add_desc(memMap,
			       devPhysAddr,
			       addrMode, (void *)handle, NULL, &add_desc);
	if (rc < 0) {
		printk(KERN_ERR "%s: dma_mmap_add_desc failed: %d\n", __func__,
		       rc);
		goto out;
	}

	rc = 0;

out:
	dma_mmap_unlock_map(memMap);

	return rc;
}
EXPORT_SYMBOL(sdma_map_create_descriptor_ring);

/****************************************************************************/
/**
*   Setup a descriptor ring for a device with a command to the device
*
*   It is assumed that the descriptor ring has already been initialized, and
*   this routine will only reallocate a new descriptor ring if the existing
*   one is too small.
*
*   @return     0 on success, error code otherwise.
*/
/****************************************************************************/
int sdma_map_create_descriptor_ring_with_command(SDMA_Handle_t handle,
		DMA_MMAP_CFG_T *memMap,	/* Memory map that will be used */
		dma_addr_t commandSrcPhysAddr,
				/* Physical address of the command source */
		dma_addr_t commandRegPhysAddr,
				/* Physical address of the command register */
		dma_addr_t devPhysAddr,	/* Physical address of device */
		DMA_UpdateMode_t updateMode
	)
{
	int rc;
	int numDescriptors;
	SDMA_Channel_t *channel;
	SDMA_DeviceAttribute_t *devAttr;
	DMA_MMAP_DEV_ADDR_MODE_T addrMode;

	channel = HandleToChannel(handle);
	if (channel == NULL)
		return -ENODEV;

	if (updateMode == DMA_UPDATE_MODE_INC)
		addrMode = DMA_MMAP_DEV_ADDR_INC;
	else
		addrMode = DMA_MMAP_DEV_ADDR_NOINC;

	devAttr = &SDMA_gDeviceAttribute[channel->devType];

	dma_mmap_lock_map(memMap);

	/* Figure out how many descriptors we need */
	numDescriptors = dma_mmap_calc_desc_cnt(memMap,
						devPhysAddr,
						addrMode,
						(void *)handle,
							/* user data 1 */
						NULL,	/* user data 2 */
						&calc_desc_cnt)
	    + 1;		/* One extra needed for setting a command to
				 * Read Req register
				 */

	if (numDescriptors < 0) {
		rc = numDescriptors;
		printk(KERN_ERR "%s: dma_mmap_calc_desc_cnt failed: %d\n",
		       __func__, rc);
		goto out;
	}

	rc = sdma_prealloc_descriptors_by_count(handle, numDescriptors);
	if (rc != 0) {
		printk(KERN_ERR
		       "%s: sdma_prealloc_descriptors_by_count for "
		       "%d descriptors failed: %d\n",
		       __func__, numDescriptors, rc);
		goto out;
	}

	/* Set descriptors to channel */
	set_descriptors(handle, numDescriptors);

	/* Add command for MPHI Read Req register */
	rc = add_device_command(handle, commandSrcPhysAddr, commandRegPhysAddr);

	if (rc < 0) {
		printk(KERN_ERR "%s: add_command_to_descriptor failed: %d\n",
		       __func__, rc);
		goto out;
	}

	/* Populate the descriptors */
	rc = dma_mmap_add_desc(memMap,
			       devPhysAddr,
			       addrMode, (void *)handle, NULL, &add_desc);
	if (rc < 0) {
		printk(KERN_ERR "%s: dma_mmap_add_desc failed: %d\n", __func__,
		       rc);
		goto out;
	}

	rc = 0;

out:
	dma_mmap_unlock_map(memMap);

	return rc;
}
EXPORT_SYMBOL(sdma_map_create_descriptor_ring_with_command);

#ifdef CONFIG_MACH_BCMHANA_CHIP_TEST
/****************************************************************************/
/**
*   Return DMA driver handle for testing/debugging purposes
*   @return
*       Secure/non-secure DMA handle
*/
/****************************************************************************/

int sdma_get_handle(int secure)
{
	return secure ? (int)gSecDmaHandle : (int)gOpenDmaHandle;
}
EXPORT_SYMBOL(sdma_get_handle);

/****************************************************************************/
/**
*   Aquire/release driver lock for testing/debugging purposes
*   @return
*       lock status
*/
/****************************************************************************/

int sdma_lock(int aquire)
{
	static unsigned long flags;

	if (aquire) {
		down(&gSDMA.lock);
		spin_lock_irqsave(&gHwDmaLock, flags);
	} else {
		spin_unlock_irqrestore(&gHwDmaLock, flags);
		up(&gSDMA.lock);
	}

	return 0;
}
EXPORT_SYMBOL(sdma_lock);
#endif /* CONFIG_MACH_BCMHANA_CHIP_TEST */
