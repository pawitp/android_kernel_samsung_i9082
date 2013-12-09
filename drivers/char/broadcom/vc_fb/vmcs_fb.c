/*****************************************************************************
* Copyright 2011 Broadcom Corporation.  All rights reserved.
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
 * Description:
 *    This is the videocore framebuffer driver. A framebuffer device for each
 *    of the supported screens is registered with the Linux system. The actual
 *    allocation of the framebuffer memory is not done until someone invokes
 *    the open command on the framebuffer device. This driver uses the
 *    framebuffer service, which provides (but not limited) the following:
 *       - information (resolution, bpp) for each attached screen
 *       - allocation of memory for framebuffer on videocore
 *       - panning/swapping of framebuffers
 *    Since the allocation is not done until the device open is invoked, users
 *    can modify certain parameters that affects the framebuffer via proc
 *    entries. The following properties can be modified:
 *       - alpha per pixel
 *       - default alpha (if alpha per pixel is not used)
 *       - h/w scaling
 *       - resolution override (this is the resolution reported to linux, which
 *         can be smaller or greater than the actual resolution of the screen)
 *       - z-ordering of the framebuffer
 *
 * Notes:
 *    1. Terminology:
 *       - display: display device that has one or more screens attached
 *       - screen: attached to a display that shows actual content, e.g. LCD
 *    2. Currently only one instance of videocore is supported
 *    3. STR not yet supported
 *    4. Rotation not yet supported
 *    5. TODO: Add a 'info' proc entry to show number of active users, current
 *       resolution, memory address/length, dev path (/dev/fb0), etc.
 */

/* ---- Include Files ---------------------------------------------------- */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/semaphore.h>
#include <linux/proc_fs.h>
#include <linux/dma-mapping.h>
#include <linux/pfn.h>
#include <linux/hugetlb.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <chal/chal_ipc.h>
#include <linux/syscalls.h>

#include <vc_mem.h>
#include <vc_dt.h>

#include "debug_sym.h"
#include "vchiq_connected.h"
#include "vc_vchi_fb.h"
#include "vc_fb_ipc.h"

/* ---- Private Constants and Types -------------------------------------- */

/* Uncomment the following line to enable debug messages */
/* #define ENABLE_LOG_DBG */

/* Logging macros (for remapping to other logging mechanisms) */
#ifdef ENABLE_LOG_DBG
#define LOG_DBG(fmt, arg...)  printk(KERN_INFO "[D] " fmt "\n", ##arg)
#else
#define LOG_DBG(fmt, arg...)
#endif
#define LOG_INFO(fmt, arg...)  printk(KERN_INFO "[I] " fmt "\n", ##arg)
#define LOG_ERR(fmt, arg...)  printk(KERN_ERR  "[E] " fmt "\n", ##arg)

/* #define VC_ROUND_UP_WH(wh)  (((wh) + 15) & ~15) */

/* Default values for framebuffer creation modifiable parameters */
#define DEFAULT_ALPHA            (255)
#define DEFAULT_ALPHA_PER_PIXEL  (0)
#define DEFAULT_BITS_PER_PIXEL   (32)
#define DEFAULT_KEEP_RESOURCE    (0)
#define DEFAULT_SCALE            (1)
#define DEFAULT_Z_ORDER          (-50)
#define DEFAULT_ADJUST           (0)
#define PROC_WRITE_BUF_SIZE      128

/* How long to wait for read FB to complete before
 * timing out for no blocking call.
 */
#define DEFAULT_READ_TO_MSEC     (20000)
#define FB_IPC_DOORBELL          (0xD)

struct SCRN_INFO_T {
	VC_FB_SCRN scrn;

	uint32_t user_cnt;	/* Number of active users of this screen */
	struct mutex user_cnt_mutex;	/* Mutex to protect user count */
	struct fb_info fb_info;	/* Kernel framebuffer info */
	uint32_t res_handle;	/* Videocore resource handle */
	uint32_t cmap[16];	/* Color map */

	/* Per framebuffer device proc directory */
	struct proc_dir_entry *fb_cfg_directory;

	/* Modifiable paramters for framebuffer creation (upon fb_open call) */
	uint32_t alpha;		/* global alpha value to use */
	uint32_t alpha_per_pixel;	/* 1 = pixel has own alpha value */
	uint32_t bpp_override;	/* Bits per pixel override */
	uint32_t keep_resource;	/* Keep resource open even on release */
	uint32_t scale;		/* Scale the image to fit the screen */
	uint32_t z_order;	/* Z-order of framebuffer */
	uint32_t width_override;	/* Width override */
	uint32_t height_override;	/* Height override */
	uint32_t adjust;	/* Adjust dimensions to 16x32 factor */

	/* Proc entries corresponding to the modifiable parameters */
	struct proc_dir_entry *alpha_cfg_entry;
	struct proc_dir_entry *alpha_per_pixel_cfg_entry;
	struct proc_dir_entry *bpp_override_cfg_entry;
	struct proc_dir_entry *keep_resource_cfg_entry;
	struct proc_dir_entry *res_override_cfg_entry;
	struct proc_dir_entry *scale_cfg_entry;
	struct proc_dir_entry *z_order_cfg_entry;
	struct proc_dir_entry *action_cfg_entry;
	struct proc_dir_entry *adjust_cfg_entry;
};

struct FB_STATE_T {
	VC_VCHI_FB_HANDLE_T fb_handle;
	struct SCRN_INFO_T *scrn_info[VC_FB_SCRN_MAX];
	struct proc_dir_entry *cfg_directory;

	/* non blocking read operation.
	 */
	struct mutex read_mutex;
	uint32_t read_handle;
	int32_t read_size;
	void *read_buf;
	int32_t read_status;
	struct task_struct *read_task;
	struct semaphore read_sema;
	struct semaphore read_resp_sema;
};

/* ---- Private Variables ------------------------------------------------ */

static struct FB_STATE_T *fb_state;
static int fb_inited;

/* Constant strings for the proc entries */
static const char *fb_cfg_dir_name[VC_FB_SCRN_MAX] = {
	"0",
	"1",
};

static CHAL_IPC_HANDLE ipcHandle;
static struct DISPLAY_IPC_T display_local;
static VC_MEM_ACCESS_HANDLE_T display_vc_handle;
static VC_MEM_ADDR_T display_vc_addr;
static int display_initialised;

static int vc_display_init(void)
{
	int rc = 0;
	const char *display_sym_name = "display_ipc";
	size_t mem_size;

	if (display_initialised)
		return 0;

	ipcHandle = chal_ipc_config(NULL);
	rc = OpenVideoCoreMemory(&display_vc_handle);
	if (rc != 0) {
		LOG_ERR("%s: OpenVideoCoreMemory failed: %d", __func__, rc);
		return rc;
	}

	if (LookupVideoCoreSymbol(display_vc_handle,
			display_sym_name, &display_vc_addr, &mem_size) < 0) {
		LOG_ERR("Symbol '%s' not found", display_sym_name);
		CloseVideoCoreMemory(display_vc_handle);
		return -ENOENT;
	}

	if (mem_size != sizeof(struct DISPLAY_IPC_T)) {
		LOG_ERR("%s has a size of %d bytes, expecting %d",
			display_sym_name,
			mem_size,
			sizeof(struct DISPLAY_IPC_T));
		CloseVideoCoreMemory(display_vc_handle);
		return -EIO;
	}

	display_initialised = 1;
	return rc;
}

