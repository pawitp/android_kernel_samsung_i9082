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

/*
*
*****************************************************************************
*
*  perftest.c
*
*  PURPOSE:
*       performance tests
*
*  NOTES:
*       Enable CONFIG_BCM_PERFTEST_SUPPORT
*
*****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */

#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/string.h>
#include <linux/broadcom/bcm_sysctl.h>
#include <linux/version.h>
//#include <linux/broadcom/perfcnt.h>
#include <linux/broadcom/knllog.h>
#include <linux/interrupt.h>
//#include <linux/broadcom/timer.h>             // Alamy: porting from bcmhana (No such file)
#include <linux/mm.h>
#include <linux/completion.h>
//#include <csp/chal_gptimer.h>
//#include <cfg_global.h>                       // Alamy: porting from bcmhana (No such file)
//#include <mach/csp/mm_io.h>
#include <mach/io_map.h>	// Alamy: porting from bcmhana (use KONA_xxx definition)
#include <asm-generic/sizes.h>	// Alamy: porting from bcmhana (SZ_xxx definition)
#include <linux/cpumask.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <mach/sdma.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>

/*
 *  ENABLE_DMA_MEMCPY_PERF_TEST
 *      Disabled according to Ray
 *      It is only used by Ethernet which calls into low-level interface directly,
 *      not necessary to have DMA interface on "island" platform
 *
 *  ENABLE_ARM_PERF_MONITOR_TEST
 *      Disabled according to Ray
 *      There is already ARM performance related function somewhere
 *
 *  ENABLE_SRAM_TEST
 *      See the comment below in the sramtest() thread function.
 *      The memory address, KONA_SRAM_VA or KONA_INT_SRAM_BASE, and its size need to be verified.
 *      Pending: need to be verified by Ray (Nov-21, 2011)
 *
 */
#define ENABLE_DMA_MEMCPY_PERF_TEST     0	// Alamy: porting from bcmhana
#define ENABLE_ARM_PERF_MONITOR_TEST    0	// Alamy: porting from bcmhana
#define ENABLE_SRAM_TEST                1	// Alamy: porting from bcmhana

/* ---- Constants and Types ---------------------------------------------- */

#define START_COUNT           0xA000

typedef struct {
	void *virtPtr;
	dma_addr_t physPtr;
	size_t numBytes;

} dma_mem_t;

typedef enum {
	CMD_ENABLE_IRQ,
	CMD_DISABLE_IRQ,
#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
	CMD_MEMCPY_TEST,
#endif
	CMD_MEMCPY_ARMLIB_TEST,
	CMD_SDMA_MEMCPY_TEST,
#if (ENABLE_DMA_MEMCPY_PERF_TEST == 1)	// Alamy: porting from bcmhana
	CMD_DMA_MEMCPY_TEST,
#endif
#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
	CMD_NOP_TEST,
#endif
	CMD_NOPLOOP,
#if (ENABLE_SRAM_TEST == 1)
	CMD_SRAM_TEST,
#endif
#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
	CMD_EVENT
#endif
} PERFCMD;

/* The perftmr object */
typedef struct {
//    uint32_t tmrStart;
	timer_tick_count_t tick_start;
	uint32_t elapsedTimeTotal;	// in usec
} PERFTMR_OBJ;

/* The perfcmd object */
typedef struct {
	PERFCMD cmd;
	int perfcmdWake;
} PERFCMD_OBJ;

/* The perftest object (per cpu) */
typedef struct {
	PERFCMD_OBJ perfcmd;
	long perfcmdThreadPid;
	struct completion perfcmdExited;
	wait_queue_head_t perfcmdWakeQ;
	int cpu_enum;
	int memcpyIter;
	int nopIter;
	volatile int nop_pid;	// Alamy (fix): We need this variable to be voltile or compiler optimized the loop
	struct completion noplooptestExited;
#if (ENABLE_SRAM_TEST == 1)
	int sram_pid;
	struct completion sramtestExited;
#endif
} PERFTEST_OBJ;

/* The perfflag object */
typedef struct {
	int verbose;
	int memcpysize;		/* memcpy size */
	int enableint;		/* enable interrupt */
	int disableint;		/* disable interrupt */
#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
	int memcpy;		/* memcpy test with perf counters */
#endif
	int memcpy_armlib;	/* memcpy test - arm library function */
	int sdma_memcpy;	/* sdma memcpy test */
#if (ENABLE_DMA_MEMCPY_PERF_TEST == 1)	// Alamy: porting from bcmhana
	int dma_memcpy;		/* adma memcpy test */
#endif
	int nop;		/* nop test */
	int noploop;		/* nop at lowest prio */
#if (ENABLE_SRAM_TEST == 1)
	int sramtest;		/* sramtest at lowest prio */
#endif
	int cpu_nr;		/* selects which CPU to talk to */
	int quiet;		/* disable printk's for scripting/performance testing */
	int verify;		/* verify results of memcpy */
} PERFFLAG_OBJ;

/* storage for the objects */

#if (ENABLE_DMA_MEMCPY_PERF_TEST == 1)
volatile DMA_Handle_t gDmaHandle;
struct semaphore gDmaDoneSem;
#endif
struct semaphore gSDmaDoneSem;

static PERFTEST_OBJ perftest[NR_CPUS];
static PERFFLAG_OBJ perfflag;
#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
static ARM_PERF_EVT event[ARM_PERF_PMCNT_MAX];
#endif

#define nop1() asm(" nop");
#define nop2() nop1() nop1();
#define nop4() nop2() nop2();
#define nop8() nop4() nop4();
#define nop16() nop8() nop8();
#define nop32() nop16() nop16();
#define nop64() nop32() nop32();
#define nop128() nop64() nop64();
#define nop256() nop128() nop128();
#define nop512() nop256() nop256();
#define nop1024() nop512() nop512();
#define nop2048() nop1024() nop1024();
#define nop4096() nop2048() nop2048();
#define nop8192() nop4096() nop4096();
#define nop16k() nop8192() nop8192();
#define nop32k() nop16k() nop16k();
#define nop64k() nop32k() nop32k();
#define nop128k() nop64k() nop64k();
#define nop256k() nop128k() nop128k();

//static CHAL_HANDLE timer_handle;

#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
void *mymemcpy(void *dest, const void *src, unsigned int n);
extern void clean_invalidate_dcache(void);
#endif // ENABLE_ARM_PERF_MONITOR_TEST
#if defined(CPU_CACHE_V7)
extern void v7_flush_dcache_all(void);
#endif

