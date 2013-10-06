/*
 * driver/misc/tsu6721.c - TSU6721 micro USB switch device driver
 *
 * Copyright (C) 2013 Samsung Electronics
 * Jeongrae Kim <jryu.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c/tsu6721.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/input.h>
#include <linux/switch.h>


#define INT_MASK1					0x5C
#define INT_MASK2					0xA0

/* DEVICE ID */
#define TSU6721_DEV_ID				0x0A
#define TSU6721_DEV_ID_REV			0x12

/* TSU6721 I2C registers */
#define REG_DEVICE_ID				0x01
#define REG_CONTROL					0x02
#define REG_INT1					0x03
#define REG_INT2					0x04
#define REG_INT_MASK1				0x05
#define REG_INT_MASK2				0x06
#define REG_ADC						0x07
#define REG_TIMING_SET1			0x08
#define REG_TIMING_SET2			0x09
#define REG_DEVICE_TYPE1			0x0a
#define REG_DEVICE_TYPE2			0x0b
#define REG_BUTTON1				0x0c
#define REG_BUTTON2				0x0d
#define REG_MANUAL_SW1			0x13
#define REG_MANUAL_SW2			0x14
#define REG_DEVICE_TYPE3			0x15
#define REG_RESET					0x1B
#define REG_TIMER_SET				0x20
#define REG_OCL_OCP_SET1			0x21
#define REG_OCL_OCP_SET2			0x22
#define REG_DEVICE_TYPE4			0x23

#define DATA_NONE					0x00

/* Control */
#define CON_SWITCH_OPEN	(1 << 4)
#define CON_RAW_DATA		(1 << 3)
#define CON_MANUAL_SW		(1 << 2)
#define CON_WAIT			(1 << 1)
#define CON_INT_MASK		(1 << 0)
#define CON_MASK			(CON_SWITCH_OPEN | CON_RAW_DATA |CON_MANUAL_SW | CON_WAIT)

/* Device Type 1 */
#define DEV_USB_OTG			(1 << 7)
#define DEV_DEDICATED_CHG		(1 << 6)
#define DEV_USB_CHG			(1 << 5)
#define DEV_CAR_KIT				(1 << 4)
#define DEV_UART				(1 << 3)
#define DEV_USB					(1 << 2)
#define DEV_AUDIO_2				(1 << 1)
#define DEV_AUDIO_1				(1 << 0)

#define DEV_T1_USB_MASK		(DEV_USB_OTG | DEV_USB_CHG | DEV_USB)
#define DEV_T1_UART_MASK		(DEV_UART)
#define DEV_T1_CHARGER_MASK	(DEV_DEDICATED_CHG | DEV_CAR_KIT)

/* Device Type 2 */
#define DEV_AUDIO_DOCK		(1 << 8)
#define DEV_SMARTDOCK		(1 << 7)
#define DEV_AV				(1 << 6)
#define DEV_TTY				(1 << 5)
#define DEV_PPD				(1 << 4)
#define DEV_JIG_UART_OFF	(1 << 3)
#define DEV_JIG_UART_ON	(1 << 2)
#define DEV_JIG_USB_OFF	(1 << 1)
#define DEV_JIG_USB_ON		(1 << 0)

#define DEV_T2_USB_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON)
#define DEV_T2_UART_MASK		(DEV_JIG_UART_OFF)
#if !defined(CONFIG_MACH_CAPRI_SS_CRATER)
#define DEV_T2_JIG_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON |DEV_JIG_UART_OFF)
#else
#define DEV_T2_JIG_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON |DEV_JIG_UART_OFF| DEV_JIG_UART_ON)
#endif
#define DEV_T2_JIG_ALL_MASK	(DEV_JIG_USB_OFF | DEV_JIG_USB_ON |DEV_JIG_UART_OFF | DEV_JIG_UART_ON)

/* Device Type 3 */
#define DEV_MHL				(1 << 0)
#define DEV_VBUS_DEBOUNCE	(1 << 1)
#define DEV_NON_STANDARD	(1 << 2)
#define DEV_AV_VBUS			(1 << 4)
#define DEV_APPLE_CHARGER	(1 << 5)
#define DEV_U200_CHARGER	(1 << 6)

