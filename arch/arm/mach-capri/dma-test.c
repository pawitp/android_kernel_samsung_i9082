/*****************************************************************************
* Copyright 2006 - 2008 Broadcom Corporation.  All rights reserved.
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

/* ---- Include Files ---------------------------------------------------- */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#if defined(CONFIG_DMAC_PL330)
#include <mach/dma.h>
#elif defined(CONFIG_MAP_SDMA)
#include <mach/sdma.h>
#else
#error "No DMA driver configured"
#endif

#include <mach/io_map.h>
#include <mach/profile_timer.h>

/* ---- Public Variables ------------------------------------------------- */

/* ---- Private Constants and Types -------------------------------------- */

#define START_COUNT           0xA000

#define TEST_DMA_REQUEST_FREE 0

typedef struct {
	void *virtPtr;
	dma_addr_t physPtr;
	size_t numBytes;

} dma_mem_t;

#ifdef CONFIG_MAP_SDMA
static const DMA_Device_t device[] = {
	DMA_DEVICE_MEM_TO_MEM_0,
	DMA_DEVICE_MEM_TO_MEM_1,
	DMA_DEVICE_MEM_TO_MEM_2,
	DMA_DEVICE_MEM_TO_MEM_3,
	DMA_DEVICE_MEM_TO_MEM_4,
	DMA_DEVICE_MEM_TO_MEM_5,
	DMA_DEVICE_MEM_TO_MEM_6,
	DMA_DEVICE_MEM_TO_MEM_7,
};
#endif

#define DMA_NUM_CHANNELS 8

/* ---- Private Variables ------------------------------------------------ */
static struct semaphore gDmaDoneSem[DMA_NUM_CHANNELS];

static timer_tick_count_t gDmaStartTime[DMA_NUM_CHANNELS];

static volatile int gDmaTestRunning[2];

static int test_channels = DMA_NUM_CHANNELS;

static size_t alloc_size = 1024;

static int report_speeds;

/* ---- Private Function Prototypes -------------------------------------- */

/* ---- Functions -------------------------------------------------------- */

/****************************************************************************
*
*   Allocates a block of memory
*
***************************************************************************/

static void *alloc_mem(dma_mem_t * mem, size_t numBytes)
{
	mem->numBytes = numBytes;
	mem->virtPtr =
	    dma_alloc_writecombine(NULL, numBytes, &mem->physPtr, GFP_KERNEL);

	if (mem->virtPtr == NULL)
		pr_err("dma_alloc_writecombine of %d bytes failed\n", numBytes);

	pr_warn
	    ("dma_alloc_writecombine of %d bytes returned virtPtr: "
	     "%p physPstr: %08x\n",
	     numBytes, mem->virtPtr, mem->physPtr);

	return mem->virtPtr;
}

static void *free_mem(dma_mem_t * mem)
{

	if (mem->virtPtr != NULL) {
		dma_free_writecombine(NULL, mem->numBytes, mem->virtPtr,
				      mem->physPtr);
	}
	mem->virtPtr = NULL;

	return NULL;
}

#if defined(CONFIG_DMAC_PL330)
/****************************************************************************
*
*   dma_callback
*
***************************************************************************/

static void dma_callback(void *userData, enum pl330_xfer_status status)
{
	if (status == DMA_PL330_XFER_OK) {
		const int chan = (int)userData;

		up(&gDmaDoneSem[chan]);
	} else {
		pr_err("%s: called with status %d", __func__, status);
	}
}
#elif defined(CONFIG_MAP_SDMA)
/****************************************************************************
*
*   Handler called when the DMA finishes.
*
***************************************************************************/

static void dmaHandler(DMA_Device_t dev, int reason, void *userData)
{
	const int chan = (int)userData;

	up(&gDmaDoneSem[chan]);

	pr_warn("dmaHandler called: dev: %d, reason = 0x%x\n", dev, reason);
}
#endif

