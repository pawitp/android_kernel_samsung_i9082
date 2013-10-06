/*****************************************************************************
* Copyright 2012 Broadcom Corporation.  All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/statfs.h>
#include <linux/crc32.h>
#include <linux/gpio.h>

#include "vc_cam.h"

#include "interface/vchi/vchi.h"
#include "vchiq_util.h"
#include "vchiq_connected.h"
#include "debug_sym.h"

#define DRIVER_NAME           "vc-cam"
#define CLASS_NAME            "camera"
#define CAMD_FW_SIZE_DEFAULT	0x100000
#define CAMD_FW_NAME_DEFAULT	128
#define WRITE_SIZE            1024
#define VC_PARTITION_BLOCKS   2048
#define VC_WAIT_TIMEOUT			2000

/* name of the partition which holds the vc-firmware */
#define CAMD_FW_DEV_PATH \
	"/dev/block/platform/sdhci.1/by-name/vc-firmware"
#define CAMD_FW_DEV_PATH_ALT \
	"/dev/block/platform/sdhci.1/by-name/VC-Firmware"
/* default udpate firmware name */
#define CAMD_FW_DEFAULT_UPDATE \
	"/sdcard/SlimISP.bin"
/* default dump (internal) firmware name */
#define CAMD_FW_DEFAULT_DUMP \
	"/sdcard/CamdDump.pak"
/* location of main vc image size (firmware+bootFS) */
#define CAMD_VC_IMAGE_SZ_OFF	0x24
/* location of recovery vc image offset */
#define CAMD_VC_IMAGE_PM_OFF	0x4
/* location of recovery vc image size (firmware+bootFS) */
#define CAMD_VC_IMAGE_PM_SZ	0x8
/* location of version marker */
#define CAMD_VC_IMAGE_VR_OFF	0x0

#define CAMD_VC_ISP_CORE_VOLT	"1.2"

/* Partition layout.
 *
 *  -------------------------------
 *   VC image (firmware)
 *  -------------------------------
 *   VC main bootFS
 *  -------------------------------
 *   (optional) pad (aligned @4096)
 *  -------------------------------
 *   (optional) VC recovery
 *  -------------------------------
 *   (optional) VC bootFS
 *  -------------------------------
 *   >>>> 4 bytes CAMD FW size <<<<
 *  -------------------------------
 *   >>>> CAMD FW              <<<<
 *  -------------------------------
 */

#define LOG_DBG(fmt, ...) \
	if (vc_cam_debug) \
		printk(KERN_INFO fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) \
	printk(KERN_ERR fmt "\n", ##__VA_ARGS__)

/* 'CAMD' - Camera Dualization Service. */
#define VC_CAM_FOURCC  MAKE_FOURCC("CAMD")
/* 'CAMN' - Camera Dualization Notification. */
#define VC_NOT_FOURCC  MAKE_FOURCC("CAMN")

#define VC_CAM_VERSION     3
#define VC_CAM_MIN_VERSION 1
#define VC_CAM_MAX_PARAMS_PER_MSG \
	((VCHIQ_MAX_MSG_SIZE - sizeof(unsigned int))/sizeof(unsigned int))

enum {
	VC_CAM_MSG_QUIT,
	VC_CAMDUAL_CMD_SEND_GET_BUF = 1000,
	VC_CAMDUAL_CMD_SEND_DONE,
	VC_CAMDUAL_CMD_RECEIVE_GET_BUF,
	VC_CAMDUAL_CMD_RECEIVE_DONE,
	VC_CAMDUAL_CMD_ISP_NAME,
	VC_CAMDUAL_CMD_ISP_VERSION,
	VC_CAMDUAL_CMD_CONFIG,
	VC_CAMDUAL_CMD_CLEAR_FW,
	VC_CAMDUAL_NOTIFY_FW_CHECKED = 2000,
	VC_CAMDUAL_NOTIFY_ISP_UPDATE,
	VC_CAMDUAL_NOTIFY_FW_DONE,
	VC_CAM_MSG_MAX
};

enum {
	VC_QUERY_STATE_NOT_STARTED = 0,
	VC_QUERY_STATE_FRONT_CAM_ISP,
	VC_QUERY_STATE_FRONT_CAM_VER,
	VC_QUERY_STATE_REAR_CAM_ISP,
	VC_QUERY_STATE_REAR_CAM_VER,
	VC_QUERY_STATE_DONE,
};

struct cam_msg {
	unsigned int type;
	unsigned int params[VC_CAM_MAX_PARAMS_PER_MSG];
};

/* Device (/dev) related variables */
static dev_t vc_cam_devnum;
static struct class *vc_cam_class;
static struct cdev vc_cam_cdev;
static int vc_cam_inited;
static int vc_cam_debug;
static unsigned int vc_cam_fw_size;
static unsigned int vc_cam_fw_crc;
static unsigned int vc_cam_vc_size;
static unsigned int vc_cam_fw_vc_addr;
static VC_MEM_ACCESS_HANDLE_T vc_cam_vc_mem_hndl;
static char vc_cam_fw[CAMD_FW_NAME_DEFAULT];
static char vc_cam_isp_name_front[CAMD_FW_NAME_DEFAULT];
static char vc_cam_isp_version_front[CAMD_FW_NAME_DEFAULT];
static char vc_cam_isp_name_rear[CAMD_FW_NAME_DEFAULT];
static char vc_cam_isp_version_rear[CAMD_FW_NAME_DEFAULT];
static char vc_cam_isp_core_rear[CAMD_FW_NAME_DEFAULT];
static struct device *vc_cam_front_cam_dev;
static struct device *vc_cam_rear_cam_dev;
static int vc_cam_query_state;
static int vc_cam_fw_trash_emmc;
static int vc_cam_fw_forced_update;
static int vc_cam_fw_emmc2vc_status;
static int vc_cam_fw_vc2emmc_status;
static int vc_cam_fw_ignore_notification;

/* Run time dump override (dump name padded to
 * add fixed prefix and suffix extension)
 */
static int vc_cam_fw_dump_to_fs;
static char vc_cam_fw_dump[CAMD_FW_NAME_DEFAULT+20];

/* Run time overrides per configuration. */
static char vc_cam_fw_cfg[CAMD_FW_NAME_DEFAULT];
static unsigned int vc_cam_fw_cfg_size;
static unsigned int vc_cam_fw_cfg_status;
static int vc_cam_fw_cfg_upload;

/* Proc entry */
static struct proc_dir_entry *vc_cam_proc_entry;
static struct proc_dir_entry *vc_cam_proc_cmd;
static struct proc_dir_entry *vc_cam_proc_cfg;

static struct platform_device vc_cam_device = {
	.name = DRIVER_NAME,
	.id = 0,
};

static VCHIQ_INSTANCE_T cam_instance;
static VCHIQ_SERVICE_HANDLE_T cam_service;
static VCHIQ_SERVICE_HANDLE_T not_service;
static VCHIU_QUEUE_T cam_msg_queue;
static VCHIU_QUEUE_T not_msg_queue;
static struct task_struct *cam_worker;
static struct task_struct *not_worker;
static struct task_struct *fwchk_worker;
static DEFINE_SEMAPHORE(vc_cam_worker_queue_push_mutex);
static DEFINE_SEMAPHORE(vc_not_worker_queue_push_mutex);

static VCHIQ_STATUS_T cam_service_callback(VCHIQ_REASON_T reason,
					   VCHIQ_HEADER_T *header,
					   VCHIQ_SERVICE_HANDLE_T service,
					   void *bulk_userdata);
static VCHIQ_STATUS_T not_service_callback(VCHIQ_REASON_T reason,
					   VCHIQ_HEADER_T *header,
					   VCHIQ_SERVICE_HANDLE_T service,
					   void *bulk_userdata);
static void send_vc_msg(unsigned int type,
			unsigned int param1, unsigned int param2);
static bool send_worker_msg(VCHIQ_HEADER_T *msg);
static bool send_notifier_msg(VCHIQ_HEADER_T *msg);

ssize_t vc_cam_rear_name_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", vc_cam_isp_name_rear);
}

ssize_t vc_cam_rear_ver_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", vc_cam_isp_version_rear);
}

ssize_t vc_cam_rear_isp_core_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", vc_cam_isp_core_rear);
}

ssize_t vc_cam_front_name_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", vc_cam_isp_name_front);
}

