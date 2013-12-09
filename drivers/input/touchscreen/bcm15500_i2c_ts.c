/*****************************************************************************
* Copyright (c) 2011 Broadcom Corporation.  All rights reserved.
*
* This program is the proprietary software of Broadcom Corporation and/or
* its licensors, and may only be used, duplicated, modified or distributed
* pursuant to the terms and conditions of a separate, written license
* agreement executed between you and Broadcom (an "Authorized License").
* Except as set forth in an Authorized License, Broadcom grants no license
* (express or implied), right to use, or waiver of any kind with respect to
* the Software, and Broadcom expressly reserves all rights in and to the
* Software and all intellectual property rights therein.  IF YOU HAVE NO
* AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
* WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
* THE SOFTWARE.
*
* Except as expressly set forth in the Authorized License,
* 1. This program, including its structure, sequence and organization,
*    constitutes the valuable trade secrets of Broadcom, and you shall use
*    all reasonable efforts to protect the confidentiality thereof, and to
*    use this information only in connection with your use of Broadcom
*    integrated circuit products.
* 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
*    IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS
*    FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS,
*    QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU
*    ASSUME THE ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
* 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR ITS
*    LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT,
*    OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
*    YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN
*    ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS
*    OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1, WHICHEVER
*    IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
*    ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*****************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/freezer.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <asm/system.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/kfifo.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <linux/input/mt.h>
#include <linux/time.h>

#include <linux/i2c/bcm15500_i2c_ts.h>

/* -------------------------------------- */
/* - BCM Touch Controller Driver Macros - */
/* -------------------------------------- */

/* -- SPM addresses -- */
#define BCMTCH_SPM_REG_REVISIONID   0x40
#define BCMTCH_SPM_REG_CHIPID0      0x41
#define BCMTCH_SPM_REG_CHIPID1      0x42
#define BCMTCH_SPM_REG_CHIPID2      0x43

#define BCMTCH_SPM_REG_SPI_I2C_SEL  0x44
#define BCMTCH_SPM_REG_I2CS_CHIPID  0x45

#define BCMTCH_SPM_REG_PSR          0x48

#define BCMTCH_SPM_REG_RQST_FROM_HOST   0x4c
#define BCMTCH_SPM_REG_MSG_TO_HOST      0x4d
#define BCMTCH_SPM_REG_MSG_FROM_HOST    0x4e

#define BCMTCH_SPM_REG_SOFT_RESETS  0x59
#define BCMTCH_SPM_REG_FLL_STATUS   0x5c

#define BCMTCH_SPM_REG_ALFO_CTRL    0x60
#define BCMTCH_SPM_REG_LPLFO_CTRL   0x61

#define BCMTCH_SPM_REG_DMA_ADDR     0x80
#define BCMTCH_SPM_REG_DMA_STATUS   0x89
#define BCMTCH_SPM_REG_DMA_WFIFO    0x92
#define BCMTCH_SPM_REG_DMA_RFIFO    0xa2

/* -- SYS addresses -- */
#define BCMTCH_SYS_ADDR_BASE                    0x30000000
#define BCMTCH_SYS_ADDR_SPM_PWR_CTRL	\
		(BCMTCH_SYS_ADDR_BASE + 0x00100000 + 0x1c)
#define BCMTCH_SYS_ADDR_SPM_STICKY_BITS	\
		(BCMTCH_SYS_ADDR_BASE + 0x00100000 + 0x144)
#define BCMTCH_SYS_ADDR_COMMON_ARM_REMAP	\
		(BCMTCH_SYS_ADDR_BASE + 0x00110000 + 0x00)
#define BCMTCH_SYS_ADDR_COMMON_SYS_HCLK_CTRL	\
		(BCMTCH_SYS_ADDR_BASE + 0x00110000 + 0x20)
#define BCMTCH_SYS_ADDR_COMMON_CLOCK_ENABLE	\
		(BCMTCH_SYS_ADDR_BASE + 0x00110000 + 0x48)
#define BCMTCH_SYS_ADDR_COMMON_FLL_CTRL0	\
		(BCMTCH_SYS_ADDR_BASE + 0x00110000 + 0x104)
#define BCMTCH_SYS_ADDR_COMMON_FLL_LPF_CTRL2	\
		(BCMTCH_SYS_ADDR_BASE + 0x00110000 + 0x114)
#define BCMTCH_SYS_ADDR_COMMON_FLL_TEST_CTRL1	\
		(BCMTCH_SYS_ADDR_BASE + 0x00110000 + 0x144)
#define BCMTCH_SYS_ADDR_TCH_VER	\
		(BCMTCH_SYS_ADDR_BASE + 0x00300000 + 0x00)

/* -- SYS MEM addresses -- */
#define BCMTCH_ADDR_VECTORS     0x00000000
#define BCMTCH_ADDR_CODE        0x10000000
#define BCMTCH_ADDR_DATA        0x10009000

/* -- constants -- */
#define BCMTCH_SUCCESS      0

#define BCMTCH_MAX_TOUCH    10
#define BCMTCH_MAX_X        4096
#define BCMTCH_MAX_Y        4096

#define BCMTCH_DMA_MODE_READ    1
#define BCMTCH_DMA_MODE_WRITE   3

#define BCMTCH_IF_I2C_SEL       0
#define BCMTCH_IF_SPI_SEL       1

#define BCMTCH_IF_I2C_COMMON_CLOCK  0x387B
#define BCMTCH_IF_SPI_COMMON_CLOCK  0x387F

#define BCMTCH_COMMON_CLOCK_USE_FLL (0x1 << 18)

#define BCMTCH_POWER_STATE_SLEEP        0
#define BCMTCH_POWER_STATE_RETENTION    1
#define BCMTCH_POWER_STATE_IDLE         3
#define BCMTCH_POWER_STATE_ACTIVE       4

#define BCMTCH_POWER_MODE_SLEEP     0x01
#define BCMTCH_POWER_MODE_WAKE      0x02
#define BCMTCH_POWER_MODE_NOWAKE    0x00

#define BCMTCH_RESET_MODE_SOFT_CLEAR    0x00
#define BCMTCH_RESET_MODE_SOFT_CHIP     0x01
#define BCMTCH_RESET_MODE_SOFT_ARM      0x02
#define BCMTCH_RESET_MODE_HARD          0x04

#define BCMTCH_MEM_ROM_BOOT 0x00
#define BCMTCH_MEM_RAM_BOOT 0x01
#define BCMTCH_MEM_MAP_1000 0x00
#define BCMTCH_MEM_MAP_3000 0x02

#define BCMTCH_SPM_STICKY_BITS_PIN_RESET    0x02

/* operations */
#define BCMTCH_USE_FAST_I2C     1
#define BCMTCH_USE_BUS_LOCK     0
#define BCMTCH_USE_WORK_LOCK    1

/* development and test */
#define PROGRESS() (printk(KERN_INFO "%s : %d\n", __func__, __LINE__))

/* -------------------------------------- */
/* - Touch Firmware Environment (ToFE)  - */
/* -------------------------------------- */
#define TOFE_BUILD_ID_SIZE          8
#define TOFE_SIGNATURE_MAGIC_SIZE   4

#define TOFE_MESSAGE_FW_READY   128

enum tofe_command_e {
	TOFE_COMMAND_NO_COMMAND = 0,
	TOFE_COMMAND_INTERRUPT_ACK,
	TOFE_COMMAND_SCAN_START,
	TOFE_COMMAND_SCAN_STOP,
	TOFE_COMMAND_SCAN_SET_RATE,
	TOFE_COMMAND_SET_MODE,
	TOFE_COMMAND_CALIBRATE,
	TOFE_COMMAND_AFEREGREAD,
	TOFE_COMMAND_AFEREGWRITE,
	TOFE_COMMAND_RUN_SEM,

	TOFE_COMMAND_LAST,
	TOFE_COMMAND_MAX = 0xff
};
#define tofe_command_e enum tofe_command_e

/**
    @struct tofe_signature_t
    @brief Firmware ROM image signature structure.
*/
struct  tofe_signature_t {
	const char magic[TOFE_SIGNATURE_MAGIC_SIZE];
	const char build_release[4];
	const char build_version[TOFE_BUILD_ID_SIZE];
	const char build_date[TOFE_BUILD_ID_SIZE];
	const char build_time[TOFE_BUILD_ID_SIZE];
};
#define tofe_signature_t struct  tofe_signature_t

/* ToFE Signature */
#define TOFE_SIGNATURE_SIZE sizeof(tofe_signature_t)

#define iterator_t uint16_t

/**
    @enum tofe_channel_flag_t
    @brief Channel flag field bit assignment.
*/
enum tofe_channel_flag_t {
	TOFE_CHANNEL_FLAG_STATUS_OVERFLOW = 1 << 0,
	TOFE_CHANNEL_FLAG_STATUS_LEVEL_TRIGGER = 1 << 1,
	TOFE_CHANNEL_FLAG_OVERFLOW_STALL = 1 << 6,	/* Stall on overflow */
	TOFE_CHANNEL_FLAG_INBOUND = 1 << 7,	/* max */
};
#define tofe_channel_flag_t enum tofe_channel_flag_t

enum tofe_toc_index {
	TOFE_TOC_INDEX_CHANNEL = 2
};
#define tofe_toc_index_e enum tofe_toc_index

enum tofe_channel_id_t {
	TOFE_CHANNEL_ID_TOUCH = 0,
	TOFE_CHANNEL_ID_COMMAND = 1,
	TOFE_CHANNEL_ID_LOG = 2,
	TOFE_CHANNEL_ID_RESPONSE = 6
};
#define tofe_channel_id_t enum tofe_channel_id_t	/* Used as index. */

struct tofe_channel_header_t {
	uint32_t write;
	uint8_t entry_num; /* Number of entries.  Limited to 255 entries. */
	uint8_t entry_size; /* Entry size in bytes.  Limited to 255 bytes. */
	/* Number of entries in channel to trigger notification */
	uint8_t trig_level;
	uint8_t flags;	/* Bit definitions shared with configuration. */
	uint32_t read;
	int32_t data_offset; /* Offset from header to data.  May be negative. */
	iterator_t read_iterator;
	iterator_t write_iterator;
};
#define tofe_channel_header_t struct tofe_channel_header_t

struct tofe_channel_instance_cfg_t {
	uint8_t entry_num;	/* Must be > 0. */
	uint8_t entry_size;	/* Range [1..255]. */
	uint8_t trig_level;	/* 0 - entry_num */
	uint8_t flags;
	tofe_channel_header_t *channel_header;
	void *channel_data;
};
#define tofe_channel_instance_cfg_t struct tofe_channel_instance_cfg_t