#define DEV_T3_CHARGER_MASK	(DEV_NON_STANDARD | DEV_APPLE_CHARGER | DEV_U200_CHARGER)

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 */
#define SW_VAUDIO		((4 << 5) | (4 << 2) | (1 << 1) | (1 << 0))
#define SW_UART			((3 << 5) | (3 << 2))
#define SW_AUDIO		((2 << 5) | (2 << 2) | (1 << 0))
#define SW_DHOST		((1 << 5) | (1 << 2) | (1 << 0))
#define SW_AUTO			((0 << 5) | (0 << 2))
#define SW_USB_OPEN	(1 << 0)
#define SW_ALL_OPEN	(0)

/* Interrupt 1 */
#define INT_OXP_DISABLE			(1 << 7)
#define INT_OCP_ENABLE			(1 << 6)
#define INT_OVP_ENABLE			(1 << 5)
#define INT_LONG_KEY_RELEASE	(1 << 4)
#define INT_LONG_KEY_PRESS		(1 << 3)
#define INT_KEY_PRESS			(1 << 2)
#define INT_DETACH				(1 << 1)
#define INT_ATTACH				(1 << 0)

/* Interrupt 2 */
#define INT_VBUS				(1 << 7)
#define INT_OTP_ENABLE			(1 << 6)
#define INT_CONNECT				(1 << 5)
#define INT_STUCK_KEY_RCV		(1 << 4)
#define INT_STUCK_KEY			(1 << 3)
#define INT_ADC_CHANGE			(1 << 2)
#define INT_RESERVED_ATTACH	(1 << 1)
#define INT_AV_CHANGE			(1 << 0)

/* ADC VALUE */
#define	ADC_OTG				0x00
#define	ADC_MHL				0x01
#define 	ADC_SMART_DOCK		0x10
#define 	ADC_AUDIO_DOCK		0x12
#define	ADC_JIG_USB_OFF		0x18
#define	ADC_JIG_USB_ON		0x19
#define	ADC_DESKDOCK			0x1a
#define	ADC_JIG_UART_OFF		0x1c
#define	ADC_JIG_UART_ON		0x1d
#define	ADC_CARDOCK			0x1d
#define	ADC_OPEN				0x1f

int uart_connecting;
EXPORT_SYMBOL(uart_connecting);
int detached_status;
EXPORT_SYMBOL(detached_status);
static int jig_state;

struct tsu6721_usbsw {
	struct i2c_client		*client;
	struct tsu6721_platform_data	*pdata;
	int				dev1;
	int				dev2;
	int				dev3;
	int				mansw;
	int				dock_attached;
	int				dev_id;

	struct delayed_work	init_work;
	struct delayed_work	detect_work;
	struct mutex		mutex;
	int				adc;
};

static struct tsu6721_usbsw *local_usbsw;

static struct switch_dev switch_dock = {
	.name = "dock",
};

static int tsu6721_dock_init(void)
{
	int ret;

	ret = switch_dev_register(&switch_dock);
	if (ret < 0) {
		pr_err("Failed to register dock switch. %d\n", ret);
		return ret;
	}
	return 0;
}

static void tsu6721_disable_interrupt(void)
{
	struct i2c_client *client = local_usbsw->client;
	int value, ret;

	dev_info(&local_usbsw->client->dev, "%s\n", __func__);

	value = i2c_smbus_read_byte_data(client, REG_CONTROL);
	value |= CON_INT_MASK;

	ret = i2c_smbus_write_byte_data(client, REG_CONTROL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

}

static void tsu6721_enable_interrupt(void)
{
	struct i2c_client *client = local_usbsw->client;
	int value, ret;

	dev_info(&local_usbsw->client->dev, "%s\n", __func__);

	value = i2c_smbus_read_byte_data(client, REG_CONTROL);
	value &= (~CON_INT_MASK);

	ret = i2c_smbus_write_byte_data(client, REG_CONTROL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

}

static void tsu6721_dock_control(struct tsu6721_usbsw *usbsw,
	int state, int path)
{
	struct i2c_client *client = usbsw->client;
	int ret;

	if (state) {
		usbsw->mansw = path;

		ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1, path);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_CONTROL);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_write_byte_data(client,
				REG_CONTROL, ret & ~CON_MANUAL_SW);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
	} else {

		ret = i2c_smbus_read_byte_data(client, REG_CONTROL);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_CONTROL,
			ret | CON_MANUAL_SW | CON_RAW_DATA);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
	}

	switch_set_state(&switch_dock, state);
	
}

