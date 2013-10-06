/*
 * linux/drivers/video/backlight/ktd_bl.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
* 	@file	drivers/video/backlight/ktd_bl.c
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
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
//#include <linux/broadcom/lcd.h>
#include <linux/spinlock.h>
#include <linux/broadcom/PowerManager.h>
#include <linux/rtc.h>
#include <linux/lcd.h>

int current_intensity;
static int backlight_pin = 135;
static int backlight_pwm = 144;

static DEFINE_SPINLOCK(bl_ctrl_lock);
static int lcd_brightness = 0;
int real_level = 17;
EXPORT_SYMBOL(real_level);

#ifdef CONFIG_HAS_EARLYSUSPEND
/* early suspend support */
//extern int gLcdfbEarlySuspendStopDraw;
#endif
static int backlight_mode=1;

#define MAX_BRIGHTNESS_IN_BLU	33

#define DIMMING_VALUE		32

#define MAX_BRIGHTNESS_VALUE	255
#define MIN_BRIGHTNESS_VALUE	20
#define BACKLIGHT_DEBUG 1
#define BACKLIGHT_SUSPEND 0
#define BACKLIGHT_RESUME 1

#if BACKLIGHT_DEBUG
#define BLDBG(fmt, args...) printk(fmt, ## args)
#else
#define BLDBG(fmt, args...)
#endif

struct platform_ktd259b_backlight_data {
      	unsigned int max_brightness;
     	unsigned int dft_brightness;
     	unsigned int ctrl_pin;
      unsigned int auto_brightness;
};

struct ktd259b_bl_data {
	struct platform_device *pdev;
	unsigned int ctrl_pin;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend_desc;
#endif
};

struct brt_value{
	int level;				/* Platform setting values */
	int tune_level;	      /* Chip Setting values */
};

struct lcd_info {
	struct device			*dev;
	struct spi_device		*spi;
	unsigned int			power;
	unsigned int			gamma_mode;
	unsigned int			current_gamma_mode;
	unsigned int			current_bl;
      unsigned int			bl;
	unsigned int			auto_brightness;
	unsigned int			ldi_enable;
      unsigned int			acl_enable;
      unsigned int			cur_acl;
	struct mutex			lock;
	struct mutex			bl_lock;
	struct lcd_device		*ld;
	struct backlight_device		*bd;
	struct lcd_platform_data	*lcd_pd;
	struct early_suspend		early_suspend;

#ifdef SMART_DIMMING
	unsigned char			id[3];
	struct str_smart_dim		smart;
#endif
};


#if defined (CONFIG_MACH_CAPRI_SS_BAFFIN)
struct brt_value brt_table_ktd[] = {
  { MIN_BRIGHTNESS_VALUE,  32 },  /* Min pulse 32 */
   { 27,  32 },
   { 39,  31 },
   { 51,  29 },  
   { 63,  27 }, 
   { 75,  26 }, 
   { 87,  25 }, 
   { 99,  22 }, 
   { 111,  20 }, 
   { 123,  18 }, 
   { 135,  17 }, 
   { 147,  16 }, 
   { 159,  16 }, 
   { 171,  15 }, 
   { 183,  15 },   
   { 195,  14 },
   { 207,  13 }, 
   { 220,  13 }, 
   { 230,  12 },
   { 235,  12 },
   { 240,  11 },
   { MAX_BRIGHTNESS_VALUE,  10 },

};

