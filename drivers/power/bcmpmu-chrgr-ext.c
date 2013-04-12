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
#include <linux/init.h>
#include <linux/bug.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>

#include <linux/mfd/bcmpmu.h>

#define BCMPMU_PRINT_ERROR (1U << 0)
#define BCMPMU_PRINT_INIT (1U << 1)
#define BCMPMU_PRINT_FLOW (1U << 2)
#define BCMPMU_PRINT_DATA (1U << 3)

static int debug_mask = BCMPMU_PRINT_ERROR | BCMPMU_PRINT_INIT;
#define pr_chrgr(debug_level, args...) \
	do { \
		if (debug_mask & BCMPMU_PRINT_##debug_level) { \
			pr_info(args); \
		} \
	} while (0)

enum {
	DET_STATE_IDLE,
	DET_STATE_DEBOUNCE,
	DET_STATE_CONNECT,
};

struct bcmpmu_chrgr {
	struct bcmpmu *bcmpmu;
	struct power_supply chrgr;
	struct power_supply usb;
	struct power_supply ac;
	wait_queue_head_t wait;
	struct mutex lock;
	struct wake_lock wake_lock;
	struct workqueue_struct *workq;
	struct work_struct work;
	struct delayed_work dwork;
	int eoc;
	int chrgrcurr_max;
	enum bcmpmu_chrgr_type_t chrgrtype;
	enum bcmpmu_usb_type_t usbtype;
	int chrgr_online;
	int usb_online;
	int ac_online;
	struct bcmpmu_ext_chrgr_info *ext_chrgr_info;
	int det_state;
	int sig_state;
	int db_cnt;
	int irq_chrgr_ac5v;
	int irq_chrgr_flt;
	int irq_chrgr_chg;
};

static char *usb_names[PMU_USB_TYPE_MAX] = {
	[PMU_USB_TYPE_NONE]	= "none",
	[PMU_USB_TYPE_SDP]	= "sdp",
	[PMU_USB_TYPE_CDP]	= "cdp",
	[PMU_USB_TYPE_ACA]	= "aca",
};

static char *chrgr_names[PMU_CHRGR_TYPE_MAX] = {
	[PMU_CHRGR_TYPE_NONE]	= "none",
	[PMU_CHRGR_TYPE_SDP]	= "sdp",
	[PMU_CHRGR_TYPE_CDP]	= "cdp",
	[PMU_CHRGR_TYPE_DCP]	= "dcp",
	[PMU_CHRGR_TYPE_PS2]	= "ps2",
	[PMU_CHRGR_TYPE_TYPE1]	= "type1",
	[PMU_CHRGR_TYPE_TYPE2]	= "type2",
	[PMU_CHRGR_TYPE_ACA]	= "aca",
};


static enum power_supply_property bcmpmu_chrgr_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_MODEL_NAME,
};

static enum power_supply_property bcmpmu_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_MODEL_NAME,
};

static enum power_supply_property bcmpmu_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_MODEL_NAME,
};