static int vc_display_send(int wait, int timeout)
{
	int rc = 0;
	static uint32_t request_counter = 1;

	vc_display_init();

	display_local.req = ++request_counter;
	display_local.ack = ~display_local.req;

	if (!WriteVideoCoreMemory(
			display_vc_handle,
			&display_local,
			display_vc_addr,
			sizeof(struct DISPLAY_IPC_T))) {
		LOG_ERR("Error writing %d bytes to vc addr 0x%08x",
			sizeof(struct DISPLAY_IPC_T), display_vc_addr);
		return -EIO;
	}

	chal_ipc_int_vcset(ipcHandle, FB_IPC_DOORBELL);

	if (wait) {
		uint32_t ack_value = ~request_counter;
		int iter = 0;
		VC_MEM_ADDR_T vc_addr_ack = display_vc_addr +
			offsetof(struct DISPLAY_IPC_T, ack);

		while (ack_value != request_counter) {
			if (!ReadVideoCoreMemory(
					display_vc_handle,
					&ack_value,
					vc_addr_ack,
					sizeof(uint32_t)) != 0) {
				LOG_ERR(
					"Error reading %d bytes from vc addr 0x%08x",
					sizeof(uint32_t), vc_addr_ack);
				return -EIO;
			}
			if (timeout) {
				if (iter < timeout) {
					iter++;
					mdelay(1);
				} else {
					LOG_ERR(
						"Timedout waiting for response on request %d",
						request_counter);
					return -EIO;
				}
			}
		}

		if (!ReadVideoCoreMemory(
				display_vc_handle,
				&display_local,
				display_vc_addr,
				sizeof(display_local)) != 0) {
			LOG_ERR(
				"Error reading %d bytes from vc addr 0x%08x",
				sizeof(display_local), display_vc_addr);
			return -EIO;
		}
	}

	return rc;
}

/* extern char Capri_FB_capture_buffer[]; */
/* char *vc_fb_snapshot = Capri_FB_capture_buffer; */
char *vc_fb_snapshot;
static int vc_fb_snapshot_allocated;
static int vc_fb_snapshot_size;
void vc_framebuffer_snapshot(int action)
{
	int rc;

	/* one shot 'framebuffer' snapshot functionality.
	 */
	display_local.command =
		action ? DISPLAY_IPC_SNAP_TAKE : DISPLAY_IPC_SNAP_TERM;
	display_local.display_number = 0;

	LOG_INFO("[%s]: vc_framebuffer_snapshot(%s)",
		__func__, action ? "snap" : "free");

	/* issue command to VC via doorbell isr, cannot block as
	 * this is possibly called from isr context.
	 */
	rc = vc_display_send(1, 500);
	if (rc == 0) {
		LOG_INFO("[%s]: vc_display_send(%d)",
			__func__, rc);
		if (display_local.response.take_snap.buffer_size &&
			display_local.response.take_snap.data_buffer) {
			LOG_INFO("[%s]: snapshot is at %p, size %u",
				__func__,
				display_local.response.take_snap.data_buffer,
				display_local.response.take_snap.buffer_size);
			if (action && vc_fb_snapshot == NULL) {
				vc_fb_snapshot =
					kzalloc(sizeof(uint8_t)*
						display_local.response.
							take_snap.buffer_size,
						GFP_KERNEL);
				if (vc_fb_snapshot)
					vc_fb_snapshot_allocated = 1;
			}

			if (action && vc_fb_snapshot) {
				VC_MEM_ADDR_T vc_buf_addr =
					(VC_MEM_ADDR_T)display_local.
						response.take_snap.data_buffer;
				LOG_INFO(
					"[%s]: copy sz:%u - %p -> %p",
					__func__,
					display_local.response.
						take_snap.buffer_size,
					display_local.response.
						take_snap.data_buffer,
					vc_fb_snapshot);
				vc_fb_snapshot_size =
					display_local.response.
						take_snap.buffer_size;
				if (!ReadVideoCoreMemory(display_vc_handle,
					(void *)vc_fb_snapshot,
					vc_buf_addr,
					display_local.response.
						take_snap.buffer_size) != 0)
					LOG_ERR(
						"[%s]: error reading %d bytes from vc addr %p",
						__func__,
						display_local.response.
							take_snap.buffer_size,
						display_local.response.
							take_snap.data_buffer);
			} else if (!action) {
				if (vc_fb_snapshot_allocated) {
					kfree(vc_fb_snapshot);
					vc_fb_snapshot_allocated = 0;
				}
		      CloseVideoCoreMemory(display_vc_handle);
			}
		}
	}

	return;
}
EXPORT_SYMBOL(vc_framebuffer_snapshot);

#define DEFAULT_STATIC_FB_DUMP	 "/sdcard/fb-dump"
static void vc_framebuffer_snapshot_dump(void)
{
	int fd = -1;
	int write_len;
	mm_segment_t camd_fs;

	camd_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = sys_open(DEFAULT_STATIC_FB_DUMP, O_WRONLY|O_CREAT, 0);
	if (fd >= 0) {
		write_len = sys_write(fd,
			vc_fb_snapshot, vc_fb_snapshot_size);
		sys_close(fd);
		LOG_INFO("[%s]: write %u bytes",
			__func__, write_len);
	}

	set_fs(camd_fs);
	return;
}

static int z_order_read_proc(char *buffer,
			     char **start,
			     off_t off, int count, int *eof, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	int len = 0;

	(void)start;
	(void)off;
	(void)count;
	(void)eof;

	if (scrn_info != NULL) {
		len += sprintf(buffer + len,
			       "%u\n", (unsigned int)scrn_info->z_order);
	}

	return len;
}

static int scale_read_proc(char *buffer,
			   char **start,
			   off_t off, int count, int *eof, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	int len = 0;

	(void)start;
	(void)off;
	(void)count;
	(void)eof;

	if (scrn_info != NULL) {
		len += sprintf(buffer + len,
			       "%u\n", (unsigned int)scrn_info->scale);
	}

	return len;
}

static int res_override_read_proc(char *buffer,
				  char **start,
				  off_t off, int count, int *eof, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	int len = 0;

	(void)start;
	(void)off;
	(void)count;
	(void)eof;

	if (scrn_info != NULL) {
		len += sprintf(buffer + len,
			       "%ux%u\n",
			       (unsigned int)scrn_info->width_override,
			       (unsigned int)scrn_info->height_override);
	}

	return len;
}

static int keep_resource_read_proc(char *buffer,
				   char **start,
				   off_t off, int count, int *eof, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	int len = 0;

	(void)start;
	(void)off;
	(void)count;
	(void)eof;

	if (scrn_info != NULL) {
		len += sprintf(buffer + len,
			       "%u\n", (unsigned int)scrn_info->keep_resource);
	}

	return len;
}

static int bpp_override_read_proc(char *buffer,
				  char **start,
				  off_t off, int count, int *eof, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	int len = 0;

	(void)start;
	(void)off;
	(void)count;
	(void)eof;

	if (scrn_info != NULL) {
		len += sprintf(buffer + len,
			       "%u\n", (unsigned int)scrn_info->bpp_override);
	}

	return len;
}

static int alpha_per_pixel_read_proc(char *buffer,
				     char **start,
				     off_t off, int count, int *eof, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	int len = 0;

	(void)start;
	(void)off;
	(void)count;
	(void)eof;

	if (scrn_info != NULL) {
		len += sprintf(buffer + len,
			       "%u\n",
			       (unsigned int)scrn_info->alpha_per_pixel);
	}

	return len;
}

static int alpha_read_proc(char *buffer,
			   char **start,
			   off_t off, int count, int *eof, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	int len = 0;

	(void)start;
	(void)off;
	(void)count;
	(void)eof;

	if (scrn_info != NULL) {
		len += sprintf(buffer + len,
			       "%u\n", (unsigned int)scrn_info->alpha);
	}

	return len;
}

static int adjust_read_proc(char *buffer,
			     char **start,
			     off_t off, int count, int *eof, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	int len = 0;

	(void)start;
	(void)off;
	(void)count;
	(void)eof;

	if (scrn_info != NULL) {
		len += sprintf(buffer + len,
			       "%u\n", (unsigned int)scrn_info->adjust);
	}

	return len;
}