/* ------------------------------------- */
/* - BCM Touch Controller Driver Enums - */
/* ------------------------------------- */

enum bcmtch_channel_e {
	/* NOTE : see above tofe_channel_id_t : changed locally */
	BCMTCH_CHANNEL_TOUCH,
	BCMTCH_CHANNEL_COMMAND,
	BCMTCH_CHANNEL_RESPONSE,
	BCMTCH_CHANNEL_LOG,

	/* last */
	BCMTCH_CHANNEL_MAX
};
#define bcmtch_channel_e enum bcmtch_channel_e

enum bcmtch_mutex_e {
	BCMTCH_MUTEX_BUS,
	BCMTCH_MUTEX_WORK,

	/* last */
	BCMTCH_MUTEX_MAX,
};
#define bcmtch_mutex_e enum bcmtch_mutex_e

/* event types from BCM Touch Controller */
enum bcmtch_event_type_e {
	BCMTCH_EVENT_TYPE_INVALID,	/* Don't use zero. */

	/* Core events. */
	BCMTCH_EVENT_TYPE_FRAME,

	BCMTCH_EVENT_TYPE_DOWN,
	BCMTCH_EVENT_TYPE_MOVE,
	BCMTCH_EVENT_TYPE_UP,

	/* Auxillary events. */
	BCMTCH_EVENT_TYPE_TIMESTAMP,
};
#define bcmtch_event_type_e enum bcmtch_event_type_e

enum _bcmtch_touch_status {
	BCMTCH_TOUCH_STATUS_INACTIVE,
	BCMTCH_TOUCH_STATUS_UP,
	BCMTCH_TOUCH_STATUS_DOWN,
	BCMTCH_TOUCH_STATUS_MOVE,
};
#define bcmtch_touch_status_e enum _bcmtch_touch_status

/* -------------------------------------- */
/* - BCM Touch Controller Device Tables - */
/* -------------------------------------- */

static const uint32_t const BCMTCH_CHIP_IDS[] = {
	0x15300,
	0x15500,

	0			/* last entry must be 0 */
};

/* ------------------------------------------ */
/* - BCM Touch Controller Driver Parameters - */
/* ------------------------------------------ */
#define BCMTCH_BOOT_FLAG_RESET_ON_LOAD  0x00000001
#define BCMTCH_BOOT_FLAG_RAM_BOOT       0x00000002

static int bcmtch_boot_flag = (BCMTCH_BOOT_FLAG_RAM_BOOT);

module_param_named(boot_flag, bcmtch_boot_flag, int, S_IRUGO);
MODULE_PARM_DESC(boot_flag, "Boot bit-fields [RAM|RESET]");

/*-*/

#define BCMTCH_CHANNEL_FLAG_USE_TOUCH       0x00000001
#define BCMTCH_CHANNEL_FLAG_USE_CMD_RESP    0x00000002
#define BCMTCH_CHANNEL_FLAG_USE_LOG         0x00000004

static int bcmtch_channel_flag = BCMTCH_CHANNEL_FLAG_USE_TOUCH;

module_param_named(channel_flag, bcmtch_channel_flag, int, S_IRUGO);
MODULE_PARM_DESC(channel_flag, "Channels allowed bit-fields [L|C/R|T]");

/*-*/

#define BCMTCH_DEBUG_FLAG_FRAME             0x00000001
#define BCMTCH_DEBUG_FLAG_DOWN              0x00000002
#define BCMTCH_DEBUG_FLAG_MOVE              0x00000004
#define BCMTCH_DEBUG_FLAG_UP                0x00000008

static int bcmtch_debug_flag = BCMTCH_DEBUG_FLAG_FRAME;

module_param_named(debug_flag, bcmtch_debug_flag, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_flag, "Debug bit-fields [UP|MV|DN|FR]");

/* ------------------------------------------ */
/* - BCM Touch Controller Driver Structures - */
/* ------------------------------------------ */

struct bcmtch_channel_t {
	tofe_channel_instance_cfg_t cfg;
	uint16_t queued;
	uint16_t pad;		/* intentional may use for i2c tranactions */
	tofe_channel_header_t hdr;
	uint32_t data;
};
#define bcmtch_channel_t struct bcmtch_channel_t

/* event structures from BCM Touch Controller */
struct bcmtch_event_t {
	uint32_t type:4;
	uint32_t:28;

	uint32_t _pad;
};
#define bcmtch_event_t struct bcmtch_event_t

struct  bcmtch_event_frame_t {
	uint16_t type:4;
	uint16_t:12;
	uint16_t frame_id;

	uint32_t hash;
};
#define bcmtch_event_frame_t struct  bcmtch_event_frame_t

struct bcmtch_event_touch_t {
	uint16_t type:4;
	uint16_t track_tag:5;
	uint16_t flags:4;
	uint16_t tch_class:3;	/*  Touch class. C++ does not like class.
				   Use BCMTCH_EVENT_CLASS_TOUCH. */
	uint16_t width:8;	/* x direction length of bounding box */
	uint16_t height:8;	/* y direction length of bounding box */

	uint32_t z:8;	/* force/pressure for contact and distance for hover */
	uint32_t x:12;
	uint32_t y:12;
};
#define bcmtch_event_touch_t struct bcmtch_event_touch_t

/* driver structure for a single touch point */
struct _bcmtch_touch {
	uint16_t x;		/* X Coordinate */
	uint16_t y;		/* Y Coordinate */
	/* Touch status: Down, Move, Up (Inactive) */
	bcmtch_touch_status_e status;
	bcmtch_event_type_e event;	/* Touch Event Type */
};
#define bcmtch_touch_t struct _bcmtch_touch

struct _bcmtch_data {
	/* core 0S elements */
	/* Work queue structure for defining work queue handler */
	struct work_struct work;
	/* Work queue structure for transaction handling */
	struct workqueue_struct *p_workqueue;

	/* Critical Section : Mutexes : */
	struct mutex cs_mutex[BCMTCH_MUTEX_MAX];
	/*  (1) serial bus - I2C / SPI */
	/*  (2) deferred work */

	/* Pointer to allocated memory for input device */
	struct input_dev *p_inputDevice;

	/* I2C 0S elements */
	/* SPM I2C Client structure pointer */
	struct i2c_client *p_i2c_client_spm;
	/* SYS I2C Clinet structure pointer */
	struct i2c_client *p_i2c_client_sys;

	/* Local copy of platform data structure */
	bcmtch_platform_data_t platform_data;

	/* BCM Touch elements */
	bcmtch_channel_t *p_channels[BCMTCH_CHANNEL_MAX];
	bcmtch_touch_t touch[BCMTCH_MAX_TOUCH];	/* BCMTCH touch structure */
};
#define bcmtch_data_t struct _bcmtch_data

/* Pointer to BCMTCH Data Structure */
static bcmtch_data_t *bcmtch_data_p;

/* -------------------------------------------- */
/* - BCM Touch Controller Function Prototypes - */
/* -------------------------------------------- */

/*  DEV Prototypes */
#if 0 /* not been used currently */
static int32_t bcmtch_dev_get_soft_reset(void);
#endif
static int32_t bcmtch_dev_reset(uint8_t);

/*  COM Prototypes */
static int32_t bcmtch_com_init(void);
static int32_t bcmtch_com_read_spm(uint8_t, uint8_t *);
static int32_t bcmtch_com_write_spm(uint8_t, uint8_t);
static int32_t bcmtch_com_read_sys(uint32_t, uint16_t, uint8_t *);
static int32_t bcmtch_com_write_sys(uint32_t, uint16_t, uint8_t *);
/* COM Helper */
static inline int32_t bcmtch_com_fast_write_spm(uint8_t, uint8_t *, uint8_t *);
static inline int32_t bcmtch_com_write_sys32(uint32_t, uint32_t);

/*  OS Prototypes */
static void *bcmtch_os_mem_alloc(uint32_t);
static void bcmtch_os_mem_free(void *);
static void bcmtch_os_reset(void);
static void bcmtch_os_lock_critical_section(uint8_t);
static void bcmtch_os_release_critical_section(uint8_t);

/*  OS I2C Prototypes */
static int32_t bcmtch_os_i2c_probe(struct i2c_client *,
				   const struct i2c_device_id *);
static int32_t bcmtch_os_i2c_remove(struct i2c_client *);
static int32_t bcmtch_os_i2c_read_spm(struct i2c_client *, uint8_t, uint8_t *);
static int32_t bcmtch_os_i2c_write_spm(struct i2c_client *, uint8_t, uint8_t);
static int32_t bcmtch_os_i2c_fast_write_spm(struct i2c_client *, uint8_t,
					    uint8_t *, uint8_t *);
static int32_t bcmtch_os_i2c_read_sys(struct i2c_client *, uint32_t, uint16_t,
				      uint8_t *);
static int32_t bcmtch_os_i2c_write_sys(struct i2c_client *, uint32_t, uint16_t,
				       uint8_t *);
static int32_t bcmtch_os_i2c_init_clients(struct i2c_client *);
static void bcmtch_os_i2c_free_clients(void);
static void bcmtch_os_sleep_ms(uint32_t);

/* ------------------------------------------- */
/* - BCM Touch Controller CLI Implementation - */
/* ------------------------------------------- */