ssize_t vc_cam_front_ver_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", vc_cam_isp_version_front);
}

#if defined(CONFIG_BCM_REAR_FLASH_TEST)
#define CAM_FLASH_EN_GPIO   9
#define CAM_FLASH_MODE_GPIO 13
ssize_t vc_cam_rear_flash_toggle(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (buf[0] == '0') {
		gpio_direction_output(CAM_FLASH_EN_GPIO, 0);
		gpio_direction_output(CAM_FLASH_MODE_GPIO, 0);
	} else {
		gpio_direction_output(CAM_FLASH_EN_GPIO, 0);
		gpio_direction_output(CAM_FLASH_MODE_GPIO, 1);
	}

	return count;
}
#endif

static DEVICE_ATTR(rear_camtype, 0644, vc_cam_rear_name_show, NULL);
static DEVICE_ATTR(rear_camfw, 0644, vc_cam_rear_ver_show, NULL);
static DEVICE_ATTR(isp_core, 0644, vc_cam_rear_isp_core_show, NULL);
static DEVICE_ATTR(front_camtype, 0644, vc_cam_front_name_show, NULL);
static DEVICE_ATTR(front_camfw, 0644, vc_cam_front_ver_show, NULL);
#if defined(CONFIG_BCM_REAR_FLASH_TEST)
static DEVICE_ATTR(rear_flash, 0644, NULL, vc_cam_rear_flash_toggle);
#endif

void __init vc_cam_early_init(void)
{
	int rc = platform_device_register(&vc_cam_device);
	LOG_DBG("platform_device_register -> %d", rc);
}

/****************************************************************************
*
*   vc_cam_open
*
***************************************************************************/

static int vc_cam_open(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	return 0;
}

/****************************************************************************
*
*   vc_cam_release
*
***************************************************************************/

static int vc_cam_release(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	return 0;
}

/****************************************************************************
*
*   vc_cam_ioctl
*
***************************************************************************/
static void vc_cam_fw_action(enum vc_fw_ctrl_action_e action);
static long vc_cam_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = -EPERM;
	unsigned int cmdnr;

	cmdnr = _IOC_NR(cmd);

	switch (cmdnr) {
	case VC_CAM_FW_CTRL_UPDATE:
		{
			struct vc_cam_fw_ioctl fw_ctrl;
			rc = copy_from_user(&fw_ctrl,
				(void __user *)arg,
				sizeof(struct vc_cam_fw_ioctl));
			if (rc)
				return rc;

			vc_cam_fw_action(fw_ctrl.action);
		}
		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

/****************************************************************************
*
*   File Operations for the driver.
*
***************************************************************************/

static const struct file_operations vc_cam_fops = {
	.owner = THIS_MODULE,
	.open = vc_cam_open,
	.release = vc_cam_release,
	.unlocked_ioctl = vc_cam_ioctl,
};

/****************************************************************************
*
*   vc_cam_proc_cmd_open
*
***************************************************************************/

static int vc_cam_show_info(struct seq_file *m, void *v)
{
	seq_printf(m, "Videocore Camera Firmware Dualization:\n");

	seq_printf(m, "videocore size         : 0x%08x\n",
		vc_cam_vc_size);
	seq_printf(m, "firmware name          : \'%s\'\n",
		vc_cam_fw);
	seq_printf(m, "firmware size          : 0x%08x\n",
		vc_cam_fw_size);
	seq_printf(m, "firmware addr (vc)     : 0x%08x\n",
		vc_cam_fw_vc_addr);
	seq_printf(m, "firmware crc           : 0x%08x\n",
		vc_cam_fw_crc);
	seq_printf(m, "emmc->vc status        : %d\n",
		vc_cam_fw_emmc2vc_status);
	seq_printf(m, "vc->emmc status        : %d\n",
		vc_cam_fw_vc2emmc_status);
	seq_printf(m, "------------------------\n");
	seq_printf(m, "rear/main isp name     : %s\n",
		vc_cam_isp_name_rear);
	seq_printf(m, "rear/main isp version  : %s\n",
		vc_cam_isp_version_rear);
	seq_printf(m, "front isp name         : %s\n",
		vc_cam_isp_name_front);
	seq_printf(m, "front isp version      : %s\n",
		   vc_cam_isp_version_front);

	seq_printf(m, "\n");

	return 0;
}

static int vc_cam_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, vc_cam_show_info, NULL);
}

/****************************************************************************
*
*   vc_cam_proc_cfg_open
*
***************************************************************************/
static int vc_cam_show_cfg(struct seq_file *m, void *v)
{
	seq_printf(m, "Videocore CAMD Configuration:\n");

	seq_printf(m, "configurable name      : \'%s\'\n",
		vc_cam_fw_cfg);
	seq_printf(m, "configurable size      : 0x%08x\n",
		vc_cam_fw_cfg_size);
	seq_printf(m, "configurable status    : 0x%08x\n",
		vc_cam_fw_cfg_status);
	seq_printf(m, "upload mode            : %d\n",
		vc_cam_fw_cfg_upload);

	seq_printf(m, "\n");

	return 0;
}

static int vc_cam_proc_cfg_open(struct inode *inode, struct file *file)
{
	return single_open(file, vc_cam_show_cfg, NULL);
}

static void vc_cam_fw_new(unsigned int vc_size)
{
	/* request the data from VC.
	 */
	vc_cam_fw_size = vc_size;
	send_vc_msg(VC_CAMDUAL_CMD_RECEIVE_GET_BUF, vc_cam_fw_size, 0);

	vchiq_use_service(cam_service);

}

static void vc_cam_fw_done(int vc_update, int eeprom_update)
{
	/* vc is done doing its job with the firmware that
	 * we gave it, so unblock vc suspend.
	 */
	vchiq_release_service(cam_service);

	/* if we forced an update, make sure we re-seed the
	 * partition with the actual content no matter what.
	 *
	 * if we did not force the update, re-seed the
	 * partition if we failed to update properly.
	 */
	if ((vc_cam_fw_forced_update &&
			(!vc_update && !eeprom_update)) ||
		 (!vc_cam_fw_forced_update &&
			(vc_update || eeprom_update))) {
		LOG_ERR("%s: reseeding fw (%d,%d)",
			__func__,
			vc_update,
			eeprom_update);

		send_vc_msg(VC_CAMDUAL_CMD_RECEIVE_GET_BUF, 0, 0);

		vchiq_use_service(cam_service);

	}

	/* end of any forced-update processing... */
	vc_cam_fw_forced_update = 0;
}

static void vc_cam_query_data(void)
{
	if ((vc_cam_query_state >= VC_QUERY_STATE_NOT_STARTED) &&
	    (vc_cam_query_state < VC_QUERY_STATE_DONE))
		vc_cam_query_state++;

	switch (vc_cam_query_state) {
	case VC_QUERY_STATE_FRONT_CAM_ISP:
		send_vc_msg(VC_CAMDUAL_CMD_ISP_NAME, 1, 0);
		break;
	case VC_QUERY_STATE_FRONT_CAM_VER:
		send_vc_msg(VC_CAMDUAL_CMD_ISP_VERSION, 1, 0);
		break;
	case VC_QUERY_STATE_REAR_CAM_ISP:
		send_vc_msg(VC_CAMDUAL_CMD_ISP_NAME, 0, 0);
		break;
	case VC_QUERY_STATE_REAR_CAM_VER:
		send_vc_msg(VC_CAMDUAL_CMD_ISP_VERSION, 0, 0);
		break;
	default:
		break;
	}
}

