/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
*	@file drivers/watchdog/bcm59055-wd-tapper.c
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
/**
 * @file
 * driver/watchdog/wd-tapper.c
 *
 * Watchdog Petter - A platform driver that takes care of petting the
 * PMU Watchdog after a set interval of time. This makes sure that the Board
 * does not restart in suspend.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <mach/kona_timer.h>
#include <linux/broadcom/wd-tapper.h>

static int timer_rate = 1;

/* Seconds to ticks conversion */
#define sec_to_ticks(x) ((x)*timer_rate)
#define ticks_to_sec(x) ((x)/timer_rate)

static DEFINE_SPINLOCK(tapper_lock);

#ifdef CONFIG_SEC_CHARGING_FEATURE
#define LOWVOLTAGE   3500
#define VERYLOWVOLTAGE   3400
#endif

extern int spa_get_batt_voltage_extern(void);

/* The Driver specific data */
struct wd_tapper_data {
	struct kona_timer *kt;
	unsigned int count;
	unsigned int def_count;
};

struct wd_tapper_data *wd_tapper_data;

int wd_tapper_set_timeout(unsigned int timeout_in_sec)
{
	int ret = -EINVAL;
	if (wd_tapper_data) {
		spin_lock(&tapper_lock);
		if (timeout_in_sec == TAPPER_DEFAULT_TIMEOUT)
			wd_tapper_data->count = wd_tapper_data->def_count;
		else
			wd_tapper_data->count = sec_to_ticks(timeout_in_sec);
		spin_unlock(&tapper_lock);
		pr_debug("%s(%d,%d)\n",
		__func__, wd_tapper_data->count, timeout_in_sec);
		ret = 0;
	}
	return ret;
}
EXPORT_SYMBOL(wd_tapper_set_timeout);

unsigned int wd_tapper_get_timeout(void)
{
	if (wd_tapper_data)
		return ticks_to_sec(wd_tapper_data->count);
	return 0;
}
EXPORT_SYMBOL(wd_tapper_get_timeout);

/**
 * wd_tapper_callback - The timer expiry registered callback
 *
 * Function used to pet the watchdog after the set interval
 *
 */
int wd_tapper_callback(void *dev)
{
	/* Pet the PMU Watchdog */
	pr_info("wd tapper wakeup\n");
	return 0;
}

#ifdef CONFIG_SEC_CHARGING_FEATURE
static int get_wd_tapper_count(struct platform_device *pdev)
{
   int voltage = 0;
   struct wd_tapper_platform_data *pltfm_data;
   pltfm_data = dev_get_platdata(&pdev->dev);

   voltage = spa_get_batt_voltage_extern();
   if(voltage <= VERYLOWVOLTAGE)
   {
	  pr_info("%s : wd_tapper count = %d sec, voltage = %d\n", __func__, pltfm_data->verylowbattcount, voltage);
      return  sec_to_ticks(pltfm_data->verylowbattcount);
   }
   else if(voltage <= LOWVOLTAGE)
   {
      pr_info("%s : wd_tapper count = %d sec, voltage = %d\n", __func__, pltfm_data->lowbattcount, voltage);
      return  sec_to_ticks(pltfm_data->lowbattcount);
   }
   else
   {
      pr_info("%s : wd_tapper count = %d sec, voltage = %d\n", __func__, pltfm_data->count, voltage);
      return  sec_to_ticks(pltfm_data->count);
   }
}
#endif

/**
 * wd_tapper_start - Function where the timer is started on suspend
 *
 * @return 0 on successfull set of the timer or negative error value on error
 */
static int wd_tapper_start(struct platform_device *pdev, pm_message_t state)
{
#ifdef CONFIG_SEC_CHARGING_FEATURE
	wd_tapper_data->count = get_wd_tapper_count(pdev);
#endif
	pr_debug("%s(%d)\n", __func__, wd_tapper_data->count);
	preempt_disable();
	if (kona_timer_set_match_start
	    (wd_tapper_data->kt, wd_tapper_data->count) < 0) {
		pr_err("kona_timer_set_match_start returned error \r\n");
		preempt_enable();	
		return -1;
	}
	preempt_enable();	
	return 0;
}

