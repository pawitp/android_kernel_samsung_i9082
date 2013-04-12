/*****************************************************************************
* Copyright 2010 - 2011 Broadcom Corporation.  All rights reserved.
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
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <asm/page.h>
#include <linux/broadcom/vc_dt.h>
#include <chal/chal_ipc.h>

#include <vc_mem.h>
#include <vc_debug_sym.h>

#define DRIVER_NAME  "vc-mem"

/* Uncomment to enable debug logging */
/* #define ENABLE_DBG */

#if defined(ENABLE_DBG)
#define LOG_DBG(fmt, ...)  printk(KERN_INFO fmt "\n", ##__VA_ARGS__)
#else
#define LOG_DBG(fmt, ...)
#endif
#define LOG_ERR(fmt, ...)  printk(KERN_ERR fmt "\n", ##__VA_ARGS__)

/* Device (/dev) related variables */
static dev_t vc_mem_devnum;
static struct class *vc_mem_class;
static struct cdev vc_mem_cdev;
static int vc_mem_inited;

/* Proc entry */
static struct proc_dir_entry *vc_mem_proc_entry;

unsigned long mm_vc_mem_phys_addr;
EXPORT_SYMBOL(mm_vc_mem_phys_addr);
unsigned int mm_vc_mem_size;
unsigned int mm_vc_mem_base;
unsigned int mm_vc_mem_load;

/****************************************************************************
*
*   vc_mem_open
*
***************************************************************************/

static int vc_mem_open(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	LOG_DBG("%s: called file = 0x%p", __func__, file);

	return 0;
}

/****************************************************************************
*
*   vc_mem_release
*
***************************************************************************/

static int vc_mem_release(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	LOG_DBG("%s: called file = 0x%p", __func__, file);

	return 0;
}

/****************************************************************************
*
*   vc_mem_access_mem
*
*   This routine does minimal checking (deliberate). It is used to extract
*   the size of the videocore memory, and is also called by the kernel
*   variant of AccessVideoCoreMemory from debug_sym.c
*
*   AccessVideoCoreMemory does the real checking.
*
***************************************************************************/

int vc_mem_access_mem(int write_mem, void *buf, uint32_t vc_mem_addr,
		      size_t num_bytes)
{
	uint8_t *map_addr;
	size_t map_size;
	size_t mem_offset;
	unsigned long arm_phys_addr;

	mem_offset = vc_mem_addr & ~PAGE_MASK;
	arm_phys_addr = vc_mem_addr & PAGE_MASK;
	arm_phys_addr += mm_vc_mem_phys_addr;
	map_size = (mem_offset + num_bytes + PAGE_SIZE - 1) & PAGE_MASK;

	map_addr = ioremap_nocache(arm_phys_addr, map_size);
	if (map_addr == 0) {
		printk(KERN_ERR
		       "%s: call to ioremap_nocache( phys 0x%08lx, size 0x%08x ) failed\n",
		       __func__, arm_phys_addr, map_size);
		return -EPERM;
	}

	if (write_mem)
		memcpy(map_addr + mem_offset, buf, num_bytes);
	else
		memcpy(buf, map_addr + mem_offset, num_bytes);

	iounmap(map_addr);

	return 0;
}

EXPORT_SYMBOL(vc_mem_access_mem);

/****************************************************************************
*
*   vc_mem_get_size
*
***************************************************************************/

