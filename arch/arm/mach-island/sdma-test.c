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

#include <mach/sdma.h>
#include <mach/io_map.h>

/* ---- Public Variables ------------------------------------------------- */

/* ---- Private Constants and Types -------------------------------------- */

#define START_COUNT           0xA000

#define TEST_DMA_REQUEST_FREE 0

typedef struct
{
    void       *virtPtr;
    dma_addr_t  physPtr;
    size_t      numBytes;

} dma_mem_t;

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

/* ---- Private Variables ------------------------------------------------ */
struct semaphore    gDmaDoneSem[SDMA_NUM_CHANNELS];

volatile int gDmaTestRunning[2];

/* ---- Private Function Prototypes -------------------------------------- */

/* ---- Functions -------------------------------------------------------- */

/****************************************************************************
*
*   Allocates a block of memory
*
***************************************************************************/

static void *alloc_mem( dma_mem_t *mem, size_t numBytes )
{
    mem->numBytes = numBytes;
    mem->virtPtr = dma_alloc_writecombine( NULL, numBytes, &mem->physPtr, GFP_KERNEL );

    if ( mem->virtPtr == NULL )
    {
        printk( KERN_ERR "dma_alloc_writecombine of %d bytes failed\n", numBytes );
    }

    printk( "dma_alloc_writecombine of %d bytes returned virtPtr: 0x%08lx physPstr: %08x\n",
            numBytes, (unsigned long)mem->virtPtr, mem->physPtr );

    return mem->virtPtr;
}


static void *free_mem( dma_mem_t *mem )
{

   if ( mem->virtPtr != NULL )
   {
      dma_free_writecombine( NULL, mem->numBytes, mem->virtPtr, mem->physPtr );
   }
   mem->virtPtr = NULL;

   return NULL;
}

/****************************************************************************
*
*   Handler called when the DMA finishes.
*
***************************************************************************/

static void dmaHandler( DMA_Device_t dev, int reason, void *userData )
{
    const int chan = (int)userData;

    printk( "dmaHandler called: dev: %d, reason = 0x%x\n", dev, reason );

    up( &gDmaDoneSem[chan] );
}

/****************************************************************************
*
*   Does a memcpy using DMA
*
***************************************************************************/
static int sdma_test_set( int chan )
{
  int rc;
  sema_init( &gDmaDoneSem[chan], 0 );

  BUG_ON( chan >= ( sizeof( device ) / sizeof( device[0] )));

  if (( rc = sdma_set_device_handler( device[chan], dmaHandler, (void *)chan )) != 0 )
  {
     printk( KERN_ERR "sdma_set_device_handler(%d) failed: %d\n", (int)device[chan], rc );
     return rc;
  }

  return 0;
}



static int sdma_test_wait( int chan )
{
    int rc;

    printk( "Waiting for DMA to complete\n" );
    if (( rc = down_interruptible( &gDmaDoneSem[chan] )) != 0 )
    {
        printk( KERN_ERR "down_interruptible failed: %d\n", rc );
        return rc;
    }
    printk( "DMA completed on channel %d\n", chan );

    return 0;
}

/****************************************************************************
*
*   Initializes a block of memory with a known pattern
*
***************************************************************************/

static void dma_test_init_mem( dma_mem_t *mem, uint16_t start_count )
{
    int         i;
    uint16_t   *memInt;
    uint16_t    counter = start_count;
    int         numWords;

    numWords = mem->numBytes / sizeof( *memInt );
    memInt = mem->virtPtr;

    for ( i = 0; i < numWords; i++ )
    {
        *memInt++ = counter++;
    }
}

/****************************************************************************
*
*   Checks a block of memory to see if it has the expected value
*
***************************************************************************/