static unsigned int vc_size_from_ver(uint8_t *kbuf)
{
	int vc_version = 0;
	int vc_old_size = 0, vc_size = 0;

	if (kbuf == NULL)
		return 0;

	/* lookup vc image version information.
	 */
	vc_version =
		((kbuf[CAMD_VC_IMAGE_VR_OFF + 3] << 24) |
		(kbuf[CAMD_VC_IMAGE_VR_OFF + 2] << 16) |
		(kbuf[CAMD_VC_IMAGE_VR_OFF + 1] << 8)  |
		kbuf[CAMD_VC_IMAGE_VR_OFF]);
	LOG_DBG("%s: detected version 0x%08x",
	  __func__,
	  vc_version);

	/* 'older' version have a pointer at this location
	 * so we check against some number that cannot be
	 * a proper version value.
	 */
	if (vc_version >= 0x1000)
		vc_version = 0;

	/* lookup size of the vc image (includes vc firmware
	 * and bootFS).
	 */
	vc_old_size =
		 ((kbuf[CAMD_VC_IMAGE_SZ_OFF + 3] << 24) |
		  (kbuf[CAMD_VC_IMAGE_SZ_OFF + 2] << 16) |
		  (kbuf[CAMD_VC_IMAGE_SZ_OFF + 1] << 8)  |
		  kbuf[CAMD_VC_IMAGE_SZ_OFF]);
	LOG_DBG("%s: detected legacy size of %d",
	  __func__,
	  vc_old_size);

	/* if this is a new version image, see if there is a
	 * postmortem image associated.  if yes the postmortem
	 * image will be following the main image and aligned
	 * at 4096 bytes boundary.
	 */
	if (vc_version) {
		vc_size =
			 ((kbuf[CAMD_VC_IMAGE_PM_OFF + 3] << 24) |
			  (kbuf[CAMD_VC_IMAGE_PM_OFF + 2] << 16) |
			  (kbuf[CAMD_VC_IMAGE_PM_OFF + 1] << 8)  |
			  kbuf[CAMD_VC_IMAGE_PM_OFF]);
		vc_size +=
			 ((kbuf[CAMD_VC_IMAGE_PM_SZ + 3] << 24) |
			  (kbuf[CAMD_VC_IMAGE_PM_SZ + 2] << 16) |
			  (kbuf[CAMD_VC_IMAGE_PM_SZ + 1] << 8)  |
			  kbuf[CAMD_VC_IMAGE_PM_SZ]);

		LOG_DBG("%s: detected new size of %d",
		  __func__,
		  vc_size);
		/* there was no postmortem image built-in, so
		 * fallback to old size value.
		 */
		if (!vc_size)
			vc_size = vc_old_size;
	}

	/* this is effectively the overall offset at which
	 * we would expect to find the camera firmware within
	 * the partition.
	 */
	return vc_size;
}

static void open_vc_partition(int *fd, char *name_override)
{
	if (!fd)
		return;

	*fd = -1;

	if (name_override) {
		*fd = sys_open(name_override, O_RDONLY, 0);
		LOG_DBG("%s: sys_open(%s) - returns %d",
			__func__,
			name_override,
			*fd);
	} else {
		sprintf(vc_cam_fw, "(null)");
		*fd = sys_open(CAMD_FW_DEV_PATH_ALT, O_RDWR, 0);
		LOG_DBG("%s: sys_open(%s) - returns %d",
			__func__,
			CAMD_FW_DEV_PATH_ALT,
			*fd);
		if (*fd < 0) {
			*fd = sys_open(CAMD_FW_DEV_PATH, O_RDWR, 0);
			LOG_DBG("%s: sys_open(%s) - returns %d",
				__func__,
				CAMD_FW_DEV_PATH,
				*fd);
			if (*fd >= 0)
				sprintf(vc_cam_fw, "%s", CAMD_FW_DEV_PATH);
		} else
			sprintf(vc_cam_fw, "%s", CAMD_FW_DEV_PATH_ALT);
	}
}

static void check_cam_fw_size(void)
{
	int fd = -1, rc;
	uint8_t *kbuf = NULL;
	mm_segment_t camd_fs;

	camd_fs = get_fs();
	set_fs(KERNEL_DS);

	open_vc_partition(&fd, NULL);
	if (fd < 0) {
		vc_cam_vc_size = 0;
		vc_cam_fw_size = 0;
		goto out;
	}

	kbuf = kmalloc(WRITE_SIZE, GFP_ATOMIC);
	if (kbuf == NULL) {
		vc_cam_vc_size = 0;
		vc_cam_fw_size = 0;
		goto out;
	}

	if (sys_read(fd, kbuf, WRITE_SIZE) == WRITE_SIZE) {
		vc_cam_vc_size = vc_size_from_ver(kbuf);
		LOG_DBG("%s: vc-size %d (bytes)",
			__func__,
			vc_cam_vc_size);
	} else {
		LOG_ERR("%s: failed to read %d bytes from fd %d",
	     __func__,
		  WRITE_SIZE,
		  fd);
		vc_cam_vc_size = 0;
		vc_cam_fw_size = 0;
		goto out;
	}

	if (vc_cam_vc_size) {
		/* if there is a camera firmware, it will be right
		 * at the end of the vc image.  look for it.
		 */
		rc = sys_lseek(fd, vc_cam_vc_size, SEEK_SET);
		LOG_DBG("%s: sys_lseek to %d, returns %d",
			__func__,
			vc_cam_vc_size,
			rc);
		if (sys_read(fd, kbuf, sizeof(uint32_t)) == sizeof(uint32_t)) {
			vc_cam_fw_size =
				 ((kbuf[3] << 24) |
				  (kbuf[2] << 16) |
				  (kbuf[1] << 8)  |
				  kbuf[0]);
			LOG_DBG("%s: cam-fw %d (bytes)",
				__func__,
				vc_cam_fw_size);
		}
		/* read the crc as well. */
		if (sys_read(fd, kbuf, sizeof(uint32_t)) == sizeof(uint32_t)) {
			vc_cam_fw_crc =
				 ((kbuf[3] << 24) |
				  (kbuf[2] << 16) |
				  (kbuf[1] << 8)  |
				  kbuf[0]);
			LOG_DBG("%s: cam-crc 0x%x",
				__func__,
				vc_cam_fw_crc);
		}
	} else
		vc_cam_fw_size = 0;

	/* validate that the fw size is within
	 * acceptable bounds.
	 */
	if (vc_cam_fw_size < 0) {
		LOG_ERR("%s: cam-fw sz: %d - appears invalid",
			__func__,
			vc_cam_fw_size);
		vc_cam_fw_size = 0;
	}

	if (vc_cam_fw_size > CAMD_FW_SIZE_DEFAULT) {
		LOG_ERR("%s: cam-fw sz: %d - is OOB (max %d)",
			__func__,
			vc_cam_fw_size,
			CAMD_FW_SIZE_DEFAULT);
		vc_cam_fw_size = 0;
	}

out:
	kfree(kbuf);
	if (fd >= 0)
		sys_close(fd);
	set_fs(camd_fs);
}

static void vc_cam_fw_dump_from_emmc(void)
{
	int fd, fd_w = -1;
	int read_size, total_size, write_size;
	int reading, read_this_time;
	uint8_t *kbuf = NULL;
	mm_segment_t camd_fs;

	camd_fs = get_fs();
	set_fs(KERNEL_DS);

	check_cam_fw_size();

	if (!vc_cam_vc_size || !vc_cam_fw_size)
		goto out;

	open_vc_partition(&fd, NULL);
	if (fd < 0)
		goto out;

	sys_lseek(fd,
		vc_cam_vc_size + 2 * sizeof(uint32_t),
		SEEK_SET);

	kbuf = kmalloc(WRITE_SIZE, GFP_ATOMIC);
	if (kbuf == NULL)
		goto out;

	fd_w = sys_open(CAMD_FW_DEFAULT_DUMP, O_WRONLY|O_CREAT, 0);
	LOG_DBG("%s: sys_open(%s) - returns %d",
		__func__,
		CAMD_FW_DEFAULT_DUMP,
		fd_w);
	if (fd_w < 0)
		goto out;

	read_size = total_size = 0;
	reading = 1;
	while (reading) {
		write_size = 0;
		read_this_time =
			((vc_cam_fw_size-total_size)
			> WRITE_SIZE) ?
		    WRITE_SIZE : (vc_cam_fw_size-total_size);
		read_size = sys_read(fd, kbuf, read_this_time);

		total_size += read_size;
		if (read_size)
			write_size = sys_write(fd_w, kbuf, read_size);

		LOG_DBG("%s: copy %d->%d - %d/%d/%d",
			__func__,
			fd,
			fd_w,
			read_size,
			write_size,
			total_size);

		if (total_size >= vc_cam_fw_size)
			reading = 0;
	}

out:
	kfree(kbuf);
	if (fd >= 0)
		sys_close(fd);
	if (fd_w >= 0)
		sys_close(fd_w);
	set_fs(camd_fs);
}