static void tsu6721_reg_init(struct tsu6721_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	unsigned int ctrl = CON_MASK;
	int ret;

	pr_info("tsu6721_reg_init is called\n");

	usbsw->dev_id = i2c_smbus_read_byte_data(client, REG_DEVICE_ID);
	local_usbsw->dev_id = usbsw->dev_id;
	if (usbsw->dev_id < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, usbsw->dev_id);

	dev_info(&client->dev, " tsu6721_reg_init dev ID: 0x%x\n",
			usbsw->dev_id);

	ret = i2c_smbus_write_byte_data(client, REG_INT_MASK1, INT_MASK1);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = i2c_smbus_write_byte_data(client,	REG_INT_MASK2, INT_MASK2);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	usbsw->mansw = i2c_smbus_read_byte_data(client, REG_MANUAL_SW1);
	if (usbsw->mansw < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, usbsw->mansw);

	if (usbsw->mansw)
		ctrl &= ~CON_MANUAL_SW;	/* Manual Switching Mode */
	else
		ctrl &= ~(CON_INT_MASK);

	ret = i2c_smbus_write_byte_data(client, REG_CONTROL, ctrl);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);
}

static ssize_t tsu6721_show_control(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct tsu6721_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, REG_CONTROL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return snprintf(buf, 13, "CONTROL: %02x\n", value);
}

static ssize_t tsu6721_show_device_type(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct tsu6721_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return snprintf(buf, 11, "DEVICE_TYPE: %02x\n", value);
}

static ssize_t tsu6721_show_manualsw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tsu6721_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, REG_MANUAL_SW1);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if (value == SW_VAUDIO)
		return snprintf(buf, 7, "VAUDIO\n");
	else if (value == SW_UART)
		return snprintf(buf, 5, "UART\n");
	else if (value == SW_AUDIO)
		return snprintf(buf, 6, "AUDIO\n");
	else if (value == SW_DHOST)
		return snprintf(buf, 6, "DHOST\n");
	else if (value == SW_AUTO)
		return snprintf(buf, 5, "AUTO\n");
	else
		return snprintf(buf, 4, "%x", value);
}

static ssize_t tsu6721_set_manualsw(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct tsu6721_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value, ret;
	unsigned int path = 0;

	value = i2c_smbus_read_byte_data(client, REG_CONTROL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if ((value & ~CON_MANUAL_SW) !=
			(CON_SWITCH_OPEN | CON_RAW_DATA | CON_WAIT))
		return 0;

	if (!strncmp(buf, "VAUDIO", 6)) {
		path = SW_VAUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "UART", 4)) {
		path = SW_UART;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "AUDIO", 5)) {
		path = SW_AUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "DHOST", 5)) {
		path = SW_DHOST;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "AUTO", 4)) {
		path = SW_AUTO;
		value |= CON_MANUAL_SW;
	} else {
		dev_err(dev, "Wrong command\n");
		return 0;
	}

	usbsw->mansw = path;

	ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1, path);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = i2c_smbus_write_byte_data(client, REG_CONTROL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return count;
}
static ssize_t tsu6721_show_usb_state(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct tsu6721_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int device_type1, device_type2;

	device_type1 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
	if (device_type1 < 0) {
		dev_err(&client->dev, "%s: err %d ", __func__, device_type1);
		return (ssize_t)device_type1;
	}
	device_type2 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE2);
	if (device_type2 < 0) {
		dev_err(&client->dev, "%s: err %d ", __func__, device_type2);
		return (ssize_t)device_type2;
	}

	if (device_type1 & DEV_T1_USB_MASK || device_type2 & DEV_T2_USB_MASK)
		return snprintf(buf, 22, "USB_STATE_CONFIGURED\n");

	return snprintf(buf, 25, "USB_STATE_NOTCONFIGURED\n");
}

static ssize_t tsu6721_show_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct tsu6721_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int adc;

	adc = i2c_smbus_read_byte_data(client, REG_ADC);
	if (adc < 0) {
		dev_err(&client->dev,
			"%s: err at read adc %d\n", __func__, adc);
		return snprintf(buf, 9, "UNKNOWN\n");
	}

	return snprintf(buf, 4, "%x\n", adc);
}

