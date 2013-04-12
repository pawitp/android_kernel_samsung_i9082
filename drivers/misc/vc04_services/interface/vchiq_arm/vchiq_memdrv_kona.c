/*****************************************************************************
* Copyright 2001 - 2010 Broadcom Corporation.  All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <vchiq_platform_data_memdrv_kona.h>
#include <asm-generic/gpio.h>

#include "vchiq_core.h"
#include "vchiq_memdrv.h"

#if defined(CONFIG_ARCH_ISLAND)
#include <mach/mpuHw.h>
#endif

/****************************************************************************
*
* vchiq_memdrv_kona_interface_probe
*
*   This function will be called for each "vchiq_memdrv_kona" device which is
*   registered in the board definition.
*
***************************************************************************/

static int __devinit vchiq_memdrv_kona_interface_probe(
	struct platform_device *pdev)
{
	 int i;
	 int rc;
	 VCHIQ_PLATFORM_DATA_MEMDRV_KONA_T *platform_data =
		pdev->dev.platform_data;
	 const char *name = platform_data->memdrv.common.instance_name;

	 printk(KERN_INFO "vchiq_memdrv_kona: Probing '%s' ...\n", name);
	 printk(KERN_INFO "vchiq_memdrv_kona: Shared Memory: 0x%08x\n",
		      (uint32_t)platform_data->memdrv.sharedMemVirt);

	 platform_set_drvdata(pdev, NULL);

#if defined(CONFIG_ARCH_ISLAND)
	 {
		/*
		** Set the entire SRAM to be unsecure. The API only allows
		** is to do 4K at a time instead of 3 x 32 register writes.
		*/

		printk(KERN_INFO "Calling mpu Memory Region ACCESS_OPEN\n");

		mpuHw_MEMORY_REGION_e region;

		for (region = mpuHw_MEMORY_REGION_0K_4K;
			region <= mpuHw_MEMORY_REGION_160K_164K; region++)
			mpuHw_setSRAM_AccessMode(region,
				mpuHw_MEMORY_ACCESS_OPEN);
	 }
#endif

	for (i = 0; i < platform_data->num_gpio_configs; i++) {
		VCHIQ_PLATFORM_MEMDRV_KONA_DATA_GPIO_T  *gpio_config;

		gpio_config = &platform_data->gpio_config[i];

		rc = gpio_request(gpio_config->gpio,
			gpio_config->description);
		if (rc < 0) {
			printk(KERN_ERR
				"%s: gpio_request(%d, '%s') failed: %d\n",
				__func__, gpio_config->gpio,
				gpio_config->description, rc);
			return -ENODEV;
		}

		if (gpio_config->input)
			gpio_direction_input(gpio_config->gpio);
		else
			gpio_direction_output(gpio_config->gpio,
				gpio_config->initial_level);

		printk(KERN_INFO
			"Requested GPIO %3d (%s) as %s for videocore\n",
			gpio_config->gpio, gpio_config->description,
			gpio_config->input ? "input" : "output");
	 }

	 /*
	  * This needs to be moved up into vchiq_memdrv.c
	  */

	if (vchiq_userdrv_create_instance(&platform_data->memdrv.common) !=
		VCHIQ_SUCCESS) {
		printk(KERN_ERR "vchiq_memdrv_kona: Failed to create vchiq "
			"instance for '%s'\n",
			name);

		return -ENOMEM;
	}

	printk(KERN_INFO "vchiq_memdrv_kona: connecting\n");
	vchiq_memdrv_initialise();

	return 0;
}

/****************************************************************************
*
* vchiq_memdrv_kona_interface_remove
*
*   Register a "driver". We do this so that the probe routine will be called
*   when a corresponding architecture device is registered.
*
***************************************************************************/

static int vchiq_memdrv_kona_interface_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);

	return 0;
}

/****************************************************************************
*
* vchiq_memdrv_kona_interface_suspend
*
*   Pass the suspend call down to the implementation.
*
***************************************************************************/

static int vchiq_memdrv_kona_interface_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	VCHIQ_PLATFORM_DATA_MEMDRV_KONA_T *platform_data =
		pdev->dev.platform_data;
	VCHIQ_STATUS_T status;

	(void)state;
	
	/* power down cameras */
	if (platform_data->gpio_config) {
		int i = 0;
		/* find PWDN pin */
		for (i = 0; i < platform_data->num_gpio_configs; i++) {
			if (strstr(platform_data->gpio_config[i].description, "_PWDN" ))
			{
				gpio_direction_output(platform_data->gpio_config[i].gpio, 1);
			}
		}
	}

	status = vchiq_userdrv_suspend(&platform_data->memdrv.common);

	return (status == VCHIQ_SUCCESS) ? 0 : -EIO;
}

/****************************************************************************
*
* vchiq_memdrv_kona_interface_resume
*
*   Pass the suspend call down to the implementation.
*
***************************************************************************/

static int vchiq_memdrv_kona_interface_resume(struct platform_device *pdev)
{
	VCHIQ_PLATFORM_DATA_MEMDRV_KONA_T *platform_data =
		pdev->dev.platform_data;
	VCHIQ_STATUS_T status;

	/* reset camera power down pin to initial val */
	if (platform_data->gpio_config) {
		int i = 0;
		/* find PWDN pin */
		for (i = 0; i < platform_data->num_gpio_configs; i++) {
			if (strstr(platform_data->gpio_config[i].description, "_PWDN"))
			{
				gpio_direction_output(
					platform_data->gpio_config[i].gpio,
					platform_data->gpio_config[i].initial_level);
			}
		}
	}

	status = vchiq_userdrv_resume(&platform_data->memdrv.common);

	return (status == VCHIQ_SUCCESS) ? 0 : -EIO;
}

/****************************************************************************
*
* vchiq_memdrv_kona_interface_driver
*
*   Register a "driver". We do this so that the probe routine will be called
*   when a corresponding architecture device is registered.
*
***************************************************************************/

static struct platform_driver vchiq_memdrv_kona_interface_driver = {
	.probe          = vchiq_memdrv_kona_interface_probe,
	.remove         = vchiq_memdrv_kona_interface_remove,
	.suspend        = vchiq_memdrv_kona_interface_suspend,
	.resume         = vchiq_memdrv_kona_interface_resume,
	.driver = {
		.name	    = "vchiq_memdrv_kona",
	}
};

/****************************************************************************
*
* vchiq_memdrv_kona_interface_init
*
*   Creates the instance that is used to access the videocore(s). One
*   instance is created per videocore.
*
***************************************************************************/

static int __init vchiq_memdrv_kona_interface_init(void)
{
	return platform_driver_register(&vchiq_memdrv_kona_interface_driver);
}

/****************************************************************************
*
* vchiq_memdrv_kona_interface_exit
*
*   Called when the module is unloaded.
*
***************************************************************************/

static void __exit vchiq_memdrv_kona_interface_exit(void)
{
	platform_driver_unregister(&vchiq_memdrv_kona_interface_driver);
}

module_init(vchiq_memdrv_kona_interface_init);
module_exit(vchiq_memdrv_kona_interface_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("VCHIQ Shared Memory Interface Driver");
MODULE_LICENSE("GPL");