/* sysctl */
static struct ctl_table_header *gSysCtlHeader;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
static int proc_do_perftest_intvec_memcpy(ctl_table * table, int write,
					  void __user *buffer, size_t * lenp,
					  loff_t *ppos);
#endif
static int proc_do_perftest_intvec_memcpy_armlib(ctl_table * table, int write,
						 void __user *buffer,
						 size_t * lenp, loff_t *ppos);
static int proc_do_perftest_intvec_sdma_memcpy(ctl_table * table, int write,
					       void __user *buffer,
					       size_t * lenp, loff_t *ppos);
#if (ENABLE_DMA_MEMCPY_PERF_TEST == 1)	// Alamy: porting from bcmhana
static int proc_do_perftest_intvec_dma_memcpy(ctl_table * table, int write,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos);
#endif
#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
static int proc_do_perftest_intvec_nop(ctl_table * table, int write,
				       void __user *buffer, size_t * lenp,
				       loff_t *ppos);
#endif
static int proc_do_perftest_intvec_enableint(ctl_table * table, int write,
					     void __user *buffer, size_t * lenp,
					     loff_t *ppos);
static int proc_do_perftest_intvec_disableint(ctl_table * table, int write,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos);
static int proc_do_perftest_intvec_memcpysize(ctl_table * table, int write,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos);
static int proc_do_perftest_intvec_noploop(ctl_table * table, int write,
					   void __user *buffer, size_t * lenp,
					   loff_t *ppos);
#if (ENABLE_SRAM_TEST == 1)
static int proc_do_perftest_intvec_sramtest(ctl_table * table, int write,
					    void __user *buffer, size_t * lenp,
					    loff_t *ppos);
#endif
static int proc_do_perftest_intvec_cpu(ctl_table * table, int write,
				       void __user *buffer, size_t * lenp,
				       loff_t *ppos);
static int proc_do_perftest_intvec_quiet(ctl_table * table, int write,
					 void __user *buffer, size_t * lenp,
					 loff_t *ppos);
static int proc_do_perftest_intvec_verify(ctl_table * table, int write,
					  void __user *buffer, size_t * lenp,
					  loff_t *ppos);
#else
static int proc_do_perftest_intvec_memcpy(ctl_table * table, int write,
					  struct file *filp,
					  void __user *buffer, size_t * lenp,
					  loff_t *ppos);
static int proc_do_perftest_intvec_memcpy_armlib(ctl_table * table, int write,
						 struct file *filp,
						 void __user *buffer,
						 size_t * lenp, loff_t *ppos);
static int proc_do_perftest_intvec_nop(ctl_table * table, int write,
				       struct file *filp, void __user *buffer,
				       size_t * lenp, loff_t *ppos);
static int proc_do_perftest_intvec_enableint(ctl_table * table, int write,
					     struct file *filp,
					     void __user *buffer, size_t * lenp,
					     loff_t *ppos);
static int proc_do_perftest_intvec_disableint(ctl_table * table, int write,
					      struct file *filp,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos);
static int proc_do_perftest_intvec_memcpysize(ctl_table * table, int write,
					      struct file *filp,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos);
static int proc_do_perftest_intvec_noploop(ctl_table * table, int write,
					   struct file *filp,
					   void __user *buffer, size_t * lenp,
					   loff_t *ppos);
static int proc_do_perftest_intvec_sramtest(ctl_table * table, int write,
					    struct file *filp,
					    void __user *buffer, size_t * lenp,
					    loff_t *ppos);
static int proc_do_perftest_intvec_cpu(ctl_table * table, int write,
				       struct file *filp, void __user *buffer,
				       size_t * lenp, loff_t *ppos);
static int proc_do_perftest_intvec_quiet(ctl_table * table, int write,
					 struct file *filp, void __user *buffer,
					 size_t * lenp, loff_t *ppos);
static int proc_do_perftest_intvec_verify(ctl_table * table, int write,
					  struct file *filp,
					  void __user *buffer, size_t * lenp,
					  loff_t *ppos);
#endif

static struct ctl_table gSysCtlPerftest[] = {
#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
	{
	 .procname = "memcpy",
	 .data = &perfflag.memcpy,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_memcpy},
#endif
	{
	 .procname = "memcpy_armlib",
	 .data = &perfflag.memcpy_armlib,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_memcpy_armlib},
	{
	 .procname = "sdma_memcpy",
	 .data = &perfflag.sdma_memcpy,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_sdma_memcpy},
#if (ENABLE_DMA_MEMCPY_PERF_TEST == 1)	// Alamy: porting from bcmhana
	{
	 .procname = "dma_memcpy",
	 .data = &perfflag.dma_memcpy,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_dma_memcpy},
#endif
#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
	{
	 .procname = "nop",
	 .data = &perfflag.nop,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_nop},
#endif
	{
	 .procname = "enableint",
	 .data = &perfflag.enableint,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_enableint},
	{
	 .procname = "disableint",
	 .data = &perfflag.disableint,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_disableint},
	{
	 .procname = "memcpysize",
	 .data = &perfflag.memcpysize,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_memcpysize},
	{
	 .procname = "verbose",
	 .data = &perfflag.verbose,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_dointvec},
	{
	 .procname = "noploop",
	 .data = &perfflag.noploop,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_noploop},
#if (ENABLE_SRAM_TEST == 1)
	{
	 .procname = "sramtest",
	 .data = &perfflag.sramtest,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_sramtest},
#endif
	{
	 .procname = "cpu",
	 .data = &perfflag.cpu_nr,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_cpu},
	{
	 .procname = "quiet",
	 .data = &perfflag.quiet,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_quiet},
	{
	 .procname = "verify",
	 .data = &perfflag.verify,
	 .maxlen = sizeof(int),
	 .mode = 0644,
	 .proc_handler = &proc_do_perftest_intvec_verify},
	{}
};

static struct ctl_table gSysCtl[] = {
	{
	 .procname = "perftest",
	 .mode = 0555,
	 .child = gSysCtlPerftest},
	{}
};

/****************************************************************************
*
*   Checks a block of memory to see if it has the expected value
*
***************************************************************************/
static int dma_check_mem(dma_mem_t * mem)
{
	int i;
	uint16_t *memInt;
	uint16_t counter;
	int numWords;

	numWords = mem->numBytes / sizeof(*memInt);

	memInt = mem->virtPtr;
	counter = START_COUNT;
	for (i = 0; i < numWords; i++) {
		if (*memInt != counter) {
			printk
			    ("Destination: i=%d, found 0x%4.4x, expecting 0x%4.4x\n",
			     i, *memInt, counter);
			return -EIO;
		}
		memInt++;
		counter++;
	}

	return 0;
}

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

	if (mem->virtPtr == NULL) {
		printk(KERN_ERR "dma_alloc_writecombine of %d bytes failed\n",
		       numBytes);
	}

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