static ssize_t tsu6721_reset(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct tsu6721_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int ret;

	dev_info(&usbsw->client->dev, "%s\n", __func__);

	if (!strncmp(buf, "1", 1)) {
		dev_info(&client->dev,
			"tsu6721 reset after delay 1000 msec.\n");
		msleep(1000);
		ret = i2c_smbus_write_byte_data(client,
					REG_RESET, 0x01);
		if (ret < 0)
				dev_err(&client->dev,
					"cannot soft reset, err %d\n", ret);

	dev_info(&client->dev, "tsu6721_reset_control done!\n");
	} else {
		dev_info(&client->dev,
			"tsu6721_reset_control, but not reset_value!\n");
	}

	tsu6721_reg_init(usbsw);

	return count;
}


static DEVICE_ATTR(control, S_IRUGO, tsu6721_show_control, NULL);
static DEVICE_ATTR(device_type, S_IRUGO, tsu6721_show_device_type, NULL);
static DEVICE_ATTR(switch, S_IRUGO | S_IWUSR,
		tsu6721_show_manualsw, tsu6721_set_manualsw);
static DEVICE_ATTR(usb_state, S_IRUGO, tsu6721_show_usb_state, NULL);
static DEVICE_ATTR(adc, S_IRUGO, tsu6721_show_adc, NULL);
static DEVICE_ATTR(reset_switch, S_IWUSR | S_IWGRP, NULL, tsu6721_reset);

static struct attribute *tsu6721_attributes[] = {
	&dev_attr_control.attr,
	&dev_attr_device_type.attr,
	&dev_attr_switch.attr,
	NULL
};

static const struct attribute_group tsu6721_group = {
	.attrs = tsu6721_attributes,
};

void tsu6721_otg_detach(void)
{
	unsigned int data = 0;
	int ret;
	struct i2c_client *client = local_usbsw->client;

	if (local_usbsw->dev1 & DEV_USB_OTG) {
		dev_info(&client->dev, "%s: real device\n", __func__);
		data = 0x00;
		ret = i2c_smbus_write_byte_data(client,
						REG_MANUAL_SW2, data);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		data = SW_ALL_OPEN;
		ret = i2c_smbus_write_byte_data(client,
						REG_MANUAL_SW1, data);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);

		data = 0x1A;
		ret = i2c_smbus_write_byte_data(client,
						REG_CONTROL, data);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
	} else
		dev_info(&client->dev, "%s: not real device\n", __func__);
}
EXPORT_SYMBOL(tsu6721_otg_detach);
#if defined(CONFIG_VIDEO_MHL_V2)
int dock_det(void)
{
	return local_usbsw->dock_attached;
}
EXPORT_SYMBOL(dock_det);
#endif

int check_jig_state(void)
{
	return jig_state;
}
EXPORT_SYMBOL(check_jig_state);