static ssize_t vc_fb_kread(struct SCRN_INFO_T *scrn_info, char *buf,
			   size_t count);
static int action_write_proc(struct file *file, const char __user *buffer,
			     unsigned long count, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	unsigned char kbuf[PROC_WRITE_BUF_SIZE + 1];
	int ret;
	ssize_t read_size = 0;
	void *buf = NULL;

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf, buffer, count) != 0) {
		LOG_ERR("[%s]: failed to copy-from-user", __func__);

		ret = -EFAULT;
		goto out;
	}
	kbuf[count - 1] = 0;
	ret = count;

	if (scrn_info != NULL) {
		if (!strcmp(kbuf, "read")) {
			VC_FB_SCRN_INFO_T info;
			/* Get the framebuffer configuration first, then read.
			 */
			vc_vchi_fb_get_scrn_info(fb_state->fb_handle,
						 scrn_info->scrn, &info);
			LOG_INFO("[%s]: xres: %d, yres: %d, bpp: %d",
				 __func__,
				 info.width,
				 info.height, (info.bits_per_pixel >> 3));

			count = info.width *
			    info.height * (info.bits_per_pixel >> 3);
			buf = kzalloc(count * sizeof(uint8_t), GFP_KERNEL);
			read_size = vc_fb_kread(scrn_info, buf, count);
			kfree(buf);
			LOG_INFO("[%s]: read %u bytes", __func__, read_size);
		} else if (!strcmp(kbuf, "read-locked")) {
			uint32_t fb_width, fb_height, fb_frame;
			if (!vc_dt_get_fb_config(&fb_width,
					&fb_height, &fb_frame)) {
				/* Assume we cannot get the configuration data,
				 * use some acceptable default and try to read.
				 */
				LOG_INFO(
					"[%s]: **locked** xres: %d, yres: %d, bpp: %d",
					 __func__,
					 fb_width,
					 fb_height,
					 (DEFAULT_BITS_PER_PIXEL >> 3));

				count = fb_width * fb_height *
					 (DEFAULT_BITS_PER_PIXEL >> 3);
				buf =
					kzalloc(count * sizeof(uint8_t),
						GFP_KERNEL);
				read_size = vc_fb_kread(scrn_info, buf, count);
				kfree(buf);
				LOG_INFO("[%s]: **locked** read %u bytes",
					 __func__, read_size);
			}
		} else if (!strcmp(kbuf, "snap-take")) {
			LOG_INFO("[%s]: take snapshot",
				 __func__);
			vc_framebuffer_snapshot(1);
		} else if (!strcmp(kbuf, "snap-term")) {
			LOG_INFO("[%s]: free snapshot",
				 __func__);
			vc_framebuffer_snapshot(0);
		} else if (!strcmp(kbuf, "snap-dump")) {
			LOG_INFO("[%s]: dump snapshot",
				 __func__);
			vc_framebuffer_snapshot_dump();
		}
	}

	goto out;

out:
	return ret;
}

static int z_order_write_proc(struct file *file,
	const char __user *buffer, unsigned long count, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	unsigned char kbuf[PROC_WRITE_BUF_SIZE + 1];
	int ret;

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf, buffer, count) != 0) {
		LOG_ERR("[%s]: failed to copy-from-user", __func__);

		ret = -EFAULT;
		goto out;
	}
	kbuf[count - 1] = 0;
	ret = count;

	if (scrn_info != NULL) {
		ret = kstrtou32(kbuf, 0, &scrn_info->z_order);
		if (ret != 0)
			scrn_info->z_order = 0;
		/* reset - satisfies the proc write op. */
		ret = count;
	}

	goto out;

out:
	return ret;
}

static int scale_write_proc(struct file *file,
			    const char __user *buffer,
			    unsigned long count, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	unsigned char kbuf[PROC_WRITE_BUF_SIZE + 1];
	int ret;

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf, buffer, count) != 0) {
		LOG_ERR("[%s]: failed to copy-from-user", __func__);

		ret = -EFAULT;
		goto out;
	}
	kbuf[count - 1] = 0;
	ret = count;

	if (scrn_info != NULL) {
		ret = kstrtou32(kbuf, 0, &scrn_info->scale);
		if (ret == 0)
			scrn_info->scale = !(!(scrn_info->scale));
		/* reset - satisfies the proc write op. */
		ret = count;
	}

	goto out;

out:
	return ret;
}

static int res_override_write_proc(struct file *file,
				   const char __user *buffer,
				   unsigned long count, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	unsigned char kbuf[PROC_WRITE_BUF_SIZE + 1];
	int ret;

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf, buffer, count) != 0) {
		LOG_ERR("[%s]: failed to copy-from-user", __func__);

		ret = -EFAULT;
		goto out;
	}
	kbuf[count - 1] = 0;
	ret = count;

	if (scrn_info != NULL) {
		uint32_t width, height;

		if (sscanf(kbuf, "%ux%u", &width, &height) != 2) {
			LOG_ERR("%s: invalid override resolution", __func__);
		} else {
			scrn_info->width_override = width;
			scrn_info->height_override = height;
		}
	}
	goto out;

out:
	return ret;
}

static int keep_resource_write_proc(struct file *file,
				    const char __user *buffer,
				    unsigned long count, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	unsigned char kbuf[PROC_WRITE_BUF_SIZE + 1];
	int ret;

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf, buffer, count) != 0) {
		LOG_ERR("[%s]: failed to copy-from-user", __func__);

		ret = -EFAULT;
		goto out;
	}
	kbuf[count - 1] = 0;
	ret = count;

	if (scrn_info != NULL) {
		ret = kstrtou32(kbuf, 0, &scrn_info->keep_resource);
		if (ret == 0)
			scrn_info->keep_resource =
			    !(!(scrn_info->keep_resource));
		/* reset - satisfies the proc write op. */
		ret = count;
	}

	goto out;

out:
	return ret;
}

static int bpp_override_write_proc(struct file *file,
				   const char __user *buffer,
				   unsigned long count, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	unsigned char kbuf[PROC_WRITE_BUF_SIZE + 1];
	int ret;

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf, buffer, count) != 0) {
		LOG_ERR("[%s]: failed to copy-from-user", __func__);

		ret = -EFAULT;
		goto out;
	}
	kbuf[count - 1] = 0;
	ret = count;

	if (scrn_info != NULL) {
		uint32_t input = 0;
		ret = kstrtou32(kbuf, 0, &input);
		if ((ret != 0) || ((input != 0) &&
				   (input != 16) && (input != 32))) {
			LOG_ERR("%s: invalid bits per pixel override value",
				__func__);
		} else
			scrn_info->bpp_override = input;
		/* reset - satisfies the proc write op. */
		ret = count;
	}
	goto out;

out:
	return ret;
}

static int alpha_per_pixel_write_proc(struct file *file,
				      const char __user *buffer,
				      unsigned long count, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	unsigned char kbuf[PROC_WRITE_BUF_SIZE + 1];
	int ret;

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf, buffer, count) != 0) {
		LOG_ERR("[%s]: failed to copy-from-user", __func__);

		ret = -EFAULT;
		goto out;
	}
	kbuf[count - 1] = 0;
	ret = count;

	if (scrn_info != NULL) {
		ret = kstrtou32(kbuf, 0, &scrn_info->alpha_per_pixel);
		if (ret == 0)
			scrn_info->alpha_per_pixel =
			    !(!(scrn_info->alpha_per_pixel));
		/* reset - satisfies the proc write op. */
		ret = count;
	}
	goto out;

out:
	return ret;
}