/****************************************************************************
*
*   Handler called when the DMA finishes.
*
***************************************************************************/
static void sdmaHandler(DMA_Device_t dev, int reason, void *userData)
{
	up(&gSDmaDoneSem);
}

#if (ENABLE_DMA_MEMCPY_PERF_TEST == 1)	// Alamy: porting from bcmhana
static void dmaHandler(DMA_Device_t dev, int reason, void *userData)
{
	up(&gDmaDoneSem);
}
#endif

/****************************************************************************
*
*   Does a memcpy using DMA
*
***************************************************************************/
static int sdma_test_set(void)
{
	int rc;
	sema_init(&gSDmaDoneSem, 0);

	if ((rc =
	     sdma_set_device_handler(DMA_DEVICE_MEM_TO_MEM, sdmaHandler,
				     NULL)) != 0) {
		printk(KERN_ERR "dma_set_device_handler failed: %d\n", rc);
		return rc;
	}

	return 0;
}

static int sdma_test_wait(void)
{
	int rc;

	if ((rc = down_interruptible(&gSDmaDoneSem)) != 0) {
		printk(KERN_ERR "down_interruptible failed: %d\n", rc);
		return rc;
	}

	return 0;
}

/****************************************************************************
*
*   Initializes a block of memory with a known pattern
*
***************************************************************************/

static void dma_test_init_mem(dma_mem_t * mem)
{
	int i;
	uint16_t *memInt;
	uint16_t counter;
	int numWords;

	numWords = mem->numBytes / sizeof(*memInt);
	memInt = mem->virtPtr;
	counter = START_COUNT;

	for (i = 0; i < numWords; i++) {
		*memInt++ = counter++;
	}
}

static int sdma_memcpy_perf_test(unsigned int alloc_size, int iterations)
{
	int i;
	dma_mem_t src;
	dma_mem_t dst;
	SDMA_Handle_t dmaHandle;
//    int elapsed;
//    int tmrStart;
//    int tmrEnd;
//    uint32_t lsw, msw;
	timer_tick_count_t timer_tick_start, timer_tick_end;
	int32_t elapsed;
	int rc;

	/* Allocate contiguous source memory */
	if (alloc_mem(&src, alloc_size) == NULL) {
		rc = -ENOMEM;
		goto cleanup;	// Alamy: Don't want memory-leakage
	}

	/* Allocate contiguous destination memory */
	if (alloc_mem(&dst, alloc_size) == NULL) {
		rc = -ENOMEM;
		goto cleanup;	// Alamy: Don't want memory-leakage
	}

	/* Init test settings */
	sdma_test_set();

	/* Aquire a DMA channel */
	dmaHandle = sdma_request_channel(DMA_DEVICE_MEM_TO_MEM);
	/* initialize the source memory */
	dma_test_init_mem(&src);

//   tmrStart = chal_gptimer_get_timer_current_tick(timer_handle, &lsw, &msw);
	timer_tick_start = timer_get_tick_count();

	for (i = 0; i < iterations; i++) {
		/* Start DMA operation */
		sdma_transfer_mem_to_mem(dmaHandle, src.physPtr, dst.physPtr,
					 alloc_size);

		/* Wait for DMA completion */
		sdma_test_wait();

		if (perfflag.verify) {
			/* Verify transfer */
			if ((rc = dma_check_mem(&dst)) != 0) {
				printk(KERN_ERR
				       ": ========== SDMA memcpy test failed [%03d] ==========\n\n",
				       i);
				goto cleanup;	// Alamy: Don't want memory-leakage
			}
		}
	}
	sdma_free_channel(dmaHandle);
//    tmrEnd = chal_gptimer_get_timer_current_tick(timer_handle, &lsw, &msw);
//    elapsed  = tmrEnd - tmrStart;
	timer_tick_end = timer_get_tick_count();
	elapsed = (timer_tick_end - timer_tick_start) / timer_get_tick_rate();
	if (elapsed < 0) {
		elapsed = -elapsed;
	}
	if (!perfflag.quiet) {
		printk("%s: %u usec for %u iterations\n", __func__, elapsed,
		       iterations);
	}

cleanup:
	/* Free test memory */
	free_mem(&src);
	free_mem(&dst);

	return 0;
}

#if (ENABLE_DMA_MEMCPY_PERF_TEST == 1)	// Alamy: porting from bcmhana
/****************************************************************************
*
*   Does a memcpy using DMA
*
***************************************************************************/
static int dma_memcpy(dma_mem_t * dst, dma_mem_t * src) sram
{
	int rc;

	sema_init(&gDmaDoneSem, 0);
	if ((rc =
	     dma_transfer_mem_to_mem(gDmaHandle, src->physPtr, dst->physPtr,
				     src->numBytes)) != 0) {
		printk(KERN_ERR "dma_transfer_mem_to_mem failed: %d\n", rc);
		return rc;
	}

	if ((rc = down_interruptible(&gDmaDoneSem)) != 0) {
		printk(KERN_ERR "down_interruptible failed: %d\n", rc);
		return rc;
	}

	return 0;
}

/****************************************************************************
*
*   Initializes a block of memory with a known pattern
*
***************************************************************************/
static void dma_init_mem(dma_mem_t * mem)
{
	int i;
	uint16_t *memInt;
	uint16_t counter;
	int numWords;

	numWords = mem->numBytes / sizeof(*memInt);
	memInt = mem->virtPtr;
	counter = START_COUNT;

	for (i = 0; i < numWords; i++) {
		*memInt++ = counter++;
	}
}