static int tsu6721_attach_dev(struct tsu6721_usbsw *usbsw)
{
	int adc;
	int val1, val2, val3;
	struct tsu6721_platform_data *pdata = usbsw->pdata;
	struct i2c_client *client = usbsw->client;

	dev_info(&usbsw->client->dev, "%s\n", __func__);

	val1 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
	if (val1 < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, val1);
		return val1;
	}

	val2 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE2);
	if (val2 < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, val2);
		return val2;
	}
	jig_state =  (val2 & DEV_T2_JIG_ALL_MASK) ? 1 : 0;

	val3 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE3);
	if (val3 < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, val3);
		return val3;
	}
	adc = i2c_smbus_read_byte_data(client, REG_ADC);

	if (adc == ADC_SMART_DOCK) {
		val2 = DEV_SMARTDOCK;
		val1 = 0;
	} else if (adc == 0x11 || adc == ADC_AUDIO_DOCK) {
		val2 = DEV_AUDIO_DOCK;
		val1 = 0;
	}
	dev_err(&client->dev,
			"dev1: 0x%x, dev2: 0x%x, dev3: 0x%x, ADC: 0x%x Jig:%s\n",
			val1, val2, val3, adc,
			(check_jig_state() ? "ON" : "OFF"));

	/* USB */
	if (val1 & DEV_USB || val2 & DEV_T2_USB_MASK) {
		pr_info("[MUIC] USB Connected\n");
		if( val3 & DEV_VBUS_DEBOUNCE)
			if (pdata->usb_cb)
				pdata->usb_cb(TSU6721_ATTACHED);		
	/* USB_CDP */
	} else if (val1 & DEV_USB_CHG) {
		pr_info("[MUIC] CDP Connected\n");
		if( val3 & DEV_VBUS_DEBOUNCE)
			if (pdata->usb_cb)
				pdata->usb_cb(TSU6721_ATTACHED);	
	/* UART */
	} else if (val1 & DEV_T1_UART_MASK || val2 & DEV_T2_UART_MASK) {
		uart_connecting = 1;
		pr_info("[MUIC] UART Connected\n");
		if (pdata->uart_cb)
			pdata->uart_cb(TSU6721_ATTACHED);
	/* CHARGER */
	} else if ((val1 & DEV_T1_CHARGER_MASK) ||
			(val3 & DEV_T3_CHARGER_MASK)) {
		pr_info("[MUIC] Charger Connected\n");
		if( val3 & DEV_VBUS_DEBOUNCE)
			if (pdata->charger_cb)
				pdata->charger_cb(TSU6721_ATTACHED);
	/* for SAMSUNG OTG */
	} else if (val1 & DEV_USB_OTG) {
		pr_info("[MUIC] OTG Connected\n");

		i2c_smbus_write_byte_data(client, REG_MANUAL_SW1, 0x25);
		i2c_smbus_write_byte_data(client,
					REG_MANUAL_SW2, 0x02);
		msleep(50);
		i2c_smbus_write_byte_data(client,
					REG_CONTROL, 0x1a);
	/* JIG */
	} else if (val2 & DEV_T2_JIG_MASK) {
		pr_info("[MUIC] JIG Connected\n");
		if (pdata->jig_cb)
			pdata->jig_cb(TSU6721_ATTACHED);

	/* Desk Dock */
	} else if ((val2 & DEV_AV) || (val3 & DEV_AV_VBUS)) {
		pr_info("[MUIC] Deskdock Connected\n");
		local_usbsw->dock_attached = TSU6721_ATTACHED;

		if( val3 & DEV_VBUS_DEBOUNCE){
			if (pdata->charger_cb)
				pdata->charger_cb(TSU6721_ATTACHED);		
		}else if(usbsw->dev3 & DEV_VBUS_DEBOUNCE) {
			if (pdata->charger_cb)
				pdata->charger_cb(TSU6721_DETACHED);
		}
		tsu6721_dock_control(usbsw, TSU6721_ATTACHED, SW_AUDIO);	
#if !defined(CONFIG_MACH_CAPRI_SS_CRATER)
	/* MHL */
	} else if (val3 & DEV_MHL) {
#if defined(CONFIG_VIDEO_MHL_V2)
		pr_info("[MUIC] MHL Connected\n");
		tsu6721_disable_interrupt();
		if (!poweroff_charging)
			mhl_onoff_ex(1);
		else
			pr_info("LPM mode, skip MHL sequence\n");
		tsu6721_enable_interrupt();
#endif
	/* Car Dock */
	} else if (val2 & DEV_JIG_UART_ON) {
		pr_info("[MUIC] Cardock Connected\n");
		local_usbsw->dock_attached = TSU6721_ATTACHED;
		tsu6721_dock_control(usbsw, TSU6721_ATTACHED, SW_AUDIO);
	/* SmartDock */
	} else if (val2 & DEV_SMARTDOCK) {
		pr_info("[MUIC] Smartdock Connected\n");
		tsu6721_dock_control(usbsw, TSU6721_ATTACHED, SW_DHOST);
#if defined(CONFIG_VIDEO_MHL_V2)
		mhl_onoff_ex(1);
#endif
	/* Audio Dock */
	} else if (val2 & DEV_AUDIO_DOCK) {
		pr_info("[MUIC] Audiodock Connected\n");
		tsu6721_dock_control(usbsw, TSU6721_ATTACHED, SW_DHOST);
#endif			
	/* Incompatible */
	} else if (val3 & DEV_VBUS_DEBOUNCE) {
		pr_info("[MUIC] Incompatible Charger Connected\n");
		if (pdata->charger_cb)
			pdata->charger_cb(TSU6721_ATTACHED);		
	}
	usbsw->dev1 = val1;
	usbsw->dev2 = val2;
	usbsw->dev3 = val3;
	usbsw->adc = adc;

	return adc;
}

