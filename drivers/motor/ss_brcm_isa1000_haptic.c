/*
* Haptic driver for broadcom primary vibrator(class D) 
*
* Copyright (C) 2012 kc45.kim@samsung.com
*
* This program is free software. you can redistribute it and/or modify it
* under the terms of the GNU Public License version 2 as
* published by the Free Software Foundation
*
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/serio.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/bcmpmu.h>
#include <linux/wakelock.h>
#include <linux/pwm/pwm.h>
#include <linux/isa1000_vibrator.h>

//#include <alsa/asoundlib.h>

#include "../staging/android/timed_output.h"

typedef struct
{
	struct pwm_device 	*pwm;
	struct timed_output_dev timed_dev;
	struct timer_list vib_timer;
	struct work_struct off_work;
	struct regulator *vib_regulator;
	const char *vib_vcc;
	int (*gpio_en) (bool) ;
	u16 pwm_duty;
	u16 pwm_period;	
	u16 pwm_polarity;	
	int	pwm_started;
	int	initialized;
}t_vib_desc;


static t_vib_desc vib_desc;
static int controlset(const char *name, unsigned int *value, int index);
static void vibrator_control(t_vib_desc *vib_iter, unsigned char onoff);


#define MIN_TIMEOUT 150
#define MAX_TIMEOUT 500
#define DEFAULT_TIMEOUT 170

void vibtonz_en(bool en)
{
	t_vib_desc *vib_iter =&vib_desc;

	printk("%s %s \n", __func__, (en?"enabled":"disabled"));
	if( vib_iter->initialized == 0) return;
	if(en)
	{
		vib_iter->gpio_en(en);
		pwm_start(vib_iter->pwm);
	}
	else
	{
		pwm_stop(vib_iter->pwm);
		vib_iter->gpio_en(en);
	}
}

EXPORT_SYMBOL(vibtonz_en);

void vibtonz_pwm(int nForce)
{
	static int prev_duty=0;
	t_vib_desc *vib_iter =&vib_desc;
	int pwm_period=0, pwm_duty = 0;

	printk("%s : %d \n", __func__, nForce);
	if( vib_iter->initialized == 0) return;

	pwm_period = vib_iter->pwm_period;
	pwm_duty = pwm_period/2 + ((pwm_period/2 - 2) *nForce) /127;

	if(pwm_duty > vib_iter->pwm_duty)
	{
		pwm_duty = vib_iter->pwm_duty;
	}
	else if(pwm_period - pwm_duty > vib_iter->pwm_duty)
	{
		pwm_duty = pwm_period - vib_iter->pwm_duty;
	}
	
	pwm_set_period_ns(vib_iter->pwm,vib_iter->pwm_period); 	
	pwm_set_polarity(vib_iter->pwm, vib_iter->pwm_polarity);
	pwm_set_duty_ns(vib_iter->pwm, pwm_duty); 
}

EXPORT_SYMBOL(vibtonz_pwm);


static void vibrator_control(t_vib_desc *vib_iter, unsigned char onoff)
{

#if 1
	printk("%s : Vibrator %s\n", __func__, (onoff)?"on":"off");

	if( onoff == 1)
	{
		if(!regulator_is_enabled(vib_iter->vib_regulator))
		{
			regulator_enable(vib_iter->vib_regulator);
		}
	}
	else if( onoff == 0)
	{
		if(regulator_is_enabled(vib_iter->vib_regulator))
		{
			regulator_disable(vib_iter->vib_regulator);
		}
	}
#endif
	return;
}

static void vibrator_enable_set_timeout(struct timed_output_dev *sdev, int timeout)
{
	t_vib_desc *vib_iter=container_of(sdev, t_vib_desc, timed_dev);
	int valid_timeout;

	if(timeout == 0)
	{
		vibrator_control(vib_iter, 0);
		return;
	}

	vibrator_control(vib_iter, 1);

	valid_timeout=(timeout > MIN_TIMEOUT) ? timeout: DEFAULT_TIMEOUT;
	printk(KERN_INFO "%s : Vibrator timeout = %d \n", __func__, valid_timeout);

	mod_timer(&vib_iter->vib_timer, jiffies + msecs_to_jiffies(valid_timeout));
}

static void vibrator_off_work_func(struct work_struct *work)
{
	t_vib_desc *vib_iter=container_of(work, t_vib_desc, off_work);

	vibrator_control(vib_iter, 0);
}

static void on_vibrate_timer_expired(unsigned long x)
{
	t_vib_desc *vib_iter = (t_vib_desc *)x;

	schedule_work(&vib_iter->off_work);
}

static void vibrator_get_remaining_time(struct timed_output_dev *sdev)
{
	t_vib_desc *vib_iter=container_of(sdev, t_vib_desc, timed_dev);
	int retTime=jiffies_to_msecs(jiffies-vib_iter->vib_timer.expires);
	printk(KERN_INFO "Vibrator : remaining time : %dms \n", retTime);
}


static int ss_brcm_haptic_probe(struct platform_device *pdev)
{
	struct platform_isa1000_vibrator_data *pdata = pdev->dev.platform_data;
	t_vib_desc *vib_iter;
	int ret=0;

	printk("ss_brcm_haptic_probe \n"); 

	/* vib_iter=kzalloc(sizeof(t_vib_desc), GFP_KERNEL);
	   if(vib_iter == NULL)
	   {
	   pr_err("%s : memory allocation failure \n", __func__);
	   return -ENOMEM;
	   } */
	vib_iter=&vib_desc;

	vib_iter->vib_vcc = (const char *)pdata->regulator_id;
	printk(KERN_INFO "%s: Vibrator vcc=%s \n", __func__, vib_iter->vib_vcc);

	//vib_iter->vib_regulator=regulator_get(NULL, VIB_VCC);
	vib_iter->vib_regulator=regulator_get(NULL, vib_iter->vib_vcc);
	if(IS_ERR(vib_iter->vib_regulator))
	{
		printk(KERN_INFO "%s: failed to get regulator \n", __func__);
		return -ENODEV;
	}

	regulator_enable(vib_iter->vib_regulator);

	vib_iter->gpio_en = pdata->gpio_en;
	vib_iter->pwm = pwm_request(pdata->pwm_name, "vibrator");
	if (IS_ERR(vib_iter->pwm)) 
	{
		pr_err("[VIB] Failed to request pwm.\n");
		 return -EFAULT;
	}
	vib_iter->pwm_duty = pdata->pwm_duty;
	vib_iter->pwm_period = pdata->pwm_period_ns;
	vib_iter->pwm_polarity = pdata->polarity;

	pwm_set_polarity(vib_iter->pwm , vib_iter->pwm_polarity); 

	vib_iter->timed_dev.name="vibrator";
	vib_iter->timed_dev.enable=vibrator_enable_set_timeout;
	vib_iter->timed_dev.get_time=vibrator_get_remaining_time;

	ret = timed_output_dev_register(&vib_iter->timed_dev);
	if(ret < 0)
	{
		printk(KERN_ERR "Vibrator: timed_output dev registration failure\n");
		timed_output_dev_unregister(&vib_iter->timed_dev);
	}

	init_timer(&vib_iter->vib_timer);
	vib_iter->vib_timer.function = on_vibrate_timer_expired;
	vib_iter->vib_timer.data = (unsigned long)vib_iter;

	platform_set_drvdata(pdev, vib_iter);

	INIT_WORK(&vib_iter->off_work, vibrator_off_work_func);
	
	vib_iter->initialized = 1;
	printk("%s : ss vibrator probe\n", __func__);
	return 0;

}

static int __devexit ss_brcm_haptic_remove(struct platform_device *pdev)
{
	t_vib_desc *vib_iter = platform_get_drvdata(pdev);
	timed_output_dev_unregister(&vib_iter->timed_dev);
	regulator_put(vib_iter->vib_regulator);
	return 0;
}

static struct platform_driver ss_brcm_haptic_driver = {
	.probe = ss_brcm_haptic_probe,
	.remove = ss_brcm_haptic_remove,
	.driver = {
		.name = "isa1000-vibrator",
		.owner = THIS_MODULE,
	},
};

static void __init ss_brcm_haptic_init(void)
{
	printk("ss_haptic init \n");
	platform_driver_register(&ss_brcm_haptic_driver);
}

static void __exit ss_brcm_haptic_exit(void)
{
	platform_driver_unregister(&ss_brcm_haptic_driver);
}

late_initcall(ss_brcm_haptic_init);
module_exit(ss_brcm_haptic_exit);

MODULE_DESCRIPTION("Samsung Vibrator driver");
MODULE_LICENSE("GPL");