/**
 * wd_tapper_stop - Function where the timer is stopped on resume
 *
 * @return 0 on successfull stop of the timer or negative error value on error
 */
static int wd_tapper_stop(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);
	if (kona_timer_stop(wd_tapper_data->kt) < 0) {
		pr_err
		    ("Unable to stop the timer kona_timer_stop returned error \r\n");
		return -1;
	}
	return 0;
}

/**
 * wd_tapper_pltfm_probe - Function where the timer is obtained and configured
 *
 * @return 0 on successful fetch and configuration of the timer or negative
 * error value on error
 */
static int __devinit wd_tapper_pltfm_probe(struct platform_device *pdev)
{
	struct wd_tapper_platform_data *pltfm_data;
	struct timer_ch_cfg cfg;
	unsigned int ch_num;
	pr_debug("%s\n", __func__);
	wd_tapper_data = vmalloc(sizeof(struct wd_tapper_data));

	/* Obtain the platform data */
	pltfm_data = dev_get_platdata(&pdev->dev);

	/* Validate the data obtained */
	if (!pltfm_data) {
		dev_err(&pdev->dev, "can't get the platform data\n");
		goto out;
	}

	/* Get the channel number */
	ch_num = pltfm_data->ch_num;
	if (ch_num > 3) {
		dev_err(&pdev->dev,
			"Wrong choice of channel number to match\n");
		goto out;
	}

	/* Request the timer context */
	wd_tapper_data->kt = kona_timer_request(pltfm_data->name, ch_num);
	if (wd_tapper_data->kt == NULL) {
		dev_err(&pdev->dev, "kona_timer_request returned error \r\n");
		goto out;
	}

	timer_rate = kona_timer_module_get_rate(pltfm_data->name);
	if (timer_rate <= 0) {
		dev_err(&pdev->dev, "kona_timer_module_get_rate error \r\n");
		goto out;
	}

	/* Get the time out period */
	wd_tapper_data->count = sec_to_ticks(pltfm_data->count);
	wd_tapper_data->def_count = wd_tapper_data->count;

	if (wd_tapper_data->count == 0) {
		dev_err(&pdev->dev, "count value set is 0 - INVALID\n");
		goto out;
	}

	/* Populate the timer config */
	cfg.mode = MODE_ONESHOT;
	cfg.arg = wd_tapper_data->kt;
	cfg.cb = (intr_callback) wd_tapper_callback;
	cfg.reload = wd_tapper_data->count;

	/* Configure the timer */
	if (kona_timer_config(wd_tapper_data->kt, &cfg) < 0) {
		dev_err(&pdev->dev, "kona_timer_config returned error \r\n");
		goto out;
	}

	dev_info(&pdev->dev, "Probe Success, rate=%d count=%u\n",
		timer_rate, wd_tapper_data->count);
	return 0;
out:
	dev_err(&pdev->dev, "Probe failed\n");
	vfree(wd_tapper_data);
	return -1;
}

/**
 * wd_tapper_pltfm_remove - Function where the timer is freed
 *
 * @return 0 on successful free of the timer or negative error value on error
 */
static int __devexit wd_tapper_pltfm_remove(struct platform_device *pdev)
{
	if (kona_timer_free(wd_tapper_data->kt) < 0) {
		pr_err("Unable to free the timer \r\n");
		return -1;
	}
	return 0;
}

static struct platform_driver wd_tapper_pltfm_driver = {
	.driver = {
		   .name = "wd_tapper",
		   .owner = THIS_MODULE,
		   },
	.probe = wd_tapper_pltfm_probe,
	.remove = __devexit_p(wd_tapper_pltfm_remove),
	.suspend = wd_tapper_start,
	.resume = wd_tapper_stop,
};

static int __init wd_tapper_init(void)
{
	return platform_driver_register(&wd_tapper_pltfm_driver);
}

module_init(wd_tapper_init);

static void __exit wd_tapper_exit(void)
{
	platform_driver_unregister(&wd_tapper_pltfm_driver);
}

module_exit(wd_tapper_exit);

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("Watchdog Petter");
MODULE_LICENSE("GPL");;
