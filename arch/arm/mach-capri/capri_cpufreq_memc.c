/****************************************************************************
*
* Copyright 2010 --2011 Broadcom Corporation.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
*****************************************************************************/

#include <linux/pm.h>
#include <linux/cpufreq.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/io.h>

#include <mach/io_map.h>

#include <mach/rdb/brcm_rdb_csr.h>

#define MAX_MEMC_PENDING_TICK    1000

/* CPU frequency to trigger downspeed of DDR */
#define DEFAULT_CPU_FREQ_TRIGGER    312000

/*
 * Highest DDR speed to cap to:
 * 0x3 - DDR PLL at 400 MHz
 * 0x2 - SYS PLL at 156 MHz
 * 0x1 - crystal at 26 MHz
 */
enum {
	DDR_PLL_26M = 1,
	DDR_PLL_156M,
	DDR_PLL_400M
};
#define DEFAULT_DDR_SPEED_CAP    0x2

struct cpufreq_memc_ctrl {
	spinlock_t trans_lock;
	unsigned int enable;
	unsigned int ddr_cap_activated;
	unsigned int freq_threshold;
	unsigned int ddr_cap;
};

static struct cpufreq_memc_ctrl memc_ctrl;
static struct kobject *memc_kobj;

/*
 * Function to set the cap for the max memory speed. Need to be called with
 * proper lock held
 */
static inline int __cap_max_mem_speed(unsigned int ddr_cap)
{
	uint32_t reg_val, count;

	if (ddr_cap < DDR_PLL_26M || ddr_cap > DDR_PLL_400M)
		return -EINVAL;

	/* wait for previous MEMC config to finish */
	count = 0;
	do {
		reg_val = readl(KONA_MEMC0_NS_VA +
				CSR_MEMC_PWR_STATE_PENDING_OFFSET);
	} while (reg_val &
		CSR_MEMC_PWR_STATE_PENDING_MEMC_MAX_PWR_STATE_PENDING_MASK
		&& ++count < MAX_MEMC_PENDING_TICK);
	if (count >= MAX_MEMC_PENDING_TICK)
		return -EFAULT;

	reg_val = readl(KONA_MEMC0_NS_VA +
			CSR_MEMC_MAX_PWR_STATE_OFFSET);
	reg_val &= ~CSR_MEMC_MAX_PWR_STATE_MEMC_MAX_PWR_STATE_MASK;
	reg_val |= (ddr_cap
		    << CSR_MEMC_MAX_PWR_STATE_MEMC_MAX_PWR_STATE_SHIFT);
	writel(reg_val,
	       KONA_MEMC0_NS_VA + CSR_MEMC_MAX_PWR_STATE_OFFSET);

	return 0;
}

static int cpufreq_notifier_trans(struct notifier_block *nb,
				  unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = data;
	int rc = 0;

	spin_lock(&memc_ctrl.trans_lock);

	if (memc_ctrl.enable == 0 && memc_ctrl.ddr_cap_activated == 0) {
		spin_unlock(&memc_ctrl.trans_lock);
		return 0;
	}

	switch (val) {
	case CPUFREQ_PRECHANGE:
		/*
		 * Only need to bring DDR back to DDR PLL speed when CPU
		 * frequency is above threshold
		 */
		if (freq->new <= memc_ctrl.freq_threshold ||
		    memc_ctrl.ddr_cap_activated == 0)
			break;

		/* program the max MEMC state to DDR PLL */
		rc = __cap_max_mem_speed(DDR_PLL_400M);
		if (rc < 0) {
			printk(KERN_ERR "memc freq cap failed rc=%d\n", rc);
			break;
		}
		memc_ctrl.ddr_cap_activated = 0;
		break;

	case CPUFREQ_POSTCHANGE:
		/*
		 * Only need to bring DDR speed down when CPU frequency is
		 * below threshold
		 */
		if (freq->new > memc_ctrl.freq_threshold ||
		    memc_ctrl.ddr_cap_activated == 1)
			break;

		/* program the max MEMC state to SYS PLL */
		rc = __cap_max_mem_speed(memc_ctrl.ddr_cap);
		if (rc < 0) {
			printk(KERN_ERR "memc freq cap failed rc=%d\n", rc);
			break;
		}
		memc_ctrl.ddr_cap_activated = 1;
		break;

	default:
		break;
	}

	spin_unlock(&memc_ctrl.trans_lock);

	return 0;
}

static struct notifier_block notifier_trans_block = {
	.notifier_call = cpufreq_notifier_trans
};

static ssize_t store_enable(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	static struct cpufreq_memc_ctrl *memc = &memc_ctrl;
	unsigned int enable;

	sscanf(buf, "%u", &enable);

	spin_lock(&memc->trans_lock);
	memc->enable = enable;
	spin_unlock(&memc->trans_lock);

	if (enable) {
		printk(KERN_INFO
		       "cpufreq-memc is now enabled @ CPU freq=%u KHz\n",
		       memc->freq_threshold);
	} else {
		printk(KERN_INFO "cpufreq-memc is now disabled\n");
	}

	return count;
}