struct brt_value brt_table_ktd_cabc[] = {
  { MIN_BRIGHTNESS_VALUE,  32 },  /* Min pulse 32 */
   { 27,  32 },
   { 39,  31 },
   { 51,  29 },  
   { 63,  28 }, 
   { 75,  27 }, 
   { 87,  26 }, 
   { 99,  24 }, 
   { 111,  22 }, 
   { 123,  20 }, 
   { 135,  19 }, 
   { 147,  18 }, 
   { 159,  17 }, 
   { 171,  16 }, 
   { 183,  15 },   
   { 195,  15 },
   { 207,  14 }, 
   { 220,  14 }, 
   { 230,  13 },
   { 235,  13 },
   { 240,  12 },
   { MAX_BRIGHTNESS_VALUE,  12 },

};
#elif defined(CONFIG_MACH_CAPRI_SS_CRATER)
struct brt_value brt_table_ktd[] = {
  { MIN_BRIGHTNESS_VALUE,  32 },  /* Min pulse 32 */
   { 26,  31 },
   { 33,  30 },
   { 39,  29 },  
   { 46,  28 }, 
   { 52,  27 }, 
   { 59,  26 }, 
   { 65,  25 }, 
   { 72,  24 }, 
   { 78,  23 }, 
   { 84,  22 },
   { 91,  21 }, 
   { 97,  20 },
   { 104,  19 }, 
   { 110,  18 }, 
   { 117,  17 },   
   { 123,  16 }, //default 
   { 140,  15 }, 
   { 156,  14 }, 
   { 173,  13 }, 
   { 189,  12 },
   { 206,  11 },
   { 222,  10 },
   { 239,  9 },
   { MAX_BRIGHTNESS_VALUE,  8 },

};

struct brt_value brt_table_ktd_cabc[] = {
  { MIN_BRIGHTNESS_VALUE,  32 },  /* Min pulse 32 */
   { 26,  31 },
   { 33,  31 },
   { 39,  30 },  
   { 46,  30 }, 
   { 52,  29 }, 
   { 59,  28 }, 
   { 65,  27 }, 
   { 72,  26 }, 
   { 78,  25 }, 
   { 84,  24 },
   { 91,  23 }, 
   { 97,  22 },
   { 104,  21 }, 
   { 110,  20 }, 
   { 117,  19 },   
   { 123,  18 }, //default 
   { 140,  17 }, 
   { 156,  16 }, 
   { 173,  15 }, 
   { 189,  14 },
   { 206,  13 },
   { 222,  13 },
   { 239,  12 },
   { MAX_BRIGHTNESS_VALUE,  11 },

};

#elif defined(CONFIG_MACH_CAPRI_SS_ARUBA)

struct brt_value brt_table_ktd[] = {
   { MIN_BRIGHTNESS_VALUE,  32 },  /* Min pulse 32 */
   { 27,  31 },
   { 34,  30 },
   { 41,  29 },  
   { 46,  28 }, 
   { 50,  27 }, 
   { 57,  26 }, 
   { 65,  25 }, 
   { 73,  24 }, 
   { 81,  23 }, 
   { 89,  22 }, 
   { 97,  21 }, 
   { 104,  20 }, 
   { 111,  19 }, 
   { 119,  18 },   
   { 127,  17 },
   { 135,  16 }, 
   { 143,  15 }, 
   { 151,  14 },
   { 159,  13 },
   { 167,  12 },
   { 175,  11 },
   { 183,  10 },
   { 191,  9 },
   { 199,  8 },
   { 207,  7 },
   { 215,  6 },
   { 223,  5 },
   { 231,  4 },
   { 239,  3 },
   { 247,  2 },
   { MAX_BRIGHTNESS_VALUE,  1 },  /* Max pulse 1 */
};
#else

 struct brt_value brt_table_ktd[] = {
   { MIN_BRIGHTNESS_VALUE,  32 }, /* Min pulse 32 */
   { 27,  31 },
   { 34,  30 },
   { 41,  29 },  
   { 46,  28 }, 
   { 50,  27 }, 
   { 57,  26 }, 
   { 65,  25 }, 
   { 73,  24 }, 
   { 81,  23 }, 
   { 89,  22 }, 
   { 97,  21 }, 
   { 104,  20 }, 
   { 111,  19 }, 
   { 119,  18 },   
   { 127,  17 },
   { 135,  16 }, 
   { 143,  15 }, 
   { 151,  14 },
   { 159,  13 },
   { 167,  12 },
   { 175,  11 },
   { 183,  10 },
   { 191,  9 },
   { 199,  8 },
   { 207,  7 },
   { 215,  6 },
   { 223,  5 },
   { 231,  4 },
   { 239,  3 },
   { 247,  2 },
   { MAX_BRIGHTNESS_VALUE,  1 }, /* Max pulse 1 */
};
#endif