static ssize_t bcmtch_os_cli(struct device *dev,
			     struct device_attribute *devattr, const char *buf,
			     size_t count)
{
	uint32_t in_addr;
	uint32_t addr;
	uint32_t in_value_count;
	uint32_t rBuf[8];
	uint8_t r8;

#if !BCMTCH_USE_BUS_LOCK
#if BCMTCH_USE_WORK_LOCK
	bcmtch_os_lock_critical_section(BCMTCH_MUTEX_WORK);
#else
#error "To use CLI, either BUS_LOCK or WORK_LOCK must be enabled"
#endif
#endif

	if (sscanf(buf, "poke sys %x %x", &in_addr, &in_value_count)) {
		bcmtch_com_write_sys32(in_addr, in_value_count);
		printk(KERN_INFO "BCMTCH: poke sys addr=0x%08x data=0x%08x\n",
			in_addr, in_value_count);
	} else if (sscanf(buf, "peek sys %x %x", &in_addr, &in_value_count)) {
		addr = in_addr;
		memset(rBuf, 0, 8 * sizeof(uint32_t));

		printk(KERN_INFO "BCMTCH: peek sys addr=0x%08x len=0x%08x\n",
			in_addr, in_value_count);

		while (in_value_count) {
			bcmtch_com_read_sys(addr, 8 * sizeof(uint32_t),
					    (uint8_t *)rBuf);

			printk(KERN_INFO "BCMTCH:0x%08x: 0x%08x 0x%08x 0x%08x "
				"0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
				addr, rBuf[0], rBuf[1], rBuf[2], rBuf[3],
				rBuf[4], rBuf[5], rBuf[6], rBuf[7]);

			in_value_count =
			    (in_value_count >
			     (8 * sizeof(uint32_t))) ? (in_value_count -
							(8 *
							 sizeof(uint32_t))) : 0;
			addr += (8 * sizeof(uint32_t));
		}
	} else if (sscanf(buf, "poke spm %x %x", &in_addr, &in_value_count)) {
		in_addr &= 0xff;
		in_value_count &= 0xff;
		bcmtch_com_write_spm(in_addr, in_value_count);
		printk(KERN_INFO "BCMTCH: poke spm Reg=%08x data=%08x\n",
			in_addr, in_value_count);
	} else if (sscanf(buf, "peek spm %x", &in_addr)) {
		in_addr &= 0xff;
		bcmtch_com_read_spm(in_addr, &r8);
		printk(KERN_INFO "BCMTCH: peek spm reg=0x%02x data=0x%02x\n",
			in_addr, r8);
	} else if (sscanf(buf, "debug_flag %x", &in_value_count)) {
		bcmtch_debug_flag = in_value_count;
		printk(KERN_INFO "BCMTCH: bcmtch_debug_flag=0x%08x\n",
			bcmtch_debug_flag);
	}
#if !BCMTCH_USE_BUS_LOCK
#if BCMTCH_USE_WORK_LOCK
	bcmtch_os_release_critical_section(BCMTCH_MUTEX_WORK);
#endif
#endif

	return count;
}

static struct device_attribute bcmtch_cli_attr =
__ATTR(cli, 0664, NULL, bcmtch_os_cli);

/* ------------------------------------------- */
/* - BCM Touch Controller Internal Functions - */
/* ------------------------------------------- */

unsigned bcmtch_channel_num_queued(tofe_channel_header_t *channel)
{
	if (channel->write >= channel->read)
		return channel->write - channel->read;
	else
		return channel->entry_num - (channel->read - channel->write);
}

/*
    Note: Internal use only function.
*/
static inline iterator_t
_bcmtch_inline_channel_next_index(tofe_channel_header_t *channel,
				  iterator_t iterator)
{
	return (iterator == channel->entry_num - 1) ? 0 : iterator + 1;
}

/*
    Note: Internal use only function.
*/
static inline char *_bcmtch_inline_channel_entry(tofe_channel_header_t *channel,
						 uint32_t byte_index)
{
	char *data_bytes = (char *)channel + channel->data_offset;
	return &data_bytes[byte_index];
}

/*
    Note: Internal use only function.
*/
static inline size_t
_bcmtch_inline_channel_byte_index(tofe_channel_header_t *channel,
				  iterator_t entry_index)
{
	return entry_index * channel->entry_size;
}

/**
    Check if a channel is empty.

    Events are not considered read or writen until the transaction is
    complete.  Therefore, a channel is empty even when in the middle of a
    set of writes.

    @param
	[in] channel Pointer to channel object.

    @retval
	bool True if channel is empty.

*/
static inline bool
bcmtch_inline_channel_is_empty(tofe_channel_header_t *channel)
{
	return (channel->read == channel->write);
}

/**
    Begin a read transaction on a channel.  To maintain data consistency,
    reads to a channel must be bracketed by read begin/end calls.

    @param
	[in] channel Pointer to channel object.

    @retval
	void

*/
static inline void
bcmtch_inline_channel_read_begin(tofe_channel_header_t *channel)
{
	channel->read_iterator = channel->read;
}

/**
    Read a single entry from a channel.  This function must be called during
    a read transaction.

    The pointer returned by this function points into the channel object itself.
    Callers should not modify or reuse this memory.  Callers may not free the
    memory.

    @param
	[in] channel Pointer to channel object.

    @retval
	void * Pointer to returned entry.

*/
static inline void *bcmtch_inline_channel_read(tofe_channel_header_t *channel)
{
	char *entry;
	size_t byte_index;

	/* Validate that channel has entries. */
	if (bcmtch_inline_channel_is_empty(channel))
		return NULL;

	/* Find entry in the channel. */
	byte_index =
	    _bcmtch_inline_channel_byte_index(channel, channel->read_iterator);
	entry = (char *)_bcmtch_inline_channel_entry(channel, byte_index);

	/* Update the read iterator. */
	channel->read_iterator =
	    _bcmtch_inline_channel_next_index(channel, channel->read_iterator);

	return (void *)entry;
}

/**
    Finish a read transaction on a channel.  To maintain data consistency,
    reads to a channel must be bracketed by read begin/end calls.

    @param
	[in] channel Pointer to channel object.

    @retval
	uint32_t Number of entries read from channel during this transaction.

*/
static inline uint32_t
bcmtch_inline_channel_read_end(tofe_channel_header_t *channel)
{
	uint32_t count = (channel->read_iterator >= channel->read) ?
	    (channel->read_iterator - channel->read) :
	    (channel->entry_num - (channel->read - channel->read_iterator));

	channel->read = channel->read_iterator;
	return count;
}

/* ------------------------------------------- */
/* - BCM Touch Controller DEV Functions - */
/* ------------------------------------------- */

static int32_t bcmtch_dev_alloc(void)
{
	int32_t retVal = BCMTCH_SUCCESS;

	bcmtch_data_p = bcmtch_os_mem_alloc(sizeof(bcmtch_data_t));
	if (bcmtch_data_p == NULL) {
		printk(KERN_ERR "%s: failed to alloc mem.\n", __func__);
		retVal = -ENOMEM;
	}
	return retVal;
}

static void bcmtch_dev_free(void)
{
	bcmtch_os_mem_free(bcmtch_data_p);
}

static int32_t bcmtch_dev_init_clocks(void)
{
	int32_t retVal = -EAGAIN;
	uint8_t locked;
	uint8_t waitFLL = 5;
	/* setup LPLFO */
	bcmtch_com_write_spm(BCMTCH_SPM_REG_LPLFO_CTRL, 0);

	/* setup FLL */
	bcmtch_com_write_sys32(BCMTCH_SYS_ADDR_COMMON_FLL_CTRL0, 0xe0000002);
	bcmtch_com_write_sys32(BCMTCH_SYS_ADDR_COMMON_FLL_LPF_CTRL2,
			       0x01001007);
	bcmtch_com_write_sys32(BCMTCH_SYS_ADDR_COMMON_FLL_CTRL0, 0x00000001);
	bcmtch_os_sleep_ms(1);
	bcmtch_com_write_sys32(BCMTCH_SYS_ADDR_COMMON_FLL_CTRL0, 0x00000002);

	/* Set the clock dividers for SYS bus speeds */
	bcmtch_com_write_sys32(BCMTCH_SYS_ADDR_COMMON_SYS_HCLK_CTRL, 0xF01);

	/* Enable clocks */
	bcmtch_com_write_sys32(BCMTCH_SYS_ADDR_COMMON_CLOCK_ENABLE,
			       BCMTCH_IF_I2C_COMMON_CLOCK);

	/* wait for FLL to lock */
	do {
		bcmtch_com_read_spm(BCMTCH_SPM_REG_FLL_STATUS, &locked);
		if (locked) {
			/* switch to FLL */
			bcmtch_com_write_sys32
			    (BCMTCH_SYS_ADDR_COMMON_CLOCK_ENABLE,
			     (BCMTCH_IF_I2C_COMMON_CLOCK |
			      BCMTCH_COMMON_CLOCK_USE_FLL));

			retVal = BCMTCH_SUCCESS;
			break;
		}
	} while (waitFLL--);

	return retVal;
}

static int32_t bcmtch_dev_init_memory(void)
{
	int32_t retVal = BCMTCH_SUCCESS;
	uint32_t memMap;

	memMap = (bcmtch_boot_flag & BCMTCH_BOOT_FLAG_RAM_BOOT) ?
	    BCMTCH_MEM_RAM_BOOT : BCMTCH_MEM_ROM_BOOT;

	retVal =
	    bcmtch_com_write_sys32(BCMTCH_SYS_ADDR_COMMON_ARM_REMAP, memMap);

	return retVal;
}

static int32_t bcmtch_dev_init_channel(bcmtch_channel_e chan_id,
				       tofe_channel_instance_cfg_t *p_chan_cfg)
{
	int32_t retVal = BCMTCH_SUCCESS;
	uint32_t channel_size;

	/* channel data size   */
	channel_size = (p_chan_cfg->entry_num * p_chan_cfg->entry_size)
	    +sizeof(tofe_channel_header_t)	/* channel header size */
	    +sizeof(tofe_channel_instance_cfg_t) /* channel config size */
	    +(sizeof(uint16_t) * 2); /* sizes for added elements: queued, pad */

	bcmtch_data_p->p_channels[chan_id] =
	    (bcmtch_channel_t *) bcmtch_os_mem_alloc(channel_size);

	if (bcmtch_data_p->p_channels[chan_id]) {
		bcmtch_data_p->p_channels[chan_id]->cfg = *p_chan_cfg;

		printk(KERN_ERR
		       "BCMTCH: channel [%d] h=0x%08x d=0x%08x n=%d s=%d\n",
		       chan_id,
		       (uint32_t)bcmtch_data_p->p_channels[chan_id]->cfg.
		       channel_header,
		       (uint32_t)bcmtch_data_p->p_channels[chan_id]->cfg.
		       channel_data,
		       bcmtch_data_p->p_channels[chan_id]->cfg.entry_num,
		       bcmtch_data_p->p_channels[chan_id]->cfg.entry_size);
	} else {
		retVal = -ENOMEM;
	}
	return retVal;
}

static void bcmtch_dev_free_channels(void)
{
	uint32_t chan = 0;
	while (chan < BCMTCH_CHANNEL_MAX)
		bcmtch_os_mem_free(bcmtch_data_p->p_channels[chan++]);
}