static int alpha_write_proc(struct file *file,
			    const char __user *buffer,
			    unsigned long count, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	unsigned char kbuf[PROC_WRITE_BUF_SIZE + 1];
	int ret;

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf, buffer, count) != 0) {
		LOG_ERR("[%s]: failed to copy-from-user", __func__);

		ret = -EFAULT;
		goto out;
	}
	kbuf[count - 1] = 0;
	ret = count;

	if (scrn_info != NULL) {
		ret = kstrtou32(kbuf, 0, &scrn_info->alpha);
		if ((ret != 0) || (scrn_info->alpha > 255))
			scrn_info->alpha = 255;
		/* reset - satisfies the proc write op. */
		ret = count;
	}
	goto out;

out:
	return ret;
}

static int adjust_write_proc(struct file *file,
	const char __user *buffer, unsigned long count, void *data)
{
	struct SCRN_INFO_T *scrn_info = (struct SCRN_INFO_T *)data;
	unsigned char kbuf[PROC_WRITE_BUF_SIZE + 1];
	int ret;

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf, buffer, count) != 0) {
		LOG_ERR("[%s]: failed to copy-from-user", __func__);

		ret = -EFAULT;
		goto out;
	}
	kbuf[count - 1] = 0;
	ret = count;

	if (scrn_info != NULL) {
		ret = kstrtou32(kbuf, 0, &scrn_info->adjust);
		if (ret != 0)
			scrn_info->adjust = DEFAULT_ADJUST;
		/* reset - satisfies the proc write op. */
		ret = count;
	}

	goto out;

out:
	return ret;
}

static inline struct SCRN_INFO_T *to_scrn_info(struct fb_info *fb_info)
{
	return container_of(fb_info, struct SCRN_INFO_T, fb_info);
}

static inline uint32_t convert_bitfield(int val, struct fb_bitfield *bf)
{
	unsigned int mask = (1 << bf->length) - 1;

	return (val >> (16 - bf->length) & mask) << bf->offset;
}

static int vc_fb_get_info(struct SCRN_INFO_T *scrn_info)
{
	int ret;
	int32_t success;
	VC_FB_SCRN_INFO_T info;
	uint32_t fb_width = 0, fb_height = 0, fb_frame = 0;

	LOG_DBG("%s: start (scrn_info=0x%p)", __func__, scrn_info);

	/* Get the (default) framebuffer configuration from the
	 * dt-blob.
	 */
	vc_dt_get_fb_config(&fb_width, &fb_height, &fb_frame);

	/* Get the screen info from the framebuffer service */
	success = vc_vchi_fb_get_scrn_info(fb_state->fb_handle, scrn_info->scrn,
					   &info);
	if (success != 0) {
		LOG_ERR("%s: failed to get info for screen %u (success=%d)",
			__func__, scrn_info->scrn, success);

		ret = -EPERM;
		goto out;
	} else if ((info.width == 0) || (info.height == 0)) {
		LOG_DBG(
			"%s: could not get info for screen %u - using defaults",
			__func__, scrn_info->scrn);

		info.width = fb_width;
		info.height = fb_height;
	}

	if (info.bits_per_pixel == 0) {
		LOG_DBG("%s: using default bits per pixel '%u'", __func__,
			DEFAULT_BITS_PER_PIXEL);

		info.bits_per_pixel = DEFAULT_BITS_PER_PIXEL;
	}
	if (scrn_info->bpp_override != 0) {
		LOG_DBG("%s: using bits per pixel override '%u'", __func__,
			scrn_info->bpp_override);

		info.bits_per_pixel = scrn_info->bpp_override;
	}
	/* Apply any overrides here */
	if (scrn_info->width_override != 0)
		info.width = scrn_info->width_override;

	if (scrn_info->height_override != 0)
		info.height = scrn_info->height_override;

	if (scrn_info->adjust &&
			((info.width % 16) || (info.height % 16))) {
		/* Videocore needs the dimensions to be a multiple of 16 */
		info.width &= ~15;
		info.height &= ~15;

		LOG_INFO("%s: screen %u: adjusted to %ux%u, %u bpp", __func__,
			scrn_info->scrn, info.width, info.height,
			info.bits_per_pixel);
	}

	LOG_DBG("%s: screen %u: %ux%u, %u bpp", __func__, scrn_info->scrn,
		info.width, info.height, info.bits_per_pixel);

	/* Save the info into struct fb_var_screeninfo */
	scrn_info->fb_info.var.xres = info.width;
	scrn_info->fb_info.var.yres = info.height;
	scrn_info->fb_info.var.xres_virtual = info.width;
	scrn_info->fb_info.var.yres_virtual = info.height * fb_frame;
	scrn_info->fb_info.var.bits_per_pixel = info.bits_per_pixel;
	scrn_info->fb_info.var.activate = FB_ACTIVATE_NOW;
	scrn_info->fb_info.var.height = info.height;
	scrn_info->fb_info.var.width = info.width;

	if (scrn_info->fb_info.var.bits_per_pixel == 16) {
		scrn_info->fb_info.var.red.offset = 11;
		scrn_info->fb_info.var.red.length = 5;
		scrn_info->fb_info.var.green.offset = 5;
		scrn_info->fb_info.var.green.length = 6;
		scrn_info->fb_info.var.blue.offset = 0;
		scrn_info->fb_info.var.blue.length = 5;
	} else {
		scrn_info->fb_info.var.red.offset = 16;
		scrn_info->fb_info.var.red.length = 8;
		scrn_info->fb_info.var.green.offset = 8;
		scrn_info->fb_info.var.green.length = 8;
		scrn_info->fb_info.var.blue.offset = 0;
		scrn_info->fb_info.var.blue.length = 8;
		scrn_info->fb_info.var.transp.offset = 24;
		scrn_info->fb_info.var.transp.length = 8;
	}

	ret = fb_set_var(&scrn_info->fb_info, &scrn_info->fb_info.var);
	if (ret != 0) {
		LOG_ERR("%s: fb_set_var failed (ret=%d)", __func__, ret);

		goto out;
	}

out:
	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return ret;
}

