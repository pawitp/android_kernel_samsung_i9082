/*****************************************************************************
*  Copyright 2001 - 2008 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/licenses/old-license/gpl-2.0.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/fs.h>

#include <linux/mfd/bcmpmu.h>

#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
#include <linux/reboot.h>
#include <linux/workqueue.h>
#endif
extern void rtc_sysfs_add_device(struct rtc_device *rtc);

#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN_CMCC)||defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)
#define SEC_YEAR_BASE 			13  /* 2013 */
#else
#define SEC_YEAR_BASE 			12  /* 2012 */
#endif

#define BCMPMU_PRINT_ERROR (1U << 0)
#define BCMPMU_PRINT_INIT (1U << 1)
#define BCMPMU_PRINT_FLOW (1U << 2)
#define BCMPMU_PRINT_DATA (1U << 3)
static int dbg_mask = BCMPMU_PRINT_ERROR | BCMPMU_PRINT_INIT;

#define pr_rtc(debug_level, args...) \
	do { \
		if (dbg_mask & BCMPMU_PRINT_##debug_level) { \
			pr_info(args); \
		} \
	} while (0)
static ssize_t
bcmpmu_rtc_show_dbgmsk(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%X\n", dbg_mask);
}
static ssize_t
bcmpmu_rtc_set_dbsmsk(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	unsigned long val = simple_strtoul(buf, NULL, 0);
	if (val > 0xFF || val == 0)
		return -EINVAL;
	dbg_mask = val;
	return n;
}
static DEVICE_ATTR(dbgmask, S_IRUGO | S_IWUSR,
		bcmpmu_rtc_show_dbgmsk, bcmpmu_rtc_set_dbsmsk);

struct bcmpmu_rtc {
	struct rtc_device *rtc;
	struct bcmpmu *bcmpmu;
	wait_queue_head_t wait;
	struct mutex lock;
	int alarm_irq_enabled;
	int update_irq_enabled;
};

#if defined(CONFIG_MFD_BCM59056)
#define RTC_CAL_HW
#endif
#if defined(CONFIG_BCM_RTC_CAL) && defined(RTC_CAL_HW)
extern void bcm_rtc_cal_init(struct bcmpmu_rtc *rdata);
extern void bcm_rtc_cal_shutdown(void);
#endif /* CONFIG_BCM_RTC_CAL && RTC_CAL_HW */

static int bcmpmu_alarm_irq_enable(struct device *dev, unsigned int enabled);
#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
static void bcmpmu_check_alarm_lpm(struct work_struct *work);
static int bcmpmu_read_time(struct device *dev, struct rtc_time *tm);
static int bcmpmu_read_alarm(struct device *dev, struct rtc_wkalrm *alarm);
extern void request_suspend_state(suspend_state_t new_state);
u32 lpcharging_boot_mode;
#define BOOT_MODE_LPM			1
/* boot up 1 minute ahead before alarm time */
#define PRE_ONE_MINUTE          1

struct rtc_wkalrm *current_alarm_time = NULL;
struct rtc_time *current_rtc_time = NULL;
struct platform_device *info_autopower_rtc = NULL;
struct delayed_work pollling_work_alarmboot;

extern void emergency_restart(void);
extern void arm_machine_restart(char mode, const char *cmd);
extern void kernel_restart(char *cmd);
EXPORT_SYMBOL(lpcharging_boot_mode);

static __init int setup_boot_mode(char *opt)
{
	lpcharging_boot_mode = (u32) memparse(opt, &opt);
	return 0;
}
__setup("lpcharge=", setup_boot_mode);


static void bcmpmu_check_alarm_lpm(struct work_struct *work)
{
	bcmpmu_read_time(&info_autopower_rtc->dev,current_rtc_time);
	printk( "%s:  time=%d.%d.%d.%d.%d.%d\n",
		__func__, 
		current_rtc_time->tm_year,current_rtc_time->tm_mon,current_rtc_time->tm_mday,
		current_rtc_time->tm_hour,current_rtc_time->tm_min,current_rtc_time->tm_sec);

	if( (current_rtc_time->tm_year==current_alarm_time->time.tm_year)
		&& (current_rtc_time->tm_mon==current_alarm_time->time.tm_mon)
		&& (current_rtc_time->tm_mday==current_alarm_time->time.tm_mday)
		&& (current_rtc_time->tm_hour==current_alarm_time->time.tm_hour)
		&& (current_rtc_time->tm_min==(current_alarm_time->time.tm_min-PRE_ONE_MINUTE )) )
	{
		kfree(current_rtc_time);
		kfree(current_alarm_time);
		printk("%s: auto power on alarm occurs at lpm charging\n", __func__);
		request_suspend_state(PM_SUSPEND_ON);
		kernel_restart(NULL);
	}
	schedule_delayed_work(&pollling_work_alarmboot, 3000);
}