static int dma_test_check_mem( dma_mem_t *mem, uint16_t start_count )
{
    int         i;
    uint16_t   *memInt;
    uint16_t    counter = start_count;
    int         numWords;

    numWords = mem->numBytes / sizeof( *memInt );
    memInt = mem->virtPtr;

    for ( i = 0; i < numWords; i++ )
    {
        if ( *memInt != counter )
        {
            printk( "Destination: base=%p, i=%d, found 0x%4.4x, expecting 0x%4.4x\n",
                    mem->virtPtr, i, *memInt, counter );
            return -EIO;
        }
        memInt++;
        counter++;
    }

    return 0;
}


static uint16_t channel_to_start( int channel )
{
    const uint16_t chan = (uint16_t)channel;

    return (chan << 12) & (chan << 8) & (chan << 4) & (chan << 0);
}


#define ALLOC_SIZE  1024

static int memcpy_tests( long iterations )
{
   int           rc;
   int           i;
   dma_mem_t     src[SDMA_NUM_CHANNELS];
   dma_mem_t     dst[SDMA_NUM_CHANNELS];
   SDMA_Handle_t dmaHandle[SDMA_NUM_CHANNELS];
   int           chan;

   int run_one_test( int test_number )
   {
      for ( chan = 0; chan < SDMA_NUM_CHANNELS; chan++ )
      {
         /* Start DMA operation */
         sdma_transfer_mem_to_mem(dmaHandle[chan], src[chan].physPtr, dst[chan].physPtr, ALLOC_SIZE );
      }

      for ( chan = 0; chan < SDMA_NUM_CHANNELS; chan++ )
      {
         /* Wait for DMA completion */
         sdma_test_wait( chan );

         /* Verify transfer */
         if (( rc = dma_test_check_mem( &dst[chan], channel_to_start( chan ))) != 0 )
         {
            printk( KERN_ERR ": ========== DMA memcpy test%d channel %d failed [%03d] ==========\n\n", test_number, chan, i );
            return rc;
         }

         printk( "\n========== DMA memcpy test%d channel %d passed [%03d] ==========\n\n", test_number, chan, i );

         /* Reset destination memory */
         memset( dst[chan].virtPtr, 0, dst[chan].numBytes );
      }

      return 0;
   }

   printk( "\n::Entering into test thread::\n\n");

   for ( chan = 0; chan < SDMA_NUM_CHANNELS; chan++ )
   {
      /* Allocate contiguous source memory */
      if ( alloc_mem( &src[chan], ALLOC_SIZE ) == NULL )
      {
         return -ENOMEM;
      }

      /* Allocate contiguous destination memory */
      if ( alloc_mem( &dst[chan], ALLOC_SIZE ) == NULL )
      {
         return -ENOMEM;
      }

      /* Init test settings */
      sdma_test_set( chan );

      /* initialize the source memory */
      dma_test_init_mem( &src[chan], channel_to_start( chan ));
   }

   for ( i = 0; i < iterations; i++ )
   {
      for ( chan = 0; chan < SDMA_NUM_CHANNELS; chan++ )
      {
         /* Aquire a DMA channel */
         dmaHandle[chan] = sdma_request_channel( device[chan] );
         if ( dmaHandle[chan] < 0 )
         {
            return (int)dmaHandle[chan];
         }
      }

      if (( rc = run_one_test( 1 )) != 0 )
      {
         return rc;
      }

      /*
       * Now test to see if we can do back-to-back DMA transfers
       * and reuse the same DMA descriptor
       */
      if (( rc = run_one_test( 2 )) != 0 )
      {
         return rc;
      }

      for ( chan = 0; chan < SDMA_NUM_CHANNELS; chan++ )
      {
         sdma_free_channel( dmaHandle[chan] );
      }
   }

   for ( chan = 0; chan < SDMA_NUM_CHANNELS; chan++ )
   {
      /* Free test memory */
      free_mem( &src[chan] );
      free_mem( &dst[chan] );
   }

   printk( "\n ::Exiting from test thread:: \n\n");

   return 0;
}

#if TEST_DMA_REQUEST_FREE
volatile SDMA_Handle_t gTestHandle[ DMA_NUM_DEVICE_ENTRIES ];