static int dma_memcpy_perf_test(unsigned int alloc_size, int iterations)
{
	int rc;
	int i;
	dma_mem_t src = 0;
	dma_mem_t dst = 0;
//    int elapsed;
//    int tmrStart;
//    int tmrEnd;
//    uint32_t lsw, msw;
	timer_tick_count_t timer_tick_start, timer_tick_end;
	int32_t elapsed;

	gDmaHandle = dma_request_channel(DMA_DEVICE_MEM_TO_MEM_32);
	if (gDmaHandle < 0) {
		printk(KERN_ERR "dma_request_channel failed: %d\n", gDmaHandle);
		rc = -EBUSY;
		goto cleanup;	// Alamy: Don't want memory-leakage
	}
	if (alloc_mem(&src, alloc_size) == NULL) {
		rc = -ENOMEM;
		goto cleanup;	// Alamy: Don't want memory-leakage
	}
	if (alloc_mem(&dst, alloc_size) == NULL) {
		rc = -ENOMEM;
		goto cleanup;	// Alamy: Don't want memory-leakage
	}

	dma_init_mem(&src);

	if ((rc =
	     dma_set_device_handler(DMA_DEVICE_MEM_TO_MEM_32, dmaHandler,
				    NULL)) != 0) {
		printk(KERN_ERR "dma_set_device_handler failed: %d\n", rc);
		goto cleanup;	// Alamy: Don't want memory-leakage
	}
//    tmrStart = chal_gptimer_get_timer_current_tick(timer_handle, &lsw, &msw);
	timer_tick_start = timer_get_tick_count();

	for (i = 0; i < iterations; i++) {
		dma_memcpy(&dst, &src);
		if (perfflag.verify) {
			if ((rc = dma_check_mem(&dst)) != 0) {
				printk(KERN_ERR
				       ": ========== DMA memcpy test failed [%03d] ==========\n\n",
				       i);
				goto cleanup;	// Alamy: Don't want memory-leakage
			}
		}
	}

//    tmrEnd = chal_gptimer_get_timer_current_tick(timer_handle, &lsw, &msw);
//    elapsed  = tmrEnd - tmrStart;
	timer_tick_end = timer_get_tick_count();
	elapsed = (timer_tick_end - timer_tick_start) / timer_get_tick_rate();
	if (elapsed < 0) {
		elapsed = -elapsed;
	}
	if (!perfflag.quiet) {
		printk("%s: %u usec for %u iterations\n", __func__, elapsed,
		       iterations);
	}

cleanup:
	if (gDmaHandle >= 0) {
		dma_free_channel(gDmaHandle);
		gDmaHandle = -1;	// Invalidate it!
	}

	/* Free test memory */
	free_mem(&src);
	free_mem(&dst);

	return 0;
}
#endif // ENABLE_DMA_MEMCPY_PERF_TEST

#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
/****************************************************************************/
/**
*  @brief   initialize performance counters and timer
*
*  @return
*/
/****************************************************************************/
static void perf_init_start(PERFTMR_OBJ * tmr, ARM_PERF_EVT * evt)
{
//    uint32_t lsw, msw;

	arm_perf_init_start(evt);
//    tmr->tmrStart = chal_gptimer_get_timer_current_tick(timer_handle, &lsw, &msw);
	tmr->tick_start = timer_get_tick_count();
}

/****************************************************************************/
/**
*  @brief   initialize performance counters and timer
*
*  @return
*/
/****************************************************************************/
static void perf_clear(PERFTMR_OBJ * tmr)
{
	arm_perf_clear();
	tmr->elapsedTimeTotal = 0;
}

/****************************************************************************/
/**
*  @brief   initialize performance counters and timer
*
*  @return
*/
/****************************************************************************/
static void perf_stop_save(PERFTMR_OBJ * tmr)
{
//   int elapsed;
//   int tmrEnd;
//   uint32_t lsw, msw;
	timer_tick_count_t timer_tick_end;
	int32_t elapsed;

	arm_perf_stop_save();
//    tmrEnd = chal_gptimer_get_timer_current_tick(timer_handle, &lsw, &msw);
//    elapsed  = tmrEnd - tmr->tmrStart;
	timer_tick_end = timer_get_tick_count();
	elapsed = (timer_tick_end - tmr->tick_start) / timer_get_tick_rate();
	if (elapsed < 0) {
		elapsed = -elapsed;
	}
	tmr->elapsedTimeTotal += (uint32_t)elapsed;
}

/****************************************************************************/
/**
*  @brief   Simple profiled nop test of a certain block size and a number of iterations.
*
*  @return
*/
/****************************************************************************/
static void nop_perf_test(int iterations, int verbose, char *banner)
{
	int i = 0;
	int j;
	PERFTMR_OBJ tmr;

	/* running the test a first time ensures instructions are in L2 */
	nop64k();
	perf_clear(&tmr);
	while (arm_perf_evt_group_tbl[i][0] != (ARM_PERF_EVT) ~ 0) {
		perf_init_start(&tmr, &arm_perf_evt_group_tbl[i][0]);
		for (j = 0; j < iterations; j++) {
			nop64k();
		}
		perf_stop_save(&tmr);
		if (verbose >= 2) {
			arm_perf_print_mon_cnts(printk, verbose);
		}
		i++;
	}
	arm_perf_print_summary(printk, banner, verbose);
}
#endif // ENABLE_ARM_PERF_MONITOR_TEST

/****************************************************************************
*
*   send_perfcmd
*
*   help function to send perf cmd.
*
***************************************************************************/
static inline void send_perfcmd(int cpu, PERFCMD cmd)
{
	perftest[cpu].perfcmd.cmd = cmd;
	perftest[cpu].perfcmd.perfcmdWake = 1;
	wake_up_interruptible(&perftest[cpu].perfcmdWakeQ);
}

/****************************************************************************/
/**
*  @brief   Simple profiled memcpy test arm library copy.
*
*  @return
*/
/****************************************************************************/