static int vc_fb_open(struct fb_info *fb_info, int user)
{
	int ret = 0;
	struct SCRN_INFO_T *scrn_info = to_scrn_info(fb_info);
	struct resource *res;

	LOG_DBG("%s: start (fb_info=0x%p, user=%d)", __func__, fb_info, user);

	mutex_lock(&scrn_info->user_cnt_mutex);

	LOG_DBG("%s: scrn_info->user_cnt=%u", __func__, scrn_info->user_cnt);

	/* Only allocate the framebuffer if its the first user AND
	 *      we do not already have one allocated
	 */
	if ((scrn_info->user_cnt == 0) && (scrn_info->res_handle == 0)) {
		int32_t success;
		VC_FB_ALLOC_T alloc;
		VC_FB_ALLOC_RESULT_T alloc_result;
		uint32_t vc_addr;

		ret = vc_fb_get_info(scrn_info);
		if (ret != 0) {
			LOG_ERR("%s: failed to get info for screen %u (ret=%d)",
				__func__, scrn_info->scrn, ret);

			goto out;
		}

		alloc.scrn = scrn_info->scrn;
		alloc.width = fb_info->var.xres;
		alloc.height = fb_info->var.yres;
		alloc.bits_per_pixel = fb_info->var.bits_per_pixel;
		alloc.num_frames =
			fb_info->var.yres_virtual / fb_info->var.yres;
		alloc.layer = scrn_info->z_order;
		alloc.alpha_per_pixel = scrn_info->alpha_per_pixel;
		alloc.default_alpha = scrn_info->alpha;
		alloc.scale = scrn_info->scale;
		alloc.nopad = scrn_info->adjust ? 0 : 1;
		alloc.alloc =
			VC_FB_ALLOC_INTERNAL_HEAP | VC_FB_ALLOC_ALLOW_FALLBACK;

		LOG_INFO("%s: allocating framebuffer for screen %u", __func__,
			 alloc.scrn);
		LOG_INFO
		    ("%s:\t%ux%u, bpp=%u, num_frames=%u, z-order=%u, scale=%u",
		     __func__, alloc.width, alloc.height, alloc.bits_per_pixel,
		     alloc.num_frames, alloc.layer, alloc.scale);
		LOG_INFO("%s:\talpha_per_pixel=%u, default_alpha=%u, nopad=%u",
		    __func__, alloc.alpha_per_pixel, alloc.default_alpha,
		    alloc.nopad);

		/* Allocate memory for the framebuffer */
		success =
		    vc_vchi_fb_alloc(fb_state->fb_handle, &alloc,
				     &alloc_result);
		if ((success != 0) || (alloc_result.res_handle == 0)) {
			LOG_ERR
			    ("%s: failed to allocate framebuffer (%d)",
			     __func__, success);

			ret = -ENOMEM;
			goto out;
		}

		LOG_DBG
		    ("%s: alloc_result: res_handle=0x%08x, res_mem=0x%p,"
		     " line_bytes=%u, frame_bytes=%u",
		     __func__, alloc_result.res_handle, alloc_result.res_mem,
		     alloc_result.line_bytes, alloc_result.frame_bytes);

		/* Save the resource handle */
		scrn_info->res_handle = alloc_result.res_handle;

		vc_addr = (uint32_t)alloc_result.res_mem & 0x3FFFFFFF;

		/* Request an I/O memory region for remapping */
		res = request_mem_region(vc_addr + mm_vc_mem_phys_addr,
					 alloc_result.frame_bytes *
					 alloc.num_frames, "vc_fb");
		if (res == NULL) {
			LOG_ERR("%s: failed to request I/O memory region",
				__func__);

			ret = -EIO;
			goto err_free_fb;
		}
		/* I/O remap the framebuffer */
		fb_info->screen_base =
		    ioremap_nocache(res->start, resource_size(res));
		if (fb_info->screen_base == NULL) {
			LOG_ERR("%s: failed to I/O remap framebuffer",
				__func__);

			ret = -ENOMEM;
			goto err_release_mem_region;
		}
		/* Fill out the rest of the framebuffer info */
		fb_info->fix.smem_start = res->start;
		fb_info->fix.smem_len = resource_size(res);
		fb_info->fix.line_length = alloc_result.line_bytes;

		LOG_DBG
		    ("%s: screen_base=0x%p, smem_start=0x%08x,"
		     " smem_len=%u, line_length=%u",
		     __func__, fb_info->screen_base,
		     (uint32_t)fb_info->fix.smem_start, fb_info->fix.smem_len,
		     fb_info->fix.line_length);
		LOG_DBG("%s: virt_to_phys=0x%p", __func__,
			(void *)virt_to_phys(fb_info->screen_base));
	}
	/* Increase the user count by one */
	scrn_info->user_cnt++;

	goto out;

err_release_mem_region:
	release_mem_region(res->start, resource_size(res));

err_free_fb:
	vc_vchi_fb_free(fb_state->fb_handle, scrn_info->res_handle);
	scrn_info->res_handle = 0;

out:
	mutex_unlock(&scrn_info->user_cnt_mutex);

	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return ret;
}

static int vc_fb_release(struct fb_info *fb_info, int user)
{
	int ret = 0;
	struct SCRN_INFO_T *scrn_info = to_scrn_info(fb_info);

	LOG_DBG("%s: start (fb_info=0x%p, user=%d)", __func__, fb_info, user);

	mutex_lock(&scrn_info->user_cnt_mutex);

	LOG_DBG("%s: scrn_info->user_cnt=%u", __func__, scrn_info->user_cnt);

	/* Only free the framebuffer if there are no more users AND
	 * we do not wantto keep the resource around
	 */
	if ((--scrn_info->user_cnt == 0) && (scrn_info->keep_resource == 0)) {
		int32_t success;

		LOG_DBG("%s: freeing videocore framebuffer", __func__);

		/* Unmap the videocore memory */
		iounmap(fb_info->screen_base);
		fb_info->screen_base = NULL;

		/* Release the I/O memory region */
		release_mem_region(fb_info->fix.smem_start,
				   fb_info->fix.smem_len);
		fb_info->fix.smem_start = 0;
		fb_info->fix.smem_len = 0;

		success =
		    vc_vchi_fb_free(fb_state->fb_handle, scrn_info->res_handle);
		if (success != 0) {
			LOG_ERR("%s: failed to free framebuffer (success=%d)",
				__func__, success);

			/* Even if we failed to release it, we should
			 * continue on as if it succeeded - this might
			 * lead to memory leaks on the videocore!!
			 */
		}

		scrn_info->res_handle = 0;
	}

	mutex_unlock(&scrn_info->user_cnt_mutex);

	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return ret;
}

static int vc_fb_check_var(struct fb_var_screeninfo *var,
			   struct fb_info *fb_info)
{
	int ret = 0;

	LOG_DBG("%s: start (var=0x%p, fb_info=0x%p)", __func__, var, fb_info);

	/* Check for parameters that we cannot change */
	if ((var->xoffset != fb_info->var.xoffset) ||
	    (var->grayscale != fb_info->var.grayscale)) {
		ret = -EINVAL;
		goto out;
	}
	/* Handle bit depth changes - we only handle */
	if (var->bits_per_pixel != fb_info->var.bits_per_pixel) {
		if (var->bits_per_pixel == 16) {
			var->red.offset = 11;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 6;
			var->blue.offset = 0;
			var->blue.length = 5;
		} else if (var->bits_per_pixel == 32) {
			var->red.offset = 16;
			var->red.length = 8;
			var->green.offset = 8;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			var->transp.offset = 24;
			var->transp.length = 8;
		} else {
			LOG_ERR("%s: bit depth of '%u' not supported", __func__,
				var->bits_per_pixel);

			ret = -EINVAL;
			goto out;
		}
	}

	if ((var->xres > fb_info->var.xres) ||
		(var->yres > fb_info->var.yres)) {
		LOG_INFO
		    ("%s: request resolution %ux%u is larger than"
		     " supported %ux%u",
		     __func__, var->xres, var->yres, fb_info->var.xres,
		     fb_info->var.yres);

		ret = -EINVAL;
		goto out;
	}

out:
	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return ret;
}

static int vc_fb_set_par(struct fb_info *fb_info)
{
	int ret = 0;
	int32_t success;
	VC_FB_CFG_T cfg;
	VC_FB_CFG_RESULT_T cfg_result;
	struct SCRN_INFO_T *scrn_info = to_scrn_info(fb_info);

	LOG_DBG("%s: start (fb_info=0x%p)", __func__, fb_info);

	/* TODO Support rotation */

	memset(&cfg, 0, sizeof(cfg));
	cfg.res_handle = scrn_info->res_handle;
	cfg.bits_per_pixel = fb_info->var.bits_per_pixel;
	cfg.alpha_per_pixel = scrn_info->alpha_per_pixel;
	cfg.default_alpha = scrn_info->alpha;

	success = vc_vchi_fb_cfg(fb_state->fb_handle, &cfg, &cfg_result);
	if (success != 0) {
		LOG_ERR("%s: failed to configure framebuffer (success=%d)",
			__func__, success);

		ret = -EPERM;
		goto out;
	}

	LOG_DBG("%s: cfg_result: success=%d, line_bytes=%u, frame_bytes=%u",
		__func__, cfg_result.success, cfg_result.line_bytes,
		cfg_result.frame_bytes);

	/* Update the fixed variables */
	fb_info->fix.line_length = cfg_result.line_bytes;

out:
	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return ret;
}