/****************************************************************************
*
*   Does a memcpy using DMA
*
***************************************************************************/
static int dma_test_set(int chan)
{
	sema_init(&gDmaDoneSem[chan], 0);

#ifdef CONFIG_MAP_SDMA
	{
		int rc;

		BUG_ON(chan >= ARRAY_SIZE(device));

		rc = sdma_set_device_handler(device[chan], dmaHandler,
					     (void *)chan);
		if (rc != 0) {
			pr_err("sdma_set_device_handler(%d) failed: %d\n",
			       (int)device[chan], rc);
			return rc;
		}
	}
#endif

	return 0;
}

static int dma_test_wait(int chan)
{
	int rc;
	unsigned long elapsed;
	timer_tick_rate_t rate;

	/* pr_warn( "Awaiting DMA (chan %d)\n", chan ); */
	rc = down_interruptible(&gDmaDoneSem[chan]);
	if (rc != 0) {
		pr_err("down_interruptible failed: %d\n", rc);
		return rc;
	}
	if (report_speeds) {
		elapsed =
		    (unsigned long)(timer_get_tick_count() -
				    gDmaStartTime[chan]);
		pr_info("gDmaStartTime[%d] = %llu\n", chan,
			(long long unsigned)gDmaStartTime[chan]);
		pr_info("elapsed (chan %d) = %lu, HZ = %u\n", chan, elapsed,
			(unsigned)HZ);
		rate = timer_get_tick_rate();
		if (rate) {
			pr_info("timer_ticks_to_msec(elapsed) = %u\n",
				timer_ticks_to_msec(elapsed));
			pr_info("timer_get_tick_rate() = %lu\n",
				(unsigned long)rate);
			pr_warn
			    ("DMAd %zu bytes on channel %d in %lums "
			     "(%lu bytes/s)\n",
			     alloc_size, chan,
			     (unsigned long)timer_ticks_to_msec(elapsed),
			     alloc_size * rate / elapsed);
		} else {
			rate = 455000000;
			pr_info
			    ("timer_get_tick_rate() returned 0, "
			     "assuming rate is %luHz\n", (unsigned long)rate);
			pr_warn
			    ("DMAd %lu bytes on channel %d in %luus"
			     " (%zu bytes/s)\n",
			     alloc_size, chan,
			     elapsed / (rate / 1000000),
			     alloc_size * (rate / elapsed));
		}
	} else {
		pr_warn("DMAd %zu bytes on channel %d\n",
			alloc_size, chan);
	}

	return 0;
}

/****************************************************************************
*
*   Initializes a block of memory with a known pattern
*
***************************************************************************/

static void dma_test_init_mem(dma_mem_t * mem, uint16_t start_count)
{
	int i;
	uint16_t *memInt;
	uint16_t counter = start_count;
	int numWords;

	numWords = mem->numBytes / sizeof(*memInt);
	memInt = mem->virtPtr;

	for (i = 0; i < numWords; i++)
		*memInt++ = counter++;
}

/****************************************************************************
*
*   Checks a block of memory to see if it has the expected value
*
***************************************************************************/

static int dma_test_check_mem(dma_mem_t * mem, uint16_t start_count)
{
	int i;
	uint16_t *memInt;
	uint16_t counter = start_count;
	int numWords;

	numWords = mem->numBytes / sizeof(*memInt);
	memInt = mem->virtPtr;

	for (i = 0; i < numWords; i++) {
		if (*memInt != counter) {
			pr_warn
			    ("Destination: base=%p, i=%d, found 0x%4.4x,"
			     " expecting 0x%4.4x\n",
			     mem->virtPtr, i, *memInt, counter);
			return -EIO;
		}
		memInt++;
		counter++;
	}

	return 0;
}

static uint16_t channel_to_start(int channel)
{
	const uint16_t chan = (uint16_t)channel;

	return (chan << 12) & (chan << 8) & (chan << 4) & (chan << 0);
}