static void write_cam_fw_size(int real_size, int crc)
{
	int fd = -1, rc;
	uint8_t *kbuf = NULL;
	int write_len, check;
	mm_segment_t camd_fs;
	int size_to_write = 0;

	camd_fs = get_fs();
	set_fs(KERNEL_DS);

	if (real_size) {
		size_to_write = vc_cam_fw_size;
		vc_cam_fw_crc = crc;
	} else
		vc_cam_fw_crc = 0;

	open_vc_partition(&fd, NULL);
	if (fd < 0)
		goto out;

	kbuf = kmalloc(WRITE_SIZE, GFP_ATOMIC);
	if (kbuf == NULL)
		goto out;

	if (sys_read(fd, kbuf, WRITE_SIZE) == WRITE_SIZE) {
		vc_cam_vc_size = vc_size_from_ver(kbuf);
		LOG_DBG("%s: vc-size %d (bytes)",
			__func__,
			vc_cam_vc_size);
	} else {
		LOG_ERR("%s: failed to read %d bytes from fd %d",
	     __func__,
		  WRITE_SIZE,
		  fd);
		goto out;
	}

	if (vc_cam_vc_size) {
		/* write the size of the camera firmware image at
		 * the end of the vc image content.
		 */
		rc = sys_lseek(fd, vc_cam_vc_size, SEEK_SET);
		LOG_DBG("%s: sys_lseek to %d, returns %d",
			__func__,
			vc_cam_vc_size,
			rc);
		kbuf[3] = (size_to_write >> 24) & 0xff;
		kbuf[2] = (size_to_write >> 16) & 0xff;
		kbuf[1] = (size_to_write >> 8) & 0xff;
		kbuf[0] = size_to_write & 0xff;
		check =
			((kbuf[3] << 24) |
			  (kbuf[2] << 16) |
			  (kbuf[1] << 8)  |
				kbuf[0]);
		write_len = sys_write(fd, kbuf, sizeof(uint32_t));

		LOG_DBG("%s: cam-fw %d (%d) - ret %d",
			__func__,
			size_to_write,
			check,
			write_len);
		/* write the crc right after that.
		 */
		kbuf[3] = (crc >> 24) & 0xff;
		kbuf[2] = (crc >> 16) & 0xff;
		kbuf[1] = (crc >> 8) & 0xff;
		kbuf[0] = crc & 0xff;
		check =
			((kbuf[3] << 24) |
			  (kbuf[2] << 16) |
			  (kbuf[1] << 8)  |
				kbuf[0]);
		write_len = sys_write(fd, kbuf, sizeof(uint32_t));
		LOG_DBG("%s: cam-crc 0x%x (0x%x) - ret %d",
			__func__,
			crc,
			check,
			write_len);
	}

out:
	kfree(kbuf);
	if (fd >= 0)
		sys_close(fd);
	set_fs(camd_fs);
}

static int access_vc_partition(int emmc_to_vc, u32 *result_crc)
{
	int fd = -1, rc;
	uint8_t *kbuf = NULL;
	mm_segment_t camd_fs;
	VC_MEM_ADDR_T vc_mem_addr;
	int read_len, read_tot, read_this_time;
	int written_size, write_len, size, ix;
	int failure = -EPERM;
	int vc_size = 0;
	int fw_size = 0;
	struct statfs f_stats;
	char *virtual_partition = NULL;
	int flags = O_RDWR;
	uint32_t crc = 0;

	camd_fs = get_fs();
	set_fs(KERNEL_DS);

	vc_size = vc_cam_vc_size;
	fw_size = vc_cam_fw_size;
	virtual_partition = vc_cam_fw;
	if (emmc_to_vc && vc_cam_fw_cfg_upload) {
		fw_size = vc_cam_fw_cfg_size;
		virtual_partition = vc_cam_fw_cfg;
	} else if (!emmc_to_vc && vc_cam_fw_dump_to_fs) {
		virtual_partition = vc_cam_fw_dump;
		flags = O_WRONLY|O_CREAT;
	}

	LOG_DBG("%s: using(%s), vc: %d, fw: %d",
		__func__,
		virtual_partition,
		vc_size,
		fw_size);

	vc_mem_addr = (VC_MEM_ADDR_T) vc_cam_fw_vc_addr;
	if (!vc_size)
		goto out;

	fd = sys_open(virtual_partition, flags, 0);
	LOG_DBG("%s: sys_open(%s) - returns %d",
		__func__,
		virtual_partition,
		fd);
	if (fd < 0)
		goto out;

	kbuf = kmalloc(WRITE_SIZE, GFP_ATOMIC);
	if (kbuf == NULL)
		goto out;

	/* set forced for now until i figure
	 * how to get the data from the system
	 * correctly...
	 */
	sys_fstatfs(fd, &f_stats);
	f_stats.f_blocks = VC_PARTITION_BLOCKS;
	LOG_DBG("%s: sys_newfstat(%s) - size %u - blks %u",
		__func__,
		virtual_partition,
		f_stats.f_bsize,
		f_stats.f_blocks);

	if (!emmc_to_vc && vc_cam_fw_trash_emmc) {
		/* this is a debug mode to purposefully trash the
		 * vc partition by writing the cam firmware right
		 * from the start of the partition.  needless to
		 * say this is dangerous.
		 */
		LOG_ERR("%s: **** TRASH MODE ****", __func__);
		rc = sys_lseek(fd, 0, SEEK_SET);
		LOG_DBG("%s: sys_lseek to START, returns %d",
			__func__, rc);
	} else {
		rc = sys_lseek(fd,
			(vc_cam_fw_dump_to_fs || vc_cam_fw_cfg_upload) ?
				0 : vc_size + 2 * sizeof(uint32_t),
			SEEK_SET);
		LOG_DBG("%s: sys_lseek to %d, returns %d",
			__func__,
			(vc_cam_fw_dump_to_fs || vc_cam_fw_cfg_upload) ?
				0 : vc_size + 2 * sizeof(uint32_t),
			rc);
	}

	/* basic validation. */
	if (!emmc_to_vc) {
		/* writing from vc to emmc, make sure we would not
		 * overrun the partition.
	    */
		if ((vc_size + fw_size) >
			(f_stats.f_blocks *
			f_stats.f_bsize)) {
			LOG_ERR("%s: overrun size %u > %u",
				__func__,
				(vc_size + fw_size),
				(f_stats.f_blocks *
				f_stats.f_bsize));
			goto out;
		}
	}

	if (emmc_to_vc) {
		LOG_DBG("%s: read-op", __func__);
		if (OpenVideoCoreMemory(&vc_cam_vc_mem_hndl) == 0) {
			read_tot = 0;
			crc = 0;
			while (read_tot < fw_size) {
				read_this_time =
					((fw_size-read_tot)
					> WRITE_SIZE) ?
				    WRITE_SIZE : (fw_size-read_tot);
				read_len = sys_read(fd, kbuf, read_this_time);
				if (read_len > 0) {
					crc = crc32_be(crc,
						(const uint8_t *)&kbuf[0],
						read_len);
					WriteVideoCoreMemory(
						vc_cam_vc_mem_hndl,
						kbuf,
						(VC_MEM_ADDR_T)
						(vc_mem_addr+read_tot),
						read_len);

					LOG_DBG("%s: wr %u @0x%08x - tot %u/%u",
					  __func__,
					  read_len,
					  vc_mem_addr+read_tot,
					  read_tot,
					  fw_size);

					read_tot += read_len;
				} else {
					read_tot = fw_size;
					goto out;
				}
			}
			CloseVideoCoreMemory(vc_cam_vc_mem_hndl);
			/* success. */
			failure = 0;
		}
	} else {
		LOG_DBG("%s: write-op", __func__);
		if (OpenVideoCoreMemory(&vc_cam_vc_mem_hndl) == 0) {
			written_size = fw_size;
			ix = 0;
			crc = 0;
			while (written_size > 0) {
				size = (written_size > WRITE_SIZE) ?
					WRITE_SIZE : written_size;
				rc = ReadVideoCoreMemory(
					vc_cam_vc_mem_hndl,
				   kbuf,
					(VC_MEM_ADDR_T)
					(vc_mem_addr+(ix*WRITE_SIZE)),
					size);
				if (rc == 0) {
					LOG_ERR
						("%s: fail @%d,%u s:0x%08x",
				     __func__,
				     ix,
					  size,
				     (vc_mem_addr+(ix*WRITE_SIZE)));
					written_size = 0;
					goto out;
				}
				write_len = sys_write(fd, kbuf, size);
				written_size -= size;

				LOG_DBG("%s: wr %u @0x%08x,%u,%u/%u - rt %d",
			     __func__,
				  size,
			     vc_mem_addr+(ix*WRITE_SIZE),
			     ix,
				  written_size,
			     fw_size,
			     write_len);

				crc = crc32_be(crc,
					(const uint8_t *)&kbuf[0],
					size);
				ix++;
			};
			CloseVideoCoreMemory(vc_cam_vc_mem_hndl);
			/* success. */
			failure = 0;
		}
	}

out:
	*result_crc = crc;

	kfree(kbuf);
	if (fd >= 0)
		sys_close(fd);
	set_fs(camd_fs);

	LOG_DBG("%s: %s - status %s (crc 0x%x)",
		__func__,
		emmc_to_vc ? "EMMC->VC" : "VC->EMMC",
		failure ? "FAILED" : "success",
		crc);

	return failure;
}

