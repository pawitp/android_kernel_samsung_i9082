/*****************************************************************************
 *  Copyright 2001 - 2008 Broadcom Corporation.	All rights reserved.
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

/*
 * Capri BRT uses Maxim's MAX8903 external charger (EC) to charge
 * battery. MAX8903 IC is mounted on PMU daughter card and works in tight
 * collaboration with PMU.
 *
 * EC charges battery from the following sources:
 * - AC/DC adapter
 * - USB wall charger - Dedicated Charging Port (DCP)
 * - USB PC host port - Standard Downstream Port (SDP)
 *
 * EC driver uses linux power supply (PSY) framework (defined in
 * linux/power_supply.h) to present 2 types of PSYs POWER_SUPPLY_TYPE_MAINS and
 * POWER_SUPPLY_TYPE_USB to Android.
 *
 * There is a convention in the industry to report charging from USB wall
 * charger as "AC" (POWER_SUPPLY_TYPE_MAINS is online) and charging from USB
 * host as "USB" (POWER_SUPPLY_TYPE_USB is online).
 *
 * If both AC/DC and USB charger are connected the charging happens from AC/DC
 * and should be reported as "AC".
 *
 *  This gives the following reporting table:
 *
 * #	AC/DC	USB	Report
 * 1	0	0	No charger
 * 2	0	Wall	AC
 * 3	0	Host	USB
 * 4	1	0	AC
 * 5	1	Wall	AC
 * 6	1	Host	AC
 *
 * Connection / disconnection of AC/DC adapter is detected by ISR and obtained
 * as the state of corresponding GPIO.
 *
 * Connection / disconnection status of USB cable is not directly available in
 * EC driver. It is detected in drivers/mfd/bcmpmu-accy.c and is passed to EC
 * driver via setting POWER_SUPPLY_PROP_TYPE property.
 *
 * EC is defined as one of the PMU "fellow" devices and as such
 * has access to struct bcmpmu *bcmpmu; via platform data pointer.
 *
 * Notes on Battery Temperature Monitoring:
 * From a charging perspective, battery temperature monitoring is needed to
 * stop charging when the battery temperature is out of range. Although the
 * MAX8903C is capable of monitoring battery temperature, the hardware design
 * for capri BRT is such that it is disabled, due to the fact
 * that the temperature reading can only be fed to either the charger or the
 * PMU, but not both.  Since battery temperature is required by software (e.g.
 * fuel gauge), the battery temperature data is made available to the PMU.
 * This driver is responsible for monitoring battery temperature and disabling
 * charging when it is out of range. Some known limitations of this design:
 *   - No battery monitoring at uboot stage when booting or when kernel crashes
 *   - No battery monitoring when device suspeneded (although the device should
 *     never be suspended when charging, so this should be fine.)
 */

#define DEBOUNCE_TIME_USECS	   128000

#define BCMPMU_PRINT_ERROR (1U << 0)
#define BCMPMU_PRINT_INIT (1U << 1)
#define BCMPMU_PRINT_FLOW (1U << 2)
#define BCMPMU_PRINT_DATA (1U << 3)

/* Period to poll battery temperature in ms */
#define BCMPMU_BATT_MON_INTERVAL 5000 /* 5s */

/* Battery-specific charging temperature range.  This is for the BRT battery. */
#define BCMPMU_BATT_CHARGE_UNDERTEMP 0	/* Min charging temp, degCx10 (0C) */
#define BCMPMU_BATT_CHARGE_OVERTEMP 450 /* Max charging temp, degCx10 (45C) */

static int debug_mask = BCMPMU_PRINT_ERROR | BCMPMU_PRINT_INIT;
#define pr_chrgr(debug_level, args...)				\
	do {							\
		if (debug_mask & BCMPMU_PRINT_##debug_level) {	\
			pr_info(args);				\
		}						\
	} while (0)