static void memcpy_tests(long iterations)
{
	int rc = 0;
	int i;
	dma_mem_t src[DMA_NUM_CHANNELS];
	dma_mem_t dst[DMA_NUM_CHANNELS];
	int chan;
#if defined(CONFIG_DMAC_PL330)
	unsigned int dma_cfg;
	unsigned int dmaHandle[DMA_NUM_CHANNELS];
#elif defined(CONFIG_MAP_SDMA)
	SDMA_Handle_t dmaHandle[DMA_NUM_CHANNELS];
#endif
	bool dmaHandleValid[DMA_NUM_CHANNELS];

	int run_one_test(int test_number) {
		for (chan = 0; chan < test_channels; chan++) {
			/* Skip any channel where we didn't get a handle */
			if (!dmaHandleValid[chan]) {
				pr_warn("Skipping channel %d\n", chan);
				continue;
			}

			/* Start DMA operation */
			gDmaStartTime[chan] = timer_get_tick_count();
#if defined(CONFIG_DMAC_PL330)
			dma_cfg = DMA_CFG_BURST_SIZE_8 |
			    DMA_CFG_BURST_LENGTH_8 |
			    DMA_CFG_SRC_ADDR_INCREMENT |
			    DMA_CFG_DST_ADDR_INCREMENT;
			rc = dma_setup_transfer(dmaHandle[chan],
						src[chan].physPtr,
						dst[chan].physPtr,
						alloc_size,
						DMA_DIRECTION_MEM_TO_MEM,
						dma_cfg);
			if (rc < 0) {
				pr_err("dma_setup_transfer(%d) failed - %d\n",
				       chan, rc);
				return rc;
			}
			rc = dma_start_transfer(dmaHandle[chan]);
			if (rc != 0) {
				pr_err("dma_start_transfer(%d) failed - %d",
				       chan, rc);
				return rc;
			}
#elif defined(CONFIG_MAP_SDMA)
			sdma_transfer_mem_to_mem(dmaHandle[chan],
						 src[chan].physPtr,
						 dst[chan].physPtr, alloc_size);
#endif
		}

		for (chan = 0; chan < test_channels; chan++) {
			/* Skip any channel where we didn't get a handle */
			if (!dmaHandleValid[chan]) {
				pr_warn("Skipping channel %d\n", chan);
				continue;
			}

			/* Wait for DMA completion */
			dma_test_wait(chan);

#if defined(CONFIG_DMAC_PL330)
			rc = dma_stop_transfer(dmaHandle[chan]);
			if (rc < 0) {
				pr_err("dma_stop_transfer(%d) failed - %d\n",
				       chan, rc);
			}
#endif

			/* Verify transfer */
			rc = dma_test_check_mem(&dst[chan],
						channel_to_start(chan));
			if (rc != 0) {
				pr_err
				    (": ========== DMA memcpy test%d "
				     "channel %d failed [%03d] ==========\n\n",
				     test_number, chan, i);
				return rc;
			}

			pr_warn
			    ("\n========== DMA memcpy test%d channel %d "
			     "passed [%03d] ==========\n\n",
			     test_number, chan, i);

			/* Reset destination memory */
			memset(dst[chan].virtPtr, 0, dst[chan].numBytes);
		}

		return 0;
	}

	pr_warn("\n::Entering into test thread::\n\n");

	/* In case of errors, zero out src[] and dst[] */
	memset(src, 0, sizeof(src));
	memset(dst, 0, sizeof(dst));

	for (chan = 0; chan < test_channels; chan++) {
		/* Allocate contiguous source memory */
		if (alloc_mem(&src[chan], alloc_size) == NULL) {
			rc = -ENOMEM;
			goto out;
		}

		/* Allocate contiguous destination memory */
		if (alloc_mem(&dst[chan], alloc_size) == NULL) {
			rc = -ENOMEM;
			goto out;
		}

		/* Init test settings */
		dma_test_set(chan);

		/* initialize the source memory */
		dma_test_init_mem(&src[chan], channel_to_start(chan));

		/* Ensure that we won't try to free unallocated handles */
		dmaHandleValid[chan] = false;
	}

	for (i = 0; (i < iterations) && !rc; i++) {
		for (chan = 0; chan < test_channels; chan++) {
			/* Acquire a DMA channel */
			pr_info("Requesting channel %d\n", chan);
#if defined(CONFIG_DMAC_PL330)
			rc = dma_request_chan(&dmaHandle[chan],
					      NULL /*memory<->memory */ );
			if (rc < 0) {
				pr_warn
				    ("Failed to get handle for channel "
				     "%d (%d)\n", chan, rc);
			} else {
				dmaHandleValid[chan] = true;
				rc = dma_register_callback(dmaHandle[chan],
							   dma_callback,
							   (void *)chan);
				if (rc < 0) {
					pr_err
					    ("dma_register_callback(%d) failed - %d\n",
					     chan, rc);
				}
			}
#elif defined(CONFIG_MAP_SDMA)
			dmaHandle[chan] = sdma_request_channel(device[chan]);
			if (dmaHandle[chan] < 0) {
				rc = (int)dmaHandle[chan];
				pr_warn
				    ("Failed to get handle for channel "
				     "%d (%d)\n", chan, rc);
			} else {
				dmaHandleValid[chan] = true;
			}
#endif
		}

		rc = run_one_test(1);
		if (rc != 0)
			goto abort;

		/*
		 * Now test to see if we can do back-to-back DMA transfers
		 * and reuse the same DMA descriptor
		 */
		rc = run_one_test(2);

abort:
		for (chan = 0; chan < test_channels; chan++) {
			if (dmaHandleValid[chan]) {
#if defined(CONFIG_DMAC_PL330)
				rc = dma_free_chan(dmaHandle[chan]);
				if (rc < 0) {
					pr_err
					    ("dma_free_chan(%d) failed - %d\n",
					     chan, rc);
				}
#elif defined(CONFIG_MAP_SDMA)
				sdma_free_channel(dmaHandle[chan]);
#endif
			}
		}
	}

out:
	for (chan = 0; chan < test_channels; chan++) {
		/* Free test memory */
		free_mem(&src[chan]);
		free_mem(&dst[chan]);
	}

	pr_warn("\n ::Exiting from test thread (rc = %d)::\n\n", rc);
}