static void vc_mem_get_size(void)
{
	CHAL_IPC_HANDLE ipc_handle;
	uint32_t wakeup_register;

	mm_vc_mem_base &= 0x3fffffff;
	mm_vc_mem_load &= 0x3fffffff;

	/* Get the videocore memory size from the IPC mailbox if not yet
	 * assigned.
	 */
	if (mm_vc_mem_size == 0) {
		ipc_handle = chal_ipc_config(NULL);
		if (ipc_handle == NULL) {
			LOG_ERR("%s: failed to get IPC handlle", __func__);
			return;
		}

		chal_ipc_query_wakeup_vc(ipc_handle, &wakeup_register);
		if ((wakeup_register & ~1) == 0) {
			LOG_DBG("%s: videocore not yet loaded, skipping...",
				__func__);
		} else {
			struct {
				VC_DEBUG_HEADER_T header;
				VC_DEBUG_PARAMS_T params;

			} vc_dbg;

			/*
			 * When we switch to using an Open Kernel
			 * (versus a Secure Kernel) we get the videocore memory
			 * size by reading the debug header.
			 *
			 * The debug Header & params are described using
			 * the VC_DEBUG_HEADER_T and VC_DEBUG_PARAMS_T
			 * structures.
			 * The VC_DEBUG_HEADER_T can be found in the firmware
			 * at an offset of VC_DEBUG_HEADER_OFFSET.
			 */

			if (vc_mem_access_mem
			    (0, &vc_dbg,
			     mm_vc_mem_load + VC_DEBUG_HEADER_OFFSET,
			     sizeof(vc_dbg)) == 0) {
				if ((vc_dbg.header.magic ==
				     VC_DEBUG_HEADER_MAGIC)
				    && (vc_dbg.header.paramSize >=
					sizeof(vc_dbg.params))) {
					/*
					 * Looks like a valid debug header.
					 * Grab the size.
					 */
					mm_vc_mem_size =
					    vc_dbg.params.vcMemSize;
				}
			}

			if (mm_vc_mem_size == 0) {
				if (chal_ipc_read_mailbox(ipc_handle,
							  IPC_MAILBOX_ID_0,
							  &mm_vc_mem_size) !=
				    BCM_SUCCESS) {
					LOG_ERR
					    ("%s: failed to read from IPC mailbox",
					     __func__);
				}
			}
		}
	}
}

/****************************************************************************
*
*   vc_mem_get_current_size
*
***************************************************************************/

int vc_mem_get_current_size(void)
{
	vc_mem_get_size();
	printk(KERN_INFO "vc-mem: current size check = 0x%08x (%u MiB)\n",
	       mm_vc_mem_size, mm_vc_mem_size / (1024 * 1024));
	return mm_vc_mem_size;
}

EXPORT_SYMBOL_GPL(vc_mem_get_current_size);

/****************************************************************************
*
*   vc_mem_get_current_base
*
***************************************************************************/

int vc_mem_get_current_base(void)
{
	vc_mem_get_size();
	printk(KERN_INFO "vc-mem: current base check = 0x%08x (%u MiB)\n",
	       mm_vc_mem_base, mm_vc_mem_base / (1024 * 1024));
	return mm_vc_mem_base;
}

EXPORT_SYMBOL_GPL(vc_mem_get_current_base);

/****************************************************************************
*
*   vc_mem_get_current_load
*
***************************************************************************/

int vc_mem_get_current_load(void)
{
	vc_mem_get_size();
	printk(KERN_INFO "vc-mem: current load check = 0x%08x (%u MiB)\n",
	       mm_vc_mem_load, mm_vc_mem_load / (1024 * 1024));
	return mm_vc_mem_load;
}

EXPORT_SYMBOL_GPL(vc_mem_get_current_load);

/****************************************************************************
*
*   vc_mem_ioctl
*
***************************************************************************/

static long vc_mem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;

	(void)cmd;
	(void)arg;

	LOG_DBG("%s: called file = 0x%p", __func__, file);

	switch (cmd) {
	case VC_MEM_IOC_MEM_PHYS_ADDR:
		{
			LOG_DBG("%s: VC_MEM_IOC_MEM_PHYS_ADDR=0x%p",
				__func__, (void *)mm_vc_mem_phys_addr);

			if (copy_to_user((void *)arg, &mm_vc_mem_phys_addr,
					 sizeof(mm_vc_mem_phys_addr)) != 0) {
				rc = -EFAULT;
			}
			break;
		}
	case VC_MEM_IOC_MEM_SIZE:
		{
			/* Get the videocore memory size first */
			vc_mem_get_size();

			LOG_DBG("%s: VC_MEM_IOC_MEM_SIZE=%u", __func__,
				mm_vc_mem_size);

			if (copy_to_user((void *)arg, &mm_vc_mem_size,
					 sizeof(mm_vc_mem_size)) != 0) {
				rc = -EFAULT;
			}
			break;
		}
	case VC_MEM_IOC_MEM_BASE:
		{
			/* Get the videocore memory size first */
			vc_mem_get_size();

			LOG_DBG("%s: VC_MEM_IOC_MEM_BASE=%u", __func__,
				mm_vc_mem_base);

			if (copy_to_user((void *)arg, &mm_vc_mem_base,
					 sizeof(mm_vc_mem_base)) != 0) {
				rc = -EFAULT;
			}
			break;
		}
	case VC_MEM_IOC_MEM_LOAD:
		{
			/* Get the videocore memory size first */
			vc_mem_get_size();

			LOG_DBG("%s: VC_MEM_IOC_MEM_LOAD=%u", __func__,
				mm_vc_mem_load);

			if (copy_to_user((void *)arg, &mm_vc_mem_load,
					 sizeof(mm_vc_mem_load)) != 0) {
				rc = -EFAULT;
			}
			break;
		}
	default:
		{
			return -ENOTTY;
		}
	}
	LOG_DBG("%s: file = 0x%p returning %d", __func__, file, rc);

	return rc;
}

