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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/param.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/io.h>

#include <linux/interrupt.h>
#include <mach/rdb/brcm_rdb_tmon.h>

#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#include <linux/clk.h>
#include <linux/irq.h>

#define USE_INTERRUPT

#ifdef CONFIG_KONA_CPU_FREQ_DRV
	#include <plat/kona_cpufreq_drv.h>
#endif

#include <linux/broadcom/kona-tmon.h>

#undef TEMP_DEBUG

static unsigned long test_current_temp = 30000;

struct platform_drvdata {
	struct platform_device *pdev;
	int irq;
	void __iomem *virt_base;
	int alarm_set;
	struct work_struct alarm_work;
	struct clk *tmon_1m_clk;
	struct clk *tmon_apb_clk;
};

static struct platform_drvdata *pdrv_staticdata;

static struct timer_list polling_timer;

static inline long raw_to_celcius(unsigned long raw)
{
	return 416000 - (552 * raw);
}

static inline unsigned long celcius_to_raw(long celcius)
{
	return (416000 - celcius) / 552;
}

static ssize_t kona_tmon_get_name(struct device *dev,
				  struct device_attribute *devattr,
				  char *buf)
{
	return sprintf(buf, "soctemp\n");
}

static ssize_t kona_tmon_get_value(struct device *dev,
				   struct device_attribute *devattr,
				   char *buf)
{
	unsigned long raw;
	long celcius = 0;
	struct platform_drvdata *pdata = dev_get_drvdata(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct tmon_data *board;

	board = pdata->pdev->dev.platform_data;

	if (attr->index == 1) {
		/* Index 1: First Threshold Temperature INT */
		raw = readl(pdata->virt_base + TMON_CFG_INT_THRESH_OFFSET) \
			& TMON_CFG_INT_THRESH_INT_THRESH_MASK;
	} else if (attr->index == 2) {
		/* Index 2: Second Threshold Temperature RESET */
		raw = readl(pdata->virt_base + TMON_CFG_RST_THRESH_OFFSET) \
			& TMON_CFG_RST_THRESH_RST_THRESH_MASK;
	} else {
		/* Index 0: Current Temperature */
		raw = readl(pdata->virt_base + TMON_TEMP_VAL_OFFSET) & \
			TMON_TEMP_VAL_TEMP_VAL_MASK;
	}

	dev_dbg(dev, "raw value = 0x%lx\n", raw);

	celcius = raw_to_celcius(raw);

	return sprintf(buf, "%ld\n", celcius);
}

static ssize_t kona_tmon_set_value(struct device *dev,
				   struct device_attribute *devattr,
				   const char *buf, size_t count)
{
	unsigned long raw;
	long celcius;
	struct platform_drvdata *pdata = dev_get_drvdata(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct tmon_data *board;

	board = pdata->pdev->dev.platform_data;

	if (strict_strtol(buf, 10, &celcius))
		return -EINVAL;

	raw = celcius_to_raw(celcius);

	dev_dbg(dev, "setting raw value = 0x%lx\n", raw);

	if (attr->index == 1) {
		/* Index 1: First Threshold Temperature - INT  */
		writel(raw, pdata->virt_base + TMON_CFG_INT_THRESH_OFFSET);
		board->warning_temp_thresholds[0] = celcius;
	} else if (attr->index == 2) {
		/* Index 2: Second Threshold Temperature - RESET */
		writel(raw, pdata->virt_base + TMON_CFG_RST_THRESH_OFFSET);
		board->critical_temp = celcius;
	}

	return count;
}

static ssize_t kona_tmon_get_param(struct device *dev,
				   struct device_attribute *devattr,
				   char *buf)
{
	long readvalue = 0;
	struct platform_drvdata *pdata = dev_get_drvdata(dev);
	struct tmon_data *board;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

	board = pdata->pdev->dev.platform_data;

	if (attr->index == 1) {
		/* Index 1: warning temp threshold level 1 */
		readvalue = board->warning_temp_thresholds[1];
	} else if (attr->index == 2) {
		/* Index 2: warning temp threshold level 2  */
		readvalue = board->warning_temp_thresholds[2];
	} else if (attr->index == 3) {
		/* Index 3: warning temp threshold level 3 */
		readvalue = board->warning_temp_thresholds[3];
	} else if (attr->index == 4) {
		/* Index 4: threshold level 0 enable or disable  */
		readvalue = board->threshold_enable[0];
	} else if (attr->index == 5) {
		/* Index 5: threshold level 1 enable or disable  */
		readvalue = board->threshold_enable[1];
	} else if (attr->index == 6) {
		/* Index 6: threshold level 2 enable or disable  */
		readvalue = board->threshold_enable[2];
	} else if (attr->index == 7) {
		/* Index 7: threshold level 3 enable or disable  */
		readvalue = board->threshold_enable[3];
	} else if (attr->index == 8) {
		/* Index 8: max_cpufreq 0  */
		readvalue = board->max_cpufreq[0];
	} else if (attr->index == 9) {
		/* Index 9: max_cpufreq 1  */
		readvalue = board->max_cpufreq[1];
	} else if (attr->index == 10) {
		/* Index 10: max_cpufreq 2  */
		readvalue = board->max_cpufreq[2];
	} else if (attr->index == 11) {
		/* Index 11: max_cpufreq 3  */
		readvalue = board->max_cpufreq[3];
	} else if (attr->index == 12) {
		/* Index 12: polling interval when overtemp  */
		readvalue = board->polling_interval_ms;
	} else if (attr->index == 13) {
		/* Index 13: temp_hysteresis in celcius * 1000  */
		readvalue = board->temp_hysteresis;
	} else if (attr->index == 14) {
		/* Index 14: test_current_temp  */
		readvalue = test_current_temp;
		pr_info("critical_temp = %ld\n", board->critical_temp);
		pr_info("warning_temp_thresholds[0] = %ld\n", \
			board->warning_temp_thresholds[0]);
		pr_info("warning_temp_thresholds[1] = %ld\n", \
			board->warning_temp_thresholds[1]);
		pr_info("warning_temp_thresholds[2] = %ld\n", \
			board->warning_temp_thresholds[2]);
		pr_info("warning_temp_thresholds[3] = %ld\n", \
			board->warning_temp_thresholds[3]);
		pr_info("threshold_enable[0] = %ld\n", \
			board->threshold_enable[0]);
		pr_info("threshold_enable[1] = %ld\n", \
			board->threshold_enable[1]);
		pr_info("threshold_enable[2] = %ld\n", \
			board->threshold_enable[2]);
		pr_info("threshold_enable[3] = %ld\n", \
			board->threshold_enable[3]);
		pr_info("max_cpufreq[0] = %ld\n", board->max_cpufreq[0]);
		pr_info("max_cpufreq[1] = %ld\n", board->max_cpufreq[1]);
		pr_info("max_cpufreq[2] = %ld\n", board->max_cpufreq[2]);
		pr_info("max_cpufreq[3] = %ld\n", board->max_cpufreq[3]);
		pr_info("pdrv_staticdata->alarm_set = %d\n", \
			pdrv_staticdata->alarm_set);
	}
	return sprintf(buf, "%ld\n", readvalue);
}

static ssize_t kona_tmon_set_param(struct device *dev,
				   struct device_attribute *devattr,
				   const char *buf, size_t count)
{
	unsigned long setvalue;
	struct platform_drvdata *pdata = dev_get_drvdata(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct tmon_data *board;

	board = pdata->pdev->dev.platform_data;
	if (strict_strtol(buf, 10, &setvalue))
		return -EINVAL;

	dev_dbg(dev, "setting value = 0x%ld\n", setvalue);

	if (attr->index == 1) {
		/* Index 1: warning temp threshold level 1 */
		board->warning_temp_thresholds[1] = setvalue;
	} else if (attr->index == 2) {
		/* Index 2: warning temp threshold level 2  */
		board->warning_temp_thresholds[2] = setvalue;
	} else if (attr->index == 3) {
		/* Index 3: warning temp threshold level 3 */
		board->warning_temp_thresholds[3] = setvalue;
	} else if (attr->index == 4) {
		/* Index 4: threshold level 0 enable or disable  */
		board->threshold_enable[0] = setvalue;
	} else if (attr->index == 5) {
		/* Index 5: threshold level 1 enable or disable  */
		board->threshold_enable[1] = setvalue;
	} else if (attr->index == 6) {
		/* Index 6: threshold level 2 enable or disable  */
		board->threshold_enable[2] = setvalue;
	} else if (attr->index == 7) {
		/* Index 7: threshold level 3 enable or disable  */
		board->threshold_enable[3] = setvalue;
	} else if (attr->index == 8) {
		/* Index 8: max_cpufreq 0  */
		board->max_cpufreq[0] = setvalue;
	} else if (attr->index == 9) {
		/* Index 9: max_cpufreq 1  */
		board->max_cpufreq[1] = setvalue;
	} else if (attr->index == 10) {
		/* Index 10: max_cpufreq 2  */
		board->max_cpufreq[2] = setvalue;
	} else if (attr->index == 11) {
		/* Index 11: max_cpufreq 3  */
		board->max_cpufreq[3] = setvalue;
	} else if (attr->index == 12) {
		/* Index 12: polling interval when overtemp  */
		board->polling_interval_ms = setvalue;
	} else if (attr->index == 13) {
		/* Index 13: temp_hysteresis in celcius * 1000  */
		board->temp_hysteresis = setvalue;
	} else if (attr->index == 14) {
		/* Index 14: test_current_temp  */
		test_current_temp = setvalue;
	}

	return count;
}
static ssize_t kona_tmon_get_alarm(struct device *dev,
				   struct device_attribute *devattr,
				   char *buf)
{
	struct platform_drvdata *pdata = dev_get_drvdata(dev);

	if (pdata->alarm_set) {
		pdata->alarm_set = 0;

		/* clear earlier latched alarm and re-enable isr */
		writel(TMON_CFG_CLR_INT_CLR_INT_MASK, \
		       pdata->virt_base + TMON_CFG_CLR_INT_OFFSET);
		enable_irq(pdata->irq);

		return sprintf(buf, "1\n");
	}

	return sprintf(buf, "0\n");
}

static ssize_t kona_tmon_set_interval(struct device *dev,
				      struct device_attribute *devattr,
				      const char *buf, size_t count)
{
	unsigned long msecs;
	struct platform_drvdata *pdata = dev_get_drvdata(dev);

	if (strict_strtoul(buf, 10, &msecs))
		return -EINVAL;

	writel(32 * msecs, pdata->virt_base + TMON_CFG_INTERVAL_VAL_OFFSET);

	return count;
}

static ssize_t kona_tmon_get_interval(struct device *dev,
				      struct device_attribute *devattr,
				      char *buf)
{
	unsigned long ticks;
	struct platform_drvdata *pdata = dev_get_drvdata(dev);

	ticks = readl(pdata->virt_base + TMON_CFG_INTERVAL_VAL_OFFSET);

	return sprintf(buf, "%lu\n", (ticks/32));
}

static struct sensor_device_attribute kona_attrs[] = {
	SENSOR_ATTR(name, S_IRUGO, kona_tmon_get_name, NULL, 0),
	SENSOR_ATTR(update_interval, S_IWUSR | S_IRUGO, \
		    kona_tmon_get_interval, kona_tmon_set_interval, 0),
	SENSOR_ATTR(temp1_input, S_IRUGO, kona_tmon_get_value, NULL, 0),
	/* temp1_max0 is warning temp threshold level 0 */
	SENSOR_ATTR(temp1_max0, S_IWUSR | S_IRUGO, kona_tmon_get_value, \
		    kona_tmon_set_value, 1),
	SENSOR_ATTR(temp1_crit, S_IWUSR | S_IRUGO, kona_tmon_get_value, \
		    kona_tmon_set_value, 2),
	SENSOR_ATTR(temp1_alarm, S_IRUGO, kona_tmon_get_alarm, NULL, 0),
	/* temp1_max1, temp1_max2, temp1_max3 are warning temp level 1 to 3 */
	SENSOR_ATTR(temp1_max1, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 1),
	SENSOR_ATTR(temp1_max2, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 2),
	SENSOR_ATTR(temp1_max3, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 3),
	/* threshold level is enabled or disabled (1 or 0)*/
	SENSOR_ATTR(temp1_enable0, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 4),
	SENSOR_ATTR(temp1_enable1, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 5),
	SENSOR_ATTR(temp1_enable2, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 6),
	SENSOR_ATTR(temp1_enable3, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 7),
	/* max cpufreq for each threshold level */
	SENSOR_ATTR(temp1_maxfreq0, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 8),
	SENSOR_ATTR(temp1_maxfreq1, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 9),
	SENSOR_ATTR(temp1_maxfreq2, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 10),
	SENSOR_ATTR(temp1_maxfreq3, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 11),
	SENSOR_ATTR(polling_interval, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 12),
	SENSOR_ATTR(temp_hysteresis, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 13),
	SENSOR_ATTR(test_current_temp, S_IWUSR | S_IRUGO, kona_tmon_get_param, \
		    kona_tmon_set_param, 14),
};

void polling_timer_callback(unsigned long data)
{
	struct tmon_data *board;
	unsigned long raw_current;
	unsigned long current_temp;

	board = pdrv_staticdata->pdev->dev.platform_data;

	/* Keep polling and checking if temp over another threshold */
	/* ... or go below temp1_max0 */
	raw_current = readl(pdrv_staticdata->virt_base + TMON_TEMP_VAL_OFFSET) \
		& TMON_TEMP_VAL_TEMP_VAL_MASK;

#ifdef TEMP_DEBUG
	current_temp = test_current_temp;
#else
	current_temp = raw_to_celcius(raw_current);
#endif
	pr_info("%s current temp in celcius (%ld).\n", __func__, \
		raw_to_celcius(raw_current));
	pr_info("%s current_temp debug (%ld).\n", __func__, current_temp);

	if (board->threshold_enable[0] == THRES_ENABLE) {
		if (current_temp >= (board->warning_temp_thresholds[0] - \
			board->temp_hysteresis) \
			&& (current_temp < board->warning_temp_thresholds[1])) {
#ifdef CONFIG_KONA_CPU_FREQ_DRV
			set_cpufreq_limit(board->max_cpufreq[0], MAX_LIMIT);
#endif
			mod_timer(&polling_timer, jiffies + \
				  msecs_to_jiffies(board->polling_interval_ms));

			pr_info("%s: Set cpufreq max (%ld).\n", \
				__func__, board->max_cpufreq[0]);
			pr_info("%s: Curr temp goes above temp1_max0 (%ld).\n",
				__func__, board->warning_temp_thresholds[0]);
		}
	}
	if (board->threshold_enable[1] == THRES_ENABLE) {

		if ((current_temp >= board->warning_temp_thresholds[1]) \
		    && (current_temp < board->warning_temp_thresholds[2])) {
#ifdef CONFIG_KONA_CPU_FREQ_DRV
			set_cpufreq_limit(board->max_cpufreq[1], MAX_LIMIT);
#endif
			mod_timer(&polling_timer, jiffies + \
				  msecs_to_jiffies(board->polling_interval_ms));
			pr_info("%s: Set cpufreq max (%ld).\n", \
				__func__, board->max_cpufreq[1]);
			pr_info("%s Temp in between thresh(1) (%ld) and\n", \
				__func__, board->warning_temp_thresholds[1]);
			pr_info("%s thresh(2) (%ld).\n", \
				__func__, board->warning_temp_thresholds[2]);
		}

	}
	if (board->threshold_enable[2] == THRES_ENABLE) {

		if ((current_temp >= board->warning_temp_thresholds[2]) \
		    && (current_temp < board->warning_temp_thresholds[3])) {
#ifdef CONFIG_KONA_CPU_FREQ_DRV
			set_cpufreq_limit(board->max_cpufreq[2], MAX_LIMIT);
#endif
			mod_timer(&polling_timer, jiffies + \
				  msecs_to_jiffies(board->polling_interval_ms));

			pr_info("%s: Set cpufreq max (%ld).\n", \
				__func__, board->max_cpufreq[2]);
			pr_info("%s Temp in between thresh(2) (%ld).\n", \
				__func__, board->warning_temp_thresholds[2]);
			pr_info("%s thresh(3) (%ld).\n", \
				__func__, board->warning_temp_thresholds[3]);
		}
	}
	if (board->threshold_enable[3] == THRES_ENABLE) {
		if (current_temp >= board->warning_temp_thresholds[3]) {
#ifdef CONFIG_KONA_CPU_FREQ_DRV
			set_cpufreq_limit(board->max_cpufreq[3], MAX_LIMIT);
#endif
			mod_timer(&polling_timer, jiffies + \
				  msecs_to_jiffies(board->polling_interval_ms));

			pr_info("%s: Set cpufreq max (%ld).\n", \
				__func__, board->max_cpufreq[3]);
			pr_info("%s Temp goes above thres(3) (%ld).\n", \
				__func__, board->warning_temp_thresholds[3]);
		}
	}
	if (current_temp < (board->warning_temp_thresholds[0] -
			    board->temp_hysteresis)) {
		/* Temperature falls back below max thres */
		/* Reenable IRQ since now is down below max threshold */
		pr_info("%s pdrv_staticdata->alarm_set (%d).\n", \
			__func__, pdrv_staticdata->alarm_set);

		if (pdrv_staticdata->alarm_set) {
			pdrv_staticdata->alarm_set = 0;
			pr_info("%s cleared pdrv_staticdata->alarm_set (%d).\n",
				__func__, pdrv_staticdata->alarm_set);
			/* clear earlier latched alarm and re-enable isr */
			writel(TMON_CFG_CLR_INT_CLR_INT_MASK,
			       pdrv_staticdata->virt_base +
			       TMON_CFG_CLR_INT_OFFSET);

			enable_irq(pdrv_staticdata->irq);

			/* Put back cpufreq max limit to default highest */
#ifdef CONFIG_KONA_CPU_FREQ_DRV
			set_cpufreq_limit(DEFAULT_LIMIT, MAX_LIMIT);
#endif
		}
		pr_info("%s curr temp drops below thres0 (%ld).\n",
			__func__, (board->warning_temp_thresholds[0] -
			board->temp_hysteresis));
	}


}

static void kona_tmon_notify_alarm(struct work_struct *ws)
{
	struct platform_drvdata *pdata = container_of(ws,
		struct platform_drvdata, alarm_work);

	struct tmon_data *board;

	board = pdrv_staticdata->pdev->dev.platform_data;

	pdata->alarm_set = 1;

	pr_info("kona_tmon_notify_alarm = triggered\n");
	pr_info("kona_tmon_notify_alarm = board->max_cpufreq[0] = %ld\n",
		board->max_cpufreq[0]);

	/* Warning_temp_threshold[0] is hit.  Limit cpufreq accordingly */
#ifdef CONFIG_KONA_CPU_FREQ_DRV
	if (board->threshold_enable[0] == THRES_ENABLE)
		set_cpufreq_limit(board->max_cpufreq[0], MAX_LIMIT);
#endif

	/* Fire system timer to poll every 500ms */
	mod_timer(&polling_timer, jiffies +
		  msecs_to_jiffies(board->polling_interval_ms));

	sysfs_notify(&pdata->pdev->dev.kobj, NULL, "temp1_alarm");
	kobject_uevent(&pdata->pdev->dev.kobj, KOBJ_CHANGE);
}

static irqreturn_t kona_tmon_isr(int irq, void *drvdata)
{
	unsigned long raw_current;
	unsigned long raw_threshold;
	struct platform_drvdata *pdata =  drvdata;

	/* If this isr is hit that means the threshold was exceeded.  */
	/* No hw bit to check so irq can't be shared */

	raw_current = readl(pdata->virt_base + TMON_TEMP_VAL_OFFSET) &
		TMON_TEMP_VAL_TEMP_VAL_MASK;
	raw_threshold = readl(pdata->virt_base + TMON_CFG_INT_THRESH_OFFSET) &
		TMON_CFG_INT_THRESH_INT_THRESH_MASK;

	printk(KERN_ALERT "SoC temperature threshold of %ld exceeded. ", \
	       raw_to_celcius(raw_threshold));

	printk(KERN_ALERT "Current temperature is %ld\n", \
	       raw_to_celcius(raw_current));

	/* Disable this irq so it doesn't keep firing */
	disable_irq_nosync(pdata->irq);

	/* Schedule work to notify userspace */
	schedule_work(&pdata->alarm_work);

	return IRQ_HANDLED;
}

static int kona_tmon_probe(struct platform_device *pdev)
{
	int rc = 0;
	int irq;
	struct resource *iomem, *ioarea;
	struct platform_drvdata *pdata;
	int i;
	struct tmon_data *board;
	unsigned long raw;

	/* Get register memory resource */
	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iomem) {
		dev_err(&pdev->dev, "no mem resource\n");
		return -ENODEV;
	}

	/* Get interrupt number */
	irq = platform_get_irq(pdev, 0);
	if (irq == -ENXIO) {
		dev_err(&pdev->dev, "no irq resource\n");
		return -ENODEV;
	}

	/* Mark memory region as used */
	ioarea = request_mem_region(iomem->start, resource_size(iomem),
				    pdev->name);

	if (!ioarea) {
		dev_err(&pdev->dev, "thermal region already claimed\n");
		return -EBUSY;
	}

	/* Create structure to hold register base address */
	/* and pointer to thermal zone */
	pdata = kzalloc(sizeof(struct platform_drvdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(&pdev->dev, "unable to allocate memory for data\n");
		rc = -ENOMEM;
		goto err_release_mem_region;
	}

	pdata->pdev = pdev;
	pdata->alarm_set = 0;

	/* Enable clocks */
	pdata->tmon_apb_clk = clk_get(NULL, "tmon_apb");
	if (IS_ERR(pdata->tmon_apb_clk)) {
		dev_err(&pdev->dev, "couldn't get tmon_apb_clk\n");
		rc = PTR_ERR(pdata->tmon_apb_clk);
		goto err_free_dev_mem;
	}
	clk_enable(pdata->tmon_apb_clk);

	pdata->tmon_1m_clk = clk_get(NULL, "tmon_1m_clk");
	if (IS_ERR(pdata->tmon_1m_clk)) {
		dev_err(&pdev->dev, "couldn't get tmon_1m_clk\n");
		rc = PTR_ERR(pdata->tmon_1m_clk);
		goto err_disable_apb_clk;
	}
	if (clk_set_rate(pdata->tmon_1m_clk, 1000000)) {
		dev_err(&pdev->dev, "couldn't set rate of tmon_1m_clk\n");
		rc = -ENODEV;
		goto err_put_1m_clk;
	}
	clk_enable(pdata->tmon_1m_clk);

	/* Map in the registers */
	pdata->virt_base = ioremap(iomem->start, resource_size(iomem));
	if (!pdata->virt_base) {
		dev_err(&pdev->dev, "unable to map in registers\n");
		rc = -ENOMEM;
		goto err_disable_1m_clk;
	}

	/* Init temp1_max0 and temp1_crit based on platform drv data */
	board = pdata->pdev->dev.platform_data;
	raw = celcius_to_raw(board->warning_temp_thresholds[0]);
	writel(raw, pdata->virt_base + TMON_CFG_INT_THRESH_OFFSET);

	raw = celcius_to_raw(board->critical_temp);
	writel(raw, pdata->virt_base + TMON_CFG_RST_THRESH_OFFSET);

	/* Create sysfs files */
	for (i = 0; i < ARRAY_SIZE(kona_attrs); i++)
		if (device_create_file(&pdev->dev, &kona_attrs[i].dev_attr))
			goto err_remove_files;

		/* Register hwmon device */
	hwmon_device_register(&pdev->dev);

	/* Init work */
	INIT_WORK(&pdata->alarm_work, kona_tmon_notify_alarm);

	/* Store pointer to driver specific data structure */
	/* ... in platform driver structure */
	platform_set_drvdata(pdev, pdata);

	pdrv_staticdata = pdata;

	/* Create polling timer for over max temp situation */
	setup_timer(&polling_timer, polling_timer_callback, 0);

	/* Register interrupt handler */
	pdata->irq = irq;
	if (0 != request_irq(pdata->irq, kona_tmon_isr, 0, pdev->name, pdata)) {
		dev_err(&pdev->dev, "unable to register isr\n");
		rc = -1;
		goto err_unregister_device;
	}

	return 0;

err_unregister_device:

	platform_set_drvdata(pdev, NULL);

	hwmon_device_unregister(&pdev->dev);

err_remove_files:
	for (i = i - 1; i >= 0; i--)
		device_remove_file(&pdev->dev, &kona_attrs[i].dev_attr);

	iounmap(pdata->virt_base);

err_disable_1m_clk:
	clk_disable(pdata->tmon_1m_clk);
err_put_1m_clk:
	clk_put(pdata->tmon_1m_clk);

err_disable_apb_clk:
	clk_disable(pdata->tmon_apb_clk);
	clk_put(pdata->tmon_apb_clk);

err_free_dev_mem:
	kfree(pdata);

err_release_mem_region:
	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(iomem->start, resource_size(iomem));

	return rc;

}

static int kona_tmon_remove(struct platform_device *pdev)
{
	struct platform_drvdata *pdata = platform_get_drvdata(pdev);
	struct resource *iomem;
	int i;
	int ret;

	disable_irq(pdata->irq);
	free_irq(pdata->irq, pdata);

	platform_set_drvdata(pdev, NULL);

	hwmon_device_unregister(&pdev->dev);

	for (i = ARRAY_SIZE(kona_attrs) - 1; i >= 0; i--)
		device_remove_file(&pdev->dev, &kona_attrs[i].dev_attr);

	iounmap(pdata->virt_base);

	clk_disable(pdata->tmon_1m_clk);
	clk_put(pdata->tmon_1m_clk);

	clk_disable(pdata->tmon_apb_clk);
	clk_put(pdata->tmon_apb_clk);

	ret = del_timer(&polling_timer);
	if (ret)
		pr_info("%s:The timer is still in use...\n", __func__);

	kfree(pdata);

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(iomem->start, resource_size(iomem));

	return 0;
}

#ifdef CONFIG_PM
static int kona_tmon_resume(struct platform_device *pdev)
{
	struct platform_drvdata *pdata = platform_get_drvdata(pdev);

	/* Enable clocks */
	clk_enable(pdata->tmon_apb_clk);
	clk_enable(pdata->tmon_1m_clk);
	pr_info("%s enable tmon_apb_clk.\n", __func__);

	return 0;
}

static int kona_tmon_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	struct platform_drvdata *pdata = platform_get_drvdata(pdev);

	/* Disable clocks */
	clk_disable(pdata->tmon_1m_clk);
	clk_disable(pdata->tmon_apb_clk);
	pr_info("%s disable tmon_apb_clk.\n", __func__);
	return 0;
}
#else
	#define kona_tmon_suspend    NULL
	#define kona_tmon_resume	 NULL
#endif

/*********************************************************************
*	Driver initialisation and finalization
*********************************************************************/

static struct platform_driver kona_tmon_driver = {
	.driver = {
		.name = "kona-tmon",
		.owner = THIS_MODULE,
	},
		.probe = kona_tmon_probe,
		.remove = kona_tmon_remove,
		.suspend = kona_tmon_suspend,
		.resume = kona_tmon_resume,
};

static int __init kona_tmon_init(void)
{
	return platform_driver_register(&kona_tmon_driver);
}

static void __exit kona_tmon_exit(void)
{
	platform_driver_unregister(&kona_tmon_driver);
}

module_init(kona_tmon_init);
module_exit(kona_tmon_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Broadcom SoC Temperature Monitor");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