#if TEST_DMA_REQUEST_FREE
volatile SDMA_Handle_t gTestHandle[DMA_NUM_DEVICE_ENTRIES];

int dma_test_thread(void *data)
{
	unsigned long test = (unsigned long)data;
	int devIdx;
	int rc;

	pr_warn("dma_test_thread( %lu ) called\n", test);

	for (devIdx = 0; devIdx < DMA_NUM_DEVICE_ENTRIES; devIdx++)
		gTestHandle[devIdx] = SDMA_INVALID_HANDLE;

	for (devIdx = 0; devIdx < DMA_NUM_DEVICE_ENTRIES; devIdx++) {
		if (devIdx == DMA_DEVICE_NAND_MEM_TO_MEM) {
			pr_warn("Skipping NAND device %d\n", devIdx);
			continue;
		}

		if (test == 1) {
			pr_warn("About to request channel for device %d ...\n",
				devIdx);
			gTestHandle[devIdx] = sdma_request_channel(devIdx);
			if (gTestHandle[devIdx] < 0) {
				pr_warn
				    ("Call to sdma_request_channel "
				     "failed: %d\n", gTestHandle[devIdx]);
				continue;
			}
			pr_warn("   request completed, handle = 0x%04x\n",
				gTestHandle[devIdx]);
		} else {
			if (gTestHandle[devIdx] != SDMA_INVALID_HANDLE) {
				pr_warn
				    ("About to relase channel for device %d, "
				     "handle 0x%04x\n",
				     devIdx, gTestHandle[devIdx]);
				rc = sdma_free_channel(gTestHandle[devIdx]);
				if (rc < 0) {
					pr_warn
					    ("Call to sdma_free_channel "
					     "failed: %d\n", rc);
				}
				msleep(33);
			}
		}
	}

	pr_warn("test thread %ld exiting\n", test);
	gDmaTestRunning[test - 1] = 0;

	return 0;
}
#endif