static int vc_fb_setcolreg(unsigned int regno,
			   unsigned int red,
			   unsigned int green,
			   unsigned int blue,
			   unsigned int transp, struct fb_info *fb_info)
{
	int ret = 0;
	struct SCRN_INFO_T *scrn_info = to_scrn_info(fb_info);

	LOG_DBG
	    ("%s: start (regno=%u, red=%u, green=%u, blue=%u,"
	     " transp=%u, fb_info=0x%p)",
	     __func__, regno, red, green, blue, transp, fb_info);

	/* We only support 16 color registers */
	if (regno < 16) {
		scrn_info->cmap[regno] =
		    convert_bitfield(red, &scrn_info->fb_info.var.red) |
		    convert_bitfield(green, &scrn_info->fb_info.var.green) |
		    convert_bitfield(blue, &scrn_info->fb_info.var.blue) |
		    convert_bitfield(transp, &scrn_info->fb_info.var.transp);
	} else if (regno > 255) {
		LOG_INFO("%s: invalid color register number %u", __func__,
			 regno);

		ret = 1;
	}

	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return ret;
}

static int vc_fb_pan_scrn(struct fb_var_screeninfo *var,
			  struct fb_info *fb_info)
{
	int ret = 0;
	int32_t success;
	struct SCRN_INFO_T *scrn_info = to_scrn_info(fb_info);

	LOG_DBG("%s: start (var=0x%p, fb_info=0x%p)", __func__, var, fb_info);

	success = vc_vchi_fb_pan(fb_state->fb_handle, scrn_info->res_handle,
				 var->yoffset);
	if (success != 0) {
		LOG_ERR("%s: failed to pan (success=%d)", __func__, success);

		ret = -EPERM;
	}

	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return ret;
}

static int vc_fb_read_task(void *arg)
{
	int32_t success;
	int32_t read_size;
	uint32_t read_handle;
	void *buffer;

	while (1) {
		if (!down_interruptible(&fb_state->read_sema)) {

			mutex_lock(&fb_state->read_mutex);
			buffer = fb_state->read_buf;
			read_size = fb_state->read_size;
			read_handle = fb_state->read_handle;
			mutex_unlock(&fb_state->read_mutex);

			LOG_INFO("%s: hndl: %x, size: %u, buf: %p",
				 __func__, read_handle, read_size, buffer);

			success = vc_vchi_fb_read(fb_state->fb_handle,
						  read_handle,
						  buffer, read_size);

			mutex_lock(&fb_state->read_mutex);
			fb_state->read_status = success;
			mutex_unlock(&fb_state->read_mutex);

			up(&fb_state->read_resp_sema);
		}
	}

	return 0;
}

static ssize_t vc_fb_kread(struct SCRN_INFO_T *scrn_info, char *buf,
			   size_t count)
{
	int ret = 0;
	int32_t success;

	/* Wake-up thread for reading, wait for response.
	 */
	mutex_lock(&fb_state->read_mutex);
	fb_state->read_handle = scrn_info->res_handle;
	fb_state->read_size = count;
	fb_state->read_buf = buf;
	mutex_unlock(&fb_state->read_mutex);

	up(&fb_state->read_sema);
	if (down_timeout(&fb_state->read_resp_sema,
			 msecs_to_jiffies(DEFAULT_READ_TO_MSEC)) != 0) {
		LOG_ERR("%s: timeout waiting for read", __func__);
		return -EPERM;
	} else {
		mutex_lock(&fb_state->read_mutex);
		success = fb_state->read_status;
		mutex_unlock(&fb_state->read_mutex);
	}

	if (success != 0) {
		LOG_ERR("%s: failed to read (success=%d)", __func__, success);
		ret = -EPERM;
	} else
		ret = count;

	return ret;
}

static ssize_t vc_fb_read(struct fb_info *info, char __user *buf, size_t count,
			  loff_t *ppos)
{
	int ret = 0;
	int32_t success, size;
	struct SCRN_INFO_T *scrn_info = to_scrn_info(info);
	uint8_t *kbuf;

	LOG_INFO("%s: fb_info=%x scrn_info=%x buf=%x count=%d *ppos=%d ",
		 __func__, (int)info, (int)scrn_info,
		 (int)buf, count, (int)*ppos);
	LOG_INFO("%s: scrn_info->res_handle=%x",
		 __func__, scrn_info->res_handle);

	if (!info)
		return -ENODEV;

	size = info->var.xres *
	    info->var.yres * (info->var.bits_per_pixel >> 3);

	if (!access_ok(VERIFY_WRITE, buf, size))
		return -EFAULT;

	if (size != count) {
		LOG_ERR("%s: input buffer size (%d) is not full frame size(%d)",
			__func__, count, size);
		return -EFAULT;
	}

	kbuf = kzalloc(size * sizeof(uint8_t), GFP_KERNEL);
	if (kbuf == NULL) {
		LOG_ERR("%s: failed to allocate buffer size %d",
			__func__, size);
		return -EFAULT;
	}

	success = vc_fb_kread(scrn_info, kbuf, size);
	if (success != size) {
		LOG_ERR("%s: failed to read (success=%d)", __func__, success);
		ret = -EPERM;
	} else {
		if (copy_to_user(buf, kbuf, size)) {
			LOG_ERR("%s: failed to copy data back", __func__);
			ret = -EPERM;
		} else
			ret = size;
	}

	kfree(kbuf);

	LOG_INFO("%s: ret=%d success=%d", __func__, ret, success);
	return ret;
}

static struct fb_ops vc_fb_ops = {
	.owner = THIS_MODULE,
	.fb_open = vc_fb_open,
	.fb_release = vc_fb_release,
	.fb_check_var = vc_fb_check_var,
	.fb_set_par = vc_fb_set_par,
	.fb_setcolreg = vc_fb_setcolreg,
	.fb_pan_display = vc_fb_pan_scrn,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_read = vc_fb_read,
};