static void vc_cam_fw_clear(int vc_or_emmc)
{
	if (vc_or_emmc)
		send_vc_msg(VC_CAMDUAL_CMD_CLEAR_FW, 0, 0);
	else
		write_cam_fw_size(0, 0);
}

static void vc_cam_fw_chk(void)
{
	/* check for the size of the data that we have
	 * on our partition.
	 */
	check_cam_fw_size();

	if (vc_cam_fw_size > 0) {
		/* ask vc for a buffer in which we will write the
		 * data we retrieve from the dedicated partition.
		 */
		send_vc_msg(VC_CAMDUAL_CMD_SEND_GET_BUF, vc_cam_fw_size, 0);
	} else {
		/* we have nothing, or nothing we think is valid,
		 * ask for vc to get us its latest.
		 */
		send_vc_msg(VC_CAMDUAL_CMD_RECEIVE_GET_BUF, 0, 0);
		vchiq_use_service(cam_service);
		vc_cam_fw_ignore_notification = 1;
	}
}

static void use_cfg_cam_fw(void)
{
	int fd = -1;
	int reading = 0;
	unsigned int read_size, total_size;
	uint8_t *kbuf = NULL;
	mm_segment_t camd_fs;

	camd_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = sys_open(vc_cam_fw_cfg, O_RDONLY, 0);
	LOG_DBG("%s: sys_open(%s) - returns %d",
		__func__,
		vc_cam_fw_cfg,
		fd);
	if (fd < 0)
		goto out;

	kbuf = kmalloc(WRITE_SIZE, GFP_ATOMIC);
	if (kbuf == NULL)
		goto out;

	read_size = total_size = 0;
	reading = 1;
	while (reading) {
		read_size = sys_read(fd, kbuf, WRITE_SIZE);
		total_size += read_size;

		if (!read_size)
			reading = 0;
	}

	if (total_size > 0) {
		vc_cam_fw_cfg_upload = 1;
		vc_cam_fw_cfg_size = total_size;
		send_vc_msg(VC_CAMDUAL_CMD_SEND_GET_BUF, vc_cam_fw_cfg_size, 0);
	}

out:
	kfree(kbuf);
	if (fd >= 0)
		sys_close(fd);
	set_fs(camd_fs);

	return;
}

void vc_cam_fw_action(enum vc_fw_ctrl_action_e action)
{
	switch (action) {
	case VC_CAM_FW_CTRL_UPDATE:
		LOG_DBG("%s: forceful camd-fw update, source %s",
			__func__,
			CAMD_FW_DEFAULT_UPDATE);
		memset(vc_cam_fw_cfg, 0, sizeof(vc_cam_fw_cfg));
		snprintf(vc_cam_fw_cfg, sizeof(vc_cam_fw_cfg),
			"%s", CAMD_FW_DEFAULT_UPDATE);
		use_cfg_cam_fw();
		break;

	case VC_CAM_FW_CTRL_DUMP:
	{
		int ix;
		LOG_DBG("%s: camd-fw dump",
			__func__);
		memset(vc_cam_fw_dump, 0, sizeof(vc_cam_fw_dump));
		snprintf(vc_cam_fw_dump, sizeof(vc_cam_fw_dump),
			"/sdcard/%s_dump.bin", vc_cam_isp_name_rear);
		for (ix = 0 ; ix < strlen(vc_cam_fw_dump) ; ix++)
			if (vc_cam_fw_dump[ix] == ' ')
				vc_cam_fw_dump[ix] = '_';
		vc_cam_fw_dump_to_fs = 1;
		vc_cam_fw_new(vc_cam_fw_size);
		break;
	}

	default:
		LOG_ERR("%s: unknown action %d",
			__func__,
			action);
		break;
	}
}


/****************************************************************************
*
*   vc_cam_proc_cmd_write
*
***************************************************************************/

#define CAMD_VC_CHECK           "check"
#define CAMD_GET_ISP_QUERY      "query"
#define CAMD_EMMC_2_VC          "emmc2vc"
#define CAMD_EMMC_2_VC_FORCED   "emmc2vc-f"
#define CAMD_VC_2_EMMC          "vc2emmc"
#define CAMD_VC_TRASH_EMMC      "vcTRASHemmc"
#define CAMD_USE_CFG            "usecfg"
#define CAMD_ACTION_DUMP        "action-dump"
#define CAMD_ACTION_UPDATE      "action-update"
#define CAMD_DUMP_PARTITION     "dump"
#define CAMD_CLEAR_FW_VC        "clear-vc"
#define CAMD_CLEAR_FW_EMMC      "clear-emmc"

static int vc_cam_proc_cmd_write(struct file *file,
			     const char __user *buffer,
			     size_t size, loff_t *ppos)
{
	int rc = -EFAULT;
	char input_str[20];

	memset(input_str, 0, sizeof(input_str));

	if (size > sizeof(input_str)) {
		LOG_ERR("%s: input string length too long", __func__);
		goto out;
	}

	if (copy_from_user(input_str, buffer, size - 1)) {
		LOG_ERR("%s: failed to get input string", __func__);
		goto out;
	}

	LOG_DBG("%s: input string \'%s\'", __func__, input_str);

	if (strncmp(input_str, CAMD_EMMC_2_VC,
			strlen(CAMD_EMMC_2_VC)) == 0) {
		vc_cam_fw_chk();
		rc = size;
	} else if (strncmp(input_str, CAMD_EMMC_2_VC_FORCED,
			   strlen(CAMD_EMMC_2_VC_FORCED)) == 0) {
		vc_cam_fw_forced_update = 1;
		vc_cam_fw_chk();
		rc = size;
	} else if (strncmp(input_str, CAMD_VC_2_EMMC,
			   strlen(CAMD_VC_2_EMMC)) == 0) {
		vc_cam_fw_new(vc_cam_fw_size);
		rc = size;
	} else if (strncmp(input_str, CAMD_VC_TRASH_EMMC,
			   strlen(CAMD_VC_TRASH_EMMC)) == 0) {
		vc_cam_fw_trash_emmc = 1;
		vc_cam_fw_new(vc_cam_fw_size);
		rc = size;
	} else if (strncmp(input_str, CAMD_GET_ISP_QUERY,
			   strlen(CAMD_GET_ISP_QUERY)) == 0) {
		vc_cam_query_state = VC_QUERY_STATE_NOT_STARTED;
		vc_cam_query_data();
		rc = size;
	} else if (strncmp(input_str, CAMD_VC_CHECK,
			   strlen(CAMD_VC_CHECK)) == 0) {
		check_cam_fw_size();
		rc = size;
	} else if (strncmp(input_str, CAMD_USE_CFG,
			   strlen(CAMD_USE_CFG)) == 0) {
		use_cfg_cam_fw();
		rc = size;
	} else if (strncmp(input_str, CAMD_ACTION_DUMP,
			   strlen(CAMD_ACTION_DUMP)) == 0) {
		vc_cam_fw_action(VC_CAM_FW_CTRL_DUMP);
		rc = size;
	} else if (strncmp(input_str, CAMD_ACTION_UPDATE,
			   strlen(CAMD_ACTION_UPDATE)) == 0) {
		vc_cam_fw_action(VC_CAM_FW_CTRL_UPDATE);
		rc = size;
	} else if (strncmp(input_str, CAMD_DUMP_PARTITION,
			   strlen(CAMD_DUMP_PARTITION)) == 0) {
		vc_cam_fw_dump_from_emmc();
		rc = size;
	} else if (strncmp(input_str, CAMD_CLEAR_FW_VC,
			   strlen(CAMD_CLEAR_FW_VC)) == 0) {
		vc_cam_fw_clear(1);
		rc = size;
	} else if (strncmp(input_str, CAMD_CLEAR_FW_EMMC,
			   strlen(CAMD_CLEAR_FW_EMMC)) == 0) {
		vc_cam_fw_clear(0);
		rc = size;
	} else
		LOG_ERR("%s: input string \'%s\' unrecognized",
			__func__, input_str);

out:
	return rc;
}