static void memcpy_armlib_perf_test(size_t n, uint32_t iterations, int verbose,
				    char *banner)
{
	uint32_t iter;
	char modbanner[80];
	char *bufp;
	uint8_t *da;
	uint8_t *sa;
	size_t i;
//    int elapsed;
//    int tmrStart;
//    int tmrEnd;
//    uint32_t lsw, msw;
	timer_tick_count_t timer_tick_start, timer_tick_end;
	int32_t elapsed;

	if (!perfflag.quiet) {
		printk
		    ("%s: vmalloc, arm library memcpy, size=%u iterations=%u\n",
		     __func__, n, iterations);
	}
	bufp = vmalloc(2 * n);
	if (!bufp) {
		printk("memcpy test: failed to allocate memory\n");
		return;
	}
	sa = bufp + n;
	da = bufp;

	for (i = 0; i < n; i++) {
		sa[i] = i & 0xff;
	}

//    clean_invalidate_dcache();        // in old-linux source that we don't want to port
#if defined(CPU_CACHE_V7)
	v7_flush_dcache_all();	// cache-v7.S
#else
	clean_dcache_area(da, 2 * n);
//    L1_DCCISW;                          // dm_support_ops.S provides the same function
//    flush_cache_range(vma, start, end);
//    flush_cache_all();                  // This may stabilize system ?
#endif

//    tmrStart = chal_gptimer_get_timer_current_tick(timer_handle, &lsw, &msw);
	timer_tick_start = timer_get_tick_count();

	for (iter = 0; iter < iterations; iter++) {
		memcpy(da, sa, n);
	}
	if (perfflag.verify) {
		if (memcmp(da, sa, n)) {
			printk(KERN_ERR
			       ": ========== armlib memcpy test failed [%03d] ==========\n\n",
			       iter);
			return;
		}
	}
//    tmrEnd = chal_gptimer_get_timer_current_tick(timer_handle, &lsw, &msw);
//    elapsed  = tmrEnd - tmrStart;
	timer_tick_end = timer_get_tick_count();
	elapsed = (timer_tick_end - timer_tick_start) / timer_get_tick_rate();
	if (elapsed < 0) {
		elapsed = -elapsed;
	}
	if (!perfflag.quiet) {
		printk("%s: %u usec for %u iterations\n", __func__, elapsed,
		       iterations);
	}
	sprintf(modbanner, "%s size = 0x%05x, iterations = %d : ", banner, n,
		iterations);
	vfree(bufp);
}

#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
/****************************************************************************/
/**
*  @brief   Simple profiled memcpy test of a certain block size and a number of iterations.
*
*  @return
*/
/****************************************************************************/
static void memcpy_perf_test(size_t n, uint32_t iterations, int verbose,
			     char *banner)
{
	uint32_t iter;
	int i = 0;
	char modbanner[80];
	char *da;
	char *sa;
	PERFTMR_OBJ tmr;

	if (!perfflag.quiet) {
		printk("%s: n=%u iterations=%u\n", __func__, n, iterations);
	}
	da = vmalloc(n);
	sa = vmalloc(n);
	if (!da || !sa) {
		printk("memcpy test: failed to allocate memory\n");
		if (da) {
			vfree(da);
		}
		if (sa) {
			vfree(sa);
		}
		return;
	}
	if (!perfflag.quiet) {
		printk("da = 0x%08x, sa = 0x%08x\n", (unsigned int)da,
		       (unsigned int)sa);
	}

	perf_clear(&tmr);
	clean_invalidate_dcache();
	while (arm_perf_evt_group_tbl[i][0] != (ARM_PERF_EVT) ~ 0) {
		perf_init_start(&tmr, &arm_perf_evt_group_tbl[i][0]);
		for (iter = 0; iter < iterations; iter++) {
			mymemcpy(da, sa, n);
		}
		perf_stop_save(&tmr);
		if (verbose >= 2) {
			arm_perf_print_mon_cnts(printk, verbose);
		}
		i++;
	}
	sprintf(modbanner, "%s size = 0x%05x, iterations = %d : ", banner, n,
		iterations);
	arm_perf_print_summary(printk, modbanner, verbose);
	vfree(sa);
	vfree(da);
}

