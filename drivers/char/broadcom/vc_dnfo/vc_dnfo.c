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
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

#include <vc_dnfo.h>

#define DRIVER_NAME  "vc-dnfo"
#define PROC_WRITE_BUF_SIZE      128

#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)

/* Uncomment to enable debug logging */
/* #define ENABLE_DBG */

#if defined(ENABLE_DBG)
#define LOG_DBG(fmt, ...)  printk(KERN_INFO fmt "\n", ##__VA_ARGS__)
#else
#define LOG_DBG(fmt, ...)
#endif
#define LOG_ERR(fmt, ...)  printk(KERN_ERR fmt "\n", ##__VA_ARGS__)

/* Device (/dev) related variables */
static dev_t vc_dnfo_devnum;
static struct class *vc_dnfo_class;
static struct cdev vc_dnfo_cdev;
static struct vc_dnfo_display_info vc_dnfo_info;
/* Proc entry */
static struct proc_dir_entry *vc_dnfo_proc_entry;

#define VC_DNFO_UINT32 0
#define VC_DNFO_SINT32 1
#define VC_DNFO_FRAC   2

struct vc_dnfo_lut_entry {
	char   name[8];
	size_t offset;
	int    type;
};

#define DNFO_FOFF(field) offsetof(struct vc_dnfo_display_info, field)

static const struct vc_dnfo_lut_entry vc_dnfo_lut[] = {
	{"width"   , DNFO_FOFF(width)  , VC_DNFO_UINT32},
	{"height"  , DNFO_FOFF(height) , VC_DNFO_UINT32},
	{"scale"   , DNFO_FOFF(scale)  , VC_DNFO_UINT32},
	{"swidth"  , DNFO_FOFF(swidth) , VC_DNFO_UINT32},
	{"sheight" , DNFO_FOFF(sheight), VC_DNFO_UINT32},
	{"bpp"     , DNFO_FOFF(bpp)    , VC_DNFO_UINT32},
	{"smtex"   , DNFO_FOFF(smtex)  , VC_DNFO_UINT32},

	{"layer"   , DNFO_FOFF(layer)  , VC_DNFO_SINT32},

	{"xdpi"    , DNFO_FOFF(xdpi)   , VC_DNFO_FRAC  },
	{"ydpi"    , DNFO_FOFF(ydpi)   , VC_DNFO_FRAC  },
	{"fps"     , DNFO_FOFF(fps)    , VC_DNFO_FRAC  },
};

/****************************************************************************
*
*   vc_dnfo_open
*
***************************************************************************/

static int vc_dnfo_open(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	LOG_DBG("[%s]: called file = 0x%p", __func__, file);

	return 0;
}

/****************************************************************************
*
*   vc_dnfo_release
*
***************************************************************************/

static int vc_dnfo_release(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	LOG_DBG("[%s]: called file = 0x%p", __func__, file);

	return 0;
}

/****************************************************************************
*
*   vc_dnfo_ioctl
*
***************************************************************************/

static long vc_dnfo_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	int rc = 0;

	(void)cmd;
	(void)arg;

	LOG_DBG("[%s]: called file = 0x%p", __func__, file);

	switch (cmd) {
	case VC_DNFO_IOC_DISPLAY_INFO:
		{
			if (copy_to_user((void *)arg,
					 &vc_dnfo_info,
					 sizeof(vc_dnfo_info)) != 0) {
				rc = -EFAULT;
			}
			break;
		}
	default:
		{
			return -ENOTTY;
		}
	}
	LOG_DBG("[%s]: file = 0x%p returning %d", __func__, file, rc);

	return rc;
}

/****************************************************************************
*
*   File Operations for the driver.
*
***************************************************************************/

static const struct file_operations vc_dnfo_fops = {
	.owner = THIS_MODULE,
	.open = vc_dnfo_open,
	.release = vc_dnfo_release,
	.unlocked_ioctl = vc_dnfo_ioctl,
};

/****************************************************************************
*
*   vc_dnfo_proc_read
*
***************************************************************************/