/****************************************************************************
*
*   vc_mem_mmap
*
***************************************************************************/

static int vc_mem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int rc = 0;
	unsigned long length = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	LOG_DBG("%s: vm_start = 0x%08lx vm_end = 0x%08lx vm_pgoff = 0x%08lx",
		__func__, (long)vma->vm_start, (long)vma->vm_end,
		(long)vma->vm_pgoff);

	if (offset < mm_vc_mem_base) {
		LOG_ERR("%s: offset %ld is too small", __func__, offset);
		return -EINVAL;
	}

	if (offset >= mm_vc_mem_base + mm_vc_mem_size) {
		LOG_ERR("%s: offset %ld is too big", __func__, length);
		return -EINVAL;
	}

	if (offset + length > mm_vc_mem_base + mm_vc_mem_size) {
		LOG_ERR("%s: length %ld is too big", __func__, length);
		return -EINVAL;
	}
	/* Do not cache the memory map */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	rc = remap_pfn_range(vma, vma->vm_start,
			     (mm_vc_mem_phys_addr >> PAGE_SHIFT) +
			     vma->vm_pgoff, length, vma->vm_page_prot);
	if (rc != 0)
		LOG_ERR("%s: remap_pfn_range failed (rc=%d)", __func__, rc);

	return rc;
}

/****************************************************************************
*
*   File Operations for the driver.
*
***************************************************************************/

static const struct file_operations vc_mem_fops = {
	.owner = THIS_MODULE,
	.open = vc_mem_open,
	.release = vc_mem_release,
	.unlocked_ioctl = vc_mem_ioctl,
	.mmap = vc_mem_mmap,
};

/****************************************************************************
*
*   vc_mem_proc_read
*
***************************************************************************/

static int vc_mem_proc_read(char *buf, char **start, off_t offset, int count,
			    int *eof, void *data)
{
	char *p = buf;

	(void)start;
	(void)count;
	(void)data;

	if (offset > 0) {
		*eof = 1;
		return 0;
	}
	/* Get the videocore memory size first */
	vc_mem_get_size();

	p += sprintf(p, "Videocore memory:\n");
	p += sprintf(p, "   Physical address: 0x%p\n",
		     (void *)mm_vc_mem_phys_addr);
	p += sprintf(p, "   Base Offset:      0x%08x (%u MiB)\n",
		     mm_vc_mem_base, mm_vc_mem_base >> 20);
	p += sprintf(p, "   Load Offset:      0x%08x (%u MiB)\n",
		     mm_vc_mem_load, mm_vc_mem_load >> 20);
	p += sprintf(p, "   Length (bytes):   %u (%u MiB)\n", mm_vc_mem_size,
		     mm_vc_mem_size >> 20);

	*eof = 1;
	return p - buf;
}

/****************************************************************************
*
*   vc_mem_proc_write
*
***************************************************************************/

static int vc_mem_proc_write(struct file *file, const char __user *buffer,
			     unsigned long count, void *data)
{
	int rc = -EFAULT;
	char input_str[10];

	memset(input_str, 0, sizeof(input_str));

	if (count > sizeof(input_str)) {
		LOG_ERR("%s: input string length too long", __func__);
		goto out;
	}

	if (copy_from_user(input_str, buffer, count - 1)) {
		LOG_ERR("%s: failed to get input string", __func__);
		goto out;
	}

	if (strncmp(input_str, "connect", strlen("connect")) == 0)
		/* Get the videocore memory size from the videocore */
		vc_mem_get_size();

out:
	return rc;
}

/****************************************************************************
*
*   vc_mem_connected_init
*
*   This function is called once the videocore has been connected.
*
***************************************************************************/