/****************************************************************************/
/**
*  @brief   Process echo 1 >memcpy3 - perform memcpy test
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_memcpy(ctl_table * table, int write,
					  void __user *buffer, size_t * lenp,
					  loff_t *ppos)
#else
static int proc_do_perftest_intvec_memcpy(ctl_table * table, int write,
					  struct file *filp,
					  void __user *buffer, size_t * lenp,
					  loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
	rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
	if (rc < 0) {
		return rc;
	}

	if (write) {
		perftest[perfflag.cpu_nr].memcpyIter = perfflag.memcpy;
		send_perfcmd(perfflag.cpu_nr, CMD_MEMCPY_TEST);
	}

	return rc;
}
#endif // ENABLE_ARM_PERF_MONITOR_TEST

/****************************************************************************/
/**
*  @brief   Process echo 1 >memcpy_armlib - perform memcpy test
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_memcpy_armlib(ctl_table * table, int write,
						 void __user *buffer,
						 size_t * lenp, loff_t *ppos)
#else
static int proc_do_perftest_intvec_memcpy_armlib(ctl_table * table, int write,
						 struct file *filp,
						 void __user *buffer,
						 size_t * lenp, loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
	rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
	if (rc < 0) {
		return rc;
	}

	if (write) {
		perftest[perfflag.cpu_nr].memcpyIter = perfflag.memcpy_armlib;
		send_perfcmd(perfflag.cpu_nr, CMD_MEMCPY_ARMLIB_TEST);
	}

	return rc;
}

/****************************************************************************/
/**
*  @brief   Process echo 1 >sdma_memcpy - perform sdma_memcpy test
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_sdma_memcpy(ctl_table * table, int write,
					       void __user *buffer,
					       size_t * lenp, loff_t *ppos)
#else
static int proc_do_perftest_intvec_sdma_memcpy(ctl_table * table, int write,
					       struct file *filp,
					       void __user *buffer,
					       size_t * lenp, loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
	rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
	if (rc < 0) {
		return rc;
	}

	if (write) {
		perftest[perfflag.cpu_nr].memcpyIter = perfflag.sdma_memcpy;
		send_perfcmd(perfflag.cpu_nr, CMD_SDMA_MEMCPY_TEST);
	}

	return rc;
}

#if (ENABLE_DMA_MEMCPY_PERF_TEST == 1)	// Alamy: porting from bcmhana
/****************************************************************************/
/**
*  @brief   Process echo 1 >dma_memcpy - perform dma_memcpy test
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_dma_memcpy(ctl_table * table, int write,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos)
#else
static int proc_do_perftest_intvec_dma_memcpy(ctl_table * table, int write,
					      struct file *filp,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
	rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
	if (rc < 0) {
		return rc;
	}

	if (write) {
		perftest[perfflag.cpu_nr].memcpyIter = perfflag.dma_memcpy;
		send_perfcmd(perfflag.cpu_nr, CMD_DMA_MEMCPY_TEST);
	}

	return rc;
}
#endif // ENABLE_DMA_MEMCPY_PERF_TEST

#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
/****************************************************************************/
/**
*  @brief   Process echo 1 >nop - run 64k nop test
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_nop(ctl_table * table, int write,
				       void __user *buffer, size_t * lenp,
				       loff_t *ppos)
#else
static int proc_do_perftest_intvec_nop(ctl_table * table, int write,
				       struct file *filp, void __user *buffer,
				       size_t * lenp, loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
	rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
	if (rc < 0) {
		return rc;
	}

	if (write) {
		perftest[perfflag.cpu_nr].nopIter = perfflag.nop;
		send_perfcmd(perfflag.cpu_nr, CMD_NOP_TEST);
	}

	return rc;
}
#endif // ENABLE_ARM_PERF_MONITOR_TEST

/****************************************************************************/
/**
*  @brief   Process echo 0x1000 >memcpysize - configure memcpy size
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_memcpysize(ctl_table * table, int write,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos)
#else
static int proc_do_perftest_intvec_memcpysize(ctl_table * table, int write,
					      struct file *filp,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
	rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
	if (rc < 0) {
		return rc;
	}
#if 0
	if (write) {
		if (perfflag.memcpysize > 0x10000) {
			printk
			    ("memcpy size too large (must be 0x10000 or less)\n");
			perfflag.memcpysize = 0x1000;
		}
	}
#endif

	return rc;
}

/****************************************************************************/
/**
*  @brief   Process echo 1 >enableint - re-enable interrupts (irq and fiq)
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_enableint(ctl_table * table, int write,
					     void __user *buffer, size_t * lenp,
					     loff_t *ppos)
#else
static int proc_do_perftest_intvec_enableint(ctl_table * table, int write,
					     struct file *filp,
					     void __user *buffer, size_t * lenp,
					     loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
	rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
	if (rc < 0) {
		return rc;
	}

	if (write) {
		send_perfcmd(perfflag.cpu_nr, CMD_ENABLE_IRQ);
	}

	return rc;
}

/****************************************************************************/
/**
*  @brief   Process echo 1 >disableint - disable interrupts (irq and fiq)
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_disableint(ctl_table * table, int write,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos)
#else
static int proc_do_perftest_intvec_disableint(ctl_table * table, int write,
					      struct file *filp,
					      void __user *buffer,
					      size_t * lenp, loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
	rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
	if (rc < 0) {
		return rc;
	}

	if (write) {
		send_perfcmd(perfflag.cpu_nr, CMD_DISABLE_IRQ);
	}

	return rc;
}

/****************************************************************************/
/**
*  @brief   low priority noploop to avoid CPU sleeping and stress i-cache
*
*  @return
*/
/****************************************************************************/
static int noploop(void *data)
{
	int cpu_enum = *(int *)data;
	char strg[20];
	int rc;
	struct sched_param param = {.sched_priority = 0 };
	unsigned int cpuid;

	if (cpu_enum > num_online_cpus()) {
		printk(KERN_ERR "perfcnt: bad cpu number %d\n", cpu_enum);
		return -1;
	}

	/*
	 * Set this thread to idle policy, and it's priority is hence is below all other
	 * default policy threads
	 */

	rc = sched_setscheduler(current, SCHED_IDLE, &param);
	if (rc) {
		printk(KERN_ERR "noploop sched_setscheduler failed, rc=%d\n",
		       rc);
		return rc;
	}

	sprintf(strg, "noploop/%d", cpu_enum);
	daemonize(strg);

	printk
	    ("noploop (PID=%d) starting on CPU%d, 'echo 0 > noploop' to stop...\n",
	     perftest[cpu_enum].nop_pid, cpu_enum);
	while (perftest[cpu_enum].nop_pid != -1) {
		nop64k();
	}

	cpuid = smp_processor_id();
	if (cpu_enum != cpuid) {
		printk(KERN_ERR
		       "noploop test running on wrong CPU. CPU%d instead of CPU%d\n",
		       cpuid, cpu_enum);
	} else {
		printk("noploop test exiting on CPU%d\n", cpuid);
	}
	complete_and_exit(&perftest[cpu_enum].noplooptestExited, 0);
}

/****************************************************************************/
/**
*  @brief   Run low priority noploop to avoid CPU sleeping and stress i-cache
*
*  @return
*/
/****************************************************************************/

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_noploop(ctl_table * table, int write,
					   void __user *buffer, size_t * lenp,
					   loff_t *ppos)
#else
static int proc_do_perftest_intvec_noploop(ctl_table * table, int write,
					   struct file *filp,
					   void __user *buffer, size_t * lenp,
					   loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
	rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
	if (rc < 0) {
		return rc;
	}

	if (write) {
		/*  perfflag.cpu_nr may be changed later
		 *  (although it's impossible that the code run so fast comparing to huham's typing).
		 *      perftest[perfflag.cpu_nr] is safe for perfflag.cpu_nr is guaranteed in the safe range.
		 *
		 *  perfflag.cpu_nr == perftest[perfflag.cpu_nr].cpu_enum
		 */
		int cpu_enum = perfflag.cpu_nr;

		if (perfflag.noploop > 0) {
			if (perftest[cpu_enum].nop_pid == -1) {
				cpumask_t cpu_mask;
				cpumask_set_cpu(cpu_enum, &cpu_mask);
				init_completion(&perftest[cpu_enum].
						noplooptestExited);
				perftest[cpu_enum].nop_pid =
				    kernel_thread(noploop, &cpu_enum, 0);
				sched_setaffinity(perftest[cpu_enum].nop_pid,
						  &cpu_mask);
				printk
				    ("PID %d created for noploop test on CPU%d\n",
				     perftest[cpu_enum].nop_pid, cpu_enum);
			} else {
				printk(KERN_ERR
				       "noploop test already running with PID %d on CPU%d\n",
				       perftest[cpu_enum].nop_pid, cpu_enum);
			}
		} else {
			/* inform thread to shut itself down */
			if (perftest[cpu_enum].nop_pid != -1) {
				printk("Stopping PID %d on CPU%d ...\n",
				       perftest[cpu_enum].nop_pid, cpu_enum);
				perftest[cpu_enum].nop_pid = -1;
			} else {
				printk(KERN_ERR
				       "no noploop test running on CPU%d\n",
				       cpu_enum);
			}
		}
	}
	return rc;
}