static int vc_dnfo_proc_read(char *buf, char **start, off_t offset, int count,
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

	p += sprintf(p, "Display Information for User Application:\n\n");
	p += sprintf(p, "     \'width\'       : %d (pix)\n",
		     vc_dnfo_info.width);
	p += sprintf(p, "     \'height\'      : %d (pix)\n",
		     vc_dnfo_info.height);
	p += sprintf(p, "     \'scale\'       : %s\n",
		     vc_dnfo_info.scale ? "yes" : "no");
	if (vc_dnfo_info.scale) {
		p += sprintf(p, "     \'swidth\'      : %d (pix)\n",
			     vc_dnfo_info.swidth);
		p += sprintf(p, "     \'sheight\'     : %d (pix)\n",
			     vc_dnfo_info.sheight);
	}
	p += sprintf(p, "     \'bpp\'         : %d\n", vc_dnfo_info.bpp);
	p += sprintf(p, "     \'layer\'       : %d (VC layer)\n",
		     vc_dnfo_info.layer);
	p += sprintf(p,
		"     \'xdpi\'        : %d.%0" STRINGIZE(VC_DNFO_FRAC_DIGITS) "d\n",
		vc_dnfo_info.xdpi / VC_DNFO_FRAC_MULTIPLIER,
		vc_dnfo_info.xdpi % VC_DNFO_FRAC_MULTIPLIER);
	p += sprintf(p,
		"     \'ydpi\'        : %d.%0" STRINGIZE(VC_DNFO_FRAC_DIGITS) "d\n",
		vc_dnfo_info.ydpi / VC_DNFO_FRAC_MULTIPLIER,
		vc_dnfo_info.ydpi % VC_DNFO_FRAC_MULTIPLIER);
	p += sprintf(p, "     \'smtex\'       : %s\n",
		     vc_dnfo_info.smtex ? "yes" : "no");
	p += sprintf(p,
		"     \'fps\'         : %d.%0" STRINGIZE(VC_DNFO_FRAC_DIGITS) "d\n",
		vc_dnfo_info.fps / VC_DNFO_FRAC_MULTIPLIER,
		vc_dnfo_info.fps % VC_DNFO_FRAC_MULTIPLIER);
	*eof = 1;
	return p - buf;
}

/* Fractional value scanner. */
static int vc_dnfo_scan_frac(char *field, int *value)
{
	int ret = -1;
	int working;
	char *integer;
	char *fraction = field;

	if (!field || !value)
		return -1;

	/* Split string. */
	integer = strsep(&fraction, ".");

	if (sscanf(integer, "%d", &working) == 1) {
		working *= VC_DNFO_FRAC_MULTIPLIER;
		ret = 0;
	}

	if ((ret == 0) && fraction && *fraction) {
		int digit_count = strspn(fraction, "0123456789");
		int unsigned frac = 0;
		int logmult;

		/* Truncate to max digit count + 1, to allow for rounding. */
		if (digit_count > (VC_DNFO_FRAC_DIGITS + 1))
			digit_count = VC_DNFO_FRAC_DIGITS + 1;

		fraction[digit_count] = 0;
		logmult = VC_DNFO_FRAC_DIGITS + 1 - digit_count;

		if (sscanf(fraction, "%u", &frac) == 1) {
			/* Scale. */
			while (logmult--)
				frac *= 10;

			/* Round. */
			frac += 5;
			frac /= 10;

			if (working >= 0)
				working += frac;
			else
				working -= frac;
		}
	}

	if (ret == 0)
		*value = working;

	return ret;
}


/****************************************************************************
*
*   vc_dnfo_proc_write
*
***************************************************************************/

static int vc_dnfo_proc_write(struct file *file, const char __user *buffer,
			      unsigned long count, void *data)
{
	int ret;
	int i;
	char kbuf[PROC_WRITE_BUF_SIZE + 1];
	char *name;
	char *source;
	const struct vc_dnfo_lut_entry *lut_entry = NULL;

	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (copy_from_user(kbuf, buffer, count) != 0) {
		LOG_ERR("[%s]: failed to copy-from-user", __func__);

		ret = -EFAULT;
		goto out;
	}

	/* Return read value no matter what from there on.
	 */
	kbuf[count] = '\0';
	ret = count;

	/* Extract name and move on to value field. */
	source = kbuf;
	name = strsep(&source, " \t");

	for (i = 0; i < ARRAY_SIZE(vc_dnfo_lut); i++) {
		if (strcmp(name, vc_dnfo_lut[i].name) == 0) {
			lut_entry = &vc_dnfo_lut[i];
			break;
		}
	}

	if (!lut_entry) {
		LOG_ERR("[%s]: attribute '%s' is **unknown** to driver '%s'",
				__func__, name, DRIVER_NAME);
	} else if (lut_entry->type == VC_DNFO_FRAC) {
		/* Fractional format. */
		int *field = (int *)((char *)&vc_dnfo_info + lut_entry->offset);

		if (vc_dnfo_scan_frac(source, field)) {
			LOG_ERR("[%s]: echo <field> <value> > /proc/%s",
				__func__, DRIVER_NAME);
		}
	} else {
		/* Either signed or unsigned format. */
		int value;
		int *field = (int *)((char *)&vc_dnfo_info + lut_entry->offset);

		if (sscanf(source, "%d", &value) != 1) {
			/* Bad format. */
			LOG_ERR("[%s]: echo <field> <value> > /proc/%s",
				__func__, DRIVER_NAME);
		} else if ((value < 0) && lut_entry->type == VC_DNFO_UINT32) {
			/* Out of range. */
			LOG_ERR("[%s]: attribute '%s' requires '<value> >= 0'",
				__func__, name);
		} else {
			*field = value;
		}
	}

out:
	return ret;
}