static int tsu6721_detach_dev(struct tsu6721_usbsw *usbsw)
{
	struct tsu6721_platform_data *pdata = usbsw->pdata;
	struct i2c_client *client = usbsw->client;

	dev_info(&usbsw->client->dev, "%s\n", __func__);

	/* USB */
	if (usbsw->dev1 & DEV_USB ||
			usbsw->dev2 & DEV_T2_USB_MASK) {
		pr_info("[MUIC] USB Disonnected\n");
		if (pdata->usb_cb)
			pdata->usb_cb(TSU6721_DETACHED);
	} else if (usbsw->dev1 & DEV_USB_CHG) {
		if (pdata->usb_cb)
			pdata->usb_cb(TSU6721_DETACHED);

	/* UART */
	} else if (usbsw->dev1 & DEV_T1_UART_MASK ||
			usbsw->dev2 & DEV_T2_UART_MASK) {
		pr_info("[MUIC] UART Disonnected\n");
		if (pdata->uart_cb)
			pdata->uart_cb(TSU6721_DETACHED);
		uart_connecting = 0;
	/* CHARGER */
	} else if ((usbsw->dev1 & DEV_T1_CHARGER_MASK) ||
			(usbsw->dev3 & DEV_T3_CHARGER_MASK)) {
		pr_info("[MUIC] Charger Disonnected\n");
		if (pdata->charger_cb)
			pdata->charger_cb(TSU6721_DETACHED);
	/* for SAMSUNG OTG */
	} else if (usbsw->dev1 & DEV_USB_OTG) {
		pr_info("[MUIC] OTG Disonnected\n");
		i2c_smbus_write_byte_data(client,REG_CONTROL, 0x1E);
	/* JIG */
	} else if (usbsw->dev2 & DEV_T2_JIG_MASK) {
		pr_info("[MUIC] JIG Disonnected\n");
		if (pdata->jig_cb)
			pdata->jig_cb(TSU6721_DETACHED);

	/* Desk Dock */
	} else if ((usbsw->dev2 & DEV_AV) ||(usbsw->dev3 & DEV_AV_VBUS)) {
		pr_info("[MUIC] Deskdock Disonnected\n");
		local_usbsw->dock_attached = TSU6721_DETACHED;

		if (usbsw->dev3 & DEV_VBUS_DEBOUNCE) {
			if (pdata->charger_cb)
				pdata->charger_cb(TSU6721_DETACHED);
		}	

		tsu6721_dock_control(usbsw, TSU6721_DETACHED, SW_ALL_OPEN);
#if !defined(CONFIG_MACH_CAPRI_SS_CRATER)		
	/* MHL */
	} else if (usbsw->dev3 & DEV_MHL) {
		pr_info("[MUIC] MHL Disonnected\n");
#if defined CONFIG_MHL_D3_SUPPORT
		mhl_onoff_ex(false);
		detached_status = 1;
#endif

	/* Car Dock */
	} else if (usbsw->dev2 & DEV_JIG_UART_ON) {
		pr_info("[MUIC] Cardock Disonnected\n");
		local_usbsw->dock_attached = TSU6721_DETACHED;
		tsu6721_dock_control(usbsw, TSU6721_DETACHED, SW_ALL_OPEN);
	/* Smart Dock */
	} else if (usbsw->dev2 == DEV_SMARTDOCK) {
		pr_info("[MUIC] Smartdock Disonnected\n");
		tsu6721_dock_control(usbsw, TSU6721_DETACHED, SW_ALL_OPEN);
#if defined(CONFIG_VIDEO_MHL_V2)
		mhl_onoff_ex(false);
#endif
	/* Audio Dock */
	} else if (usbsw->dev2 == DEV_AUDIO_DOCK) {
		pr_info("[MUIC] Audiodock Disonnected\n");
		tsu6721_dock_control(usbsw, TSU6721_DETACHED, SW_ALL_OPEN);
#endif	
	/* Incompatible */
	} else if (usbsw->dev3 & DEV_VBUS_DEBOUNCE) {
		pr_info("[MUIC] Incompatible Charger Disonnected\n");
		if (pdata->charger_cb)
			pdata->charger_cb(TSU6721_DETACHED);		
	}
	usbsw->dev1 = 0;
	usbsw->dev2 = 0;
	usbsw->dev3 = 0;
	usbsw->adc = 0;

	return 0;

}
static irqreturn_t tsu6721_irq_thread(int irq, void *data)
{
	struct tsu6721_usbsw *usbsw = data;
	
	schedule_delayed_work(&usbsw->detect_work,msecs_to_jiffies(0));

	return IRQ_HANDLED;
}