//#define PWM_BRIGHTNESS

#define MAX_BRT_STAGE_KTD (int)(sizeof(brt_table_ktd)/sizeof(struct brt_value))

#define MAX_BRT_STAGE_KTD_CABC (int)(sizeof(brt_table_ktd_cabc)/sizeof(struct brt_value))

extern int lcd_pm_update(PM_CompPowerLevel compPowerLevel, PM_PowerLevel sysPowerLevel);
extern int cabc_status;

struct backlight_device g_bl;

#ifdef PWM_BRIGHTNESS    
extern int vc_display_bus_write(int unsigned display,
			                       uint8_t destination,
			                       const uint8_t *data,
			                       size_t count);
extern int vc_display_bus_read(int unsigned display,
			                      uint8_t source,
			                      uint8_t *data,
			                      size_t count);
#endif
static void lcd_backlight_control(int num)
{
    volatile int limit;
    unsigned long flags;

    limit = num;

    spin_lock_irqsave(&bl_ctrl_lock, flags);

    for(;limit>0;limit--)
    {
       udelay(2);
       gpio_set_value(backlight_pin,0);
       udelay(2); 
       gpio_set_value(backlight_pin,1);
    }

   spin_unlock_irqrestore(&bl_ctrl_lock, flags);
}

/* input: intensity in percentage 0% - 100% */
static int ktd259b_backlight_update_status(struct backlight_device *bd)
{
    int user_intensity = bd->props.brightness;
    int tune_level = 0;
    int pulse;
    int i;

    BLDBG("[BACKLIGHT] ktd259b_backlight_update_status ==> user_intensity  : %d\n", user_intensity);

    g_bl.props.brightness = user_intensity;
    current_intensity = user_intensity;

    if(backlight_mode==BACKLIGHT_RESUME){
       #ifdef PWM_BRIGHTNESS    
        return vc_display_bus_write(0,0x51,&user_intensity,1);
       #endif
        if(user_intensity > 0) {
            if(user_intensity < MIN_BRIGHTNESS_VALUE) {
                tune_level = DIMMING_VALUE; /* DIMMING */
            }else if (user_intensity == MAX_BRIGHTNESS_VALUE) {

                 if(cabc_status){
                   tune_level = brt_table_ktd_cabc[MAX_BRT_STAGE_KTD_CABC-1].tune_level;
                }else{
                tune_level = brt_table_ktd[MAX_BRT_STAGE_KTD-1].tune_level;
                }
            }else{

                if(cabc_status){

                   BLDBG("[BACKLIGHT] cabc ON!\n");
                   for(i = 0; i < MAX_BRT_STAGE_KTD_CABC; i++) {
                       if(user_intensity <= brt_table_ktd_cabc[i].level ) {
                          tune_level = brt_table_ktd_cabc[i].tune_level;
                           break;
                        }
                   }

                }else{
                    BLDBG("[BACKLIGHT] cabc OFF!\n");
                for(i = 0; i < MAX_BRT_STAGE_KTD; i++) {
                    if(user_intensity <= brt_table_ktd[i].level ) {
                        tune_level = brt_table_ktd[i].tune_level;
                        break;
                    }
                }

               }
            }
        }

        if (real_level==tune_level){
            return 0;
        }else{          
            if(tune_level<=0){
                gpio_set_value(backlight_pin,0);
                mdelay(3); 

	      }else{
                if( real_level<=tune_level){
                    pulse = tune_level - real_level;
                }else{
                    pulse = 32 - (real_level - tune_level);
                }

                if (pulse==0){
                    return 0;
                }

                BLDBG("[BACKLIGHT] ktd259b_backlight_update_status ==> tune_level : %d & pulse = %d\n", tune_level, pulse);

                lcd_backlight_control(pulse); 
            }

            real_level = tune_level;
            return 0;
        }

    }
      
    return 0;
}