static ssize_t show_enable(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%u\n", memc_ctrl.enable);
}

static ssize_t store_freq_threshold(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	static struct cpufreq_memc_ctrl *memc = &memc_ctrl;
	unsigned int freq;

	sscanf(buf, "%u", &freq);

	spin_lock(&memc->trans_lock);
	memc->freq_threshold = freq;
	spin_unlock(&memc->trans_lock);

	printk(KERN_INFO "cpufreq-memc is set to CPU freq=%u KHz\n", freq);

	return count;
}

static ssize_t show_freq_threshold(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", memc_ctrl.freq_threshold);
}

static ssize_t store_ddr_cap(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	static struct cpufreq_memc_ctrl *memc = &memc_ctrl;
	unsigned int ddr_cap;

	sscanf(buf, "%u", &ddr_cap);

	if (ddr_cap < DDR_PLL_26M || ddr_cap > DDR_PLL_400M) {
		printk(KERN_ERR "DDR cap index is out of range\n");
		return count;
	}

	spin_lock(&memc->trans_lock);
	memc->ddr_cap = ddr_cap;
	spin_unlock(&memc->trans_lock);

	printk(KERN_INFO "cpufreq-memc DDR cap is set to %u\n", ddr_cap);

	return count;
}

static ssize_t show_ddr_cap(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	return sprintf(buf, "%u\n", memc_ctrl.ddr_cap);
}

int capri_cpufreq_memc_ctrl(int enable, unsigned int freq_threshold,
			    unsigned int ddr_cap)
{
	static struct cpufreq_memc_ctrl *memc = &memc_ctrl;
	int rc;

	if (ddr_cap < DDR_PLL_26M || ddr_cap > DDR_PLL_400M) {
		printk(KERN_ERR "DDR cap index is out of range\n");
		return -1;
	}

	spin_lock(&memc->trans_lock);

	memc->enable = enable;
	if (enable) {
		memc->freq_threshold = freq_threshold;
		memc->ddr_cap = ddr_cap;
	} else {
		/*
		 * in the case of disabling, de-activate previous cap
		 * immediately if they are still active
		 */
		if (memc->ddr_cap_activated) {
			/* program the max MEMC state to DDR PLL */
			rc = __cap_max_mem_speed(DDR_PLL_400M);
			if (rc < 0) {
				printk(KERN_ERR "memc freq cap failed rc=%d\n",
					rc);
			}
			memc->ddr_cap_activated = 0;
		}
	}

	spin_unlock(&memc->trans_lock);

	return 0;
}

EXPORT_SYMBOL(capri_cpufreq_memc_ctrl);

static DEVICE_ATTR(enable, 0644, show_enable, store_enable);
static DEVICE_ATTR(freq_threshold, 0644, show_freq_threshold,
		   store_freq_threshold);
static DEVICE_ATTR(ddr_cap, 0644, show_ddr_cap, store_ddr_cap);

static struct attribute *cpufreq_memc_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_freq_threshold.attr,
	&dev_attr_ddr_cap.attr,
	NULL
};

static struct attribute_group cpufreq_memc_attr_group = {
	.attrs = cpufreq_memc_attrs,
};

static int __init capri_cpufreq_memc_init(void)
{
	int ret;

	memset(&memc_ctrl, 0, sizeof(memc_ctrl));
	memc_ctrl.freq_threshold = DEFAULT_CPU_FREQ_TRIGGER;
	memc_ctrl.ddr_cap = DEFAULT_DDR_SPEED_CAP;
	spin_lock_init(&memc_ctrl.trans_lock);

	ret = cpufreq_register_notifier(&notifier_trans_block,
					CPUFREQ_TRANSITION_NOTIFIER);
	if (ret) {
		printk(KERN_ERR "failed to register cpufreq notifier ret=%d\n",
		       ret);
		return ret;
	}

	if (cpufreq_global_kobject != NULL) {
		memc_kobj = kobject_create_and_add("memc",
						   cpufreq_global_kobject);
	} else {
		memc_kobj = kobject_create_and_add("memc", NULL);
	}
	if (memc_kobj == NULL) {
		ret = -ENOMEM;
		goto err_unregister_notifier;
	}

	ret = sysfs_create_group(memc_kobj, &cpufreq_memc_attr_group);
	if (ret < 0)
		goto err_rm_kobj;

	printk(KERN_INFO "capri cpufreq-memc initialized\n");

	return 0;

err_rm_kobj:
	kobject_put(memc_kobj);
err_unregister_notifier:
	cpufreq_unregister_notifier(&notifier_trans_block,
				    CPUFREQ_TRANSITION_NOTIFIER);
	return ret;
}

static void __exit capri_cpufreq_memc_exit(void)
{
	sysfs_remove_group(memc_kobj, &cpufreq_memc_attr_group);
	kobject_put(memc_kobj);
	cpufreq_unregister_notifier(&notifier_trans_block,
				    CPUFREQ_TRANSITION_NOTIFIER);
}

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Driver to control cpufreq-memc operation");
MODULE_LICENSE("GPL");

module_init(capri_cpufreq_memc_init);
module_exit(capri_cpufreq_memc_exit);