static int32_t bcmtch_dev_init_channels(uint32_t mem_addr, uint8_t *mem_data)
{
	int32_t retVal = BCMTCH_SUCCESS;

	uint32_t *p_cfg = NULL;
	tofe_channel_instance_cfg_t *p_chan_cfg = NULL;

	/* find channel configs */
	p_cfg = (uint32_t *)(mem_data + TOFE_SIGNATURE_SIZE);
	p_chan_cfg = (tofe_channel_instance_cfg_t *)
	    ((uint32_t)mem_data + p_cfg[TOFE_TOC_INDEX_CHANNEL] - mem_addr);

	/* check if processing channel(s) - add */
	if (bcmtch_channel_flag & BCMTCH_CHANNEL_FLAG_USE_TOUCH) {
		retVal =
		    bcmtch_dev_init_channel(BCMTCH_CHANNEL_TOUCH,
					    &p_chan_cfg[TOFE_CHANNEL_ID_TOUCH]);
	}

	if (retVal || !(bcmtch_channel_flag & BCMTCH_CHANNEL_FLAG_USE_TOUCH)) {
		printk(KERN_ERR
		       "%s: [%d] Touch Event Channel not initialized!\n",
		       __func__, retVal);
	}

	if (!retVal &&
		(bcmtch_channel_flag & BCMTCH_CHANNEL_FLAG_USE_CMD_RESP)) {
		retVal =
		    bcmtch_dev_init_channel(BCMTCH_CHANNEL_COMMAND,
					    &p_chan_cfg
					    [TOFE_CHANNEL_ID_COMMAND]);
		retVal |=
		    bcmtch_dev_init_channel(BCMTCH_CHANNEL_RESPONSE,
					    &p_chan_cfg
					    [TOFE_CHANNEL_ID_RESPONSE]);
	}

	if (!retVal && (bcmtch_channel_flag & BCMTCH_CHANNEL_FLAG_USE_LOG)) {
		retVal =
		    bcmtch_dev_init_channel(BCMTCH_CHANNEL_LOG,
					    &p_chan_cfg[TOFE_CHANNEL_ID_LOG]);
	}

	return retVal;
}

static int32_t bcmtch_dev_read_channel(bcmtch_channel_t *chan)
{
	int32_t retVal = BCMTCH_SUCCESS;
	int16_t readSize;
	uint32_t wbAddr;

	/* read channel header and data all-at-once : need combined size */
	readSize =
	    sizeof(chan->hdr) + (chan->cfg.entry_num * chan->cfg.entry_size);

	/* read channel header & channel data buffer */
	retVal =
	    bcmtch_com_read_sys((uint32_t)chan->cfg.channel_header, readSize,
				(uint8_t *)&chan->hdr);

	/* get count */
	chan->queued = bcmtch_channel_num_queued(&chan->hdr);

	/* write back to update channel */
	if (chan->queued) {
		wbAddr =
		    (uint32_t)chan->cfg.channel_header +
		    offsetof(tofe_channel_header_t, read);
		retVal = bcmtch_com_write_sys32(wbAddr, chan->hdr.write);
	}

	return retVal;
}

static int32_t bcmtch_dev_read_channels(void)
{
	int32_t retVal = BCMTCH_SUCCESS;
	uint32_t channel = 0;
	uint32_t channels_read = 0;

	while (channel < BCMTCH_CHANNEL_MAX) {
		if (bcmtch_data_p->p_channels[channel]
		    && !(bcmtch_data_p->p_channels[channel]->cfg.
			 flags & TOFE_CHANNEL_FLAG_INBOUND)) {
			retVal =
			    bcmtch_dev_read_channel(bcmtch_data_p->
						    p_channels[channel]);
			channels_read++;
		}
		channel++;
	}

	return retVal;
}

static int32_t bcmtch_dev_process_event_frame(bcmtch_event_frame_t *
					      p_frame_event)
{
	int32_t retVal = BCMTCH_SUCCESS;

	struct input_dev *pInputDevice = bcmtch_data_p->p_inputDevice;
	bcmtch_touch_t *pTouch;
	uint32_t numTouches = 0;
	uint32_t touchIndex = 0;

	for (touchIndex = 0; touchIndex < BCMTCH_MAX_TOUCH; touchIndex++) {
		pTouch = (bcmtch_touch_t *) &bcmtch_data_p->touch[touchIndex];

		if (pTouch->status > BCMTCH_TOUCH_STATUS_UP) {
			numTouches++;
			input_report_abs(pInputDevice, ABS_MT_POSITION_X,
					 pTouch->x);
			input_report_abs(pInputDevice, ABS_MT_POSITION_Y,
					 pTouch->y);
			input_mt_sync(pInputDevice);
		}
	}

	input_report_key(pInputDevice, BTN_TOUCH, (numTouches > 0));
	input_sync(pInputDevice);

	if (bcmtch_debug_flag & BCMTCH_DEBUG_FLAG_FRAME)
		printk(KERN_INFO "BCMTCH: FR: T=%d ID=%d\n",
			numTouches, p_frame_event->frame_id);

	return retVal;
}

static int32_t bcmtch_dev_process_event_touch(bcmtch_event_touch_t *
					      p_touch_event)
{
	int32_t retVal = BCMTCH_SUCCESS;
	bcmtch_touch_t *p_touch;

	if (p_touch_event->track_tag < BCMTCH_MAX_TOUCH) {
		p_touch =
		    (bcmtch_touch_t *) &bcmtch_data_p->touch[p_touch_event->
							      track_tag];

#if BCMTCH_HW_AXIS_SWAP_X
		p_touch_event->x = BCMTCH_MAX_X - p_touch_event->x;
#endif
#if BCMTCH_HW_AXIS_SWAP_Y
		p_touch_event->y = BCMTCH_MAX_Y - p_touch_event->y;
#endif

#if BCMTCH_HW_AXIS_SWAP_X_Y
		p_touch->x = p_touch_event->y;
		p_touch->y = p_touch_event->x;
#else
		p_touch->x = p_touch_event->x;
		p_touch->y = p_touch_event->y;
#endif

		switch (p_touch_event->type) {
		case BCMTCH_EVENT_TYPE_DOWN:
			p_touch->event = p_touch_event->type;
			p_touch->status = BCMTCH_TOUCH_STATUS_DOWN;

			if (bcmtch_debug_flag & BCMTCH_DEBUG_FLAG_DOWN)
				printk(KERN_INFO
					"BCMTCH: DN: T%d: (%4d , %4d)\n",
					p_touch_event->track_tag,
					p_touch->x, p_touch->y);
			break;

		case BCMTCH_EVENT_TYPE_UP:
			p_touch->event = p_touch_event->type;
			p_touch->status = BCMTCH_TOUCH_STATUS_UP;

			if (bcmtch_debug_flag & BCMTCH_DEBUG_FLAG_UP)
				printk(KERN_INFO
					"BCMTCH: UP: T%d: (%4d , %4d)\n",
					p_touch_event->track_tag,
					p_touch->x, p_touch->y);
			break;

		case BCMTCH_EVENT_TYPE_MOVE:
			p_touch->event = p_touch_event->type;
			p_touch->status = BCMTCH_TOUCH_STATUS_MOVE;

			if (bcmtch_debug_flag & BCMTCH_DEBUG_FLAG_MOVE)
				printk(KERN_INFO
					"BCMTCH: MV: T%d: (%4d , %4d)\n",
					p_touch_event->track_tag,
					p_touch->x, p_touch->y);
			break;
		}
	} else {

	}
	return retVal;
}

static int32_t bcmtch_dev_process_channel_touch(bcmtch_channel_t *chan)
{
	int32_t retVal = BCMTCH_SUCCESS;

	bcmtch_event_t *ptch_event;
	tofe_channel_header_t *chan_hdr = (tofe_channel_header_t *)&chan->hdr;

	bcmtch_inline_channel_read_begin(chan_hdr);

	while ((ptch_event =
		(bcmtch_event_t *) bcmtch_inline_channel_read(chan_hdr))) {
		switch (ptch_event->type) {
		case BCMTCH_EVENT_TYPE_DOWN:
		case BCMTCH_EVENT_TYPE_UP:
		case BCMTCH_EVENT_TYPE_MOVE:
			bcmtch_dev_process_event_touch((bcmtch_event_touch_t *)
						       ptch_event);
			break;

		case BCMTCH_EVENT_TYPE_FRAME:
			bcmtch_dev_process_event_frame((bcmtch_event_frame_t *)
						       ptch_event);
			break;

		case BCMTCH_EVENT_TYPE_TIMESTAMP:
			break;

		default:
			break;
		}

		/* Finished processing event, so update read pointer. */
		bcmtch_inline_channel_read_end(chan_hdr);
	}

	return retVal;
}

static int32_t bcmtch_dev_process_channels(void)
{
	int32_t retVal = BCMTCH_SUCCESS;
	uint32_t channel = 0;

	while (channel < BCMTCH_CHANNEL_MAX) {
		switch (channel) {
		case BCMTCH_CHANNEL_TOUCH:
			if (bcmtch_data_p->p_channels[BCMTCH_CHANNEL_TOUCH]) {
				bcmtch_dev_process_channel_touch(
						bcmtch_data_p->
						p_channels
						[BCMTCH_CHANNEL_TOUCH]);
			}
			break;

		case BCMTCH_CHANNEL_COMMAND:
		case BCMTCH_CHANNEL_RESPONSE:
		case BCMTCH_CHANNEL_LOG:
		default:
			break;

		}

		channel++;
	}

	return retVal;
}

#define BCMTCH_FIRMWARE_FLAGS_CONFIGS   0x01
#define BCMTCH_FIRMWARE_FLAGS_COMBI     0x10

#define BCMTCH_COMBI_TEST 1

struct _combi_entry {
	uint32_t offset;
	uint32_t addr;
	uint32_t length;
	uint32_t flags;
};
#define bcmtch_combi_entry_t struct _combi_entry

struct _firmware_load_info {
	uint8_t *filename;
	uint32_t addr;
	uint32_t flags;
};
#define bcmtch_firmware_load_info_t struct _firmware_load_info

static const bcmtch_firmware_load_info_t bcmtch_binaries[] = {
#if BCMTCH_COMBI_TEST
	{"bcmtchfw_bin", 0, BCMTCH_FIRMWARE_FLAGS_COMBI},
#else
	{"bcmtchfw_vect", BCMTCH_ADDR_VECTORS, 0},
	{"bcmtchfw_code", BCMTCH_ADDR_CODE, 0},
	{"bcmtchfw_data", BCMTCH_ADDR_DATA, BCMTCH_FIRMWARE_FLAGS_CONFIGS},
#endif
	{0, 0, 0}
};

static int32_t bcmtch_dev_wait_for_firmware_ready(int32_t count)
{
	int32_t retVal = BCMTCH_SUCCESS;
	uint8_t ready;

	do {
		retVal =
		    bcmtch_com_read_spm(BCMTCH_SPM_REG_MSG_TO_HOST, &ready);
	} while ((!retVal) && (ready != TOFE_MESSAGE_FW_READY) && (count--));

	if (count <= 0) {
		printk(KERN_ERR
		       "ERROR: Failed to communicate with Napa FW. Error: 0x%x\n",
		       ready);
		retVal = -1;
	}

	return retVal;
}

