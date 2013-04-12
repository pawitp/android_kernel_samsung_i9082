/*
 * linux/drivers/video/backlight/s2c_bl.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
* 	@file	drivers/video/backlight/s2c_bl.c
*
* Unless you and Broadcom execute a separate written software license agreement
* governing use of this software, this software is licensed to you under the
* terms of the GNU General Public License version 2, available at
* http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*******************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/broadcom/lcd.h>
//#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <mach/gpio.h>

#define KEYLED_DEBUG 1
#define KEYPAD_LED_MAX	100
#define	KEYPAD_LED_MIN	0

#if KEYLED_DEBUG
#define KLDBG(fmt, args...) printk(fmt, ## args)
#else
#define KLDBG(fmt, args...)
#endif

struct keypad_led_data {
	struct platform_device *pdev;
	unsigned int ctrl_pin;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend_desc;
#endif
};

int keyled_status = 0;
static int current_intensity = 0;

#define KEY_LED_EN 130

#define KEYLED_ON   1
#define KEYLED_OFF 0

static struct regulator *keyled_regulator=NULL;
static int keyled_suspended;
 

void keyled_power_ctrl(unsigned char on_off)
{
	int rc;
	
	if(on_off==KEYLED_ON)
	{
		KLDBG("[KEYLED] %s, %d Keyled On\n", __func__, __LINE__ );

            rc = gpio_request(KEY_LED_EN,"Touchkey_led");
		if (rc < 0)
		{
			printk("[TSP] touch_power_control unable to request GPIO pin");
			//printk(KERN_ERR "unable to request GPIO pin %d\n", TSP_INT_GPIO_PIN);
			return rc;
		}
		gpio_direction_output(KEY_LED_EN,1);
		gpio_set_value(KEY_LED_EN,1);
		gpio_free(KEY_LED_EN);

	}
	else
	{
		KLDBG("[KEYLED] %s, %d Keyled Off\n", __func__, __LINE__ );

            gpio_request(KEY_LED_EN,"Touchkey_led");
		gpio_direction_output(KEY_LED_EN,0);
		gpio_set_value(KEY_LED_EN,0);
		gpio_free(KEY_LED_EN);
		
	}

}

/* input: intensity in percentage 0% - 100% */
int keyled_set_intensity(struct backlight_device *bd)
{
	int user_intensity = bd->props.brightness;
	int plat_intensity = 0;
	u8 wled2b = 0;

     //  #if KEYPAD_BL_DEBUG
	KLDBG("[KEYLED] keyled_set_intensity = %d current_intensity = %d\n", user_intensity, current_intensity);
    //  #endif

	if (bd->props.power != FB_BLANK_UNBLANK)
		user_intensity = 0;
	if (bd->props.fb_blank != FB_BLANK_UNBLANK)
		user_intensity = 0;
		
	if (keyled_suspended)
		user_intensity = 0;

	if(user_intensity >= 100)
		plat_intensity = 0xFF;
	else
		plat_intensity = (user_intensity*256/100); // convert precentage to WLED_DUTY


	if (user_intensity && current_intensity == 0) {
	    keyled_power_ctrl(KEYLED_ON);
	} else if (user_intensity == 0 && current_intensity != 0) {
		keyled_power_ctrl(KEYLED_OFF);
	}

	current_intensity = user_intensity;

	return 0;
}


static int bcm_keyled_set_brightness(struct backlight_device *bd)
{

	KLDBG("[KEYLED] %s, brightness=%d \n", __func__, bd->props.brightness);	

    keyled_set_intensity(bd);

    return 0;
}


static int bcm_keyled_get_brightness(struct backlight_device *bl)
{
	KLDBG("[KEYLED] %s\n", __func__);	
    
	return current_intensity;
}

static struct backlight_ops bcm_keypad_led_ops = {
	.update_status	= bcm_keyled_set_brightness,
	.get_brightness	= bcm_keyled_get_brightness,
};


static int bcm_keypad_led_suspend(struct platform_device *pdev,
					pm_message_t state)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);

	KLDBG("[KEYLED] %s, %d\n", __func__, __LINE__ );

//	keyled_power_ctrl(KEYLED_OFF);
	keyled_suspended = 1;
	keyled_set_intensity(bl);
    return 0;
}

static int bcm_keypad_led_resume(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);

	KLDBG("[KEYLED] %s, %d\n", __func__, __LINE__ );

//	keyled_power_ctrl(KEYLED_ON);
	keyled_suspended = 0;
	keyled_set_intensity(bl);
    return 0;
}


static int bcm_keypad_led_probe(struct platform_device *pdev)
{
	struct backlight_device *bl;
	/*struct keypad_led_data *keyled;*/
    	struct backlight_properties props;
    int ret=0;

	KLDBG("[KEYLED] %s, %d\n", __func__, __LINE__ );

/*
	keyled = kzalloc(sizeof(*keyled), GFP_KERNEL);
	if (!keyled) {
		dev_err(&pdev->dev, "no memory for state\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
*/
	memset(&props, 0, sizeof(struct backlight_properties));

      props.brightness = KEYPAD_LED_MAX;
      props.max_brightness = KEYPAD_LED_MAX;


	bl = backlight_device_register(pdev->name, &pdev->dev,
			NULL, &bcm_keypad_led_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight\n");
		return PTR_ERR(bl);
	}

	platform_set_drvdata(pdev, bl);

#if 0
    keyled_regulator = regulator_get(NULL, "hv10");

	if(IS_ERR(keyled_regulator)){
		KLDBG("[KEYLED] can not get VKEYLED_3.3V\n");
	}	

    ret = regulator_is_enabled(keyled_regulator);
    KLDBG("[KEYLED] regulator_is_enabled : %d\n", ret);

    ret = regulator_set_voltage(keyled_regulator,3300000,3300000);	
    KLDBG("[KEYLED] regulator_set_voltage : %d\n", ret);

    ret = regulator_enable(keyled_regulator);
    KLDBG("[KEYLED] regulator_enable : %d\n", ret);
#endif

	bl->props.power = FB_BLANK_UNBLANK;
	bl->props.brightness = KEYPAD_LED_MAX;
	bl->props.max_brightness = KEYPAD_LED_MAX;

    //keyled_power_ctrl(KEYLED_ON);//test

	KLDBG("[KEYLED] Probe done!");

       
	return ret;


}

static int bcm_keypad_led_remove(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct keypad_led_data *keyled = dev_get_drvdata(&bl->dev);

	backlight_device_unregister(bl);


#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&keyled->early_suspend_desc);
#endif

	kfree(keyled);
	return 0;
}


static struct platform_driver keypad_led_driver = {
	.driver		= {
		.name	= "touchkey-led",
		.owner	= THIS_MODULE,
	},
	.probe		= bcm_keypad_led_probe,
	.remove		= bcm_keypad_led_remove,

#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend        = bcm_keypad_led_suspend,
	.resume         = bcm_keypad_led_resume,
#endif

};

static int __init bcm_keypad_led_init(void)
{
    KLDBG("[KEYLED] bcm_keypad_led_init\n");

    return platform_driver_register(&keypad_led_driver);
}
module_init(bcm_keypad_led_init);

static void __exit bcm_keypad_led_exit(void)
{
	platform_driver_unregister(&keypad_led_driver);
}
module_exit(bcm_keypad_led_exit);

MODULE_DESCRIPTION("Keyapd LED Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:keypad-led");