/****************************************************************************
*
*   vc_cam_proc_cfg_write
*
***************************************************************************/

static int vc_cam_proc_cfg_write(struct file *file,
			     const char __user *buffer,
			     size_t size, loff_t *ppos)
{
	int rc = -EFAULT;

	if (size > sizeof(vc_cam_fw_cfg)) {
		LOG_ERR("%s: input string length too long", __func__);
		goto out;
	}

	memset(vc_cam_fw_cfg, 0, sizeof(vc_cam_fw_cfg));

	if (copy_from_user(vc_cam_fw_cfg, buffer, size - 1)) {
		LOG_ERR("%s: failed to get input string", __func__);
		goto out;
	}

	LOG_DBG("%s: configurable firmware \'%s\'",
		__func__,
		vc_cam_fw_cfg);

	vc_cam_fw_cfg_size = 0;
	vc_cam_fw_cfg_status = 0;

out:
	return rc;
}

/****************************************************************************
*
*   File Operations for /proc interface.
*
***************************************************************************/

static const struct file_operations vc_cam_proc_cmd_fops = {
	.open = vc_cam_proc_cmd_open,
	.read = seq_read,
	.write = vc_cam_proc_cmd_write,
	.llseek = seq_lseek,
	.release = single_release
};

static const struct file_operations vc_cam_proc_cfg_fops = {
	.open = vc_cam_proc_cfg_open,
	.read = seq_read,
	.write = vc_cam_proc_cfg_write,
	.llseek = seq_lseek,
	.release = single_release
};

static void cam_vc_to_emmc(unsigned int vc_addr, unsigned int vc_size)
{
	int rc = 0;
	u32 crc = 0;

	vc_cam_fw_vc_addr = vc_addr;
	vc_cam_fw_size = vc_size;

	LOG_DBG("%s: vc_addr 0x%08x, vc_size %d",
		__func__, vc_addr, vc_cam_fw_size);

	/* guard against incomplete write by marking
	 * the current size to 0 and only adding the
	 * final size on success at the end.
	 */
	if (!vc_cam_fw_dump_to_fs)
		write_cam_fw_size(0, crc);

	vchiq_use_service(cam_service);
	if (vc_addr && vc_cam_fw_size)
		rc = access_vc_partition(0/*!emmc->vc*/, &crc);
	vchiq_release_service(cam_service);

	if (!vc_cam_fw_dump_to_fs && !rc && vc_addr && vc_cam_fw_size)
		write_cam_fw_size(1, crc);

	send_vc_msg(VC_CAMDUAL_CMD_RECEIVE_DONE, vc_addr, 0);
	vc_cam_fw_vc2emmc_status = rc;
	vc_cam_fw_dump_to_fs = 0;
	return;
}

static void cam_emmc_to_vc(unsigned int vc_addr)
{
	int rc = 0;
	u32 crc = 0;
	int crc_invalid = 0;

	vc_cam_fw_vc_addr = vc_addr;

	LOG_DBG("%s: vc_addr 0x%08x, vc_size %d",
		__func__,
		vc_addr,
		vc_cam_fw_cfg_upload ?
			vc_cam_fw_cfg_size :
			vc_cam_fw_size);

	vchiq_use_service(cam_service);
	if (vc_addr &&
		 ((vc_cam_fw_cfg_upload && vc_cam_fw_cfg_size) ||
		  (!vc_cam_fw_cfg_upload && vc_cam_fw_size)))
		rc = access_vc_partition(1/*emmc->vc*/, &crc);
	/* do not release the service until after vc
	 * signals it is done with the fw processing,
	 * which happens in 'vc_cam_fw_done'
	 */
	/* vchiq_release_service(cam_service); */

	if (vc_cam_fw_cfg_upload) {
		vc_cam_fw_forced_update = 1;
		vc_cam_fw_cfg_upload = 0;
		vc_cam_fw_cfg_status = rc;
	} else {
		vc_cam_fw_emmc2vc_status = rc;

		LOG_DBG("%s: calc crc 0x%08x, saved crc 0x%08x",
			__func__, crc, vc_cam_fw_crc);

		/* crc found incorrect, invalidate the upload.
		 */
		if (crc && vc_cam_fw_crc && (crc != vc_cam_fw_crc)) {
			vchiq_release_service(cam_service);
			crc_invalid = 1;
		}
	}

	send_vc_msg(VC_CAMDUAL_CMD_SEND_DONE, vc_cam_fw_forced_update,
		crc_invalid ? 1 : rc);

	/* crc found invalid, better ask vc to seed us again.
	 */
	if (crc_invalid) {
		send_vc_msg(VC_CAMDUAL_CMD_RECEIVE_GET_BUF, 0, 0);
		vchiq_use_service(cam_service);
	}

	return;
}

static VCHIQ_STATUS_T cam_service_callback(VCHIQ_REASON_T reason,
					   VCHIQ_HEADER_T *header,
					   VCHIQ_SERVICE_HANDLE_T service,
					   void *bulk_userdata)
{
	switch (reason) {
	case VCHIQ_MESSAGE_AVAILABLE:
		if (!send_worker_msg(header))
			return VCHIQ_RETRY;
		break;
	case VCHIQ_SERVICE_CLOSED:
		LOG_DBG("CAMD service closed");
		break;
	default:
		LOG_ERR("Unexpected CAMD callback reason %d", reason);
		break;
	}
	return VCHIQ_SUCCESS;
}

static VCHIQ_STATUS_T not_service_callback(VCHIQ_REASON_T reason,
					   VCHIQ_HEADER_T *header,
					   VCHIQ_SERVICE_HANDLE_T service,
					   void *bulk_userdata)
{
	switch (reason) {
	case VCHIQ_MESSAGE_AVAILABLE:
		if (!send_notifier_msg(header))
			return VCHIQ_RETRY;
		break;
	case VCHIQ_SERVICE_CLOSED:
		LOG_DBG("CAMD service closed");
		break;
	default:
		LOG_ERR("Unexpected CAMD callback reason %d", reason);
		break;
	}
	return VCHIQ_SUCCESS;
}

static void send_vc_msg(unsigned int type,
			unsigned int param1, unsigned int param2)
{
	unsigned int msg[] = { type, param1, param2 };
	VCHIQ_ELEMENT_T elem = { &msg, sizeof(msg) };
	VCHIQ_STATUS_T ret;
	LOG_DBG("%s: sending message %d", __func__, type);
	vchiq_use_service(cam_service);
	ret = vchiq_queue_message(cam_service, &elem, 1);
	vchiq_release_service(cam_service);
	if (ret != VCHIQ_SUCCESS)
		LOG_ERR("%s: vchiq_queue_message returned %x", __func__, ret);
}

static bool send_notifier_msg(VCHIQ_HEADER_T *msg)
{
	if (down_interruptible(&vc_not_worker_queue_push_mutex))
		return false;
	vchiu_queue_push(&not_msg_queue, msg);
	up(&vc_not_worker_queue_push_mutex);
	return true;
}

static bool send_worker_msg(VCHIQ_HEADER_T *msg)
{
	if (down_interruptible(&vc_cam_worker_queue_push_mutex))
		return false;
	vchiu_queue_push(&cam_msg_queue, msg);
	up(&vc_cam_worker_queue_push_mutex);
	return true;
}