#endif

#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
static int 	bcm_rtc_sleep = 0; 
static int poweron_alarm = 0; 
static struct rtc_wkalrm poweron_tm; 
#endif

static void bcmpmu_rtc_isr(enum bcmpmu_irq irq, void *data)
{
	struct bcmpmu_rtc *rdata = data;

#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
	if ( bcm_rtc_sleep == 0 ) {
		poweron_alarm = 0; 
	}
#endif

	switch (irq) {
	case PMU_IRQ_RTC_ALARM:
		rtc_update_irq(rdata->rtc, 1, RTC_IRQF | RTC_AF);
		pr_rtc(FLOW, "%s: RTC interrupt Alarm\n", __func__);
		break;
	case PMU_IRQ_RTC_SEC:
		rtc_update_irq(rdata->rtc, 1, RTC_IRQF | RTC_UF);
		pr_rtc(FLOW, "%s: RTC interrupt Sec\n", __func__);
		break;
	default:
		break;
	}
}

static int bcmpmu_read_time(struct device *dev, struct rtc_time *tm)
{
	struct bcmpmu_rtc *rdata = dev_get_drvdata(dev);
	int ret = 0;
	unsigned int val;

	mutex_lock(&rdata->lock);
	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCYR,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	tm->tm_year = val + 100;

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCMT,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	if (val >= 1)
		tm->tm_mon = val - 1;

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCDT,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	tm->tm_mday = val;

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCHR,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	tm->tm_hour = val;

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCMN,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	tm->tm_min = val;

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCSC,
		&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	tm->tm_sec = val;

	ret = rtc_valid_tm(tm);

	pr_rtc(DATA, "%s: err=%d time=%d.%d.%d.%d.%d.%d\n",
		__func__, ret,
		tm->tm_year, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
err:
	mutex_unlock(&rdata->lock);
	return ret;
}

static int bcmpmu_set_time(struct device *dev, struct rtc_time *tm)
{
	struct bcmpmu_rtc *rdata = dev_get_drvdata(dev);
	int ret;

	pr_rtc(DATA, "%s: time=%d.%d.%d.%d.%d.%d\n",
		__func__,
		tm->tm_year, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	mutex_lock(&rdata->lock);

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCYR,
				tm->tm_year - 100, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCMT,
				tm->tm_mon + 1, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCDT,
				tm->tm_mday, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCHR,
				tm->tm_hour, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCMN,
				tm->tm_min, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCSC,
				tm->tm_sec, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
err:
	mutex_unlock(&rdata->lock);
	return ret;
}

static int bcmpmu_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct bcmpmu_rtc *rdata = dev_get_drvdata(dev);
	int ret;
	unsigned int val;

	mutex_lock(&rdata->lock);

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCYR_ALM,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	alarm->time.tm_year = val + 100;

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCMT_ALM,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	if (val >= 1)
		alarm->time.tm_mon = val - 1;

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCDT_ALM,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	alarm->time.tm_mday = val;

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCHR_ALM,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	alarm->time.tm_hour = val;

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCMN_ALM,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	alarm->time.tm_min = val;

	ret = rdata->bcmpmu->read_dev(rdata->bcmpmu, PMU_REG_RTCSC_ALM,
					&val, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	alarm->time.tm_sec = val;

	alarm->enabled = rdata->alarm_irq_enabled;
	ret = rtc_valid_tm(&alarm->time);

	pr_rtc(DATA, "%s: err=%d enable=%d time=%d.%d.%d.%d.%d.%d\n",
		__func__, ret,alarm->enabled,
		alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday,
		alarm->time.tm_hour, alarm->time.tm_min, alarm->time.tm_sec);

err:
	mutex_unlock(&rdata->lock);
	return ret;
}

#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
static int bcmpmu_reset_alarm_boot(struct device *dev)
{
	struct bcmpmu_rtc *rdata = dev_get_drvdata(dev);
	int ret;

	printk("%s : write alarm(%d %04d.%02d.%02d %02d:%02d:%02d)\n", __func__,poweron_tm.enabled,
			poweron_tm.time.tm_year, poweron_tm.time.tm_mon, poweron_tm.time.tm_mday, poweron_tm.time.tm_hour,
			poweron_tm.time.tm_min, poweron_tm.time.tm_sec);			

	mutex_lock(&rdata->lock);
	
	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCYR_ALM,
				poweron_tm.time.tm_year - 100, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCMT_ALM,
				poweron_tm.time.tm_mon + 1, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCDT_ALM,
				poweron_tm.time.tm_mday, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCHR_ALM,
				poweron_tm.time.tm_hour, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCMN_ALM,
				poweron_tm.time.tm_min, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCSC_ALM,
				poweron_tm.time.tm_sec, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	
	if (poweron_tm.enabled)
	bcmpmu_alarm_irq_enable(dev, poweron_tm.enabled);
	
	printk("%s : end write(%d %04d.%02d.%02d %02d:%02d:%02d)\n", __func__,poweron_tm.enabled,
			poweron_tm.time.tm_year, poweron_tm.time.tm_mon, poweron_tm.time.tm_mday, poweron_tm.time.tm_hour,
			poweron_tm.time.tm_min, poweron_tm.time.tm_sec);			
err:
	mutex_unlock(&rdata->lock);
	return ret;
}

static int bcmpmu_set_alarm_boot(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct bcmpmu_rtc *rdata = dev_get_drvdata(dev);
	int ret;

	printk("%s : write alarm(%d %04d.%02d.%02d %02d:%02d:%02d)\n", __func__,alarm->enabled,
			alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday, alarm->time.tm_hour,
			alarm->time.tm_min, alarm->time.tm_sec);			

	mutex_lock(&rdata->lock);
	
	//*poweron_tm = *alarm;
	memcpy(&poweron_tm, alarm, sizeof(struct rtc_wkalrm));
	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCYR_ALM,
				alarm->time.tm_year - 100, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCMT_ALM,
				alarm->time.tm_mon + 1, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCDT_ALM,
				alarm->time.tm_mday, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCHR_ALM,
				alarm->time.tm_hour, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCMN_ALM,
				alarm->time.tm_min, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCSC_ALM,
				alarm->time.tm_sec, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
	
	if (alarm->enabled)
	bcmpmu_alarm_irq_enable(dev, alarm->enabled);
	
	printk("%s : end write(%d %04d.%02d.%02d %02d:%02d:%02d)\n", __func__,alarm->enabled,
			alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday, alarm->time.tm_hour,
			alarm->time.tm_min, alarm->time.tm_sec);			
err:
	mutex_unlock(&rdata->lock);
	return ret;
}
#endif

static int bcmpmu_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct bcmpmu_rtc *rdata = dev_get_drvdata(dev);
	int ret;

	pr_rtc(DATA, "%s: time=%d.%d.%d.%d.%d.%d\n",
		__func__,
		alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday,
		alarm->time.tm_hour, alarm->time.tm_min, alarm->time.tm_sec);
#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
	printk("%s: time=%d.%d.%d.%d.%d.%d\n",
		__func__,
		alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday,
		alarm->time.tm_hour, alarm->time.tm_min, alarm->time.tm_sec);
#endif		
	if (alarm->enabled == 0) {
		bcmpmu_alarm_irq_enable(dev, 0);
		return 0;
	}

	mutex_lock(&rdata->lock);

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCYR_ALM,
				alarm->time.tm_year - 100, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCMT_ALM,
				alarm->time.tm_mon + 1, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCDT_ALM,
				alarm->time.tm_mday, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCHR_ALM,
				alarm->time.tm_hour, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCMN_ALM,
				alarm->time.tm_min, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;

	ret = rdata->bcmpmu->write_dev(rdata->bcmpmu, PMU_REG_RTCSC_ALM,
				alarm->time.tm_sec, PMU_BITMASK_ALL);
	if (unlikely(ret))
		goto err;
#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
	if (alarm->enabled)
		bcmpmu_alarm_irq_enable(dev, alarm->enabled);
#else
	bcmpmu_alarm_irq_enable(dev, 1);
#endif
		
err:
	mutex_unlock(&rdata->lock);
	return ret;
}

static int bcmpmu_alarm_irq_enable(struct device *dev,
		unsigned int enabled)
{
	struct bcmpmu_rtc *rdata = dev_get_drvdata(dev);
	int ret = 0;
	if (enabled)
		ret = rdata->bcmpmu->unmask_irq(rdata->bcmpmu, PMU_IRQ_RTC_ALARM);
	else
		ret = rdata->bcmpmu->mask_irq(rdata->bcmpmu, PMU_IRQ_RTC_ALARM);
	if (unlikely(ret))
		goto err;
	rdata->alarm_irq_enabled = enabled;
err:
	return ret;
}

static void bcmpmu_rtc_time_fixup(struct device *dev)
{
	struct rtc_time current_rtc_time;
	memset(&current_rtc_time, 0 , sizeof(struct rtc_time));

	bcmpmu_read_time(dev, &current_rtc_time);
	current_rtc_time.tm_year += SEC_YEAR_BASE;
	bcmpmu_set_time(dev, &current_rtc_time);
}

static int bcmpmu_rtc_suspend(struct device *dev)
{
	pr_rtc(FLOW, "%s\n", __func__);
#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
	bcm_rtc_sleep = 1; 
#endif
	return 0;
}

static int bcmpmu_rtc_resume(struct device *dev)
{
	pr_rtc(FLOW, "%s\n", __func__);

	/* This option selects temporary fix for alarm handling in 'Android'
	 * environment. This option enables code to disable alarm in the
	 * 'resume' handler of RTC driver. In the normal mode,
	 * android handles all alarms in software without using the RTC chip.
	 * Android sets the alarm in the rtc only in the suspend path (by
	 * calling .set_alarm with struct rtc_wkalrm->enabled set to 1).
	 * In the resume path, android tries to disable alarm by calling
	 * .set_alarm with struct rtc_wkalrm->enabled' field set to 0.
	 * But unfortunately, it memsets the rtc_wkalrm struct to 0, which
	 * causes the rtc lib to flag error and control does not reach this
	 * driver. Hence this workaround.
	 */
	bcmpmu_alarm_irq_enable(dev, 0);
#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
	bcm_rtc_sleep = 0; 
	bcmpmu_reset_alarm_boot(dev); 
#endif
	return 0;
}

static struct dev_pm_ops bcmpmu_rtc_pm_ops = {
	.suspend                = bcmpmu_rtc_suspend,
	.resume                 = bcmpmu_rtc_resume,
	.thaw                   = bcmpmu_rtc_resume,
	.restore                = bcmpmu_rtc_resume,
	.poweroff               = bcmpmu_rtc_suspend,
};

static struct rtc_class_ops bcmpmu_rtc_ops = {
	.read_time		= bcmpmu_read_time,
	.set_time		= bcmpmu_set_time,
	.read_alarm		= bcmpmu_read_alarm,
	.set_alarm		= bcmpmu_set_alarm,
	.alarm_irq_enable	= bcmpmu_alarm_irq_enable,
#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
	.set_alarm_boot 	= bcmpmu_set_alarm_boot,
#endif
};

static int __devinit bcmpmu_rtc_probe(struct platform_device *pdev)
{
	int ret = 0;
	unsigned int val;

	struct bcmpmu *bcmpmu = pdev->dev.platform_data;
	struct bcmpmu_rtc *rdata;
#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
	int alarm_en = 1;
#endif

	pr_rtc(INIT, "%s: called.\n", __func__);

	rdata = kzalloc(sizeof(struct bcmpmu_rtc), GFP_KERNEL);
	if (rdata == NULL) {
		dev_err(&pdev->dev, "failed to alloc mem\n");
		return -ENOMEM;
	}
	rdata->bcmpmu = bcmpmu;

	init_waitqueue_head(&rdata->wait);
	mutex_init(&rdata->lock);
	bcmpmu->rtcinfo = (void *)rdata;
#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
	current_alarm_time = kzalloc(sizeof(struct rtc_wkalrm), GFP_KERNEL);
	if(current_alarm_time == NULL)		
	{		
		pr_err("%s : memory allocation failure \n", __func__);		
		return -ENOMEM;		
	}
		current_rtc_time = kzalloc(sizeof(struct rtc_time), GFP_KERNEL);
	if(current_rtc_time == NULL)		
	{		
		pr_err("%s : memory allocation failure \n", __func__);		
		return -ENOMEM;		
	}


		rdata->update_irq_enabled = 1;
		rdata->alarm_irq_enabled = 1;		
#else
		rdata->update_irq_enabled = 0;
		rdata->alarm_irq_enabled = 0;
#endif
	platform_set_drvdata(pdev, rdata);
	rdata->rtc = rtc_device_register(pdev->name,
			&pdev->dev, &bcmpmu_rtc_ops, THIS_MODULE);
	if (IS_ERR(rdata->rtc)) {
		ret = PTR_ERR(rdata->rtc);
		goto err;
	}

	ret = bcmpmu->register_irq(bcmpmu, PMU_IRQ_RTC_ALARM, bcmpmu_rtc_isr, rdata);
	if (unlikely(ret))
		goto err1;
	bcmpmu->register_irq(bcmpmu, PMU_IRQ_RTC_SEC, bcmpmu_rtc_isr, rdata);
	if (unlikely(ret))
		goto err1;

#if defined(CONFIG_BCM_RTC_CAL) && defined(RTC_CAL_HW)
	bcm_rtc_cal_init(rdata);
#endif /* CONFIG_BCM_RTC_CAL && RTC_CAL_HW */

	/* Workarond the invalid value, to be removed after RTCADJ interrupt
	is handled properly */
	bcmpmu->read_dev(bcmpmu, PMU_REG_RTCDT, &val, PMU_BITMASK_ALL);
	if (val == 0) {
		pr_rtc(INIT, "%s: RTC adjustment.\n", __func__);		
		bcmpmu->write_dev(bcmpmu, PMU_REG_RTCDT, 1, PMU_BITMASK_ALL);
		bcmpmu->write_dev(bcmpmu, PMU_REG_RTCYR, 0, PMU_BITMASK_ALL);
		bcmpmu_rtc_time_fixup(&pdev->dev);
	}

	device_set_wakeup_capable(&pdev->dev, 1);
	rtc_sysfs_add_device(rdata->rtc);
	ret = device_create_file(&rdata->rtc->dev, &dev_attr_dbgmask);
#if defined(CONFIG_RTC_CHN_ALARM_BOOT)
	bcmpmu_read_alarm(&pdev->dev,current_alarm_time);

	if(!(current_alarm_time->time.tm_year & 0x40))
		alarm_en = 0;

	printk( "%s: enable =%d alarm time=%d.%d.%d.%d.%d.%d\n",
		__func__, alarm_en,
		current_alarm_time->time.tm_year,current_alarm_time->time.tm_mon,current_alarm_time->time.tm_mday,
		current_alarm_time->time.tm_hour,current_alarm_time->time.tm_min,current_alarm_time->time.tm_sec);
	printk("%s : boot mode is %d \n", __func__,lpcharging_boot_mode );
	if((alarm_en )&& (BOOT_MODE_LPM == lpcharging_boot_mode))
 	{ 
		info_autopower_rtc = pdev;
		INIT_DELAYED_WORK(&pollling_work_alarmboot,
			bcmpmu_check_alarm_lpm);
		schedule_delayed_work(&pollling_work_alarmboot, 300);
 	}
	else
	{
		kfree(current_rtc_time);
		kfree(current_alarm_time);
	}
#endif

	return 0;

err1:
	rtc_device_unregister(rdata->rtc);
err:
	platform_set_drvdata(pdev, NULL);
	kfree(rdata);
	return ret;
}

static int __devexit bcmpmu_rtc_remove(struct platform_device *pdev)
{
	struct bcmpmu *bcmpmu = pdev->dev.platform_data;

	bcmpmu->unregister_irq(bcmpmu, PMU_IRQ_RTC_ALARM);
	bcmpmu->unregister_irq(bcmpmu, PMU_IRQ_RTC_SEC);
#if defined(CONFIG_BCM_RTC_CAL) && defined(RTC_CAL_HW)
	bcm_rtc_cal_shutdown();
#endif /* CONFIG_BCM_RTC_CAL && RTC_CAL_HW */
	kfree(bcmpmu->rtcinfo);
	return 0;
}

static struct platform_driver bcmpmu_rtc_driver = {
	.driver = {
		.name = "bcmpmu_rtc",
		.pm   = &bcmpmu_rtc_pm_ops,
	},
	.probe = bcmpmu_rtc_probe,
	.remove = __devexit_p(bcmpmu_rtc_remove),
};

static int __init bcmpmu_rtc_init(void)
{
	return platform_driver_register(&bcmpmu_rtc_driver);
}
module_init(bcmpmu_rtc_init);

static void __exit bcmpmu_rtc_exit(void)
{
	platform_driver_unregister(&bcmpmu_rtc_driver);
}
module_exit(bcmpmu_rtc_exit);

MODULE_DESCRIPTION("BCM PMIC RTC driver");
MODULE_LICENSE("GPL");