static void dma_test_run(long iterations)
{
	pr_warn("\n========== Starting DMA memory copy tests ==========\n\n");
	memcpy_tests(iterations);

#if TEST_DMA_REQUEST_FREE
	pr_warn("\n========== Starting request/free test ==========\n\n");

	gDmaTestRunning[0] = 1;
	gDmaTestRunning[1] = 1;
	pr_warn("Starting request thread\n");
	kthread_run(dma_test_thread, (void *)1, "dmaRequest");
	msleep(100);
	pr_warn("Starting release thread\n");
	kthread_run(dma_test_thread, (void *)2, "dmaRelease");

	while (gDmaTestRunning[0] || gDmaTestRunning[1]) {
		pr_warn("Waiting for tests to complete... %d %d\n",
			gDmaTestRunning[0], gDmaTestRunning[1]);
		msleep(1000);
	}
	pr_warn("\n========== Done request/free test ==========\n\n");
#endif
}

static int dma_read_proc_tst_speed(char *buffer,
				   char **buffer_location,
				   off_t offset, int buffer_length,
				   int *eof, void *data)
{
	/* All done on the first call */
	if (offset > 0)
		return 0;
	return snprintf(buffer, buffer_length, "%d\n", report_speeds);
}

static ssize_t dma_write_proc_tst_speed(struct file *file,
					const char __user *buf,
					unsigned long count, void *data)
{
	char lbuf[12];
	long value;

	if (count >= sizeof(lbuf))
		return -EINVAL;

	if (copy_from_user(lbuf, buf, count))
		return -EFAULT;
	lbuf[count] = '\0';

	if (strict_strtol(lbuf, 10, &value))
		return -EINVAL;

	report_speeds = (int)value;

	return count;
}

static int dma_register_tst_speed_file(void)
{
	struct proc_dir_entry *gDmaDir;

	gDmaDir = create_proc_entry("dma_test/test_speed",
				    S_IRUGO | S_IWUGO, NULL);

	if (gDmaDir == NULL)
		pr_err("Unable to create /proc/dma_test/test_speed\n");
	else
		gDmaDir->read_proc = dma_read_proc_tst_speed;
		gDmaDir->write_proc = dma_write_proc_tst_speed;

	return 0;
}

static void dma_unregister_tst_speed_file(void)
{
	remove_proc_entry("dma_test/test_speed", NULL);
}

static int dma_read_proc_tst_chans(char *buffer,
				   char **buffer_location,
				   off_t offset, int buffer_length,
				   int *eof, void *data)
{
	/* All done on the first call */
	if (offset > 0)
		return 0;
	return snprintf(buffer, buffer_length, "%d\n", test_channels);
}

static ssize_t dma_write_proc_tst_chans(struct file *file,
					const char __user *buf,
					unsigned long count, void *data)
{
	char lbuf[12];
	long value;

	if (count >= sizeof(lbuf))
		return -EINVAL;

	if (copy_from_user(lbuf, buf, count))
		return -EFAULT;
	lbuf[count] = '\0';

	if (strict_strtol(lbuf, 10, &value))
		return -EINVAL;

	if (value > DMA_NUM_CHANNELS)
		return -EINVAL;

	if (value < 1)
		return -EINVAL;

	test_channels = (int)value;

	return count;
}