static int fwchk_worker_proc(void *param)
{
	int loop = 1;
	/*static struct cam_msg reply; */
	(void)param;

	while (loop) {
		int fd = -1;
		mm_segment_t camd_fs;

		camd_fs = get_fs();
		set_fs(KERNEL_DS);

		open_vc_partition(&fd, NULL);
		if (fd < 0) {
			set_fs(camd_fs);
			msleep(VC_WAIT_TIMEOUT);
		} else {
			sys_close(fd);
			set_fs(camd_fs);
			loop = 0;
		}
	}

	LOG_DBG("%s: apply check now...", __func__);
	/* waited until the device block is
	 * ready, now time to execute the action
	 * waited on...
	 */
	vc_cam_fw_chk();
	return 0;
}

static int not_worker_proc(void *param)
{
	/*static struct cam_msg reply; */
	(void)param;

	while (1) {
		VCHIQ_HEADER_T *msg;
		static struct cam_msg msg_copy;
		struct cam_msg *cam_msg = &msg_copy;
		int type, msg_size;

		msg = vchiu_queue_pop(&not_msg_queue);
		if ((unsigned int)msg >= VC_CAM_MSG_MAX) {
			msg_size = msg->size;
			memcpy(&msg_copy, msg->data, msg_size);
			type = cam_msg->type;
			vchiq_release_message(not_service, msg);
		} else {
			msg_size = 0;
			type = (int)msg;
			if (type == VC_CAM_MSG_QUIT)
				break;
			else {
				BUG();
				continue;
			}
		}

		switch (type) {
		case VC_CAMDUAL_NOTIFY_FW_CHECKED:{
				unsigned int vc_update;
				unsigned int vc_size;
				vc_update = cam_msg->params[0];
				vc_size = cam_msg->params[1];
				LOG_DBG
				    ("VC_CAMDUAL_NOTIFY_FW_CHECKED(update %u)",
				     vc_update);
				if (vc_update && !vc_cam_fw_ignore_notification)
					vc_cam_fw_new(vc_size);
				vc_cam_fw_ignore_notification = 0;
				break;
			}

		case VC_CAMDUAL_NOTIFY_ISP_UPDATE:{
				LOG_DBG("VC_CAMDUAL_NOTIFY_ISP_UPDATE");
				vc_cam_query_state = VC_QUERY_STATE_NOT_STARTED;
				vc_cam_query_data();
				break;
			}

		case VC_CAMDUAL_NOTIFY_FW_DONE:{
				unsigned int vc_update;
				unsigned int vc_update_eeprom;
				vc_update = cam_msg->params[0];
				vc_update_eeprom = cam_msg->params[1];
				LOG_DBG("VC_CAMDUAL_NOTIFY_FW_DONE(%d,%d)",
					vc_update, vc_update_eeprom);
				vc_cam_fw_done(vc_update, vc_update_eeprom);
				break;
			}

		default:
			LOG_ERR("%s: unexpected msg type %d", __func__, type);
			break;
		}
	}

	return 0;
}

static int cam_worker_proc(void *param)
{
	/*static struct cam_msg reply; */
	(void)param;

	while (1) {
		VCHIQ_HEADER_T *msg;
		static struct cam_msg msg_copy;
		struct cam_msg *cam_msg = &msg_copy;
		int type, msg_size;

		msg = vchiu_queue_pop(&cam_msg_queue);
		if ((unsigned int)msg >= VC_CAM_MSG_MAX) {
			msg_size = msg->size;
			memcpy(&msg_copy, msg->data, msg_size);
			type = cam_msg->type;
			vchiq_release_message(cam_service, msg);
		} else {
			msg_size = 0;
			type = (int)msg;
			if (type == VC_CAM_MSG_QUIT)
				break;
			else {
				BUG();
				continue;
			}
		}

		switch (type) {
		case VC_CAMDUAL_CMD_SEND_GET_BUF:{
				unsigned int vc_addr;
				vc_addr = cam_msg->params[0];
				LOG_DBG("VC_CAMDUAL_CMD_SEND_GET_BUF(0x%08x)",
					vc_addr);
				cam_emmc_to_vc(vc_addr);
				break;
			}

		case VC_CAMDUAL_CMD_RECEIVE_GET_BUF:{
				unsigned int vc_addr;
				unsigned int vc_size;
				vc_addr = cam_msg->params[0];
				vc_size = cam_msg->params[1];
				LOG_DBG
			    ("VC_CAMDUAL_CMD_RECEIVE_GET_BUF(0x%08x,%u)",
			     vc_addr, vc_size);
				vchiq_release_service(cam_service);
				cam_vc_to_emmc(vc_addr, vc_size);
				break;
			}

		case VC_CAMDUAL_CMD_ISP_NAME:{
				unsigned int vc_index;
				unsigned int vc_size;
				char *vc_name;
				vc_index = cam_msg->params[0];
				vc_size = cam_msg->params[1];
				vc_name = (char *)&cam_msg->params[2];
				LOG_DBG
				    ("VC_CAMDUAL_CMD_ISP_NAME(%u,%u)",
				     vc_index, vc_size);
				memset((!vc_index ? vc_cam_isp_name_rear :
					vc_cam_isp_name_front), 0,
				       CAMD_FW_NAME_DEFAULT);
				memcpy((!vc_index ? vc_cam_isp_name_rear :
					vc_cam_isp_name_front), vc_name,
				       vc_size);
				vc_cam_query_data();
				break;
			}

		case VC_CAMDUAL_CMD_ISP_VERSION:{
				unsigned int vc_index;
				unsigned int vc_size;
				char *vc_name;
				vc_index = cam_msg->params[0];
				vc_size = cam_msg->params[1];
				vc_name = (char *)&cam_msg->params[2];
				LOG_DBG
				    ("VC_CAMDUAL_CMD_ISP_VERSION(%u,%u)",
				     vc_index, vc_size);
				memset((!vc_index ? vc_cam_isp_version_rear :
					vc_cam_isp_version_front), 0,
				       CAMD_FW_NAME_DEFAULT);
				memcpy((!vc_index ? vc_cam_isp_version_rear :
					vc_cam_isp_version_front), vc_name,
				       vc_size);
				vc_cam_query_data();
				break;
			}

		default:
			LOG_ERR("%s: unexpected msg type %d", __func__, type);
			break;
		}
	}

	return 0;
}

static void vc_cam_connected_init(void)
{
	VCHIQ_SERVICE_PARAMS_T service_params;

	vc_cam_debug = 1;

	LOG_DBG("%s: connected init", DRIVER_NAME);

	if (!vchiu_queue_init(&cam_msg_queue, 16)) {
		LOG_ERR("%s: could not create camd queue", __func__);
		goto fail_camd_queue;
	}

	if (!vchiu_queue_init(&not_msg_queue, 4)) {
		LOG_ERR("%s: could not create camn queue", __func__);
		goto fail_camn_queue;
	}

	if (vchiq_initialise(&cam_instance) != VCHIQ_SUCCESS)
		goto fail_vchiq_init;

	vchiq_connect(cam_instance);

	service_params.fourcc = VC_CAM_FOURCC;
	service_params.callback = cam_service_callback;
	service_params.userdata = NULL;
	service_params.version = VC_CAM_VERSION;
	service_params.version_min = VC_CAM_MIN_VERSION;

	if (vchiq_open_service(cam_instance, &service_params,
			       &cam_service) != VCHIQ_SUCCESS) {
		LOG_ERR("%s: failed to open service - already in use?",
			__func__);
		goto fail_vchiq_open;
	}

	printk(KERN_INFO "%s: service opened\n", DRIVER_NAME);

	vchiq_release_service(cam_service);

	cam_worker = kthread_create(cam_worker_proc, NULL, "kcamd");
	if (!cam_worker) {
		LOG_ERR("%s: could not create CAMD kernel thread", __func__);
		goto fail_worker;
	}
	set_user_nice(cam_worker, -20);
	wake_up_process(cam_worker);

	service_params.fourcc = VC_NOT_FOURCC;
	service_params.callback = not_service_callback;
	service_params.userdata = NULL;
	service_params.version = VC_CAM_VERSION;
	service_params.version_min = VC_CAM_MIN_VERSION;

	if (vchiq_open_service(cam_instance, &service_params,
			       &not_service) != VCHIQ_SUCCESS) {
		LOG_ERR("%s: failed to open notifier - already in use?",
			__func__);
		goto fail_worker;
	}

	printk(KERN_INFO "%s: notifier opened\n", DRIVER_NAME);

	vchiq_release_service(not_service);

	not_worker = kthread_create(not_worker_proc, NULL, "kcamn");
	if (!cam_worker) {
		LOG_ERR("%s: could not create CAMN kernel thread", __func__);
		goto fail_notifier;
	}
	set_user_nice(not_worker, -20);
	wake_up_process(not_worker);

	fwchk_worker = kthread_create(fwchk_worker_proc, NULL, "kcamfwchk");
	if (!fwchk_worker)
		/* not fatal per say, but embarrassing...
		 */
		LOG_ERR("%s: could not create CAMFWCHK kernel thread",
			__func__);
	else
		/* this worker is only there to wait for the device
		 * blocks to be ready in order to start accessing
		 * the data.  there is no point doing anything before
		 * that since we will not be able to access the partition
		 * in which the firmware is stored until we can access
		 * those devices.
		 */
		wake_up_process(fwchk_worker);

	return;

fail_notifier:
	vchiq_close_service(not_service);
fail_worker:
	vchiq_close_service(cam_service);
fail_vchiq_open:
	vchiq_shutdown(cam_instance);
fail_vchiq_init:
	vchiu_queue_delete(&not_msg_queue);
fail_camn_queue:
	vchiu_queue_delete(&cam_msg_queue);
fail_camd_queue:
	return;
}