static int32_t bcmtch_dev_run_firmware(void)
{
	int32_t retVal = BCMTCH_SUCCESS;

	retVal = bcmtch_dev_reset(BCMTCH_RESET_MODE_SOFT_CLEAR);
	retVal |=
	    bcmtch_com_write_sys32(BCMTCH_SYS_ADDR_SPM_STICKY_BITS,
				   BCMTCH_SPM_STICKY_BITS_PIN_RESET);

	if (bcmtch_dev_wait_for_firmware_ready(1000)) {
		uint8_t xaddr = 0x40;
		uint8_t xdata;
		while (xaddr <= 0x61) {
			bcmtch_com_read_spm(xaddr, &xdata);
			printk(KERN_ERR "%s: addr = 0x%02x  data = 0x%02x\n",
			       __func__, xaddr++, xdata);
		}
	}

	return retVal;
}

static int32_t bcmtch_dev_download_firmware(uint8_t *fw_name, uint32_t fw_addr,
					    uint32_t fw_flags)
{
	const struct firmware *p_fw;
	int32_t retVal = BCMTCH_SUCCESS;

	uint32_t entryId = 0;
	bcmtch_combi_entry_t *p_entry = NULL;
	bcmtch_combi_entry_t default_entry[] = {
		{.addr = fw_addr, .flags = fw_flags,},
		{0, 0, 0, 0},
	};

	/* request firmware binary from OS */
	retVal =
	    request_firmware(&p_fw, fw_name,
			     &bcmtch_data_p->p_i2c_client_spm->dev);
	if (retVal) {
		printk(KERN_ERR "%s: Firmware request failed (%d) for %s\n",
		       __func__, retVal, fw_name);
	} else {
		printk(KERN_INFO
		       "BCMTCH: file=%s addr=0x%08x flags=0x%08x size=%d\n",
		       fw_name, fw_addr, fw_flags, p_fw->size);

		/* pre-process binary according to flags */
		if (fw_flags & BCMTCH_FIRMWARE_FLAGS_COMBI) {
			p_entry = (bcmtch_combi_entry_t *) p_fw->data;
		} else {
			p_entry = default_entry;
			p_entry[entryId].length = p_fw->size;
		}

		while (p_entry[entryId].length) {
			if (p_entry[entryId].
			    flags & BCMTCH_FIRMWARE_FLAGS_CONFIGS) {
				bcmtch_dev_init_channels(p_entry[entryId].addr,
							 (uint8_t *)((uint32_t)
								     p_fw->
								     data +
								     p_entry
								     [entryId].
								     offset));
			}

			printk(KERN_INFO
			       "BCMTCH: addr=0x%08x flags=0x%08x size=%d\n",
			       p_entry[entryId].addr, p_entry[entryId].flags,
			       p_entry[entryId].length);

	    /** download to chip **/
			retVal = bcmtch_com_write_sys(p_entry[entryId].addr,
						      p_entry[entryId].length,
						      (uint8_t *)((uint32_t)
								  p_fw->data +
								  p_entry
								  [entryId].
								  offset));

			/* next */
			entryId++;
		}
	}

	/* free kernel structures */
	release_firmware(p_fw);
	return retVal;
}

static int32_t bcmtch_dev_init_firmware(void)
{
	int32_t retVal = BCMTCH_SUCCESS;
	uint8_t binFile = 0;

	while (bcmtch_binaries[binFile].filename) {
		retVal =
		    bcmtch_dev_download_firmware(bcmtch_binaries[binFile].
						 filename,
						 bcmtch_binaries[binFile].addr,
						 bcmtch_binaries[binFile].
						 flags);
		binFile++;
	}

	if (!retVal)
		retVal = bcmtch_dev_run_firmware();

	return retVal;
}

static int32_t bcmtch_dev_init_platform(struct device *p_device)
{
	int32_t retVal = BCMTCH_SUCCESS;
	struct bcmtch_platform_data *p_platform_data;

	p_platform_data =
	    (struct bcmtch_platform_data *)p_device->platform_data;

	if (p_platform_data && bcmtch_data_p) {
		bcmtch_data_p->platform_data.i2c_bus_id =
		    p_platform_data->i2c_bus_id;
		bcmtch_data_p->platform_data.i2c_addr_sys =
		    p_platform_data->i2c_addr_sys;

		bcmtch_data_p->platform_data.gpio_reset_pin =
		    p_platform_data->gpio_reset_pin;
		bcmtch_data_p->platform_data.gpio_reset_polarity =
		    p_platform_data->gpio_reset_polarity;
		bcmtch_data_p->platform_data.gpio_reset_time_ms =
		    p_platform_data->gpio_reset_time_ms;

		bcmtch_data_p->platform_data.gpio_interrupt_pin =
		    p_platform_data->gpio_interrupt_pin;
		bcmtch_data_p->platform_data.gpio_interrupt_trigger =
		    p_platform_data->gpio_interrupt_trigger;
	} else {
		printk(KERN_ERR "%s() error, platform data == NULL\n",
		       __func__);
		retVal = -ENODATA;
	}

	return retVal;
}

static int32_t bcmtch_dev_request_power_mode(uint8_t mode,
					     tofe_command_e command)
{
	int32_t retVal = BCMTCH_SUCCESS;

#if BCMTCH_USE_FAST_I2C
	uint8_t regs[5];
	uint8_t data[5];
#endif

	switch (mode) {
	case BCMTCH_POWER_MODE_SLEEP:
		retVal =
		    bcmtch_com_write_spm(BCMTCH_SPM_REG_MSG_FROM_HOST, command);
		retVal |=
		    bcmtch_com_write_spm(BCMTCH_SPM_REG_RQST_FROM_HOST,
					 BCMTCH_POWER_MODE_SLEEP);
		break;

	case BCMTCH_POWER_MODE_WAKE:
		retVal =
		    bcmtch_com_write_spm(BCMTCH_SPM_REG_MSG_FROM_HOST, command);
		retVal |=
		    bcmtch_com_write_spm(BCMTCH_SPM_REG_RQST_FROM_HOST,
					 BCMTCH_POWER_MODE_WAKE);
		break;

	case BCMTCH_POWER_MODE_NOWAKE:
#if BCMTCH_USE_FAST_I2C
		regs[0] = BCMTCH_SPM_REG_MSG_FROM_HOST;
		data[0] = command;
		regs[1] = BCMTCH_SPM_REG_RQST_FROM_HOST;
		data[1] = BCMTCH_POWER_MODE_NOWAKE;
		retVal = bcmtch_com_fast_write_spm(2, regs, data);
#else
		retVal =
		    bcmtch_com_write_spm(BCMTCH_SPM_REG_MSG_FROM_HOST, command);
		retVal |=
		    bcmtch_com_write_spm(BCMTCH_SPM_REG_RQST_FROM_HOST,
					 BCMTCH_POWER_MODE_NOWAKE);
#endif
		break;

	default:
		PROGRESS();
		break;
	}

	return retVal;
}

static int32_t bcmtch_dev_get_power_state(void)
{
	int32_t retVal = BCMTCH_SUCCESS;
	uint8_t power_state;

	retVal = bcmtch_com_read_spm(BCMTCH_SPM_REG_PSR, &power_state);

	return (retVal) ? (retVal) : ((uint32_t)power_state);
}

static int32_t bcmtch_dev_set_power_state(uint8_t power_state)
{
	int32_t retVal = BCMTCH_SUCCESS;
	int32_t state = power_state;

	switch (power_state) {
	case BCMTCH_POWER_STATE_SLEEP:
		retVal =
		    bcmtch_dev_request_power_mode(BCMTCH_POWER_MODE_SLEEP,
						  TOFE_COMMAND_NO_COMMAND);
		retVal |=
		    bcmtch_com_write_sys(BCMTCH_SYS_ADDR_SPM_PWR_CTRL,
					 sizeof(int32_t), (uint8_t *)&state);
		break;

	case BCMTCH_POWER_STATE_RETENTION:
		PROGRESS();
		break;

	case BCMTCH_POWER_STATE_IDLE:
		PROGRESS();
		break;

	case BCMTCH_POWER_STATE_ACTIVE:
		PROGRESS();
		break;

	default:
		PROGRESS();
		break;
	}

	return retVal;
}

static int32_t bcmtch_dev_check_power_state(uint8_t power_state,
					    uint8_t wait_count)
{
	int32_t retVal = -EAGAIN;
	int32_t read_state;

	do {
		read_state = bcmtch_dev_get_power_state();
		if (read_state == power_state) {
			retVal = BCMTCH_SUCCESS;
			break;
		}
		bcmtch_os_sleep_ms(1);
	} while (wait_count--);

	return retVal;
}

#if 0 /* not been used currently */
static int32_t bcmtch_dev_get_soft_reset(void)
{
	int32_t retVal = BCMTCH_SUCCESS;
	uint8_t soft_reset;

	retVal = bcmtch_com_read_spm(BCMTCH_SPM_REG_SOFT_RESETS, &soft_reset);

	return (retVal) ? (retVal) : ((uint32_t)soft_reset);

	return retVal;
}
#endif

static int32_t bcmtch_dev_reset(uint8_t mode)
{
	int32_t retVal = BCMTCH_SUCCESS;

	switch (mode) {
	case BCMTCH_RESET_MODE_HARD:
		bcmtch_os_reset();
		break;

	case BCMTCH_RESET_MODE_SOFT_CHIP:
		break;

	case BCMTCH_RESET_MODE_SOFT_ARM:
		break;

	case (BCMTCH_RESET_MODE_SOFT_CHIP | BCMTCH_RESET_MODE_SOFT_ARM):
		break;

	case BCMTCH_RESET_MODE_SOFT_CLEAR:
		retVal =
		    bcmtch_com_write_spm(BCMTCH_SPM_REG_SOFT_RESETS,
					 BCMTCH_RESET_MODE_SOFT_CLEAR);
		break;

	default:
		break;
	}

	return retVal;
}

static int32_t bcmtch_dev_verify_chip_id(void)
{
	int32_t retVal = -ENXIO;
	uint32_t chipID;
	uint8_t revID;
	uint8_t id[3];
	uint32_t *pChips = (uint32_t *)BCMTCH_CHIP_IDS;

	retVal = bcmtch_com_read_spm(BCMTCH_SPM_REG_REVISIONID, &revID);
	retVal |= bcmtch_com_read_spm(BCMTCH_SPM_REG_CHIPID0, &id[0]);
	retVal |= bcmtch_com_read_spm(BCMTCH_SPM_REG_CHIPID1, &id[1]);
	retVal |= bcmtch_com_read_spm(BCMTCH_SPM_REG_CHIPID2, &id[2]);

	chipID = ((id[2] << 16) | (id[1] << 8) | id[0]);

	while (*pChips) {
		if (*pChips++ == chipID) {
			retVal = BCMTCH_SUCCESS;
			break;
		}
	}

	printk(KERN_INFO "BCMTCH: ChipId = 0x%06X  Rev = 0x%2X : %s\n",
	       chipID,
	       revID, ((retVal) ? "Error - Unknown device" : "Verified"));

	return retVal;
}