static int dma_register_tst_chans_file(void)
{
	struct proc_dir_entry *gDmaDir;

	gDmaDir = create_proc_entry("dma_test/test_channels",
				    S_IRUGO | S_IWUGO, NULL);

	if (gDmaDir == NULL)
		pr_err("Unable to create /proc/dma_test/test_channels\n");
	else
		gDmaDir->read_proc = dma_read_proc_tst_chans;
		gDmaDir->write_proc = dma_write_proc_tst_chans;

	return 0;
}

static void dma_unregister_tst_chans_file(void)
{
	remove_proc_entry("dma_test/test_channels", NULL);
}

static int dma_read_proc_tst_size(char *buffer,
				  char **buffer_location,
				  off_t offset, int buffer_length,
				  int *eof, void *data)
{
	/* All done on the first call */
	if (offset > 0)
		return 0;
	return snprintf(buffer, buffer_length, "%zu\n", alloc_size);
}

static ssize_t dma_write_proc_tst_size(struct file *file,
				       const char __user *buf,
				       unsigned long count, void *data)
{
	char lbuf[12];
	long value;

	if (count >= sizeof(lbuf))
		return -EINVAL;

	if (copy_from_user(lbuf, buf, count))
		return -EFAULT;
	lbuf[count] = '\0';

	if (strict_strtol(lbuf, 10, &value))
		return -EINVAL;

	alloc_size = (size_t) value;

	return count;
}

static int dma_register_tst_size_file(void)
{
	struct proc_dir_entry *gDmaDir;

	gDmaDir = create_proc_entry("dma_test/test_size",
				    S_IRUGO | S_IWUGO, NULL);

	if (gDmaDir == NULL)
		pr_err("Unable to create /proc/dma_test/test_size\n");
	else
		gDmaDir->read_proc = dma_read_proc_tst_size;
		gDmaDir->write_proc = dma_write_proc_tst_size;

	return 0;
}

static void dma_unregister_tst_size_file(void)
{
	remove_proc_entry("dma_test/test_size", NULL);
}

static ssize_t dma_write_proc_test(struct file *file,
				   const char __user *buf,
				   unsigned long count, void *data)
{
	char lbuf[12];
	long value;

	if (count >= sizeof(lbuf))
		return -EINVAL;

	if (copy_from_user(lbuf, buf, count))
		return -EFAULT;
	lbuf[count] = '\0';

	if (strict_strtol(lbuf, 10, &value))
		return -EINVAL;

	dma_test_run(value);

	return count;
}

static int dma_register_test_file(void)
{
	struct proc_dir_entry *gDmaDir;

	gDmaDir = create_proc_entry("dma_test/test", S_IWUGO, NULL);

	if (gDmaDir == NULL)
		pr_err("Unable to create /proc/dma_test/test\n");
	else
		gDmaDir->write_proc = dma_write_proc_test;

	return 0;
}

static void dma_unregister_test_file(void)
{
	remove_proc_entry("dma_test/test", NULL);
}

/****************************************************************************
*
*   Called to perform module initialization when the module is loaded
*
***************************************************************************/
static int __init dma_test_init(void)
{
	int rv;
	struct proc_dir_entry *gDmaDir;

	/* Create /proc/dma */
	gDmaDir =
	    create_proc_entry("dma_test", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
	if (gDmaDir == NULL) {
		pr_err("Unable to create /proc/dma_test\n");
		return -EIO;
	}

	rv = dma_register_tst_chans_file();

	if (rv)
		return rv;

	rv = dma_register_tst_size_file();

	if (rv)
		return rv;

	rv = dma_register_tst_speed_file();

	if (rv)
		return rv;

	return dma_register_test_file();
}

/****************************************************************************
*
*   Called to perform module cleanup when the module is unloaded.
*
***************************************************************************/

static void __exit dma_test_exit(void)
{
	dma_unregister_tst_chans_file();
	dma_unregister_tst_size_file();
	dma_unregister_tst_speed_file();
	dma_unregister_test_file();
	remove_proc_entry("dma_test", NULL);
}

/****************************************************************************/

module_init(dma_test_init);
module_exit(dma_test_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Broadcom System DMA Test");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