void backlight_update_CABC()
{
   BLDBG("[BACKLIGHT] backlight_update_CABC\n");
   ktd259b_backlight_update_status(&g_bl);
}
EXPORT_SYMBOL(backlight_update_CABC);

extern void backlight_cabc_on(void);
extern void backlight_cabc_off(void);

static int ktd259b_backlight_get_brightness(struct backlight_device *bl)
{
    BLDBG("[BACKLIGHT] ktd259b_backlight_get_brightness\n");
    
    return current_intensity;
}

static struct backlight_ops ktd259b_backlight_ops = {
    .update_status	= ktd259b_backlight_update_status,
    .get_brightness	= ktd259b_backlight_get_brightness,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ktd259b_backlight_earlysuspend(struct early_suspend *desc)
{
    struct timespec ts;
    struct rtc_time tm;
 
    backlight_mode=BACKLIGHT_SUSPEND; 
    
    getnstimeofday(&ts);
    rtc_time_to_tm(ts.tv_sec, &tm);
    gpio_set_value(backlight_pwm,0);
    gpio_set_value(backlight_pin,0);

    real_level = 0;

    printk("[%02d:%02d:%02d.%03lu][BACKLIGHT] earlysuspend\n", tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
}

static void ktd259b_backlight_earlyresume(struct early_suspend *desc)
{
    struct ktd259b_bl_data *ktd259b = container_of(desc, struct ktd259b_bl_data,
                                                                          early_suspend_desc);
    struct backlight_device *bl = platform_get_drvdata(ktd259b->pdev);
    struct timespec ts;
    struct rtc_time tm;
#ifdef PWM_BRIGHTNESS    
    int temp_brightness = 0;
#endif   
    
    gpio_set_value(backlight_pwm,1);
    
#ifdef PWM_BRIGHTNESS 
    vc_display_bus_write(0,0x51,&temp_brightness,1);    
    mdelay(100);   
    lcd_backlight_control(5);
#endif
    
    getnstimeofday(&ts);
    rtc_time_to_tm(ts.tv_sec, &tm);
    backlight_mode=BACKLIGHT_RESUME;

     if(cabc_status)
     {
        backlight_cabc_on();
     }
     else
     {
        backlight_cabc_off();
     }
    
    printk("[%02d:%02d:%02d.%03lu][BACKLIGHT] earlyresume\n", tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
    backlight_update_status(bl);
}
#else
#ifdef CONFIG_PM
static int ktd259b_backlight_suspend(struct platform_device *pdev,
					pm_message_t state)
{
    struct backlight_device *bl = platform_get_drvdata(pdev);
    struct ktd259b_bl_data *ktd259b = dev_get_drvdata(&bl->dev);
    
    BLDBG("[BACKLIGHT] ktd259b_backlight_suspend\n");
        
    return 0;
}

static int ktd259b_backlight_resume(struct platform_device *pdev)
{
    struct backlight_device *bl = platform_get_drvdata(pdev);

    BLDBG("[BACKLIGHT] ktd259b_backlight_resume\n");
        
    backlight_update_status(bl);
        
    return 0;
}
#else
#define ktd259b_backlight_suspend  NULL
#define ktd259b_backlight_resume   NULL
#endif
#endif

static ssize_t auto_brightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	char temp[3];

	sprintf(temp, "%d\n", lcd->auto_brightness);
	strcpy(buf, temp);

	return strlen(buf);
}

static ssize_t auto_brightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct lcd_info *lcd = dev_get_drvdata(dev);
	int value;
	int rc;

	printk("auto_brightness_store called\n");

	rc = strict_strtoul(buf, (unsigned int)0, (unsigned long *)&value);
	if (rc < 0)
		return rc;
	else {
		if (lcd->auto_brightness != value) {
			dev_info(dev, "%s - %d, %d\n", __func__, lcd->auto_brightness, value);
			//mutex_lock(&lcd->bl_lock);
			lcd->auto_brightness = value;
		//	mutex_unlock(&lcd->bl_lock);
			//if (lcd->ldi_enable)
				//update_brightness(lcd, 0);

			if(lcd->auto_brightness == 0)
			{
                        cabc_status = 0;
                        backlight_cabc_off();
			}
			else if(lcd->auto_brightness >= 1 && lcd->auto_brightness < 5)
			{
                        cabc_status = 1;
                        backlight_cabc_on();
			}
			else if(lcd->auto_brightness >= 5)
			{
                        cabc_status = 0;
                        backlight_cabc_off();
			}
            
			//backlight_update_CABC();
            
		}
	}
	return size; 
}

static DEVICE_ATTR(auto_brightness, 0644, auto_brightness_show, auto_brightness_store);

static int ktd259b_backlight_probe(struct platform_device *pdev)
{
	struct platform_ktd259b_backlight_data *data = pdev->dev.platform_data;
	struct backlight_device *bl;
	struct ktd259b_bl_data *ktd259b;
    	struct backlight_properties props;
	int ret;

        BLDBG("[BACKLIGHT] ktd259b_backlight_probe\n");

	if (!data) {
		dev_err(&pdev->dev, "failed to find platform data\n");
		return -EINVAL;
	}

	ktd259b = kzalloc(sizeof(*ktd259b), GFP_KERNEL);
	if (!ktd259b) {
		dev_err(&pdev->dev, "no memory for state\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	ktd259b->ctrl_pin = data->ctrl_pin;
    
	memset(&props, 0, sizeof(struct backlight_properties));
	props.max_brightness = data->max_brightness;
	props.type = BACKLIGHT_PLATFORM;

	bl = backlight_device_register(pdev->name, &pdev->dev,
			ktd259b, &ktd259b_backlight_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight\n");
		ret = PTR_ERR(bl);
		goto err_bl;
	}

       lcd_device_register("panel", &pdev->dev, NULL, NULL);


      ret = device_create_file(&bl->dev, &dev_attr_auto_brightness);

	if (ret < 0)
		dev_err(&pdev->dev, "failed to add sysfs entries\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	ktd259b->pdev = pdev;
	ktd259b->early_suspend_desc.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	ktd259b->early_suspend_desc.suspend = ktd259b_backlight_earlysuspend;
	ktd259b->early_suspend_desc.resume = ktd259b_backlight_earlyresume;
	register_early_suspend(&ktd259b->early_suspend_desc);
#endif

	bl->props.max_brightness = data->max_brightness;
	bl->props.brightness = data->dft_brightness;

	platform_set_drvdata(pdev, bl);

      //	ktd259b_backlight_update_status(bl);
	return 0;

err_bl:
	kfree(ktd259b);
err_alloc:
	return ret;
}

static int ktd259b_backlight_remove(struct platform_device *pdev)
{
    struct backlight_device *bl = platform_get_drvdata(pdev);
    struct ktd259b_bl_data *ktd259b = dev_get_drvdata(&bl->dev);

    backlight_device_unregister(bl);


#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ktd259b->early_suspend_desc);
#endif

    kfree(ktd259b);
    return 0;
}


static struct platform_driver ktd259b_backlight_driver = {
	.driver		= {
		.name	= "panel",
		.owner	= THIS_MODULE,
	},
	.probe		= ktd259b_backlight_probe,
	.remove		= ktd259b_backlight_remove,

#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend        = ktd259b_backlight_suspend,
	.resume         = ktd259b_backlight_resume,
#endif

};

static int __init ktd259b_backlight_init(void)
{
      printk("ktd259b_backlight_init\n");
	return platform_driver_register(&ktd259b_backlight_driver);
}
module_init(ktd259b_backlight_init);

static void __exit ktd259b_backlight_exit(void)
{
	platform_driver_unregister(&ktd259b_backlight_driver);
}
module_exit(ktd259b_backlight_exit);

MODULE_DESCRIPTION("ktd259b based Backlight Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ktd259b-backlight");

