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

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

#include <linux/broadcom/flightmode_opt.h>
#include <mach/clock.h>
#include <mach/cpufreq_memc.h>
#include <mach/pm.h>

#define DRIVER_NAME "flightmode-opt"

static dev_t flightmode_opt_dev;
static struct cdev flightmode_opt_cdev;
static struct class *flightmode_opt_class;

/****************************************************************************
*
*   flightmode_opt_open
*
***************************************************************************/

static int flightmode_opt_open(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	return 0;
}

/****************************************************************************
*
*   flightmode_opt_release
*
***************************************************************************/

static int flightmode_opt_release(struct inode *inode, struct file *file)
{
	(void)inode;
	(void)file;

	return 0;
}

/****************************************************************************
*
*   flightmode_opt_ioctl
*
***************************************************************************/

static long flightmode_opt_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	int rc = 0;

	switch (cmd) {
	case FLIGHTMODE_OPT_IOCTL_ENABLE_OPT:
		printk(KERN_INFO "%s: FLIGHTMODE_OPT_IOCTL_ENABLE_OPT\n",
		       __func__);
		/* There is currently a known issue with switching the VAR_*
		   clock sources while CAPH is enabled that causes dap_sys_clk
		   to lock up on Rhea and Capri platforms, so we avoid moving
		   the VAR_* clocks on these platforms */
#if !defined(CONFIG_ARCH_CAPRI) && !defined(CONFIG_ARCH_RHEA)
		/* Move all clocks from PLL1 -> PLL0 */
		clk_switch_src_pll0_pll1(1);
#endif


		/* Enable MEMC capping to SYSPLL when A9 @ 312 MHz or lower */
		capri_cpufreq_memc_ctrl(1, 312000, 2);

		/* Reduce EMI idle time before power down */
		capri_reduce_emi_idle_time_before_pwrdn(true);

		break;
	case FLIGHTMODE_OPT_IOCTL_DISABLE_OPT:
		printk(KERN_INFO "%s: FLIGHTMODE_OPT_IOCTL_DISABLE_OPT\n",
		       __func__);

		/* Restore EMI idle time before power down */
		capri_reduce_emi_idle_time_before_pwrdn(false);

		/* Disable MEMC capping */
		capri_cpufreq_memc_ctrl(0, 312000, 2);

#if !defined(CONFIG_ARCH_CAPRI) && !defined(CONFIG_ARCH_RHEA)
		/* Restore all clocks moved from PLL1 -> PLL0 */
		clk_switch_src_pll0_pll1(0);
#endif

		break;
	default:
		printk(KERN_INFO "%s: unknown command received 0x%x\n",
		       __func__, cmd);
		return -ENOTTY;
	}

	return rc;
}

/****************************************************************************
*
*   File Operations for the driver.
*
***************************************************************************/

static const struct file_operations flightmode_opt_fops = {
	.owner = THIS_MODULE,
	.open = flightmode_opt_open,
	.release = flightmode_opt_release,
	.unlocked_ioctl = flightmode_opt_ioctl,
};

static int __init flightmode_opt_init(void)
{
	int rc;
	struct device *dev;

	printk(KERN_INFO "%s: called\n", __func__);

	rc = alloc_chrdev_region(&flightmode_opt_dev, 0, 1, DRIVER_NAME);
	if (rc < 0) {
		printk(KERN_ERR "%s: alloc_chrdev_region failed (rc=%d)\n",
		       __func__, rc);
		return rc;
	}
	cdev_init(&flightmode_opt_cdev, &flightmode_opt_fops);
	flightmode_opt_cdev.owner = THIS_MODULE;

	rc = cdev_add(&flightmode_opt_cdev, flightmode_opt_dev, 1);
	if (rc != 0) {
		printk(KERN_ERR "%s: cdev_add failed (rc=%d)\n", __func__, rc);
		goto out_unregister;
	}

	flightmode_opt_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(flightmode_opt_class)) {
		rc = PTR_ERR(flightmode_opt_class);
		printk(KERN_ERR "%s: class_create failed (rc=%d)\n", __func__,
		       rc);
		goto out_cdev_del;
	}
	dev = device_create(flightmode_opt_class, NULL, flightmode_opt_dev,
			    NULL, DRIVER_NAME);
	if (IS_ERR(dev)) {
		rc = PTR_ERR(dev);
		printk(KERN_ERR "%s: device_create failed (rc=%d)\n", __func__,
		       rc);
		goto out_class_del;
	}

	return 0;

out_class_del:
	class_destroy(flightmode_opt_class);
	flightmode_opt_class = NULL;

out_cdev_del:
	cdev_del(&flightmode_opt_cdev);

out_unregister:
	unregister_chrdev_region(flightmode_opt_dev, 1);

	return rc;
}

static void __exit flightmode_opt_exit(void)
{
	device_destroy(flightmode_opt_class, flightmode_opt_dev);
	class_destroy(flightmode_opt_class);
	cdev_del(&flightmode_opt_cdev);
	unregister_chrdev_region(flightmode_opt_dev, 1);
}

module_init(flightmode_opt_init);
module_exit(flightmode_opt_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom Corporation");