static void tsu6721_detect_work(struct work_struct *work)
{
	
	struct tsu6721_usbsw *usbsw = container_of(work,
			struct tsu6721_usbsw, detect_work.work);;
	struct i2c_client *client = usbsw->client;
	int intr1, intr2;
	int val1, val3, adc;
	/* TSU6721 : Read interrupt -> Read Device */
	pr_info("tsu6721_irq_thread is called\n");

	tsu6721_disable_interrupt();
	intr1 = i2c_smbus_read_byte_data(client, REG_INT1);
	intr2 = i2c_smbus_read_byte_data(client, REG_INT2);
	dev_info(&client->dev, "%s: intr : 0x%x intr2 : 0x%x\n",
		__func__, intr1, intr2);

	/* MUIC OVP Check */
	if (intr1 & INT_OVP_ENABLE){
		pr_info("tsu6721_irq_thread ovp enabled\n");
		//usbsw->pdata->ovp_cb(ENABLE);
	}
	else if (intr1 & INT_OXP_DISABLE){
		pr_info("tsu6721_irq_thread ovp disabled\n");
		//usbsw->pdata->ovp_cb(DISABLE);
	}

	/* device detection */
	mutex_lock(&usbsw->mutex);

	/* interrupt both attach and detach */
	if (intr1 == (INT_ATTACH + INT_DETACH)) {
		val1 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
		val3 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE3);
		adc = i2c_smbus_read_byte_data(client, REG_ADC);
		if ((adc == ADC_OPEN) && (val1 == DATA_NONE) &&
				((val3 == DATA_NONE) ||
				 (val3 == DEV_VBUS_DEBOUNCE)))
			tsu6721_detach_dev(usbsw);
		else
			tsu6721_attach_dev(usbsw);
	/* interrupt attach */
	} else if (intr1 & INT_ATTACH || intr2 &
			(INT_AV_CHANGE | INT_RESERVED_ATTACH))
		tsu6721_attach_dev(usbsw);
	/* interrupt detach */
	else if (intr1 & INT_DETACH)
		tsu6721_detach_dev(usbsw);
	else if( intr1==0 && intr2==0){
		dev_info(&client->dev, "tsu6721 check interrupt\n");

		val3 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE3);
		dev_info(&client->dev, "%s: val3 : 0x%x\n",__func__, val3);

		if(val3 & DEV_VBUS_DEBOUNCE)
			tsu6721_attach_dev(usbsw);
		else
			tsu6721_detach_dev(usbsw);
	}
	mutex_unlock(&usbsw->mutex);

	tsu6721_enable_interrupt();

}

static int tsu6721_irq_init(struct tsu6721_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int ret;

	dev_info(&usbsw->client->dev, "%s\n", __func__);

	if (client->irq) {
		ret = request_threaded_irq(client->irq, NULL,
			tsu6721_irq_thread, IRQF_TRIGGER_FALLING|IRQF_NO_SUSPEND,
			"tsu6721 micro USB", usbsw);
		if (ret) {
			dev_err(&client->dev, "failed to reqeust IRQ\n");
			return ret;
		}

		ret = enable_irq_wake(client->irq);
		if (ret < 0)
			dev_err(&client->dev,
				"failed to enable wakeup src %d\n", ret);
	}

	return 0;
}

static void tsu6721_init_detect(struct work_struct *work)
{
	struct tsu6721_usbsw *usbsw = container_of(work,
			struct tsu6721_usbsw, init_work.work);
	int ret = 0;

	dev_info(&usbsw->client->dev, "%s\n", __func__);

	mutex_lock(&usbsw->mutex);
	tsu6721_attach_dev(usbsw);
	mutex_unlock(&usbsw->mutex);

	ret = tsu6721_irq_init(usbsw);
	if (ret)
		dev_info(&usbsw->client->dev,
				"failed to enable  irq init %s\n", __func__);

	ret = i2c_smbus_read_byte_data(usbsw->client, REG_INT1);
	dev_info(&usbsw->client->dev, "%s: intr1 : 0x%x\n",
		__func__, ret);

	ret = i2c_smbus_read_byte_data(usbsw->client, REG_INT2);
	dev_info(&usbsw->client->dev, "%s: intr2 : 0x%x\n",
		__func__, ret);
}