static int __init vc_mem_init(void)
{
	int rc = -EFAULT;
	struct device *dev;
	uint32_t base, load, size;

	/*
	 * NOTE: vc_mem can't use vchiq_add_connected_callback,
	 *       otherwise it will create a module dependancy loop.
	 *
	 * As long as u-boot has initialized the videocore, then this doesn't
	 * cause any problems. The only corner case occurs when the vc_mem
	 * module is statically linked into the kernel and the kernel does the
	 * videocore firmware initialization. In that case we'll report that
	 * the memory size isn't available, and calls to retrieve the memory
	 * size will return zero until the videocore has been initialized.
	 */

	printk(KERN_INFO "vc-mem: Videocore memory driver\n");

	if (vc_dt_get_mem_config(&base, &load, &size) == 0) {
		mm_vc_mem_phys_addr = (unsigned long)(base & 0xC0000000);
		mm_vc_mem_base = (unsigned int)(base & 0x3FFFFFFF);
		mm_vc_mem_load = (unsigned int)(load & 0x3FFFFFFF);
		mm_vc_mem_size = size;
		printk(KERN_INFO
		       "vc-mem: p: 0x%08lx, b: 0x%08x, l: 0x%08x, s: 0x%08x\n",
		       mm_vc_mem_phys_addr, base, load, size);
	} else {
		/* fallback to default VC EMI, Capri >= A2 */
		mm_vc_mem_phys_addr = 0xC0000000;
		mm_vc_mem_base = (unsigned int)mm_vc_mem_phys_addr;
		mm_vc_mem_load = (unsigned int)mm_vc_mem_phys_addr;
		printk(KERN_WARNING
		       "vc-mem: legacy-base = 0x%08x, update your dt-blob!\n",
		       mm_vc_mem_base);
	}

	vc_mem_get_size();

	rc = alloc_chrdev_region(&vc_mem_devnum, 0, 1, DRIVER_NAME);
	if (rc < 0) {
		LOG_ERR("%s: alloc_chrdev_region failed (rc=%d)", __func__, rc);
		goto out_err;
	}

	cdev_init(&vc_mem_cdev, &vc_mem_fops);
	rc = cdev_add(&vc_mem_cdev, vc_mem_devnum, 1);
	if (rc != 0) {
		LOG_ERR("%s: cdev_add failed (rc=%d)", __func__, rc);
		goto out_unregister;
	}

	vc_mem_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(vc_mem_class)) {
		rc = PTR_ERR(vc_mem_class);
		LOG_ERR("%s: class_create failed (rc=%d)", __func__, rc);
		goto out_cdev_del;
	}

	dev = device_create(vc_mem_class, NULL, vc_mem_devnum, NULL,
			    DRIVER_NAME);
	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		LOG_ERR("%s: device_create failed (rc=%d)", __func__, rc);
		goto out_class_destroy;
	}

	vc_mem_proc_entry = create_proc_entry(DRIVER_NAME, 0444, NULL);
	if (vc_mem_proc_entry == NULL) {
		rc = -EFAULT;
		LOG_ERR("%s: create_proc_entry failed", __func__);
		goto out_device_destroy;
	}
	vc_mem_proc_entry->read_proc = vc_mem_proc_read;
	vc_mem_proc_entry->write_proc = vc_mem_proc_write;

	vc_mem_inited = 1;
	return 0;

out_device_destroy:
	device_destroy(vc_mem_class, vc_mem_devnum);

out_class_destroy:
	class_destroy(vc_mem_class);
	vc_mem_class = NULL;

out_cdev_del:
	cdev_del(&vc_mem_cdev);

out_unregister:
	unregister_chrdev_region(vc_mem_devnum, 1);

out_err:
	return -1;
}

/****************************************************************************
*
*   vc_mem_exit
*
***************************************************************************/

static void __exit vc_mem_exit(void)
{
	LOG_DBG("%s: called", __func__);

	if (vc_mem_inited) {
		remove_proc_entry(vc_mem_proc_entry->name, NULL);
		device_destroy(vc_mem_class, vc_mem_devnum);
		class_destroy(vc_mem_class);
		cdev_del(&vc_mem_cdev);
		unregister_chrdev_region(vc_mem_devnum, 1);
	}
}

module_init(vc_mem_init);
module_exit(vc_mem_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom Corporation");