struct bcmpmu_chrgr {
	struct bcmpmu *bcmpmu;
	struct power_supply usb;
	struct power_supply ac;
	struct mutex lock;
	struct wake_lock wake_lock;
	struct workqueue_struct *workq;
	struct work_struct isr_work;
	struct delayed_work batt_mon_work;
	int batt_mon_count;	/* debug only: batt temp mon counter */
	int dbg_temp;		/* debug only: override batt temp, .1 degC */
	bool batt_mon_en;	/* debug only: batt temp mon enabled */
	bool batt_temp_oor;	/* battery temperature out-of-range */
	bool usb_chrgr_en;	/* software request to enable usb charger */
	int eoc;
	enum bcmpmu_usb_type_t usbtype;
	int usb_wallcharger_connected;
	int usb_connected;
	int ac_connected;
	struct bcmpmu_ext_chrgr_info *ext_chrgr_info;
	int charge_state;
	int fault_state;
	int irq_chrgr_ac5v;
	int irq_chrgr_flt;
	int irq_chrgr_chg;
};

static enum power_supply_property bcmpmu_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TYPE,
};

static enum power_supply_property bcmpmu_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

/*
 * Report AC and USB online property in accordance with this truth table:
 * #	AC/DC	USB	Report
 * 1	0	0	No charger
 * 2	0	Wall	AC
 * 3	0	Host	USB
 * 4	1	0	AC
 * 5	1	Wall	AC
 * 6	1	Host	AC
 */
static int is_ac_online(struct bcmpmu_chrgr *pchrgr)
{
	pr_chrgr(FLOW, "%s, ac_connected %d usb_wallcharger_connected %d\n",
		__func__, pchrgr->ac_connected,
		pchrgr->usb_wallcharger_connected);

	/* if AC/DC cable is connected */
	if (pchrgr->ac_connected)
		return true;

	/* if USB wall charger is connected */
	if (pchrgr->usb_wallcharger_connected)
		return true;

	/* AC is not online otherwise */
	return false;
}

static int is_usb_online(struct bcmpmu_chrgr *pchrgr)
{
	/* if AC/DC is disconnected and USB is connected in host mode */
	if (!pchrgr->ac_connected && pchrgr->usb_connected &&
		pchrgr->usbtype == PMU_USB_TYPE_SDP)
		return true;
	return false;
}

static void charging_enable_update(struct bcmpmu_chrgr *pchrgr)
{
	int cen = gpio_get_value(pchrgr->ext_chrgr_info->cen); /* active low */

	pr_chrgr(FLOW,
		"%s(): cen=%d, batt_temp_oor=%d, usb_chrgr_en=%d, "
		"ac_connected=%d\n",
		__func__,
		cen,
		pchrgr->batt_temp_oor,
		pchrgr->usb_chrgr_en,
		pchrgr->ac_connected);
	/*
	 * Charging is disabled when battery temperature is out of range, or
	 * when software specifically request it and no AC charger is available
	 */
	if (pchrgr->batt_temp_oor ||
		(!pchrgr->usb_chrgr_en && !pchrgr->ac_connected)) {

		if (!cen) {
			pr_chrgr(FLOW, "%s(): Disable charger.\n", __func__);
			gpio_set_value(pchrgr->ext_chrgr_info->cen, 1);
		}

	/* In all other cases, enable charger by default */
	} else if (cen) {
		pr_chrgr(FLOW, "%s(): Enable charger.\n", __func__);
		gpio_set_value(pchrgr->ext_chrgr_info->cen, 0);
	}

	return;
}

/**
 * starts or stops the battery monitoring
 */
void bcmpmu_batt_mon_update(struct bcmpmu_chrgr *pchrgr)
{
	pr_chrgr(FLOW, "%s(): ac_connected=%d, usb_connected=%d, "
		"usb_wallcharger_connected=%d, batt_mon_en=%d\n",
		__func__,
		pchrgr->ac_connected,
		pchrgr->usb_connected,
		pchrgr->usb_wallcharger_connected,
		pchrgr->batt_mon_en);

	/* Enable battery monitor if any charger is connected */
	if (pchrgr->ac_connected || pchrgr->usb_connected
		|| pchrgr->usb_wallcharger_connected) {
		pr_chrgr(FLOW,
			"%s(): Enabling battery temp monitor.\n",
			__func__);
		queue_delayed_work(pchrgr->workq, &pchrgr->batt_mon_work, 0);
		pchrgr->batt_mon_en = true;

	} else {
		pr_chrgr(FLOW,
			"%s(): Disabling battery temp monitor.\n",
			__func__);
		cancel_delayed_work_sync(&pchrgr->batt_mon_work);
		pchrgr->batt_mon_en = false;
	}

	return;
}