#if (ENABLE_SRAM_TEST == 1)
/****************************************************************************/
/**
*  @brief   low priority sramtest to stress sram
*
*  @return
*/
/****************************************************************************/
static unsigned int pattern = 0;
static int sramtest(void *data)
{
	int cpu_enum = *(int *)data;
	char strg[20];
	int rc;
	struct sched_param param = {.sched_priority = 0 };
	unsigned int cpuid;

	if (cpu_enum > num_online_cpus()) {
		printk(KERN_ERR "perfcnt: bad cpu number %d\n", cpu_enum);
		return -1;
	}

	/*
	 * Set this thread to idle policy, and it's priority is hence is below all other
	 * default policy threads
	 */

	rc = sched_setscheduler(current, SCHED_IDLE, &param);
	if (rc) {
		printk(KERN_ERR "sramtest sched_setscheduler failed, rc=%d\n",
		       rc);
		return rc;
	}

	sprintf(strg, "sramtest/%d", cpu_enum);
	daemonize(strg);

	printk
	    ("sramtest (%d) starting on CPU%d, 'echo 0 > sramtest' to stop...\n",
	     perftest[cpu_enum].sram_pid, cpu_enum);
	while (perftest[cpu_enum].sram_pid != -1) {
		/* Write incrementing pattern to SRAM, validate, then increment starting word. */
		unsigned int val = pattern;
		unsigned int *dst = (unsigned int *)KONA_SRAM_VA;
		unsigned int size = (SZ_128K + SZ_32K);	// See mach-island/include/aram_layout.h
		unsigned int i;
		for (i = 0; i < size; i += sizeof(*dst)) {
			*dst++ = val++;
		}
		val = pattern;
		dst = (unsigned int *)KONA_SRAM_VA;
		for (i = 0; i < size; i += sizeof(*dst), dst++, val++) {
			if (*dst != val) {
				printk(KERN_ERR
				       "ERROR: %s: dst=0x%p, *dst=0x%x, good=0x%x\n",
				       __FUNCTION__, dst, *dst, val);
			}
		}
		pattern++;
	}

	cpuid = smp_processor_id();
	if (cpu_enum != cpuid) {
		printk(KERN_ERR
		       "sramtest running on wrong CPU. CPU%d instead of CPU%d\n",
		       cpuid, cpu_enum);
	} else {
		printk("sramtest exiting on CPU%d\n", cpuid);
	}
	complete_and_exit(&perftest[cpu_enum].sramtestExited, 0);
}

/****************************************************************************/
/**
*  @brief   low priority sramtest to stress aram
*
*  @return
*/
/****************************************************************************/

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_sramtest(ctl_table * table, int write,
					    void __user *buffer, size_t * lenp,
					    loff_t *ppos)
#else
static int proc_do_perftest_intvec_sramtest(ctl_table * table, int write,
					    struct file *filp,
					    void __user *buffer, size_t * lenp,
					    loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
	rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
	rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
	if (rc < 0) {
		return rc;
	}

	if (write) {
		/*  perfflag.cpu_nr may be changed later
		 *  (although it's impossible that the code run so fast comparing to huham's typing).
		 *      perftest[perfflag.cpu_nr] is safe for perfflag.cpu_nr is guaranteed in the safe range.
		 *
		 *  perfflag.cpu_nr == perftest[perfflag.cpu_nr].cpu_enum
		 */
		int cpu_enum = perfflag.cpu_nr;

		if (perfflag.sramtest > 0) {
			if (perftest[cpu_enum].sram_pid == -1) {
				cpumask_t cpu_mask;
				cpumask_set_cpu(cpu_enum, &cpu_mask);
				init_completion(&perftest[cpu_enum].
						sramtestExited);
				perftest[cpu_enum].sram_pid =
				    kernel_thread(sramtest, &cpu_enum, 0);
				sched_setaffinity(perftest[cpu_enum].sram_pid,
						  &cpu_mask);
				printk
				    ("PID %d created for sram test on CPU%d\n",
				     perftest[cpu_enum].sram_pid, cpu_enum);
			} else {
				printk(KERN_ERR
				       "sramtest already running with PID %d on CPU%d\n",
				       perftest[cpu_enum].sram_pid, cpu_enum);
			}
		} else {
			/* inform thread to shut itself down */
			if (perftest[cpu_enum].sram_pid != -1) {
				printk("Stopping PID %d on CPU%d ...\n",
				       perftest[cpu_enum].sram_pid, cpu_enum);
				perftest[cpu_enum].sram_pid = -1;
			} else {
				printk(KERN_ERR
				       "no sramtest running on CPU%d\n",
				       cpu_enum);
			}
		}
	}

	return rc;
}
#endif // ENABLE_SRAM_TEST

/****************************************************************************/
/**
*  @brief   Process echo 1 >cpu - set CPU number
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_cpu(ctl_table * table, int write,
				       void __user *buffer, size_t * lenp,
				       loff_t *ppos)
#else
static int proc_do_perftest_intvec_cpu(ctl_table * table, int write,
				       struct file *filp, void __user *buffer,
				       size_t * lenp, loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

	if (write) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
		rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
		rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
		if (rc < 0) {
			return rc;
		}
		if (perfflag.cpu_nr >= num_online_cpus()) {
			printk(KERN_ERR "CPU number must be less than %d\n",
			       num_online_cpus());
			perfflag.cpu_nr = 0;
		}
	} else {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
		rc = proc_dointvec(table, write, buffer, lenp, ppos);	/* No special processing for read. */
#else
		rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);	/* No special processing for read. */
#endif
		return rc;
	}
	return rc;
}

/****************************************************************************/
/**
*  @brief   Process echo 1 >quiet - disable printk
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_quiet(ctl_table * table, int write,
					 void __user *buffer, size_t * lenp,
					 loff_t *ppos)
#else
static int proc_do_perftest_intvec_quiet(ctl_table * table, int write,
					 struct file *filp, void __user *buffer,
					 size_t * lenp, loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

	if (write) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
		rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
		rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
		if (rc < 0) {
			return rc;
		}
	} else {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
		rc = proc_dointvec(table, write, buffer, lenp, ppos);	/* No special processing for read. */
#else
		rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);	/* No special processing for read. */
#endif
		return rc;
	}

	return rc;
}

/****************************************************************************/
/**
*  @brief   Process echo 1 >quiet - disable printk
*
*  @return
*/
/****************************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
static int proc_do_perftest_intvec_verify(ctl_table * table, int write,
					  void __user *buffer, size_t * lenp,
					  loff_t *ppos)
#else
static int proc_do_perftest_intvec_verify(ctl_table * table, int write,
					  struct file *filp,
					  void __user *buffer, size_t * lenp,
					  loff_t *ppos)
#endif
{
	int rc = 0;

	if (!table || !table->data)
		return -EINVAL;

	if (write) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
		rc = proc_dointvec(table, write, buffer, lenp, ppos);
#else
		rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);
#endif
		if (rc < 0) {
			return rc;
		}
	} else {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)
		rc = proc_dointvec(table, write, buffer, lenp, ppos);	/* No special processing for read. */
#else
		rc = proc_dointvec(table, write, filp, buffer, lenp, ppos);	/* No special processing for read. */