static int __init vc_cam_init(void)
{
	int rc = -EFAULT;
	struct device *dev;

	printk(KERN_INFO "%s: Videocore Camera Firmware driver\n", DRIVER_NAME);

	sprintf(vc_cam_fw, "(null)");
	vc_cam_fw_size = 0;

	vchiq_add_connected_callback(vc_cam_connected_init);

	rc = alloc_chrdev_region(&vc_cam_devnum, 0, 1, DRIVER_NAME);
	if (rc < 0) {
		LOG_ERR("%s: alloc_chrdev_region failed (rc=%d)", __func__, rc);
		goto out_release;
	}

	cdev_init(&vc_cam_cdev, &vc_cam_fops);
	rc = cdev_add(&vc_cam_cdev, vc_cam_devnum, 1);
	if (rc != 0) {
		LOG_ERR("%s: cdev_add failed (rc=%d)", __func__, rc);
		goto out_unregister;
	}

	vc_cam_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(vc_cam_class)) {
		rc = PTR_ERR(vc_cam_class);
		LOG_ERR("%s: class_create failed (rc=%d)", __func__, rc);
		goto out_cdev_del;
	}

	vc_cam_front_cam_dev =
	    device_create(vc_cam_class, NULL, 0, NULL, "front");
	if (!IS_ERR(vc_cam_front_cam_dev)) {
		if (device_create_file(vc_cam_front_cam_dev,
				       &dev_attr_front_camtype) < 0)
			LOG_ERR("%s: device_create_file failed (%s)",
				__func__, dev_attr_front_camtype.attr.name);
		if (device_create_file(vc_cam_front_cam_dev,
				       &dev_attr_front_camfw) < 0)
			LOG_ERR("%s: device_create_file failed (%s)",
				__func__, dev_attr_front_camfw.attr.name);
	}

	vc_cam_rear_cam_dev =
	    device_create(vc_cam_class, NULL, 0, NULL, "rear");
	if (!IS_ERR(vc_cam_rear_cam_dev)) {
		if (device_create_file(vc_cam_rear_cam_dev,
				       &dev_attr_rear_camtype) < 0)
			LOG_ERR("%s: device_create_file failed (%s)",
				__func__, dev_attr_rear_camtype.attr.name);
		if (device_create_file(vc_cam_rear_cam_dev,
				       &dev_attr_rear_camfw) < 0)
			LOG_ERR("%s: device_create_file failed (%s)",
				__func__, dev_attr_rear_camfw.attr.name);
		if (device_create_file(vc_cam_rear_cam_dev,
				       &dev_attr_isp_core) < 0)
			LOG_ERR("%s: device_create_file failed (%s)",
				__func__, dev_attr_isp_core.attr.name);
#if defined(CONFIG_BCM_REAR_FLASH_TEST)
		if (device_create_file(vc_cam_rear_cam_dev,
				       &dev_attr_rear_flash) < 0)
			LOG_ERR("%s: device_create_file failed (%s)",
				__func__, dev_attr_rear_flash.attr.name);
#endif
	}

	dev = device_create(vc_cam_class, NULL, vc_cam_devnum, NULL,
			    DRIVER_NAME);
	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		LOG_ERR("%s: device_create failed (rc=%d)", __func__, rc);
		goto out_class_destroy;
	}

	vc_cam_proc_entry = proc_mkdir(DRIVER_NAME, NULL);
	if (vc_cam_proc_entry == NULL) {
		rc = -EFAULT;
		LOG_ERR("%s: root proc entry failed", __func__);
		goto out_device_destroy;
	}

	vc_cam_proc_cmd =
		create_proc_entry("cmd", 0444, vc_cam_proc_entry);
	if (vc_cam_proc_cmd == NULL) {
		rc = -EFAULT;
		LOG_ERR("%s: command proc entry failed", __func__);
		goto out_root_proc_remove;
	}
	vc_cam_proc_cmd->proc_fops = &vc_cam_proc_cmd_fops;

	vc_cam_proc_cfg =
		create_proc_entry("cfg", 0444, vc_cam_proc_entry);
	if (vc_cam_proc_cmd == NULL) {
		rc = -EFAULT;
		LOG_ERR("%s: config proc entry failed", __func__);
		goto out_cmd_proc_remove;
	}
	vc_cam_proc_cfg->proc_fops = &vc_cam_proc_cfg_fops;


	memcpy(vc_cam_isp_core_rear,
		CAMD_VC_ISP_CORE_VOLT, sizeof(CAMD_VC_ISP_CORE_VOLT));
	vc_cam_inited = 1;
	return 0;

out_cmd_proc_remove:
	remove_proc_entry(vc_cam_proc_cmd->name, vc_cam_proc_entry);

out_root_proc_remove:
	remove_proc_entry(vc_cam_proc_entry->name, NULL);

out_device_destroy:
	device_remove_file(vc_cam_front_cam_dev, &dev_attr_front_camtype);
	device_remove_file(vc_cam_front_cam_dev, &dev_attr_front_camfw);
	device_remove_file(vc_cam_rear_cam_dev, &dev_attr_rear_camtype);
	device_remove_file(vc_cam_rear_cam_dev, &dev_attr_rear_camfw);
	device_remove_file(vc_cam_rear_cam_dev, &dev_attr_isp_core);
#if defined(CONFIG_BCM_REAR_FLASH_TEST)
	device_remove_file(vc_cam_rear_cam_dev, &dev_attr_rear_flash);
#endif
	device_destroy(vc_cam_class, vc_cam_devnum);

out_class_destroy:
	class_destroy(vc_cam_class);
	vc_cam_class = NULL;

out_cdev_del:
	cdev_del(&vc_cam_cdev);

out_unregister:
	unregister_chrdev_region(vc_cam_devnum, 1);

out_release:
	return -1;
}

/****************************************************************************
*
*   vc_cam_exit
*
***************************************************************************/

static void __exit vc_cam_exit(void)
{
	LOG_DBG("%s: called", __func__);

	if (vc_cam_inited) {
		remove_proc_entry(vc_cam_proc_cmd->name, vc_cam_proc_entry);
		remove_proc_entry(vc_cam_proc_cfg->name, vc_cam_proc_entry);
		remove_proc_entry(vc_cam_proc_entry->name, NULL);
		device_remove_file(vc_cam_front_cam_dev,
				   &dev_attr_front_camtype);
		device_remove_file(vc_cam_front_cam_dev, &dev_attr_front_camfw);
		device_remove_file(vc_cam_rear_cam_dev, &dev_attr_rear_camtype);
		device_remove_file(vc_cam_rear_cam_dev, &dev_attr_rear_camfw);
		device_remove_file(vc_cam_rear_cam_dev, &dev_attr_isp_core);
#if defined(CONFIG_BCM_REAR_FLASH_TEST)
		device_remove_file(vc_cam_rear_cam_dev, &dev_attr_rear_flash);
#endif
		device_destroy(vc_cam_class, vc_cam_devnum);
		class_destroy(vc_cam_class);
		cdev_del(&vc_cam_cdev);
		unregister_chrdev_region(vc_cam_devnum, 1);
	}
}

module_init(vc_cam_init);
module_exit(vc_cam_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom Corporation");
