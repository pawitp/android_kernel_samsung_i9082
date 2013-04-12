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

/****************************************************************************/
/**
*   @file   user-gpio.c
*
*   @brief  Implements the user-mode GPIO interface interface.
*/
/****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <asm/ioctls.h>

#include <linux/gpio.h>
#include <linux/broadcom/user-gpio.h>

/* ---- Public Variables ------------------------------------------------- */

/* ---- Private Constants and Types -------------------------------------- */

static char gBanner[] __initdata = KERN_INFO "Broadcom GPIO Driver: 1.00\n";

/* ---- Private Variables ------------------------------------------------ */

#define GPIO_DRV_DEV_NAME   "user-gpio"

static dev_t gGpioDrvDevNum;
static struct class *gGpioDrvClass;
static struct cdev gGpioDrvCDev;
static char *gpio_name[ARCH_NR_GPIOS];

/* ---- Private Function Prototypes -------------------------------------- */

/* ---- Functions  ------------------------------------------------------- */

/****************************************************************************
*
*   Called when the GPIO driver ioctl is made
*
***************************************************************************/
long gpio_drv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;

	switch (cmd) {
	case GPIO_IOCTL_REQUEST:
		{
			GPIO_Request_t request;
			int labelLen;

			if (copy_from_user(&request,
					   (GPIO_Request_t *) arg,
					   sizeof(request)) != 0)
				return -EFAULT;

			if (request.gpio >= ARCH_NR_GPIOS)
				return -EINVAL;

			/*
			 * gpiolib stores a pointer to the string that we pass
			 * in, and doesn't give us any way to release it. So we
			 * introduce a small memory leak, and allocate some
			 * memory to put a copy of the label into. It's not
			 * anticipated that many actual usermode programs will
			 * use this, and it will be a very low frequency thing,
			 * so this should be ok.
			 */
			request.label[sizeof(request.label) - 1] = '\0';
			labelLen = strlen(request.label) + 5;
			gpio_name[request.gpio] = kmalloc(labelLen, GFP_KERNEL);
			strlcpy(gpio_name[request.gpio], "UM: ", labelLen);
			strlcat(gpio_name[request.gpio], request.label,
				labelLen);
			rc = gpio_request(request.gpio,
					  gpio_name[request.gpio]);
			break;
		}

	case GPIO_IOCTL_FREE:
		{
			unsigned int gpio = (unsigned int)arg;

			if (gpio >= ARCH_NR_GPIOS)
				return -EINVAL;
			gpio_free(gpio);
			kfree(gpio_name[gpio]);
			break;
		}

	case GPIO_IOCTL_DIRECTION_INPUT:
		rc = gpio_direction_input((unsigned)arg);
		break;

	case GPIO_IOCTL_DIRECTION_OUTPUT:
		{
			GPIO_Value_t param;

			if (copy_from_user(&param,
					   (GPIO_Value_t *) arg,
					   sizeof(param)) != 0)
				return -EFAULT;
			rc = gpio_direction_output(param.gpio, param.value);
			break;
		}

#if 0
	case GPIO_IOCTL_DIRECTION_IS_OUTPUT:
		{
			GPIO_Value_t param;

			if (copy_from_user(&param,
					   (GPIO_Value_t *) arg,
					   sizeof(param)) != 0)
				return -EFAULT;
			param.value = gpio_direction_is_output(param.gpio);

			if (copy_to_user((GPIO_Value_t *) arg,
					 &param, sizeof(param)) != 0)
				return -EFAULT;
			break;
#endif

	case GPIO_IOCTL_GET_VALUE:
			{
				GPIO_Value_t param;

				if (copy_from_user(&param,
						   (GPIO_Value_t *) arg,
						   sizeof(param)) != 0)
					return -EFAULT;
				param.value = gpio_get_value(param.gpio);

				if (copy_to_user((GPIO_Value_t *) arg,
						 &param, sizeof(param)) != 0)
					return -EFAULT;
				break;
			}

	case GPIO_IOCTL_SET_VALUE:
			{
				GPIO_Value_t param;

				if (copy_from_user(&param,
						   (GPIO_Value_t *) arg,
						   sizeof(param)) != 0)
					return -EFAULT;
				gpio_set_value(param.gpio, param.value);
				break;
			}

	default:
			printk(KERN_WARNING
			       "gpio_drv:: Unrecognized ioctl: '0x%x'\n", cmd);
			return -ENOTTY;
		}

		return rc;
	}

/****************************************************************************
*
*   File Operations (these are the device driver entry points)
*
***************************************************************************/

	static const struct file_operations gpio_fops = {
		.owner = THIS_MODULE,
		.unlocked_ioctl = gpio_drv_ioctl,
	};

/****************************************************************************
*
*   Called to perform module initialization when the module is loaded
*
***************************************************************************/

	static int __init gpio_drv_init(void) {
		int rc = 0;

		printk(gBanner);

		/* Allocate a major number dynaically */
		rc = alloc_chrdev_region(&gGpioDrvDevNum, 0, 1,
					 GPIO_DRV_DEV_NAME);
		if (rc) {
			printk(KERN_WARNING
			       "gpio: alloc_chrdev_region failed; err: %d\n",
			       rc);
			return rc;
		}

		cdev_init(&gGpioDrvCDev, &gpio_fops);
		gGpioDrvCDev.owner = THIS_MODULE;

		rc = cdev_add(&gGpioDrvCDev, gGpioDrvDevNum, 1);
		if (rc) {
			printk(KERN_WARNING "gpio: cdev_add failed: %d\n", rc);
			goto out_unregister;
		}

		/*
		 * Now that we've added the device, create a class,
		 * so that udev will make the /dev entry
		 */
		gGpioDrvClass = class_create(THIS_MODULE, GPIO_DRV_DEV_NAME);
		if (IS_ERR(gGpioDrvClass)) {
			printk(KERN_WARNING "gpio: Unable to create class\n");
			rc = -1;
			goto out_cdev_del;
		}

		device_create(gGpioDrvClass,
			      NULL, gGpioDrvDevNum, NULL, GPIO_DRV_DEV_NAME);

		goto done;

out_cdev_del:
		cdev_del(&gGpioDrvCDev);

out_unregister:
		unregister_chrdev_region(gGpioDrvDevNum, 1);

done:
		return rc;
	}

/****************************************************************************
*
*   Called to perform module cleanup when the module is unloaded.
*
***************************************************************************/

	static void __exit gpio_drv_exit(void) {
		device_destroy(gGpioDrvClass, gGpioDrvDevNum);
		class_destroy(gGpioDrvClass);

		cdev_del(&gGpioDrvCDev);

		unregister_chrdev_region(gGpioDrvDevNum, 1);
	}

/****************************************************************************/

module_init(gpio_drv_init);
module_exit(gpio_drv_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Broadcom GPIO driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