static int vc_fb_create_per_scrn_proc_entries(struct SCRN_INFO_T *scrn_info)
{
	int ret;

	LOG_DBG("%s: start (scrn_info=0x%p)", __func__, scrn_info);

	/* First create a '<x>' proc directory under 'vc-fb' */
	scrn_info->fb_cfg_directory =
	    proc_mkdir(fb_cfg_dir_name[scrn_info->scrn],
		       fb_state->cfg_directory);
	if (scrn_info->fb_cfg_directory == NULL) {
		LOG_ERR("%s: failed to create proc directory entry", __func__);

		ret = -EPERM;
		goto out;
	}
	/* Now create all the proc entries for modifiable parameters */
	scrn_info->alpha_cfg_entry = create_proc_entry("alpha",
		0,
		scrn_info->fb_cfg_directory);
	if (scrn_info->alpha_cfg_entry == NULL) {
		LOG_ERR("%s: failed to create proc entry", __func__);

		ret = -EPERM;
		goto err_remove_fb_cfg_directory;
	} else {
		scrn_info->alpha_cfg_entry->data = (void *)scrn_info;
		scrn_info->alpha_cfg_entry->read_proc = &alpha_read_proc;
		scrn_info->alpha_cfg_entry->write_proc = &alpha_write_proc;
	}

	scrn_info->alpha_per_pixel_cfg_entry =
	    create_proc_entry("alpha_per_pixel", 0,
			      scrn_info->fb_cfg_directory);
	if (scrn_info->alpha_per_pixel_cfg_entry == NULL) {
		LOG_ERR("%s: failed to create proc entry", __func__);

		ret = -EPERM;
		goto err_remove_alpha_cfg_entry;
	} else {
		scrn_info->alpha_per_pixel_cfg_entry->data = (void *)scrn_info;
		scrn_info->alpha_per_pixel_cfg_entry->read_proc =
		    &alpha_per_pixel_read_proc;
		scrn_info->alpha_per_pixel_cfg_entry->write_proc =
		    &alpha_per_pixel_write_proc;
	}

	scrn_info->bpp_override_cfg_entry = create_proc_entry("bpp_override",
		0,
		scrn_info->fb_cfg_directory);
	if (scrn_info->bpp_override_cfg_entry == NULL) {
		LOG_ERR("%s: failed to create proc entry", __func__);

		ret = -EPERM;
		goto err_remove_alpha_per_pixel_cfg_entry;
	} else {
		scrn_info->bpp_override_cfg_entry->data = (void *)scrn_info;
		scrn_info->bpp_override_cfg_entry->read_proc =
		    &bpp_override_read_proc;
		scrn_info->bpp_override_cfg_entry->write_proc =
		    &bpp_override_write_proc;
	}

	scrn_info->keep_resource_cfg_entry =
	    create_proc_entry("keep_resource", 0, scrn_info->fb_cfg_directory);
	if (scrn_info->keep_resource_cfg_entry == NULL) {
		LOG_ERR("%s: failed to create proc entry", __func__);

		ret = -EPERM;
		goto err_remove_bpp_override_cfg_entry;
	} else {
		scrn_info->keep_resource_cfg_entry->data = (void *)scrn_info;
		scrn_info->keep_resource_cfg_entry->read_proc =
		    &keep_resource_read_proc;
		scrn_info->keep_resource_cfg_entry->write_proc =
		    &keep_resource_write_proc;
	}

	scrn_info->res_override_cfg_entry =
	    create_proc_entry("res_override", 0, scrn_info->fb_cfg_directory);
	if (scrn_info->res_override_cfg_entry == NULL) {
		LOG_ERR("%s: failed to create proc entry", __func__);

		ret = -EPERM;
		goto err_remove_keep_res_cfg_entry;
	} else {
		scrn_info->res_override_cfg_entry->data = (void *)scrn_info;
		scrn_info->res_override_cfg_entry->read_proc =
		    &res_override_read_proc;
		scrn_info->res_override_cfg_entry->write_proc =
		    &res_override_write_proc;
	}

	scrn_info->scale_cfg_entry =
	    create_proc_entry("scale", 0, scrn_info->fb_cfg_directory);
	if (scrn_info->scale_cfg_entry == NULL) {
		LOG_ERR("%s: failed to create proc entry", __func__);

		ret = -EPERM;
		goto err_remove_res_override_cfg_entry;
	} else {
		scrn_info->scale_cfg_entry->data = (void *)scrn_info;
		scrn_info->scale_cfg_entry->read_proc = &scale_read_proc;
		scrn_info->scale_cfg_entry->write_proc = &scale_write_proc;
	}

	scrn_info->z_order_cfg_entry = create_proc_entry("z_order",
		0,
		scrn_info->fb_cfg_directory);
	if (scrn_info->z_order_cfg_entry == NULL) {
		LOG_ERR("%s: failed to create proc entry", __func__);

		ret = -EPERM;
		goto err_remove_scale_cfg_entry;
	} else {
		scrn_info->z_order_cfg_entry->data = (void *)scrn_info;
		scrn_info->z_order_cfg_entry->read_proc = &z_order_read_proc;
		scrn_info->z_order_cfg_entry->write_proc = &z_order_write_proc;
	}

	scrn_info->action_cfg_entry = create_proc_entry("action",
		0,
		scrn_info->fb_cfg_directory);
	if (scrn_info->action_cfg_entry == NULL) {
		LOG_ERR("%s: failed to create proc entry", __func__);

		ret = -EPERM;
		goto err_remove_z_order_cfg_entry;
	} else {
		scrn_info->action_cfg_entry->data = (void *)scrn_info;
		scrn_info->action_cfg_entry->read_proc = NULL;
		scrn_info->action_cfg_entry->write_proc = &action_write_proc;
	}

	scrn_info->adjust_cfg_entry = create_proc_entry("adjust",
		0,
		scrn_info->fb_cfg_directory);
	if (scrn_info->adjust_cfg_entry == NULL) {
		LOG_ERR("%s: failed to create proc entry", __func__);

		ret = -EPERM;
		goto err_remove_action_cfg_entry;
	} else {
		scrn_info->adjust_cfg_entry->data = (void *)scrn_info;
		scrn_info->adjust_cfg_entry->read_proc = &adjust_read_proc;
		scrn_info->adjust_cfg_entry->write_proc = &adjust_write_proc;
	}

	ret = 0;
	goto out;

err_remove_action_cfg_entry:
	remove_proc_entry("action", scrn_info->fb_cfg_directory);

err_remove_z_order_cfg_entry:
	remove_proc_entry("z_order", scrn_info->fb_cfg_directory);

err_remove_scale_cfg_entry:
	remove_proc_entry("scale", scrn_info->fb_cfg_directory);

err_remove_res_override_cfg_entry:
	remove_proc_entry("res_override", scrn_info->fb_cfg_directory);

err_remove_keep_res_cfg_entry:
	remove_proc_entry("keep_resource", scrn_info->fb_cfg_directory);

err_remove_bpp_override_cfg_entry:
	remove_proc_entry("bpp_override", scrn_info->fb_cfg_directory);

err_remove_alpha_per_pixel_cfg_entry:
	remove_proc_entry("alpha_per_pixel", scrn_info->fb_cfg_directory);

err_remove_alpha_cfg_entry:
	remove_proc_entry("alpha", scrn_info->fb_cfg_directory);

err_remove_fb_cfg_directory:
	remove_proc_entry(fb_cfg_dir_name[scrn_info->scrn],
			  fb_state->cfg_directory);

out:
	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return ret;
}

static int vc_fb_remove_per_scrn_proc_entries(struct SCRN_INFO_T *scrn_info)
{
	int ret = 0;

	LOG_DBG("%s: start (scrn_info=0x%p)", __func__, scrn_info);

	remove_proc_entry("alpha", scrn_info->fb_cfg_directory);
	remove_proc_entry("alpha_per_pixel", scrn_info->fb_cfg_directory);
	remove_proc_entry("bpp_override", scrn_info->fb_cfg_directory);
	remove_proc_entry("keep_resource", scrn_info->fb_cfg_directory);
	remove_proc_entry("res_override", scrn_info->fb_cfg_directory);
	remove_proc_entry("scale", scrn_info->fb_cfg_directory);
	remove_proc_entry("z_order", scrn_info->fb_cfg_directory);
	remove_proc_entry("action", scrn_info->fb_cfg_directory);
	remove_proc_entry("adjust", scrn_info->fb_cfg_directory);

	remove_proc_entry(fb_cfg_dir_name[scrn_info->scrn],
			  fb_state->cfg_directory);

	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return ret;
}