#endif
		return rc;
	}

	return rc;
}

/****************************************************************************
*
*   do_perfcmd
*
*   function that does the actual processing of cmds.
*
***************************************************************************/
static void do_perfcmd(PERFCMD cmd)
{
	switch (cmd) {
	case CMD_ENABLE_IRQ:
		local_irq_enable();
		break;

	case CMD_DISABLE_IRQ:
		local_irq_disable();
		break;

#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
	case CMD_MEMCPY_TEST:
		memcpy_perf_test(perfflag.memcpysize,
				 perftest[perfflag.cpu_nr].memcpyIter,
				 perfflag.verbose, "mempcy test ddr ");
		break;
#endif // ENABLE_ARM_PERF_MONITOR_TEST

	case CMD_MEMCPY_ARMLIB_TEST:
		memcpy_armlib_perf_test(perfflag.memcpysize,
					perftest[perfflag.cpu_nr].memcpyIter,
					perfflag.verbose, "mempcy test ddr ");
		break;

	case CMD_SDMA_MEMCPY_TEST:
		sdma_memcpy_perf_test(perfflag.memcpysize,
				      perftest[perfflag.cpu_nr].memcpyIter);
		break;

#if (ENABLE_DMA_MEMCPY_PERF_TEST == 1)	// Alamy: disabled according to Ray (used only by Ethernet which calls into low-level interface directly, not necessary to have DMA interface on island platform)
	case CMD_DMA_MEMCPY_TEST:
		dma_memcpy_perf_test(perfflag.memcpysize,
				     perftest[perfflag.cpu_nr].memcpyIter);
		break;
#endif // ENABLE_DMA_MEMCPY_PERF_TEST

#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)	// Alamy: disabled according to Ray (already have this somewhere else)
	case CMD_EVENT:
		arm_perf_init_start(event);
		break;

	case CMD_NOP_TEST:
		nop_perf_test(perftest[perfflag.cpu_nr].nopIter,
			      perfflag.verbose, "64k nop test");
		break;
#endif // ENABLE_ARM_PERF_MONITOR_TEST

	case CMD_NOPLOOP:
		break;

#if (ENABLE_SRAM_TEST == 1)
	case CMD_SRAM_TEST:
		break;
#endif

	default:
		printk(KERN_ERR "perftest: bad command\n");
	}

}

/****************************************************************************
*
*  perfcmd_thread
*
*   Worker thread to send command to specified CPU.
*
***************************************************************************/
static int perfcmd_thread(void *data)
{
	int *cpu = (int *)data;
	char strg[20];

	if (*cpu > num_online_cpus()) {
		printk(KERN_ERR "bad cpu number\n");
		return -1;
	}

	sprintf(strg, "perftestcmd/%d", *cpu);
	daemonize(strg);

	while (1) {
		if (0 ==
		    wait_event_interruptible(perftest[*cpu].perfcmdWakeQ,
					     perftest[*cpu].perfcmd.
					     perfcmdWake)) {
			perftest[*cpu].perfcmd.perfcmdWake = 0;
			if (!perfflag.quiet) {
				printk("CPU %u :\n", *cpu);
			}
			do_perfcmd(perftest[*cpu].perfcmd.cmd);
		}
	}
	complete_and_exit(&perftest[*cpu].perfcmdExited, 0);
}

/****************************************************************************/
/**
*  @brief   Initialize by setting up the sysctl and proc/perfcnt entries, allocating
*           default storage if any, and setting variables to defaults.
*
*  @return
*/
/****************************************************************************/
static int __init perftest_init(void)
{
	cpumask_t cpu_mask;
	int i;

	/* register sysctl table */
	printk("%s\n", __FUNCTION__);

	gSysCtlHeader = register_sysctl_table(gSysCtl);
	if (gSysCtlHeader == NULL) {
		printk("%s: could not register sysctl table\n", __FUNCTION__);
	}

	memset(&perftest, 0, sizeof(PERFTEST_OBJ));	/* zero entries, idx, wrap, and enable */
	perfflag.memcpysize = 0x1000;
	perfflag.quiet = 0;
	perfflag.verify = 0;

#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
	event[0] = ARM_PERF_I_EXEC;
	event[1] = ARM_PERF_I_FETCH_CACHE_REFILL;
	event[2] = ARM_PERF_D_RW_CACHE_REFILL;
	event[3] = ARM_PERF_ICACHE_STALL;
	event[4] = ARM_PERF_DCACHE_STALL;
	event[5] = ARM_PERF_D_RW_CACHE_ACCESS;
#endif // ENABLE_ARM_PERF_MONITOR_TEST

	/* We create separate thread for each CPU.  This is the best way to allow
	 * each CPU to report its own stat
	 */
	for (i = 0; i < num_online_cpus(); i++) {
		perftest[i].nop_pid = -1;
#if (ENABLE_SRAM_TEST == 1)
		perftest[i].sram_pid = -1;
#endif
		init_waitqueue_head(&perftest[i].perfcmdWakeQ);
		/* First create dumping to file or socket thread */
		init_completion(&perftest[i].perfcmdExited);
		/* then set affinity of each thread to force it to run on separate CPU */
		cpumask_set_cpu(i, &cpu_mask);
		perftest[i].cpu_enum = i;
		perftest[i].perfcmdThreadPid =
		    kernel_thread(perfcmd_thread, &perftest[i].cpu_enum, 0);
		sched_setaffinity(perftest[i].perfcmdThreadPid, &cpu_mask);
		cpumask_clear_cpu(i, &cpu_mask);
#if (ENABLE_ARM_PERF_MONITOR_TEST == 1)
		send_perfcmd(i, CMD_EVENT);
#endif // ENABLE_ARM_PERF_MONITOR_TEST
	}

	/* since we are using the kernel global timer for profiling, it should already be enabled */
//   timer_handle = chal_gptimer_get_handle(MM_IO_BASE_SYSTMR);
//   timer_handle = chal_gptimer_get_handle(KONA_SYSTMR_VA);      // Alamy: porting from bcmhana
	return 0;
}

/****************************************************************************/
/**
*  @brief      Exit and cleanup (probably not done)
*
*  @return
*/
/****************************************************************************/
static void __exit perftest_exit(void)
{
	if (gSysCtlHeader != NULL) {
		unregister_sysctl_table(gSysCtlHeader);
	}
}

#ifdef MODULE
module_init(perftest_init);
module_exit(perftest_exit);

MODULE_LICENSE("GPL");

#else
subsys_initcall(perftest_init);

#endif