int dma_test_thread( void *data )
{
   unsigned long test = (unsigned long)data;
   int devIdx;
   int rc;

   printk( "dma_test_thread( %lu ) called\n", test );

   for ( devIdx = 0; devIdx < DMA_NUM_DEVICE_ENTRIES; devIdx++ )
   {
      gTestHandle[devIdx] = SDMA_INVALID_HANDLE;
   }

   for ( devIdx = 0; devIdx < DMA_NUM_DEVICE_ENTRIES; devIdx++ )
   {
      if ( devIdx == DMA_DEVICE_NAND_MEM_TO_MEM )
      {
         printk( "Skipping NAND device %d\n", devIdx );
         continue;
      }

      if ( test == 1 )
      {
         printk( "About to request channel for device %d ...\n", devIdx );
         if (( gTestHandle[ devIdx ] = sdma_request_channel( devIdx )) < 0 )
         {
            printk( "Call to sdma_request_channel failed: %d\n", gTestHandle[ devIdx ] );
            continue;
         }
         printk( "   request completed, handle = 0x%04x\n", gTestHandle[ devIdx ] );
      }
      else
      {
         if ( gTestHandle[ devIdx ] != SDMA_INVALID_HANDLE )
         {
            printk( "About to relase channel for device %d, handle 0x%04x\n", devIdx, gTestHandle[ devIdx ] );
            if (( rc = sdma_free_channel( gTestHandle[ devIdx ])) < 0 )
            {
               printk( "Call to sdma_free_channel failed: %d\n", rc );
            }
            msleep( 33 );
         }
      }
   }

   printk( "test thread %ld exiting\n", test );
   gDmaTestRunning[ test - 1 ] = 0;

   return 0;
}
#endif

static void sdma_test_run( long iterations )
{
   printk( "\n========== Starting DMA memory copy tests ==========\n\n" );
   memcpy_tests( iterations );

#if TEST_DMA_REQUEST_FREE
   printk( "\n========== Starting request/free test ==========\n\n" );

   gDmaTestRunning[0] = 1;
   gDmaTestRunning[1] = 1;
   printk( "Starting request thread\n" );
   kthread_run( dma_test_thread, (void *)1, "dmaRequest" );
   msleep( 100 );
   printk( "Starting release thread\n" );
   kthread_run( dma_test_thread, (void *)2, "dmaRelease" );

   while ( gDmaTestRunning[0] || gDmaTestRunning[1] )
   {
      printk( "Waiting for tests to complete... %d %d\n", gDmaTestRunning[0], gDmaTestRunning[1] );
      msleep( 1000 );
   }
   printk( "\n========== Done request/free test ==========\n\n" );
#endif
}

static ssize_t sdma_write_proc_test( struct file* file,
                                     const char* __user buf,
                                     size_t count,
                                     loff_t *pos)
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

   sdma_test_run( value );

   return count;
}

static int sdma_register_test_file( void )
{
   struct proc_dir_entry * gDmaDir;
   int ret = 0;

   gDmaDir = create_proc_entry( "sdma/test", S_IWUGO, NULL );

   if ( gDmaDir == NULL )
   {
       printk( KERN_ERR "Unable to create /proc/sdma/test\n" );
   }
   else
   {
       gDmaDir->write_proc = sdma_write_proc_test;
   }

   return 0;
}

static void sdma_unregister_test_file( void )
{
   remove_proc_entry( "sdma/test", NULL );
}

/****************************************************************************
*
*   Called to perform module initialization when the module is loaded
*
***************************************************************************/
static int __init sdma_test_init( void )
{
   return sdma_register_test_file();
}

/****************************************************************************
*
*   Called to perform module cleanup when the module is unloaded.
*
***************************************************************************/

static void __exit sdma_test_exit( void )
{
   sdma_unregister_test_file();
}

/****************************************************************************/

module_init( sdma_test_init );
module_exit( sdma_test_exit );

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION( "Broadcom System DMA Test" );
MODULE_LICENSE( "GPL" );
MODULE_VERSION( "1.0" );