static int vc_fb_create_framebuffer(VC_FB_SCRN scrn)
{
	int ret;
	struct SCRN_INFO_T *scrn_info;

	LOG_DBG("%s: start (scrn=%u)", __func__, scrn);

	if (fb_state->scrn_info[scrn] != NULL) {
		LOG_ERR("%s: framebuffer already created for screen %u",
			__func__, scrn);

		ret = -EEXIST;
		goto out;
	}
	/* Allocate memory for the screen info */
	scrn_info = kzalloc(sizeof(*scrn_info), GFP_KERNEL);
	if (scrn_info == NULL) {
		LOG_ERR("%s: failed to allocate memory for screen info",
			__func__);

		ret = -ENOMEM;
		goto out;
	}

	scrn_info->scrn = scrn;

	/* Create the user count mutex */
	mutex_init(&scrn_info->user_cnt_mutex);

	/*
	 * Fill in most of the information in fb_info,
	 * and leave out the following information that will
	 * become available later on when we ask for it from
	 * the framebuffer service:
	 *    - framebuffer memory address and length
	 *    - resolution and bpp
	 * We do not ask the framebuffer service for that
	 * information now because we want the allow the user to
	 * override the resolution via the proc entries
	 * prior to invoking fb_open.
	 */

	scrn_info->fb_info.fbops = &vc_fb_ops;
	scrn_info->fb_info.flags = FBINFO_FLAG_DEFAULT;
	scrn_info->fb_info.pseudo_palette = scrn_info->cmap;

	/* struct fb_fix_screeninfo */
	strncpy(scrn_info->fb_info.fix.id, "vc_fb",
		sizeof(scrn_info->fb_info.fix.id));
	scrn_info->fb_info.fix.type = FB_TYPE_PACKED_PIXELS;
	scrn_info->fb_info.fix.visual = FB_VISUAL_TRUECOLOR;
	scrn_info->fb_info.fix.xpanstep = 0;
	scrn_info->fb_info.fix.ypanstep = 1;
	scrn_info->fb_info.fix.ywrapstep = 0;
	scrn_info->fb_info.fix.accel = FB_ACCEL_NONE;

	/* struct fb_var_screeninfo */
	scrn_info->fb_info.var.grayscale = 0;
	scrn_info->fb_info.var.nonstd = 0;
	scrn_info->fb_info.var.activate = FB_ACTIVATE_NOW;
	scrn_info->fb_info.var.rotate = 0;
	scrn_info->fb_info.var.vmode = FB_VMODE_NONINTERLACED;

	/* Register framebuffer with the kernel */
	ret = register_framebuffer(&scrn_info->fb_info);
	if (ret != 0) {
		LOG_ERR("%s: register_framebuffer failed (ret=%d)", __func__,
			ret);

		goto err_free_mem;
	}
	/* Create the per screen (framebuffer device) proc entries */
	ret = vc_fb_create_per_scrn_proc_entries(scrn_info);
	if (ret != 0) {
		LOG_ERR("%s: failed to create proc entries (ret=%d)", __func__,
			ret);

		goto err_unregister_framebuffer;
	}
	/* Set the default values for the framebuffer creation
	 * modifiable parameters
	 */
	scrn_info->alpha = DEFAULT_ALPHA;
	scrn_info->alpha_per_pixel = DEFAULT_ALPHA_PER_PIXEL;
	scrn_info->keep_resource = DEFAULT_KEEP_RESOURCE;
	scrn_info->scale = DEFAULT_SCALE;
	scrn_info->z_order = DEFAULT_Z_ORDER;
	scrn_info->adjust = DEFAULT_ADJUST;

	/* Everything is good to go! */
	fb_state->scrn_info[scrn] = scrn_info;

	goto out;

err_unregister_framebuffer:
	unregister_framebuffer(&scrn_info->fb_info);

err_free_mem:
	kfree(scrn_info);

out:
	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return ret;
}

static int vc_fb_remove_framebuffer(VC_FB_SCRN scrn)
{
	struct SCRN_INFO_T *scrn_info = NULL;

	LOG_DBG("%s: start (scrn=%u)", __func__, scrn);

	scrn_info = fb_state->scrn_info[scrn];
	if (scrn_info == NULL) {
		LOG_DBG("%s: no framebuffer device for screen %u", __func__,
			scrn);

		goto out;
	}
	/* Remove per screen (framebuffer device) proc entries */
	vc_fb_remove_per_scrn_proc_entries(scrn_info);

	/* Unregister framebuffer device with the system */
	unregister_framebuffer(&scrn_info->fb_info);

	/* Free the memory used for the screen info */
	kfree(scrn_info);
	fb_state->scrn_info[scrn] = NULL;

out:
	LOG_DBG("%s: end (ret=%d)", __func__, ret);

	return 0;
}

static void vc_fb_connected_init(void)
{
	int ret;
	int i;
	VCHI_INSTANCE_T vchi_instance;
	VCHI_CONNECTION_T *vchi_connection = NULL;

	LOG_INFO("%s: start", __func__);

	/*TODO Check that there is at least one videocore instance */

	/* Allocate memory for the state structure */
	fb_state = kzalloc(sizeof(struct FB_STATE_T), GFP_KERNEL);
	if (fb_state == NULL) {
		LOG_ERR("%s: failed to allocate memory", __func__);

		ret = -ENOMEM;
		goto out;
	}
	/* Initialize and create a VCHI connection */
	ret = vchi_initialise(&vchi_instance);
	if (ret != 0) {
		LOG_ERR("%s: failed to initialise VCHI instance (ret=%d)",
			__func__, ret);

		ret = -EIO;
		goto err_free_mem;
	}
	ret = vchi_connect(NULL, 0, vchi_instance);
	if (ret != 0) {
		LOG_ERR("%s: failed to connect VCHI instance (ret=%d)",
			__func__, ret);

		ret = -EIO;
		goto err_free_mem;
	}
	/* Initialize an instance of the framebuffer service */
	fb_state->fb_handle =
	    vc_vchi_fb_init(vchi_instance, &vchi_connection, 1);
	if (fb_state->fb_handle == NULL) {
		LOG_ERR("%s: failed to initialize framebuffer service",
			__func__);

		ret = -EPERM;
		goto err_free_mem;
	}
	/* Create a proc directory entry */
	fb_state->cfg_directory = proc_mkdir("vc-fb", NULL);
	if (fb_state->cfg_directory == NULL) {
		LOG_ERR("%s: failed to create proc directory entry", __func__);

		ret = -EPERM;
		goto err_stop_fb_service;
	}
	/* Create a framebuffer device for each screen */
	for (i = 0; i < VC_FB_SCRN_MAX; i++) {
		ret = vc_fb_create_framebuffer(i);
		if (ret != 0) {
			LOG_ERR
			    ("%s: failed to create framebuffer screen %u",
			     __func__, i);

			goto err_remove_framebuffer;
		}
	}
	/* Create thread for non-blocking read ops. */
	sema_init(&fb_state->read_sema, 0);
	sema_init(&fb_state->read_resp_sema, 0);
	mutex_init(&fb_state->read_mutex);
	fb_state->read_task = kthread_create(&vc_fb_read_task,
					     (void *)&fb_state, "VC-FB-Read");
	if (fb_state->read_task == NULL) {
		LOG_ERR("%s: failed to create VC-FB-Read thread", __func__);

		goto err_remove_framebuffer;
	}
	set_user_nice(fb_state->read_task, -10);
	wake_up_process(fb_state->read_task);

	/* Done! */

	fb_inited = 1;
	goto out;

err_remove_framebuffer:
	for (i = 0; i < VC_FB_SCRN_MAX; i++)
		vc_fb_remove_framebuffer(i);

	remove_proc_entry("vc-fb", NULL);

err_stop_fb_service:
	vc_vchi_fb_stop(&fb_state->fb_handle);

err_free_mem:
	kfree(fb_state);

out:
	LOG_INFO("%s: end (ret=%d)", __func__, ret);
}

static int __init vc_fb_init(void)
{
	printk(KERN_INFO "vc-fb: Videocore framebuffer driver\n");

	vchiq_add_connected_callback(vc_fb_connected_init);
	return 0;
}

static void __exit vc_fb_exit(void)
{
	int i;

	LOG_INFO("%s: start", __func__);

	if (fb_inited) {
		/* Remove framebuffer device for each screen */
		for (i = 0; i < VC_FB_SCRN_MAX; i++)
			vc_fb_remove_framebuffer(i);

		/* Remove the proc directory entry */
		remove_proc_entry("vc-fb", NULL);

		/* Stop the framebuffer service */
		vc_vchi_fb_stop(&fb_state->fb_handle);

		/* Free the memory for the state structure */
		kfree(fb_state);
	}

	LOG_INFO("%s: end", __func__);
}

late_initcall(vc_fb_init);
module_exit(vc_fb_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("VC Framebuffer Driver");
MODULE_LICENSE("GPL");