/****************************************************************************
*
*   vc_dnfo_init
*
***************************************************************************/

static int __init vc_dnfo_init(void)
{
	int rc;
	struct device *dev;

	LOG_DBG("[%s]: called", __func__);

	rc = alloc_chrdev_region(&vc_dnfo_devnum, 0, 1, DRIVER_NAME);
	if (rc < 0) {
		LOG_ERR("[%s]: alloc_chrdev_region failed (rc=%d)",
			__func__, rc);
		goto out_err;
	}

	cdev_init(&vc_dnfo_cdev, &vc_dnfo_fops);
	rc = cdev_add(&vc_dnfo_cdev, vc_dnfo_devnum, 1);
	if (rc != 0) {
		LOG_ERR("[%s]: cdev_add failed (rc=%d)", __func__, rc);
		goto out_unregister;
	}

	vc_dnfo_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(vc_dnfo_class)) {
		rc = PTR_ERR(vc_dnfo_class);
		LOG_ERR("[%s]: class_create failed (rc=%d)", __func__, rc);
		goto out_cdev_del;
	}

	dev = device_create(vc_dnfo_class, NULL, vc_dnfo_devnum, NULL,
			    DRIVER_NAME);
	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		LOG_ERR("[%s]: device_create failed (rc=%d)", __func__, rc);
		goto out_class_destroy;
	}

	vc_dnfo_proc_entry = create_proc_entry(DRIVER_NAME, 0660, NULL);
	if (vc_dnfo_proc_entry == NULL) {
		rc = -EFAULT;
		LOG_ERR("[%s]: create_proc_entry failed", __func__);
		goto out_device_destroy;
	}
	vc_dnfo_proc_entry->read_proc = vc_dnfo_proc_read;
	vc_dnfo_proc_entry->write_proc = vc_dnfo_proc_write;

	/* Populate default values.
	 */
	vc_dnfo_info.width = 1024;
	vc_dnfo_info.height = 600;
	vc_dnfo_info.scale = 1;
	vc_dnfo_info.swidth = 1366;
	vc_dnfo_info.sheight = 768;
	vc_dnfo_info.bpp = 32;
	vc_dnfo_info.layer = 1;
	vc_dnfo_info.xdpi = 200 * VC_DNFO_FRAC_MULTIPLIER;
	vc_dnfo_info.ydpi = 200 * VC_DNFO_FRAC_MULTIPLIER;
	vc_dnfo_info.smtex = 0;
	vc_dnfo_info.fps = 60 * VC_DNFO_FRAC_MULTIPLIER;

	vc_dnfo_info.frac_scale = VC_DNFO_FRAC_MULTIPLIER;

	return 0;

out_device_destroy:
	device_destroy(vc_dnfo_class, vc_dnfo_devnum);

out_class_destroy:
	class_destroy(vc_dnfo_class);
	vc_dnfo_class = NULL;

out_cdev_del:
	cdev_del(&vc_dnfo_cdev);

out_unregister:
	unregister_chrdev_region(vc_dnfo_devnum, 1);

out_err:
	return rc;
}

/****************************************************************************
*
*   vc_dnfo_exit
*
***************************************************************************/

static void __exit vc_dnfo_exit(void)
{
	LOG_DBG("[%s]: called", __func__);

	remove_proc_entry(vc_dnfo_proc_entry->name, NULL);
	device_destroy(vc_dnfo_class, vc_dnfo_devnum);
	class_destroy(vc_dnfo_class);
	cdev_del(&vc_dnfo_cdev);
	unregister_chrdev_region(vc_dnfo_devnum, 1);
}

module_init(vc_dnfo_init);
module_exit(vc_dnfo_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom Corporation");