static int bcmpmu_usb_get_property(struct power_supply *ps,
				enum power_supply_property prop,
				union power_supply_propval *val)
{
	int ret = 0;
	struct bcmpmu_chrgr *pchrgr = container_of(ps,
						struct bcmpmu_chrgr, usb);

	pr_chrgr(FLOW, "%s, requested property: %d\n",
		__func__, prop);

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = is_usb_online(pchrgr);
		break;

	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

/* this allows drivers/mfd/bcmpmu-accy.c to set USB PSY
 * properties. POWER_SUPPLY_PROP_ONLINE is set when accy detects USB Host cable
 * connection. POWER_SUPPLY_PROP_TYPE sets USB type. */
static int bcmpmu_usb_set_property(struct power_supply *ps,
				enum power_supply_property prop,
				const union power_supply_propval *propval)
{
	int ret = 0;
	struct bcmpmu_chrgr *pchrgr = container_of(ps,
						struct bcmpmu_chrgr, usb);

	pr_chrgr(FLOW, "%s, property to set: %d value: %d\n",
		 __func__, prop, propval->intval);

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		pchrgr->usb_connected = propval->intval;

		/* Update battery temperature monitoring if needed */
		bcmpmu_batt_mon_update(pchrgr);

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

static int bcmpmu_ac_get_property(struct power_supply *ps,
				enum power_supply_property prop,
				union power_supply_propval *val)
{
	int ret = 0;
	struct bcmpmu_chrgr *pchrgr = container_of(ps,
						struct bcmpmu_chrgr, ac);

	pr_chrgr(FLOW, "%s, requested property: %d\n",
		__func__, prop);

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = is_ac_online(pchrgr);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/* this allows drivers/mfd/bcmpmu-accy.c to set AC "online" property that is
 * used to signal that USB wall charger is connected. Note that accy driver also
 * sets POWER_SUPPLY_PROP_CURRENT_NOW and POWER_SUPPLY_PROP_TYPE which are not
 * handled now, as there is no known use for them. */
static int bcmpmu_ac_set_property(struct power_supply *ps,
				enum power_supply_property prop,
				const union power_supply_propval *propval)
{
	int ret = 0;
	struct bcmpmu_chrgr *pchrgr = container_of(ps,
						struct bcmpmu_chrgr, ac);

	pr_chrgr(FLOW, "%s, property to set: %d value: %d\n",
		 __func__, prop, propval->intval);

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		pchrgr->usb_wallcharger_connected = propval->intval;

		/* Update battery temperature monitoring if needed */
		bcmpmu_batt_mon_update(pchrgr);

		break;

	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int bcmpmu_set_icc_qc(struct bcmpmu *bcmpmu, int curr)
{
	pr_chrgr(FLOW, "%s, called. Current %d\n", __func__, curr);
	return 0;
}

/* used by FG to pass measured charging current. Set usb current limit */
static int bcmpmu_set_icc_fc(struct bcmpmu *bcmpmu, int curr)
{
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;

	pr_chrgr(FLOW, "%s, called. Current %d\n", __func__, curr);

	if (curr > 100)
		gpio_set_value(pchrgr->ext_chrgr_info->iusb, 1);
	else
		gpio_set_value(pchrgr->ext_chrgr_info->iusb, 0);

	return 0;
}

static int bcmpmu_set_vfloat(struct bcmpmu *bcmpmu, int volt)
{
	pr_chrgr(FLOW, "%s, called. Voltage  %d\n", __func__, volt);
	return 0;
}

static int bcmpmu_set_eoc(struct bcmpmu *bcmpmu, int curr)
{
	pr_chrgr(FLOW, "%s, called. Current %d\n", __func__, curr);
	return 0;
}

/**
 * enable / disable charging. For the MAX8903C, it is not necessary to
 * explicitly disable charging in normal circumstances; it stops charging
 * when the battery is fully charged.  This funciton is implemented for
 * the sake of completeness.
 */
static int bcmpmu_chrgr_usb_en(struct bcmpmu *bcmpmu, int en)
{
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;

	pr_chrgr(FLOW, "%s, called. Enable %d\n", __func__, en);

	pchrgr->usb_chrgr_en = en;

	charging_enable_update(pchrgr);

	return 0;
}

static int bcmpmu_chrgr_wac_en(struct bcmpmu *bcmpmu, int en)
{
	pr_chrgr(FLOW, "%s, called. Enable %d\n", __func__, en);
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

#ifndef CONFIG_CAPRI_11351
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
#endif

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
get_iusb(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	struct bcmpmu *bcmpmu = dev->platform_data;
	int val;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;

	val = gpio_get_value(pchrgr->ext_chrgr_info->iusb);

	return sprintf(buf, "iusb (GPIO %d) = %d\n",
		pchrgr->ext_chrgr_info->iusb, val);
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
get_cen(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	struct bcmpmu *bcmpmu = dev->platform_data;
	int val;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;

	val = gpio_get_value(pchrgr->ext_chrgr_info->cen);

	return sprintf(buf, "cen (GPIO %d) = %d\n",
		pchrgr->ext_chrgr_info->cen, val);
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
	gpio_set_value(pchrgr->ext_chrgr_info->cen, !val);
	return n;
}

static ssize_t
set_dbg_temp(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t n)
{
	struct bcmpmu *bcmpmu = dev->platform_data;
	int val;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;
	sscanf(buf, "%d", &val);
	pchrgr->dbg_temp = val;
	return n;
}

static ssize_t
get_dbg_temp(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct bcmpmu *bcmpmu = dev->platform_data;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;

	return sprintf(buf,
		"dbg_temp = %d degCx10. Set to 0xffff to disable.\n",
		pchrgr->dbg_temp);
}

static DEVICE_ATTR(dbgmsk, 0644, dbgmsk_show, dbgmsk_set);
static DEVICE_ATTR(ac5v, 0644, get_ac5v, NULL);
static DEVICE_ATTR(flt, 0644, get_flt, NULL);
#ifndef CONFIG_CAPRI_11351
static DEVICE_ATTR(chg, 0644, get_chg, NULL);
#endif
static DEVICE_ATTR(iusb, 0644, get_iusb, set_iusb);
static DEVICE_ATTR(cen, 0644, get_cen, set_cen);
static DEVICE_ATTR(usb_en, 0644, NULL, bcmpmu_dbg_usb_en);
static DEVICE_ATTR(dbg_temp, 0644, get_dbg_temp, set_dbg_temp);

static struct attribute *bcmpmu_chrgr_attrs[] = {
	&dev_attr_dbgmsk.attr,
	&dev_attr_ac5v.attr,
	&dev_attr_flt.attr,
#ifndef CONFIG_CAPRI_11351
	&dev_attr_chg.attr,
#endif
	&dev_attr_iusb.attr,
	&dev_attr_cen.attr,
	&dev_attr_usb_en.attr,
	&dev_attr_dbg_temp.attr,
	NULL
};

static const struct attribute_group bcmpmu_chrgr_attr_group = {
	.attrs = bcmpmu_chrgr_attrs,
};
#endif

static irqreturn_t bcmpmu_chrgr_isr(int irq, void *data)
{
	struct bcmpmu_chrgr *pchrgr = data;
	queue_work(pchrgr->workq, &pchrgr->isr_work);
	return IRQ_HANDLED;
}

/**
 * ac_status_update() - Reads latest AC charger status and process accordingly
 * @pchrgr	pointer to bcmpmu_chrgr structure
 * @force	Force the processing regardless of if the status has changed.
 *		Used in the probe function to always send a notification.
 *
 * Check the AC charger status by reading the corresponding GPIO.  If a change
 * has occured sinced the last cached value:
 * 1 - cache the new value
 * 2 - update PSY
 * 3 - invoke notifier call chain
 * 4 - updates wake_lock
 */
static int ac_status_update(struct bcmpmu_chrgr *pchrgr, bool force)
{
	struct bcmpmu *bcmpmu;
	int ac_connected = 0;
	enum bcmpmu_event_t event = BCMPMU_EXTCHRGR_EVENT_DETECTION;

	bcmpmu = pchrgr->bcmpmu;

	/* get the state of ac5v GPIO. Set to low means that AC/DC cable
	 * connected, so online status is the reverse */
	ac_connected = !gpio_get_value(pchrgr->ext_chrgr_info->ac5v);

	if ((ac_connected != pchrgr->ac_connected) || force) {
		pr_chrgr(FLOW, "%s, ac_connected changed from: %d to: %d\n",
			__func__, pchrgr->ac_connected, ac_connected);

		/*
		 * cache the current value of AC/DC online and pass this info
		 * to accy.
		 */
		pchrgr->ac_connected = ac_connected;
		bcmpmu->usb_accy_data.ext_chrgr_present = ac_connected;

		/* notify about "ac" PSY change */
		power_supply_changed(&pchrgr->ac);

		/* notify interested parties */
		blocking_notifier_call_chain(
			&bcmpmu->event[event].notifiers,
			event, &pchrgr->ac_connected);

		/* Update battery temperature monitoring if needed */
		bcmpmu_batt_mon_update(pchrgr);

		/* take /release wakelock depending on AC/DC connected status */
		if (ac_connected)
			wake_lock(&pchrgr->wake_lock);
		else
			wake_unlock(&pchrgr->wake_lock);
	}

	return 0;
}

void bcmpmu_chrgr_handler(struct work_struct *work)
{
	struct bcmpmu_chrgr *pchrgr;
	struct bcmpmu *bcmpmu;
#ifndef CONFIG_CAPRI_11351
	int charge_state;
#endif
	int fault_state;

	pchrgr = container_of(work, struct bcmpmu_chrgr, isr_work);
	bcmpmu = pchrgr->bcmpmu;

	pr_chrgr(FLOW, "%s, called\n", __func__);

	/* since we have one ISR handler for AC, CHRGR and FLT interrupts we may
	 * be here due any of them, so we want to do see if data related to any
	 * of them changed. Note that alternatively we may consider having
	 * individual ISRs for individual IRQs */

	/* note that we do not need lock mutex here, as this code is executed in
	   the work queue sequentially */

	/* Update AC/DC connection status */
	ac_status_update(pchrgr, false);

#ifndef CONFIG_CAPRI_11351
	/* fixme: for now we only detect state change. Need to implement the
	   proper logic */
	charge_state = gpio_get_value(pchrgr->ext_chrgr_info->chg);
	if (charge_state != pchrgr->charge_state) {
		pr_chrgr(FLOW, "%s, charge_state changed from: %d to: %d\n",
			__func__, pchrgr->charge_state, charge_state);
		pchrgr->charge_state = charge_state;
	}
#endif
	/* fixme: for now we only detect state change. Need to implement the
	   proper logic */
	fault_state = gpio_get_value(pchrgr->ext_chrgr_info->flt);
	if (fault_state != pchrgr->fault_state) {
		pr_chrgr(FLOW, "%s, fault_state changed from: %d to: %d\n",
			__func__, pchrgr->fault_state, fault_state);
		pchrgr->fault_state = fault_state;
	}
}

/**
 * Scheduled to run periodically to check batt temp and disable charging if temp
 * out of range. Requeues itself.
 */
void bcmpmu_batt_mon_work(struct work_struct *work)
{
	struct bcmpmu_chrgr *pchrgr =
		container_of(work, struct bcmpmu_chrgr, batt_mon_work.work);
	int tempC = 0;  /* temp in 10xdegC.  eg. 100 -> 10.0degC */
	bool curr_temp_oor;
	struct power_supply *ps;
	union power_supply_propval val;

	ps = power_supply_get_by_name("battery");

	/* if we are not temp debug mode */
	if (pchrgr->dbg_temp == 0xffff) {
		/* read battery temperature */
		if (!ps) {
			pr_chrgr(ERROR,
				"%s(): Error reading battery temperature.\n",
				__func__);

			/* set temperature to out of range to be safe */
			tempC = -200;
		} else {
			ps->get_property(ps, POWER_SUPPLY_PROP_TEMP, &val);
			tempC = val.intval;
			/* handle special case - battery is disconnected and
			   temperature reading is at -20 degree celcius. No
			   need to do monitoring in this case  */
			if (tempC == -200) {
				pr_chrgr(INIT, "%s(): Disabling monitoring "
					"as battery is disconnected\n",
					__func__);
				pchrgr->batt_mon_en = false;
				return;
			}
		}
	} else
		/* override with debug temperature */
		tempC = pchrgr->dbg_temp;

	pr_chrgr(FLOW, "%s(): called, batt_mon_count=%d, temp=%d\n",
		 __func__, pchrgr->batt_mon_count, tempC);
	pchrgr->batt_mon_count++;

	/* save current state of the temperature OOR flag */
	curr_temp_oor = pchrgr->batt_temp_oor;

	if ((tempC < BCMPMU_BATT_CHARGE_UNDERTEMP) ||
		(tempC > BCMPMU_BATT_CHARGE_OVERTEMP)) {
		pr_chrgr(FLOW,
			"%s(): Battery temperature (%d) out of charging range "
			"(%d to %d).\n",
			__func__,
			tempC,
			BCMPMU_BATT_CHARGE_UNDERTEMP,
			BCMPMU_BATT_CHARGE_OVERTEMP);
		pchrgr->batt_temp_oor = true;

	} else
		pchrgr->batt_temp_oor = false;


	/* if we are in OOR state */
	if (pchrgr->batt_temp_oor) {
		/* set PSY properties indicating overheat and not charging
		 * status */
		val.intval = POWER_SUPPLY_HEALTH_OVERHEAT;
		ps->set_property(ps,
				POWER_SUPPLY_PROP_HEALTH, &val);
		val.intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		ps->set_property(ps,
				POWER_SUPPLY_PROP_STATUS, &val);

		/* notify android about PSY change*/
		power_supply_changed(ps);
	}

	/* if recovered from OOR condition */
	else if (curr_temp_oor && !pchrgr->batt_temp_oor) {
		/* set PSY properies indicating normal health and
		   charging status. */
		val.intval = POWER_SUPPLY_HEALTH_GOOD;
		ps->set_property(ps,
				POWER_SUPPLY_PROP_HEALTH, &val);
		val.intval = POWER_SUPPLY_STATUS_CHARGING;
		ps->set_property(ps,
				POWER_SUPPLY_PROP_STATUS, &val);

		/* notify android about PSY change*/
		power_supply_changed(ps);
	}

	/* Update CEN */
	charging_enable_update(pchrgr);

	/* Requeues itself */
	queue_delayed_work(pchrgr->workq, &pchrgr->batt_mon_work,
		msecs_to_jiffies(BCMPMU_BATT_MON_INTERVAL));

	return;
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
	pchrgr->ac.name = "charger";
	pchrgr->ac.type = POWER_SUPPLY_TYPE_MAINS;

	wake_lock_init(&pchrgr->wake_lock, WAKE_LOCK_SUSPEND, "ext_chrgr");

	/* Register usb and ac PSYs */
	ret = power_supply_register(&pdev->dev, &pchrgr->usb);
	if (ret) {
		pr_chrgr(ERROR, "%s, failed to register usb PSY.\n",
			__func__);
		goto err;
	}
	ret = power_supply_register(&pdev->dev, &pchrgr->ac);
	if (ret) {
		pr_chrgr(ERROR, "%s, failed to register ac PSY.\n",
			__func__);
		goto err;
	}

	pchrgr->ext_chrgr_info = pdata->ext_chrgr_info;
	if (pchrgr->ext_chrgr_info == NULL) {
		pr_chrgr(ERROR, "%s, external charger info not defined.\n",
			__func__);
		goto err;
	}

	pchrgr->dbg_temp = 0xffff;
	pchrgr->usb_chrgr_en = true;

	pchrgr->workq = create_workqueue("bcmpmu-chrgr");
	INIT_WORK(&pchrgr->isr_work, bcmpmu_chrgr_handler);

	/* Initialize battery temperature monitor work item */
	pchrgr->batt_mon_count = 0;
	pchrgr->batt_mon_en = false;
	pchrgr->batt_temp_oor = false;
	INIT_DELAYED_WORK(&pchrgr->batt_mon_work, bcmpmu_batt_mon_work);

	ret = gpio_request_one(pchrgr->ext_chrgr_info->ac5v, GPIOF_IN,
			"bcmpmu-extchrgr-ac5v");
	if (ret < 0) {
		dev_err(bcmpmu->dev,
			"%s failed at gpio_request_one for input gpio ac5v.\n",
			__func__);
		goto err;
	}
	/* debounce AC plug in/out GPIO */
	ret = gpio_set_debounce(pchrgr->ext_chrgr_info->ac5v,
				DEBOUNCE_TIME_USECS);
	if (ret < 0) {
		dev_err(bcmpmu->dev,
			"%s failed at gpio_set_debounce for gpio ac5v.\n",
			__func__);
		goto err;
	}

	pchrgr->irq_chrgr_ac5v = gpio_to_irq(pchrgr->ext_chrgr_info->ac5v);
	ret = request_irq(pchrgr->irq_chrgr_ac5v, bcmpmu_chrgr_isr,
			IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
			IRQF_NO_SUSPEND,
			"bcmpmu-ext-chrgr-ac", pchrgr);
	if (ret) {
		pr_chrgr(ERROR, "%s, failed at request irq_chrgr_ac5v.\n",
			__func__);
		goto err;
	}

	ret = gpio_request_one(pchrgr->ext_chrgr_info->iusb, GPIOF_OUT_INIT_LOW,
			"bcmpmu-extchrgr-iusb");
	if (ret < 0) {
		dev_err(bcmpmu->dev,
			"%s failed at gpio_request_one for output gpio iusb.\n",
			__func__);
		goto err;
	}

	ret = gpio_request_one(pchrgr->ext_chrgr_info->cen, GPIOF_OUT_INIT_LOW,
			"bcmpmu-extchrgr-cen");
	if (ret < 0) {
		dev_err(bcmpmu->dev,
			"%s failed at gpio_request_one for output gpio cen.\n",
			__func__);
		goto err;
	}

	ret = gpio_request_one(pchrgr->ext_chrgr_info->flt, GPIOF_IN,
			"bcmpmu-extchrgr-flt");
	if (ret < 0) {
		dev_err(bcmpmu->dev,
			"%s failed at gpio_request_one for input gpio flt.\n",
			__func__);
		goto err;
	}
	pchrgr->irq_chrgr_flt = gpio_to_irq(pchrgr->ext_chrgr_info->flt);
	ret = request_irq(pchrgr->irq_chrgr_flt, bcmpmu_chrgr_isr,
			IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
			IRQF_NO_SUSPEND,
			"bcmpmu-ext-chrgr-flt", pchrgr);
	if (ret) {
		pr_chrgr(ERROR, "%s, failed request irq_chrgr_flt.\n",
			__func__);
		goto err;
	}
#ifndef CONFIG_CAPRI_11351
	ret = gpio_request_one(pchrgr->ext_chrgr_info->chg, GPIOF_IN,
			"bcmpmu-extchrgr-chg");
	if (ret < 0) {
		dev_err(bcmpmu->dev,
			"%s failed at gpio_request_one for input gpio chg.\n",
			__func__);
		goto err;
	}
	pchrgr->irq_chrgr_chg = gpio_to_irq(pchrgr->ext_chrgr_info->chg);
	ret = request_irq(pchrgr->irq_chrgr_chg, bcmpmu_chrgr_isr,
			IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
			IRQF_NO_SUSPEND,
			"bcmpmu-ext-chrgr-chg", pchrgr);
	if (ret) {
		pr_chrgr(ERROR, "%s, failed request irq_chrgr_chg.\n",
			__func__);
		goto err;
	}
#endif
	/* cache some data from accy */
	pchrgr->usbtype = bcmpmu->usb_accy_data.usb_type;

	/* determine initial state of USB wall charger. Note that we use
	 * charger type value determined by accy driver for this */
	if ((bcmpmu->usb_accy_data.chrgr_type > PMU_CHRGR_TYPE_NONE) &&
		(bcmpmu->usb_accy_data.chrgr_type < PMU_CHRGR_TYPE_MAX))
		pchrgr->usb_wallcharger_connected = 1;
	else
		pchrgr->usb_wallcharger_connected = 0;

	/* Update AC/DC connection status */
	ac_status_update(pchrgr, true);

	/* cache fault and charge status GPIO values */
#ifndef CONFIG_CAPRI_11351
	pchrgr->charge_state = gpio_get_value(pchrgr->ext_chrgr_info->chg);
#endif
	pchrgr->fault_state = gpio_get_value(pchrgr->ext_chrgr_info->flt);

	pr_chrgr(FLOW, "%s, determined initial values usbtype: %d "
		"ac_connected: %d charge state: %d fault state: %d\n",
		__func__, pchrgr->usbtype, pchrgr->ac_connected,
		pchrgr->charge_state, pchrgr->fault_state);

#ifdef CONFIG_MFD_BCMPMU_DBG
	ret = sysfs_create_group(&pdev->dev.kobj, &bcmpmu_chrgr_attr_group);
#endif

	pr_chrgr(INIT, "%s, finished successfully\n", __func__);
	return 0;

err:
	gpio_free(pchrgr->ext_chrgr_info->ac5v);
#ifndef CONFIG_CAPRI_11351
	gpio_free(pchrgr->ext_chrgr_info->chg);
#endif
	gpio_free(pchrgr->ext_chrgr_info->flt);
	gpio_free(pchrgr->ext_chrgr_info->iusb);
	gpio_free(pchrgr->ext_chrgr_info->cen);
	power_supply_unregister(&pchrgr->usb);
	power_supply_unregister(&pchrgr->ac);
	wake_lock_destroy(&pchrgr->wake_lock);
	kfree(pchrgr);
	return ret;
}

static int __devexit bcmpmu_chrgr_remove(struct platform_device *pdev)
{
	struct bcmpmu *bcmpmu = pdev->dev.platform_data;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;

	gpio_free(pchrgr->ext_chrgr_info->ac5v);
#ifndef CONFIG_CAPRI_11351
	gpio_free(pchrgr->ext_chrgr_info->chg);
#endif
	gpio_free(pchrgr->ext_chrgr_info->flt);
	gpio_free(pchrgr->ext_chrgr_info->iusb);
	gpio_free(pchrgr->ext_chrgr_info->cen);
	power_supply_unregister(&pchrgr->usb);
	power_supply_unregister(&pchrgr->ac);
	wake_lock_destroy(&pchrgr->wake_lock);
	kfree(pchrgr);

#ifdef CONFIG_MFD_BCMPMU_DBG
	sysfs_remove_group(&pdev->dev.kobj, &bcmpmu_chrgr_attr_group);
#endif
	return 0;
}

#ifdef CONFIG_PM
static int
bcmpmu_chrgr_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct bcmpmu *bcmpmu = pdev->dev.platform_data;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;

	if (gpio_get_value(pchrgr->ext_chrgr_info->cen)) {
		/* set CEN GPIO low to keep deep-sleep current low */
		pr_chrgr(FLOW,
			"%s(): Enabling charger (minimizes sleep current).\n",
			__func__);
		gpio_set_value(pchrgr->ext_chrgr_info->cen, 0);
	}

	return 0;
}

static int bcmpmu_chrgr_resume(struct platform_device *pdev)
{
	struct bcmpmu *bcmpmu = pdev->dev.platform_data;
	struct bcmpmu_chrgr *pchrgr = bcmpmu->chrgrinfo;

	charging_enable_update(pchrgr);

	return 0;
}
#endif

static struct platform_driver bcmpmu_chrgr_driver = {
	.driver = {
		.name = "bcmpmu_chrgr",
	},
	.probe = bcmpmu_chrgr_probe,
	.remove = __devexit_p(bcmpmu_chrgr_remove),
#ifdef CONFIG_PM
	.suspend = bcmpmu_chrgr_suspend,
	.resume = bcmpmu_chrgr_resume,
#endif
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