static int32_t bcmtch_dev_verify_chip_version(void)
{
	int32_t retVal = BCMTCH_SUCCESS;
	uint32_t version;

	retVal =
	    bcmtch_com_read_sys(BCMTCH_SYS_ADDR_TCH_VER, 4,
				(uint8_t *)&version);

	printk(KERN_INFO "BCMTCH: Chip Version = 0x%08X\n", version);

	return retVal;
}

#if 0   /* not been used currently */
static int32_t bcmtch_dev_verify_firmware_version(void)
{
	int32_t retVal = BCMTCH_SUCCESS;
	return retVal;
}
#endif

static int32_t bcmtch_dev_init(void)
{
	int32_t retVal = BCMTCH_SUCCESS;

	/* verify chip id */
	retVal = bcmtch_dev_verify_chip_id();

	/* init com */
	if (!retVal)
		retVal = bcmtch_com_init();

	/* wakeup */
	bcmtch_dev_request_power_mode(BCMTCH_POWER_MODE_WAKE,
				      TOFE_COMMAND_NO_COMMAND);
	if (!retVal)
		retVal =
		    bcmtch_dev_check_power_state(BCMTCH_POWER_STATE_ACTIVE, 25);

	/* init clocks */
	if (!retVal)
		retVal = bcmtch_dev_init_clocks();

	/* init memory */
	if (!retVal)
		retVal = bcmtch_dev_init_memory();

	/* download */
	if (!retVal)
		retVal = bcmtch_dev_verify_chip_version();

	if (!retVal)
		retVal = bcmtch_dev_init_firmware();

	return retVal;
}

static void bcmtch_dev_process(void)
{
	int32_t retVal = BCMTCH_SUCCESS;

	/* read channels */
	retVal = bcmtch_dev_read_channels();

	/* release channels */
	bcmtch_dev_request_power_mode(BCMTCH_POWER_MODE_NOWAKE,
				      TOFE_COMMAND_NO_COMMAND);

	/* process channels */
	retVal = bcmtch_dev_process_channels();
}

/* -------------------------------------------------- */
/* - BCM Touch Controller Com(munication) Functions - */
/* -------------------------------------------------- */

static int32_t bcmtch_com_init(void)
{
	int32_t retVal = BCMTCH_SUCCESS;

	if (bcmtch_data_p) {
		retVal =
		    bcmtch_com_write_spm(BCMTCH_SPM_REG_SPI_I2C_SEL,
					 BCMTCH_IF_I2C_SEL);
		retVal |=
		    bcmtch_com_write_spm(BCMTCH_SPM_REG_I2CS_CHIPID,
					 bcmtch_data_p->platform_data.
					 i2c_addr_sys);
	} else {
		printk(KERN_ERR "%s() error, bcmtch_data_p == NULL\n",
		       __func__);
		retVal = -ENODATA;
	}

	return retVal;
}

static int32_t bcmtch_com_read_spm(uint8_t reg, uint8_t *data)
{
	int32_t retVal = BCMTCH_SUCCESS;

	if (bcmtch_data_p) {
#if BCMTCH_USE_BUS_LOCK
		bcmtch_os_lock_critical_section(BCMTCH_MUTEX_BUS);
#endif
		retVal =
		    bcmtch_os_i2c_read_spm(bcmtch_data_p->p_i2c_client_spm, reg,
					   data);
#if BCMTCH_USE_BUS_LOCK
		bcmtch_os_release_critical_section(BCMTCH_MUTEX_BUS);
#endif
	} else {
		printk(KERN_ERR "%s() error, bcmtch_data_p == NULL\n",
		       __func__);
		retVal = -ENODATA;
	}

	return retVal;
}

static int32_t bcmtch_com_write_spm(uint8_t reg, uint8_t data)
{
	int32_t retVal = BCMTCH_SUCCESS;

	if (bcmtch_data_p) {
#if BCMTCH_USE_BUS_LOCK
		bcmtch_os_lock_critical_section(BCMTCH_MUTEX_BUS);
#endif
		retVal =
		    bcmtch_os_i2c_write_spm(bcmtch_data_p->p_i2c_client_spm,
					    reg, data);
#if BCMTCH_USE_BUS_LOCK
		bcmtch_os_release_critical_section(BCMTCH_MUTEX_BUS);
#endif
	} else {
		printk(KERN_ERR "%s() error, bcmtch_data_p == NULL\n",
		       __func__);
		retVal = -ENODATA;
	}

	return retVal;
}

static inline int32_t bcmtch_com_fast_write_spm(uint8_t count, uint8_t *regs,
						uint8_t *data)
{
	int32_t retVal = BCMTCH_SUCCESS;

#if BCMTCH_USE_BUS_LOCK
	bcmtch_os_lock_critical_section(BCMTCH_MUTEX_BUS);
#endif
	retVal =
	    bcmtch_os_i2c_fast_write_spm(bcmtch_data_p->p_i2c_client_spm, count,
					 regs, data);
#if BCMTCH_USE_BUS_LOCK
	bcmtch_os_release_critical_section(BCMTCH_MUTEX_BUS);
#endif

	return retVal;
}

static int32_t bcmtch_com_read_sys(uint32_t sys_addr, uint16_t read_len,
				   uint8_t *read_data)
{
	int32_t retVal = BCMTCH_SUCCESS;

	if (bcmtch_data_p) {
#if BCMTCH_USE_BUS_LOCK
		bcmtch_os_lock_critical_section(BCMTCH_MUTEX_BUS);
#endif
		retVal =
		    bcmtch_os_i2c_read_sys(bcmtch_data_p->p_i2c_client_sys,
					   sys_addr, read_len, read_data);
#if BCMTCH_USE_BUS_LOCK
		bcmtch_os_release_critical_section(BCMTCH_MUTEX_BUS);
#endif
	} else {
		printk(KERN_ERR "%s() error, bcmtch_data_p == NULL\n",
		       __func__);
		retVal = -ENODATA;
	}

	return retVal;
}

static inline int32_t bcmtch_com_write_sys32(uint32_t sys_addr,
					     uint32_t write_data)
{
	return bcmtch_com_write_sys(sys_addr, 4, (uint8_t *)&write_data);
}

static int32_t bcmtch_com_write_sys(uint32_t sys_addr, uint16_t write_len,
				    uint8_t *write_data)
{
	int32_t retVal = BCMTCH_SUCCESS;

	if (bcmtch_data_p) {
#if BCMTCH_USE_BUS_LOCK
		bcmtch_os_lock_critical_section(BCMTCH_MUTEX_BUS);
#endif
		retVal =
		    bcmtch_os_i2c_write_sys(bcmtch_data_p->p_i2c_client_sys,
					    sys_addr, write_len, write_data);
#if BCMTCH_USE_BUS_LOCK
		bcmtch_os_release_critical_section(BCMTCH_MUTEX_BUS);
#endif
	} else {
		printk(KERN_ERR "%s() error, bcmtch_data_p == NULL\n",
		       __func__);
		retVal = -ENODATA;
	}

	return retVal;
}

/* ------------------------------------- */
/* - BCM Touch Controller OS Functions - */
/* ------------------------------------- */
static void bcmtch_os_sleep_ms(uint32_t ms)
{
	msleep(ms);
}

static irqreturn_t bcmtch_os_interrupt_handler(int32_t irq, void *dev_id)
{
	if ((bcmtch_data_p) && (bcmtch_data_p->p_i2c_client_spm->irq == irq)) {
		queue_work(bcmtch_data_p->p_workqueue,
			   (struct work_struct *)bcmtch_data_p);
	} else {
		printk(KERN_ERR
		       "%s : Error - IRQ Mismatch ? int=%d client_int=%d\n",
		       __func__, irq,
		       (bcmtch_data_p) ? bcmtch_data_p->p_i2c_client_spm->
		       irq : 0);
	}

	return IRQ_HANDLED;
}

static void *bcmtch_os_mem_alloc(uint32_t mem_size_req)
{
	return kzalloc(mem_size_req, GFP_KERNEL);
}

static void bcmtch_os_mem_free(void *mem_p)
{
	kfree(mem_p);
	mem_p = NULL;
}

static int32_t bcmtch_os_init_input_device(void)
{
	int32_t retVal = BCMTCH_SUCCESS;

	if (bcmtch_data_p) {
		bcmtch_data_p->p_inputDevice = input_allocate_device();
		if (bcmtch_data_p->p_inputDevice) {
			bcmtch_data_p->p_inputDevice->name =
			    "BCM15500 Touch Screen";
			bcmtch_data_p->p_inputDevice->phys = "I2C";
			bcmtch_data_p->p_inputDevice->id.bustype = BUS_I2C;
			bcmtch_data_p->p_inputDevice->id.vendor = 0x0EEF;
			bcmtch_data_p->p_inputDevice->id.product = 0x0020;
			bcmtch_data_p->p_inputDevice->id.version = 0x0000;

			set_bit(EV_SYN, bcmtch_data_p->p_inputDevice->evbit);
			set_bit(EV_ABS, bcmtch_data_p->p_inputDevice->evbit);
			__set_bit(INPUT_PROP_DIRECT,
				  bcmtch_data_p->p_inputDevice->propbit);

			set_bit(EV_KEY, bcmtch_data_p->p_inputDevice->evbit);
			set_bit(BTN_TOUCH,
				bcmtch_data_p->p_inputDevice->keybit);

			input_set_abs_params(bcmtch_data_p->p_inputDevice,
					     ABS_MT_POSITION_X, 0, BCMTCH_MAX_X,
					     0, 0);
			input_set_abs_params(bcmtch_data_p->p_inputDevice,
					     ABS_MT_POSITION_Y, 0, BCMTCH_MAX_Y,
					     0, 0);

			/* request new os input queue size for this device */
			input_set_events_per_packet(bcmtch_data_p->
						    p_inputDevice,
						    50 * BCMTCH_MAX_TOUCH);

			/* register device */
			retVal =
			    input_register_device(bcmtch_data_p->p_inputDevice);
			if (retVal) {
				printk(KERN_INFO
					"%s() Unable to register input device\n",
					__func__);
				input_free_device(bcmtch_data_p->p_inputDevice);
				bcmtch_data_p->p_inputDevice = NULL;
			}
		} else {
			printk(KERN_ERR "%s() Unable to create device\n",
			       __func__);
			retVal = -ENODEV;
		}
	} else {
		printk(KERN_ERR "%s() error, driver data structure == NULL\n",
		       __func__);
		retVal = -ENODATA;
	}

	return retVal;
}