static int __devinit tsu6721_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct tsu6721_usbsw *usbsw;
	int ret = 0;
	struct device *switch_dev;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	usbsw = kzalloc(sizeof(struct tsu6721_usbsw), GFP_KERNEL);
	if (!usbsw) {
		dev_err(&client->dev, "failed to allocate driver data\n");
		kfree(usbsw);
		return -ENOMEM;
	}

	usbsw->client = client;
	usbsw->pdata = client->dev.platform_data;
	if (!usbsw->pdata)
		goto fail1;

	i2c_set_clientdata(client, usbsw);

	mutex_init(&usbsw->mutex);

	local_usbsw = usbsw;

	tsu6721_reg_init(usbsw);

	ret = sysfs_create_group(&client->dev.kobj, &tsu6721_group);
	if (ret) {
		dev_err(&client->dev,
				"failed to create tsu6721 attribute group\n");
		goto fail2;
	}

	/* make sysfs node /sys/class/sec/switch/usb_state */
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_dev)) {
		pr_err("[TSU6721] Failed to create device (switch_dev)!\n");
		ret = PTR_ERR(switch_dev);
		goto fail2;
	}

	ret = device_create_file(switch_dev, &dev_attr_usb_state);
	if (ret < 0) {
		pr_err("[TSU6721] Failed to create file (usb_state)!\n");
		goto err_create_file_state;
	}

	ret = device_create_file(switch_dev, &dev_attr_adc);
	if (ret < 0) {
		pr_err("[TSU6721] Failed to create file (adc)!\n");
		goto err_create_file_adc;
	}

	ret = device_create_file(switch_dev, &dev_attr_reset_switch);
	if (ret < 0) {
		pr_err("[TSU6721] Failed to create file (reset_switch)!\n");
		goto err_create_file_reset_switch;
	}

	dev_set_drvdata(switch_dev, usbsw);

	tsu6721_dock_init();

	/* initial cable detection */
	INIT_DELAYED_WORK(&usbsw->init_work, tsu6721_init_detect);
	INIT_DELAYED_WORK(&usbsw->detect_work, tsu6721_detect_work);
	schedule_delayed_work(&usbsw->init_work, msecs_to_jiffies(2700));

	return 0;

err_create_file_reset_switch:
	device_remove_file(switch_dev, &dev_attr_reset_switch);
err_create_file_adc:
	device_remove_file(switch_dev, &dev_attr_adc);
err_create_file_state:
	device_remove_file(switch_dev, &dev_attr_usb_state);
fail2:
	if (client->irq)
		free_irq(client->irq, usbsw);
fail1:
	mutex_destroy(&usbsw->mutex);
	i2c_set_clientdata(client, NULL);
	kfree(usbsw);
	return ret;
}

static int __devexit tsu6721_remove(struct i2c_client *client)
{
	struct tsu6721_usbsw *usbsw = i2c_get_clientdata(client);
	cancel_delayed_work(&usbsw->init_work);
	cancel_delayed_work(&usbsw->detect_work);

	if (client->irq) {
		disable_irq_wake(client->irq);
		free_irq(client->irq, usbsw);
	}
	mutex_destroy(&usbsw->mutex);
	i2c_set_clientdata(client, NULL);

	sysfs_remove_group(&client->dev.kobj, &tsu6721_group);
	kfree(usbsw);
	return 0;
}
static const struct i2c_device_id tsu6721_id[] = {
	{"tsu6721", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, tsu6721_id);

static struct i2c_driver tsu6721_i2c_driver = {
	.driver = {
		.name = "tsu6721",
	},
	.probe = tsu6721_probe,
	.remove = __devexit_p(tsu6721_remove),
	.id_table = tsu6721_id,
};

static int __init tsu6721_init(void)
{
	return i2c_add_driver(&tsu6721_i2c_driver);
}
module_init(tsu6721_init);

static void __exit tsu6721_exit(void)
{
	i2c_del_driver(&tsu6721_i2c_driver);
}
module_exit(tsu6721_exit);

MODULE_AUTHOR("Jeongrae Kim <Jryu.kim@samsung.com>");
MODULE_DESCRIPTION("TSU6721 Micro USB Switch driver");
MODULE_LICENSE("GPL");