static int bcmpmu_chrgr_set_property(struct power_supply *ps,
		enum power_supply_property property,
		const union power_supply_propval *propval)
{
	int ret = 0;
	struct bcmpmu_chrgr *pchrgr = container_of(ps,
		struct bcmpmu_chrgr, chrgr);
	switch (property) {
	case POWER_SUPPLY_PROP_ONLINE:
		pchrgr->chrgr_online = propval->intval;
		break;

	case POWER_SUPPLY_PROP_TYPE:
		pchrgr->chrgrtype = propval->intval;
		break;

	case POWER_SUPPLY_PROP_CURRENT_NOW:
		pchrgr->chrgrcurr_max = propval->intval;
		break;

	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int bcmpmu_chrgr_get_property(struct power_supply *ps,
	enum power_supply_property prop,
	union power_supply_propval *val)
{
	int ret = 0;
	struct bcmpmu_chrgr *pchrgr = container_of(ps,
		struct bcmpmu_chrgr, chrgr);
	switch(prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = pchrgr->chrgr_online;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = pchrgr->chrgrcurr_max;
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = chrgr_names[pchrgr->chrgrtype];
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int bcmpmu_usb_set_property(struct power_supply *ps,
		enum power_supply_property property,
		const union power_supply_propval *propval)
{
	int ret = 0;
	struct bcmpmu_chrgr *pchrgr = container_of(ps,
		struct bcmpmu_chrgr, usb);
	switch (property) {
	case POWER_SUPPLY_PROP_ONLINE:
		pchrgr->usb_online = propval->intval;
		break;

	case POWER_SUPPLY_PROP_TYPE:
		pchrgr->usbtype = propval->intval;
		break;

	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int bcmpmu_usb_get_property(struct power_supply *ps,
	enum power_supply_property prop,
	union power_supply_propval *val)
{
	int ret = 0;
	struct bcmpmu_chrgr *pchrgr = container_of(ps,
		struct bcmpmu_chrgr, usb);
	switch(prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = pchrgr->usb_online;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = usb_names[pchrgr->usbtype];
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int bcmpmu_ac_set_property(struct power_supply *ps,
		enum power_supply_property property,
		const union power_supply_propval *propval)
{
	int ret = 0;
	struct bcmpmu_chrgr *pchrgr = container_of(ps,
		struct bcmpmu_chrgr, chrgr);
	switch (property) {
	case POWER_SUPPLY_PROP_ONLINE:
		pchrgr->ac_online = propval->intval;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int bcmpmu_ac_get_property(struct power_supply *ps,
	enum power_supply_property prop,
	union power_supply_propval *val)
{
	int ret = 0;
	struct bcmpmu_chrgr *pchrgr = container_of(ps,
		struct bcmpmu_chrgr, chrgr);
	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = pchrgr->ac_online;
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = "ac";
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int bcmpmu_set_icc_qc(struct bcmpmu *bcmpmu, int curr)
{
	return 0;
}

static int bcmpmu_set_icc_fc(struct bcmpmu *bcmpmu, int curr)
{
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;
	if (pchrgr->ac_online == 1)
		gpio_set_value(pchrgr->ext_chrgr_info->cen, 0);
	else if (curr == 0)
		gpio_set_value(pchrgr->ext_chrgr_info->cen, 1);
	else if (curr > 100)
		gpio_set_value(pchrgr->ext_chrgr_info->iusb, 1);
	else
		gpio_set_value(pchrgr->ext_chrgr_info->iusb, 0);
	return 0;
}

static int bcmpmu_set_vfloat(struct bcmpmu *bcmpmu, int volt)
{
	return 0;
}

static int bcmpmu_set_eoc(struct bcmpmu *bcmpmu, int curr)
{
	return 0;
}

static int bcmpmu_chrgr_usb_en(struct bcmpmu *bcmpmu, int en)
{
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;
	if (pchrgr->ac_online == 1)
		gpio_set_value(pchrgr->ext_chrgr_info->cen, 0);
	else
		gpio_set_value(pchrgr->ext_chrgr_info->cen, ~en);
	return 0;
}

static int bcmpmu_chrgr_wac_en(struct bcmpmu *bcmpmu, int en)
{
	return 0;
}


#ifdef CONFIG_MFD_BCMPMU_DBG
static ssize_t
dbgmsk_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "debug_mask is %x\n", debug_mask);
}

static ssize_t
dbgmsk_set(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	unsigned long val = simple_strtoul(buf, NULL, 0);
	if (val > 0xFF || val == 0)
		return -EINVAL;
	debug_mask = val;
	return count;
}

static ssize_t
get_ac5v(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct bcmpmu *bcmpmu = dev->platform_data;
	int val;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;
	val = gpio_get_value(pchrgr->ext_chrgr_info->ac5v);
	return sprintf(buf, "%d\n", val);
}

static ssize_t
get_flt(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct bcmpmu *bcmpmu = dev->platform_data;
	int val;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;
	val = gpio_get_value(pchrgr->ext_chrgr_info->flt);
	return sprintf(buf, "gpio %d = %d\n",
		pchrgr->ext_chrgr_info->flt, val);
}

static ssize_t
get_chg(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct bcmpmu *bcmpmu = dev->platform_data;
	int val;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;
	val = gpio_get_value(pchrgr->ext_chrgr_info->chg);
	return sprintf(buf, "gpio %d = %d\n",
		pchrgr->ext_chrgr_info->chg, val);
}

static ssize_t
set_iusb(struct device *dev, struct device_attribute *attr,
		   const char *buf, size_t n)
{
	struct bcmpmu *bcmpmu = dev->platform_data;
	int val;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;
	sscanf(buf, "%d", &val);
	gpio_set_value(pchrgr->ext_chrgr_info->iusb, val);
	return n;
}

static ssize_t
set_cen(struct device *dev, struct device_attribute *attr,
		   const char *buf, size_t n)
{
	struct bcmpmu *bcmpmu = dev->platform_data;
	int val;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;
	sscanf(buf, "%d", &val);
	gpio_set_value(pchrgr->ext_chrgr_info->cen, val);
	return n;
}

static ssize_t
bcmpmu_dbg_usb_en(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct bcmpmu *bcmpmu = dev->platform_data;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;
	unsigned long val = simple_strtoul(buf, NULL, 0);
	if ((val != 1) && (val != 0))
		return -EINVAL;
	gpio_set_value(pchrgr->ext_chrgr_info->cen, ~val);
	return n;
}

static DEVICE_ATTR(dbgmsk, 0644, dbgmsk_show, dbgmsk_set);
static DEVICE_ATTR(ac5v, 0644, get_ac5v, NULL);
static DEVICE_ATTR(flt, 0644, get_flt, NULL);
static DEVICE_ATTR(chg, 0644, get_chg, NULL);
static DEVICE_ATTR(iusb, 0644, NULL, set_iusb);
static DEVICE_ATTR(cen, 0644, NULL, set_cen);
static DEVICE_ATTR(usb_en, 0644, NULL, bcmpmu_dbg_usb_en);

static struct attribute *bcmpmu_chrgr_attrs[] = {
	&dev_attr_dbgmsk.attr,
	&dev_attr_ac5v.attr,
	&dev_attr_flt.attr,
	&dev_attr_chg.attr,
	&dev_attr_iusb.attr,
	&dev_attr_cen.attr,
	&dev_attr_usb_en.attr,
	NULL
};

static const struct attribute_group bcmpmu_chrgr_attr_group = {
	.attrs = bcmpmu_chrgr_attrs,
};
#endif

static irqreturn_t bcmpmu_chrgr_isr(int irq, void *data)
{
	struct bcmpmu_chrgr *pchrgr = data;
	disable_irq_nosync(irq);
	queue_work(pchrgr->workq, &pchrgr->work);
	return IRQ_HANDLED;
}

void bcmpmu_chrgr_handler(struct work_struct *work)
{
	struct bcmpmu_chrgr *pchrgr;
	struct bcmpmu *bcmpmu;

	pchrgr = container_of(work, struct bcmpmu_chrgr, work);
	bcmpmu = pchrgr->bcmpmu;

	mutex_lock(&pchrgr->lock);
	schedule_delayed_work(&pchrgr->dwork, msecs_to_jiffies(0));

	mutex_unlock(&pchrgr->lock);
	enable_irq(pchrgr->irq_chrgr_ac5v);
}

static void ac_det_work(struct work_struct *work)
{
	struct bcmpmu_chrgr *pchrgr =
	    container_of(work, struct bcmpmu_chrgr, dwork.work);
	struct bcmpmu *bcmpmu = pchrgr->bcmpmu;
	enum bcmpmu_event_t event = BCMPMU_EXTCHRGR_EVENT_DETECTION;
	int new_sig_state = gpio_get_value(pchrgr->ext_chrgr_info->ac5v);
	int new_det_state = pchrgr->det_state;
	struct power_supply *ps = NULL;

	if (pchrgr->sig_state == new_sig_state)
		pchrgr->db_cnt++;
	else {
		pchrgr->sig_state = new_sig_state;
		pchrgr->db_cnt = 0;
	}
	if (pchrgr->db_cnt < 5)
		new_det_state = DET_STATE_DEBOUNCE;
	else {
		pchrgr->db_cnt = 0;
		if (pchrgr->sig_state == 0) {
			new_det_state = DET_STATE_CONNECT;
			pchrgr->ac_online = 1;
			bcmpmu->usb_accy_data.ext_chrgr_present = 1;
		} else {
			new_det_state = DET_STATE_IDLE;
			pchrgr->ac_online = 0;
			bcmpmu->usb_accy_data.ext_chrgr_present = 0;
		}
	}

	if (new_det_state == DET_STATE_DEBOUNCE) {
		wake_lock(&pchrgr->wake_lock);
		schedule_delayed_work(&pchrgr->dwork, msecs_to_jiffies(100));
	} else {
		if (new_det_state != pchrgr->det_state)
			ps = power_supply_get_by_name("ac");
			if (ps)
				power_supply_changed(ps);
			blocking_notifier_call_chain(
				&bcmpmu->event[event].notifiers,
				event, &pchrgr->ac_online);
		wake_unlock(&pchrgr->wake_lock);
	}
	pchrgr->det_state = new_det_state;
}

static int __devinit bcmpmu_chrgr_probe(struct platform_device *pdev)
{
	int ret;

	struct bcmpmu *bcmpmu = pdev->dev.platform_data;
	struct bcmpmu_platform_data *pdata = bcmpmu->pdata;
	struct bcmpmu_chrgr *pchrgr;

	pr_chrgr(INIT, "%s, called\n", __func__);

	pchrgr = kzalloc(sizeof(struct bcmpmu_chrgr), GFP_KERNEL);
	if (pchrgr == NULL) {
		pr_chrgr(ERROR, "%s, failed to alloc mem.\n", __func__);
		return -ENOMEM;
	}
	init_waitqueue_head(&pchrgr->wait);
	mutex_init(&pchrgr->lock);
	pchrgr->bcmpmu = bcmpmu;
	bcmpmu->chrgrinfo = (void *)pchrgr;

	bcmpmu->chrgr_usb_en = bcmpmu_chrgr_usb_en;
	bcmpmu->chrgr_wac_en = bcmpmu_chrgr_wac_en;
	bcmpmu->set_icc_fc = bcmpmu_set_icc_fc;
	bcmpmu->set_icc_qc = bcmpmu_set_icc_qc;
	bcmpmu->set_eoc = bcmpmu_set_eoc;
	bcmpmu->set_vfloat = bcmpmu_set_vfloat;

	pchrgr->eoc = 0;

	pchrgr->chrgr.properties = bcmpmu_chrgr_props;
	pchrgr->chrgr.num_properties = ARRAY_SIZE(bcmpmu_chrgr_props);
	pchrgr->chrgr.get_property = bcmpmu_chrgr_get_property;
	pchrgr->chrgr.set_property = bcmpmu_chrgr_set_property;
	pchrgr->chrgr.name = "charger";
	pchrgr->chrgr.type = POWER_SUPPLY_TYPE_MAINS;

	pchrgr->usb.properties = bcmpmu_usb_props;
	pchrgr->usb.num_properties = ARRAY_SIZE(bcmpmu_usb_props);
	pchrgr->usb.get_property = bcmpmu_usb_get_property;
	pchrgr->usb.set_property = bcmpmu_usb_set_property;
	pchrgr->usb.name = "usb";
	pchrgr->usb.type = POWER_SUPPLY_TYPE_USB;

	pchrgr->ac.properties = bcmpmu_ac_props;
	pchrgr->ac.num_properties = ARRAY_SIZE(bcmpmu_ac_props);
	pchrgr->ac.get_property = bcmpmu_ac_get_property;
	pchrgr->ac.set_property = bcmpmu_ac_set_property;
	pchrgr->ac.name = "ac";
	pchrgr->ac.type = POWER_SUPPLY_TYPE_MAINS;

	wake_lock_init(&pchrgr->wake_lock, WAKE_LOCK_SUSPEND, "ext_chrgr");

	ret = power_supply_register(&pdev->dev, &pchrgr->chrgr);
	if (ret)
		goto err;
	ret = power_supply_register(&pdev->dev, &pchrgr->usb);
	if (ret)
		goto err;
	ret = power_supply_register(&pdev->dev, &pchrgr->ac);
	if (ret)
		goto err;

	pchrgr->chrgrcurr_max = bcmpmu->usb_accy_data.max_curr_chrgr;
	pchrgr->chrgrtype = bcmpmu->usb_accy_data.chrgr_type;
	pchrgr->usbtype = bcmpmu->usb_accy_data.usb_type;
	if ((pchrgr->chrgrtype > PMU_CHRGR_TYPE_NONE) &&
		(pchrgr->chrgrtype < PMU_CHRGR_TYPE_MAX))
		pchrgr->chrgr_online = 1;
	if ((pchrgr->usbtype > PMU_USB_TYPE_NONE) &&
		(pchrgr->usbtype < PMU_USB_TYPE_MAX))
		pchrgr->usb_online = 1;

	pchrgr->ext_chrgr_info = pdata->ext_chrgr_info;
	if (pchrgr->ext_chrgr_info == NULL) {
		pr_chrgr(ERROR, "%s, external charger info not defined.\n",
			__func__);
		goto err;
	}
	pchrgr->workq = create_workqueue("bcmpmu-chrgr");
	INIT_WORK(&pchrgr->work, bcmpmu_chrgr_handler);
	INIT_DELAYED_WORK(&pchrgr->dwork, ac_det_work);

	ret = gpio_request(pchrgr->ext_chrgr_info->ac5v,
		"bcmpmu-extchrgr-ac5v");
	if (ret < 0) {
		dev_err(bcmpmu->dev, "%s failed at gpio_request for ac5v.\n",
			__func__);
		goto err;
	}
	ret = gpio_direction_input(pchrgr->ext_chrgr_info->ac5v);
	if (ret < 0) {
		dev_err(bcmpmu->dev, "%s failed gpio dir input: ac5v.\n",
			__func__);
		goto err;
	}
	pchrgr->irq_chrgr_ac5v = gpio_to_irq(pchrgr->ext_chrgr_info->ac5v);
	ret = request_irq(pchrgr->irq_chrgr_ac5v, bcmpmu_chrgr_isr,
		IRQF_DISABLED | IRQF_TRIGGER_FALLING |
		IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
		"bcmpmu-ext-chrgr-ac", pchrgr);
	if (ret) {
		pr_chrgr(ERROR, "%s, failed at request irq_chrgr_ac5v.\n",
			__func__);
		goto err;
	}
	enable_irq(pchrgr->irq_chrgr_ac5v);

	ret = gpio_request(pchrgr->ext_chrgr_info->iusb,
		"bcmpmu-extchrgr-iusb");
	if (ret < 0) {
		dev_err(bcmpmu->dev, "%s failed at gpio_request for iusb.\n",
			__func__);
		goto err;
	}
	ret = gpio_direction_output(pchrgr->ext_chrgr_info->iusb, 0);
	if (ret < 0) {
		dev_err(bcmpmu->dev, "%s failed at gpio dir output: iusb.\n",
			__func__);
		goto err;
	}

	ret = gpio_request(pchrgr->ext_chrgr_info->cen, "bcmpmu-extchrgr-cen");
	if (ret < 0) {
		dev_err(bcmpmu->dev, "%s failed at gpio_request for cen.\n",
			__func__);
		goto err;
	}
	ret = gpio_direction_output(pchrgr->ext_chrgr_info->cen, 0);
	if (ret < 0) {
		dev_err(bcmpmu->dev, "%s failed at gpio dir output: cen.\n",
			__func__);
		goto err;
	}

	ret = gpio_request(pchrgr->ext_chrgr_info->flt, "bcmpmu-extchrgr-flt");
	if (ret < 0) {
		dev_err(bcmpmu->dev, "%s failed at gpio_request for flt.\n",
			__func__);
		goto err;
	}
	ret = gpio_direction_input(pchrgr->ext_chrgr_info->flt);
	if (ret < 0) {
		dev_err(bcmpmu->dev, "%s failed at gpio dir input: flt.\n",
			__func__);
		goto err;
	}
	pchrgr->irq_chrgr_flt = gpio_to_irq(pchrgr->ext_chrgr_info->flt);
	ret = request_irq(pchrgr->irq_chrgr_flt, bcmpmu_chrgr_isr,
		IRQF_DISABLED | IRQF_TRIGGER_FALLING |
		IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
		"bcmpmu-ext-chrgr-flt", pchrgr);
	if (ret) {
		pr_chrgr(ERROR, "%s, failed request irq_chrgr_flt.\n",
			__func__);
		goto err;
	}
	enable_irq(pchrgr->irq_chrgr_flt);

	ret = gpio_request(pchrgr->ext_chrgr_info->chg, "bcmpmu-extchrgr-chg");
	if (ret < 0) {
		dev_err(bcmpmu->dev, "%s failed at gpio_request: chg.\n",
			__func__);
		goto err;
	}
	ret = gpio_direction_input(pchrgr->ext_chrgr_info->chg);
	if (ret < 0) {
		dev_err(bcmpmu->dev, "%s failed at gpio dir input: chg.\n",
			__func__);
		goto err;
	}
	pchrgr->irq_chrgr_chg = gpio_to_irq(pchrgr->ext_chrgr_info->chg);
	ret = request_irq(pchrgr->irq_chrgr_chg, bcmpmu_chrgr_isr,
		IRQF_DISABLED | IRQF_TRIGGER_FALLING |
		IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
		"bcmpmu-ext-chrgr-chg", pchrgr);
	if (ret) {
		pr_chrgr(ERROR, "%s, failed request irq_chrgr_chg.\n",
			__func__);
		goto err;
	}
	enable_irq(pchrgr->irq_chrgr_chg);

	bcmpmu->usb_accy_data.ext_chrgr_present = 0;
	schedule_delayed_work(&pchrgr->dwork, msecs_to_jiffies(500));

#ifdef CONFIG_MFD_BCMPMU_DBG
	ret = sysfs_create_group(&pdev->dev.kobj, &bcmpmu_chrgr_attr_group);
#endif
	return 0;

err:
	gpio_free(pchrgr->ext_chrgr_info->ac5v);
	power_supply_unregister(&pchrgr->chrgr);
	power_supply_unregister(&pchrgr->usb);
	power_supply_unregister(&pchrgr->ac);
	cancel_delayed_work_sync(&pchrgr->dwork);
	wake_lock_destroy(&pchrgr->wake_lock);
	kfree(pchrgr);
	return ret;
}

static int __devexit bcmpmu_chrgr_remove(struct platform_device *pdev)
{
	struct bcmpmu *bcmpmu = pdev->dev.platform_data;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;

	power_supply_unregister(&pchrgr->chrgr);
	power_supply_unregister(&pchrgr->usb);
	power_supply_unregister(&pchrgr->ac);
	cancel_delayed_work_sync(&pchrgr->dwork);
	wake_lock_destroy(&pchrgr->wake_lock);

#ifdef CONFIG_MFD_BCMPMU_DBG
	sysfs_remove_group(&pdev->dev.kobj, &bcmpmu_chrgr_attr_group);
#endif
	return 0;
}

static struct platform_driver bcmpmu_chrgr_driver = {
	.driver = {
		.name = "bcmpmu_chrgr",
	},
	.probe = bcmpmu_chrgr_probe,
	.remove = __devexit_p(bcmpmu_chrgr_remove),
};

static int __init bcmpmu_chrgr_init(void)
{
	return platform_driver_register(&bcmpmu_chrgr_driver);
}
module_init(bcmpmu_chrgr_init);

static void __exit bcmpmu_chrgr_exit(void)
{
	platform_driver_unregister(&bcmpmu_chrgr_driver);
}
module_exit(bcmpmu_chrgr_exit);

MODULE_DESCRIPTION("BCM PMIC charger driver");
MODULE_LICENSE("GPL");