static void bcmtch_os_free_input_device(void)
{
	if (bcmtch_data_p && bcmtch_data_p->p_inputDevice) {
		input_unregister_device(bcmtch_data_p->p_inputDevice);
		bcmtch_data_p->p_inputDevice = NULL;
	}
}

static int32_t bcmtch_os_init_critical_sections(void)
{
	int32_t retVal = BCMTCH_SUCCESS;

	if (!bcmtch_data_p) {
		printk(KERN_ERR "%s() error, driver data structure == NULL\n",
		       __func__);
		retVal = -ENODATA;
		return retVal;
	}

	mutex_init(&bcmtch_data_p->cs_mutex[BCMTCH_MUTEX_BUS]);
	mutex_init(&bcmtch_data_p->cs_mutex[BCMTCH_MUTEX_WORK]);

	return retVal;
}

static void bcmtch_os_lock_critical_section(uint8_t lock_mutex)
{
	mutex_lock(&bcmtch_data_p->cs_mutex[lock_mutex]);
}

static void bcmtch_os_release_critical_section(uint8_t lock_mutex)
{
	mutex_unlock(&bcmtch_data_p->cs_mutex[lock_mutex]);
}

static void bcmtch_os_deferred_worker(struct work_struct *work)
{
#if BCMTCH_USE_WORK_LOCK
	bcmtch_os_lock_critical_section(BCMTCH_MUTEX_WORK);
#endif

	bcmtch_dev_process();

#if BCMTCH_USE_WORK_LOCK
	bcmtch_os_release_critical_section(BCMTCH_MUTEX_WORK);
#endif
}

static int32_t bcmtch_os_init_deferred_worker(void)
{
	int32_t retVal = BCMTCH_SUCCESS;

	if (bcmtch_data_p) {
		bcmtch_data_p->p_workqueue = create_workqueue("bcmtch_wq");

		if (bcmtch_data_p->p_workqueue) {
			INIT_WORK(&bcmtch_data_p->work,
				  bcmtch_os_deferred_worker);
		} else {
			printk(KERN_ERR "%s() Unable to create workqueue\n",
			       __func__);
			retVal = -ENOMEM;
		}
	} else {
		printk(KERN_ERR "%s() error, driver data structure == NULL\n",
		       __func__);
		retVal = -ENODATA;
	}

	return retVal;
}

static void bcmtch_os_free_deferred_worker(void)
{
	if (bcmtch_data_p && bcmtch_data_p->p_workqueue) {
		PROGRESS();

		flush_workqueue(bcmtch_data_p->p_workqueue);
		destroy_workqueue(bcmtch_data_p->p_workqueue);

		bcmtch_data_p->p_workqueue = NULL;
	}
}

static void bcmtch_os_reset(void)
{
	if (bcmtch_data_p
	    && gpio_is_valid(bcmtch_data_p->platform_data.gpio_reset_pin)) {

		gpio_set_value(bcmtch_data_p->platform_data.gpio_reset_pin,
			       bcmtch_data_p->platform_data.
			       gpio_reset_polarity);

		bcmtch_os_sleep_ms(bcmtch_data_p->platform_data.
				   gpio_reset_time_ms);

		gpio_set_value(bcmtch_data_p->platform_data.gpio_reset_pin,
			       !bcmtch_data_p->platform_data.
			       gpio_reset_polarity);

		bcmtch_os_sleep_ms(bcmtch_data_p->platform_data.
				   gpio_reset_time_ms);

	}
}

static int32_t bcmtch_os_init_gpio(void)
{
	int32_t retVal = BCMTCH_SUCCESS;

	if (!bcmtch_data_p) {
		printk(KERN_ERR "%s() error, driver data structure == NULL\n",
		       __func__);
		retVal = -ENODATA;
		return retVal;
	}

	/*
	 * setup a gpio pin for BCM Touch Controller reset function
	 */
	if (gpio_is_valid(bcmtch_data_p->platform_data.gpio_reset_pin)) {
		retVal =
		    gpio_request(bcmtch_data_p->platform_data.gpio_reset_pin,
				 "BCMTCH reset");
		if (retVal < 0) {
			printk(KERN_ERR
			       "ERROR: %s() - Unable to request reset pin %d\n",
			       __func__,
			       bcmtch_data_p->platform_data.gpio_reset_pin);

			/* note :
			 * it is an error if a reset pin is requested and not
			 * granted --> return
			 */
			return retVal;
		}

		/*
		 * setup reset pin as output
		 * - invert reset polarity --> don't want to hold in reset
		 */
		retVal =
		    gpio_direction_output(bcmtch_data_p->platform_data.
					  gpio_reset_pin,
					  !bcmtch_data_p->platform_data.
					  gpio_reset_polarity);

		if (retVal < 0) {
			printk(KERN_ERR
			       "ERROR: %s() - Unable to set reset pin %d\n",
			       __func__,
			       bcmtch_data_p->platform_data.gpio_reset_pin);

			/* note :
			 * it is an error if a reset pin is requested and
			 * not set --> return
			 */
			return retVal;
		}
	} else {
		printk(KERN_INFO "%s() : no reset pin configured\n", __func__);
	}

	/*
	 * setup a gpio pin for BCM Touch Controller interrupt function
	 */
	if (gpio_is_valid(bcmtch_data_p->platform_data.gpio_interrupt_pin)) {
		retVal =
		    gpio_request(bcmtch_data_p->platform_data.
				 gpio_interrupt_pin, "BCMTCH interrupt");
		if (retVal < 0) {
			printk(KERN_ERR
			       "ERROR: %s() - Unable to request interrupt pin %d\n",
			       __func__,
			       bcmtch_data_p->platform_data.gpio_interrupt_pin);

			/* note :
			 * it is an error if an interrupt pin is requested and
			 * not granted --> return
			 */
			return retVal;
		}

		/* setup interrupt pin as input */
		gpio_direction_input(bcmtch_data_p->platform_data.
				     gpio_interrupt_pin);

		/* Reserve the irq line. */
		retVal =
		    request_irq(gpio_to_irq
				(bcmtch_data_p->platform_data.
				 gpio_interrupt_pin),
				bcmtch_os_interrupt_handler,
				bcmtch_data_p->platform_data.
				gpio_interrupt_trigger, BCM15500_TSC_NAME,
				bcmtch_data_p);

		if (retVal) {
			printk(KERN_ERR
			       "ERROR: %s() - Unable to request interrupt irq %d\n",
			       __func__,
			       gpio_to_irq(bcmtch_data_p->platform_data.
					   gpio_interrupt_pin));

			/* note :
			 * it is an error if an irq is requested and not
			 * granted --> return
			 */
			return retVal;
		}
	} else {
		printk(KERN_INFO "%s() : no interrupt pin configured\n",
		       __func__);
	}

	return retVal;
}

static void bcmtch_os_free_gpio(void)
{
	if (bcmtch_data_p) {
		if (gpio_is_valid(
			bcmtch_data_p->platform_data.gpio_reset_pin)) {
			gpio_free(bcmtch_data_p->platform_data.gpio_reset_pin);
		}

		if (gpio_is_valid
		    (bcmtch_data_p->platform_data.gpio_interrupt_pin)) {
			free_irq(gpio_to_irq
				 (bcmtch_data_p->platform_data.
				  gpio_interrupt_pin), bcmtch_data_p);
			gpio_free(bcmtch_data_p->platform_data.
				  gpio_interrupt_pin);
		}
	}
}

static int32_t bcmtch_os_init_cli(struct device *p_device)
{
	int32_t retVal = BCMTCH_SUCCESS;

	retVal = device_create_file(p_device, &bcmtch_cli_attr);

	return retVal;
}

static void bcmtch_os_free_cli(struct device *p_device)
{
	device_remove_file(p_device, &bcmtch_cli_attr);
}

/* ----------------------------------------- */
/* - BCM Touch Controller OS I2C Functions - */
/* ----------------------------------------- */
static int32_t bcmtch_os_i2c_read_spm(struct i2c_client *p_i2c, uint8_t reg,
				      uint8_t *data)
{
	int32_t retVal = BCMTCH_SUCCESS;

	/* setup I2C messages for single byte read transaction */
	struct i2c_msg msg[2] = {
		/* first write register to spm */
		{.addr = p_i2c->addr, .flags = 0, .len = 1, .buf = &reg},
		/* Second read data from spm reg */
		{.addr = p_i2c->addr, .flags = I2C_M_RD, .len = 1, .buf = data}
	};

	if (i2c_transfer(p_i2c->adapter, msg, 2) != 2)
		retVal = -EIO;

	return retVal;
}

static int32_t bcmtch_os_i2c_write_spm(struct i2c_client *p_i2c, uint8_t reg,
				       uint8_t data)
{
	int32_t retVal = BCMTCH_SUCCESS;

	/* setup buffer with reg address and data */
	uint8_t buffer[2] = { reg, data };

	/* setup I2C message for single byte write transaction */
	struct i2c_msg msg[1] = {
		/* first write message to spm */
		{.addr = p_i2c->addr, .flags = 0, .len = 2, .buf = buffer}
	};

	if (i2c_transfer(p_i2c->adapter, msg, 1) != 1)
		retVal = -EIO;

	return retVal;
}

static int32_t bcmtch_os_i2c_fast_write_spm(struct i2c_client *p_i2c,
					    uint8_t count, uint8_t *regs,
					    uint8_t *data)
{
	int32_t retVal = BCMTCH_SUCCESS;

	/*
	 * support hard-coded for a max of 5 spm write messages
	 *
	 * - 1 i2c message uses 2 uint8_t buffers
	 *
	 */
	uint8_t buffer[10];	/* buffers for reg address and data */
	struct i2c_msg msg[5];
	uint32_t nMsg = 0;
	uint8_t *pBuf = buffer;

	/* setup I2C message for single byte write transaction */
	while (nMsg < count) {
		msg[nMsg].addr = p_i2c->addr;
		msg[nMsg].flags = 0;
		msg[nMsg].len = 2;
		msg[nMsg].buf = pBuf;

		*pBuf++ = regs[nMsg];
		*pBuf++ = data[nMsg];
		nMsg++;
	}

	if (i2c_transfer(p_i2c->adapter, msg, nMsg) != nMsg)
		retVal = -EIO;

	return retVal;
}

static int32_t bcmtch_os_i2c_read_sys(struct i2c_client *p_i2c,
				      uint32_t sys_addr, uint16_t read_len,
				      uint8_t *read_data)
{
	int32_t retVal = BCMTCH_SUCCESS;
	uint8_t statusReg = BCMTCH_SPM_REG_DMA_STATUS;
	uint8_t dmaReg = BCMTCH_SPM_REG_DMA_RFIFO;
	uint8_t dmaStatus;

	/* setup the DMA header for this read transaction */
	uint8_t dmaHeader[8] = {
		/* set dma controller addr */
		BCMTCH_SPM_REG_DMA_ADDR,
		/* setup dma address */
		(sys_addr & 0xFF),
		((sys_addr & 0xFF00) >> 8),
		((sys_addr & 0xFF0000) >> 16),
		((sys_addr & 0xFF000000) >> 24),
		/* setup dma length */
		(read_len & 0xFF),
		((read_len & 0xFF00) >> 8),
		/* setup dma mode */
		BCMTCH_DMA_MODE_READ
	};

	/* setup I2C messages for DMA read request transaction */
	struct i2c_msg dma_request[3] = {
		/* write DMA request header */
		{.addr = p_i2c->addr, .flags = 0, .len = 8, .buf = dmaHeader},

		/* write messages to read the DMA request status */
		{.addr = p_i2c->addr, .flags = 0, .len = 1, .buf = &statusReg},
		{.addr = p_i2c->addr, .flags = I2C_M_RD, .len = 1, .buf =
		 &dmaStatus}
	};

	/* setup I2C messages for DMA read transaction */
	struct i2c_msg dma_read[2] = {
		/* next write messages to read the DMA request status */
		{.addr = p_i2c->addr, .flags = 0, .len = 1, .buf = &dmaReg},
		{.addr = p_i2c->addr, .flags = I2C_M_RD, .len = read_len, .buf =
		 read_data}
	};

	/* send DMA request */
	if (i2c_transfer(p_i2c->adapter, dma_request, 3) != 3) {
		retVal = -EIO;
	} else {
		while (dmaStatus != 1) {
			/* read status */
			if (i2c_transfer(p_i2c->adapter, &dma_request[1], 2) !=
			    2) {
				retVal = -EIO;
				break;
			}
		}
	}

	if (dmaStatus) {
		/* read status */
		if (i2c_transfer(p_i2c->adapter, dma_read, 2) != 2)
			retVal = -EIO;
	}
	return retVal;
}

static int32_t bcmtch_os_i2c_write_sys(struct i2c_client *p_i2c,
				       uint32_t sys_addr, uint16_t write_len,
				       uint8_t *write_data)
{
	int32_t retVal = BCMTCH_SUCCESS;

	uint16_t dmaLen = write_len + 1;
	uint8_t *dmaData = bcmtch_os_mem_alloc(dmaLen);

	/* setup the DMA header for this read transaction */
	uint8_t dmaHeader[8] = {
		/* set dma controller addr */
		BCMTCH_SPM_REG_DMA_ADDR,
		/* setup dma address */
		(sys_addr & 0xFF),
		((sys_addr & 0xFF00) >> 8),
		((sys_addr & 0xFF0000) >> 16),
		((sys_addr & 0xFF000000) >> 24),
		/* setup dma length */
		(write_len & 0xFF),
		((write_len & 0xFF00) >> 8),
		/* setup dma mode */
		BCMTCH_DMA_MODE_WRITE
	};

	/* setup I2C messages for DMA read request transaction */
	struct i2c_msg dma_request[2] = {
		/* write DMA request header */
		{.addr = p_i2c->addr, .flags = 0, .len = 8, .buf = dmaHeader},
		{.addr = p_i2c->addr, .flags = 0, .len = dmaLen, .buf = dmaData}
	};

	if (dmaData) {
		/* setup dma data buffer */
		dmaData[0] = BCMTCH_SPM_REG_DMA_WFIFO;
		memcpy(&dmaData[1], write_data, write_len);

		if (i2c_transfer(p_i2c->adapter, dma_request, 2) != 2)
			retVal = -EIO;

		/* free dma buffer */
		bcmtch_os_mem_free(dmaData);
	} else {
		retVal = -ENOMEM;
	}

	return retVal;
}

static int32_t bcmtch_os_i2c_init_clients(struct i2c_client *p_i2c_client_spm)
{
	int32_t retVal = BCMTCH_SUCCESS;
	struct i2c_client *p_i2c_client_sys;

	if (bcmtch_data_p) {
		if (p_i2c_client_spm->adapter) {
			/* Configure the second I2C slave address. */
			p_i2c_client_sys =
			    i2c_new_dummy(p_i2c_client_spm->adapter,
					  bcmtch_data_p->platform_data.
					  i2c_addr_sys);

			if (p_i2c_client_sys) {
				/* assign */
				bcmtch_data_p->p_i2c_client_spm =
				    p_i2c_client_spm;
				bcmtch_data_p->p_i2c_client_sys =
				    p_i2c_client_sys;
			} else {
				printk(KERN_ERR
				       "%s() i2c_new_dummy == NULL, slave "
				       "address: 0x%x\n",
				       __func__,
				       bcmtch_data_p->platform_data.
				       i2c_addr_sys);
				retVal = -ENODEV;
			}
		} else {
			printk(KERN_ERR
			       "%s() p_i2c_adapter == NULL, adapter_id: 0x%x\n",
			       __func__,
			       bcmtch_data_p->platform_data.i2c_bus_id);

			retVal = -ENODEV;
		}
	}

	return retVal;
}

static void bcmtch_os_i2c_free_clients(void)
{
	if (bcmtch_data_p && (bcmtch_data_p->p_i2c_client_sys)) {
		i2c_unregister_device(bcmtch_data_p->p_i2c_client_sys);
		bcmtch_data_p->p_i2c_client_sys = NULL;
	}
}

/* ------------------------------------------------- */
/* - BCM Touch Controller OS I2C Driver Structures - */
/* ------------------------------------------------- */
static const struct i2c_device_id bcmtch_i2c_id[] = {
	{BCM15500_TSC_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, bcmtch_i2c_id);

static struct i2c_driver bcmtch_i2c_driver = {
	.driver = {
		   .name = BCM15500_TSC_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = bcmtch_os_i2c_probe,
	.remove = bcmtch_os_i2c_remove,
	.id_table = bcmtch_i2c_id,
};

/* ------------------------------------------------ */
/* - BCM Touch Controller OS I2C Driver Functions - */
/* ------------------------------------------------ */
static int32_t bcmtch_os_i2c_probe(struct i2c_client *p_i2c_client,
				   const struct i2c_device_id *id)
{
	int32_t retVal = BCMTCH_SUCCESS;

	/* print driver probe header */
	if (p_i2c_client)
		printk(KERN_INFO "BCMTCH: PROBE: dev=%s addr=0x%x irq=%d\n",
		       p_i2c_client->name, p_i2c_client->addr,
		       p_i2c_client->irq);
	if (id)
		printk(KERN_INFO "BCMTCH: PROBE: match id=%s\n", id->name);

	/* allocate global BCM Touch Controller driver structure */
	retVal = bcmtch_dev_alloc();
	if (retVal)
		goto probe_error;

	/* setup local platform data from client device structure */
	retVal = bcmtch_dev_init_platform(&p_i2c_client->dev);
	if (retVal)
		goto probe_error;

	/* initialize deferred worker (workqueue/tasklet/etc */
	retVal = bcmtch_os_init_deferred_worker();
	if (retVal)
		goto probe_error;

	/* setup the gpio pins
	 * - 1 gpio used for reset control signal to BCM Touch Controller
	 * - 1 gpio used as interrupt signal from BCM Touch Controller
	 */
	retVal = bcmtch_os_init_gpio();
	if (retVal)
		goto probe_error;

	/* setup the critical sections for concurrency */
	retVal = bcmtch_os_init_critical_sections();
	if (retVal)
		goto probe_error;

	/* setup the os input device */
	retVal = bcmtch_os_init_input_device();
	if (retVal)
		goto probe_error;

	/* setup the os cli */
	retVal = bcmtch_os_init_cli(&p_i2c_client->dev);
	if (retVal)
		goto probe_error;

	/*
	 * setup the i2c clients and bind (store pointers in global structure)
	 * 1. SPM I2C client
	 * 2. SYS I2C client
	 */
	retVal = bcmtch_os_i2c_init_clients(p_i2c_client);
	if (retVal)
		goto probe_error;

	/* reset the chip on driver load ? */
	if (bcmtch_boot_flag & BCMTCH_BOOT_FLAG_RESET_ON_LOAD)
		bcmtch_dev_reset(BCMTCH_RESET_MODE_HARD);

	/* perform BCM Touch Controller initialization */
	retVal = bcmtch_dev_init();
	if (retVal)
		goto probe_error;

	return BCMTCH_SUCCESS;

probe_error:
	bcmtch_os_i2c_remove(p_i2c_client);
	return retVal;
}

static int32_t bcmtch_os_i2c_remove(struct i2c_client *p_i2c_client)
{
	PROGRESS();

	printk(KERN_INFO "client 2: %s at address 0x%x using irq %d\n",
	       p_i2c_client->name, p_i2c_client->addr, p_i2c_client->irq);

#if !BCMTCH_USE_BUS_LOCK
#if BCMTCH_USE_WORK_LOCK
	bcmtch_os_lock_critical_section(BCMTCH_MUTEX_WORK);
#endif
#endif

	/* force chip to sleep berfore exiting */
	if (BCMTCH_POWER_STATE_SLEEP != bcmtch_dev_get_power_state())
		bcmtch_dev_set_power_state(BCMTCH_POWER_STATE_SLEEP);

	PROGRESS();

	/* free communication channels */
	bcmtch_dev_free_channels();

	PROGRESS();

	/* free i2c device clients */
	bcmtch_os_i2c_free_clients();

	PROGRESS();

	/* remove the os cli */
	bcmtch_os_free_cli(&p_i2c_client->dev);

	/* free input device */
	bcmtch_os_free_input_device();

	PROGRESS();

	/* free used gpio pins */
	bcmtch_os_free_gpio();

	PROGRESS();

	/* free deferred worker (queue) */
	bcmtch_os_free_deferred_worker();

	PROGRESS();

	/* free this mem last */
	bcmtch_dev_free();

	return BCMTCH_SUCCESS;
}

static int32_t __init bcmtch_os_i2c_init(void)
{
	PROGRESS();
	return i2c_add_driver(&bcmtch_i2c_driver);
}

/* init early so consumer devices can complete system boot */
subsys_initcall(bcmtch_os_i2c_init);

static void __exit bcmtch_os_i2c_exit(void)
{
	PROGRESS();
	i2c_del_driver(&bcmtch_i2c_driver);
}

module_exit(bcmtch_os_i2c_exit);

MODULE_DESCRIPTION("I2C support for BCM15500 Touchscreen");
MODULE_LICENSE("GPL");
