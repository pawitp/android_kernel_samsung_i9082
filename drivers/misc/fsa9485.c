/*
 * driver/misc/fsa9480.c - FSA9480 micro USB switch device driver
 *
 * Copyright (C) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Wonguk Jeong <wonguk.jeong@samsung.com>
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
#include <linux/i2c/fsa9485.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/input.h>

#if defined(CONFIG_SEC_DUAL_MODEM)
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#endif

#define LOCAL_BUG_ON			*(int *)0=0
/* FSA9480 I2C registers */
#define FSA9485_REG_DEVID		0x01
#define FSA9485_REG_CTRL		0x02
#define FSA9485_REG_INT1		0x03
#define FSA9485_REG_INT2		0x04
#define FSA9485_REG_INT1_MASK		0x05
#define FSA9485_REG_INT2_MASK		0x06
#define FSA9485_REG_ADC			0x07
#define FSA9485_REG_TIMING1		0x08
#define FSA9485_REG_TIMING2		0x09
#define FSA9485_REG_DEV_T1		0x0a
#define FSA9485_REG_DEV_T2		0x0b
#define FSA9485_REG_BTN1		0x0c
#define FSA9485_REG_BTN2		0x0d
#define FSA9485_REG_CK			0x0e
#define FSA9485_REG_CK_INT1		0x0f
#define FSA9485_REG_CK_INT2		0x10
#define FSA9485_REG_CK_INTMASK1		0x11
#define FSA9485_REG_CK_INTMASK2		0x12
#define FSA9485_REG_MANSW1		0x13
#define FSA9485_REG_MANSW2		0x14
#define FSA9485_REG_MANUAL_OVERRIDES1	0x1B
#define FSA9485_REG_RESERVED_1D		0x1D
#define FSA9485_REG_RESERVED_20		0x20


/* Control */
#define CON_SWITCH_OPEN		(1 << 4)
#define CON_RAW_DATA		(1 << 3)
#define CON_MANUAL_SW		(1 << 2)
#define CON_WAIT		(1 << 1)
#define CON_INT_MASK		(1 << 0)
#define CON_MASK		(CON_SWITCH_OPEN | CON_RAW_DATA | \
				CON_MANUAL_SW | CON_WAIT)

/* Device Type 1 */
#define DEV_USB_OTG		(1 << 7)
#define DEV_DEDICATED_CHG	(1 << 6)
#define DEV_USB_CHG		(1 << 5)
#define DEV_CAR_KIT		(1 << 4)
#define DEV_UART		(1 << 3)
#define DEV_USB			(1 << 2)
#define DEV_AUDIO_2		(1 << 1)
#define DEV_AUDIO_1		(1 << 0)

#define DEV_T1_USB_MASK		(DEV_USB_OTG | DEV_USB_CHG | DEV_USB)
#define DEV_T1_UART_MASK	(DEV_UART)
#define DEV_T1_CHARGER_MASK	(DEV_DEDICATED_CHG | DEV_CAR_KIT)

/* Device Type 2 */
#if defined(CONFIG_USB_OTG_AUDIODOCK)
#define DEV_AUDIODOCK	(1 << 8)
#endif
#define DEV_SMARTDOCK	(1 << 7)
#define DEV_AV			(1 << 6)
#define DEV_TTY			(1 << 5)
#define DEV_PPD			(1 << 4)
#define DEV_JIG_UART_OFF	(1 << 3)
#define DEV_JIG_UART_ON		(1 << 2)
#define DEV_JIG_USB_OFF		(1 << 1)
#define DEV_JIG_USB_ON		(1 << 0)

#define DEV_T2_USB_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON)
#define DEV_T2_UART_MASK	(DEV_JIG_UART_OFF)
#define DEV_T2_JIG_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON | \
				DEV_JIG_UART_OFF)
#define VBUS_VALID		(1 << 1)		

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 */
#define SW_VAUDIO		((4 << 5) | (4 << 2) | (1 << 1) | (1 << 0))
#define SW_UART			((3 << 5) | (3 << 2))
#define SW_AUDIO		((2 << 5) | (2 << 2) | (1 << 1) | (1 << 0))
#define SW_DHOST		((1 << 5) | (1 << 2) | (1 << 1) | (1 << 0))
#define SW_AUTO			((0 << 5) | (0 << 2))
#define SW_USB_OPEN		(1 << 0)
#define SW_ALL_OPEN		(0)

/* Interrupt 1 */
#define INT_DETACH		(1 << 1)
#define INT_ATTACH		(1 << 0)

#define	ADC_GND			0x00
#define	ADC_MHL			0x01
#define	ADC_DOCK_PREV_KEY 0x04
#define	ADC_DOCK_NEXT_KEY 0x07
#define	ADC_DOCK_VOL_DN		0x0a
#define	ADC_DOCK_VOL_UP		0x0b
#define	ADC_DOCK_PLAY_PAUSE_KEY 0x0d
#define ADC_AUDIO_DOCK	0x12
#define	ADC_CEA936ATYPE1_CHG	0x17
#define	ADC_JIG_USB_OFF		0x18
#define	ADC_JIG_USB_ON		0x19
#define	ADC_DESKDOCK		0x1a
#define	ADC_CEA936ATYPE2_CHG	0x1b
#define	ADC_JIG_UART_OFF	0x1c
#define	ADC_JIG_UART_ON		0x1d
#define	ADC_CARDOCK		0x1d
#define ADC_OPEN		0x1f

int uart_connecting;
EXPORT_SYMBOL(uart_connecting);

int detached_status;
EXPORT_SYMBOL(detached_status);

struct fsa9485_usbsw {
	struct i2c_client		*client;
	struct fsa9485_platform_data	*pdata;
	struct work_struct              work;	
	struct delayed_work		vbus_work;
	int				dev1;
	int				dev2;
	int				mhl_removed;
	int				mansw;
	int				dock_attached;
	int				ovp;
	int				acc_status;

	struct input_dev	*input;
	int			previous_key;

	struct delayed_work	init_work;
	struct mutex		mutex;
	int				adc;
	int				deskdock;
	int				vbus;
	int				check_vbus;
	int				audio_dock;
	int				unnormal_TA;
	int				init_det_done;
};

#if defined(CONFIG_SEC_DUAL_MODEM)
struct fsa9485_wq {
	struct delayed_work work_q;
	struct fsa9485_usbsw* sdata;
	struct list_head entry;
};
#endif

enum {
	DOCK_KEY_NONE			= 0,
	DOCK_KEY_VOL_UP_PRESSED,
	DOCK_KEY_VOL_UP_RELEASED,
	DOCK_KEY_VOL_DOWN_PRESSED,
	DOCK_KEY_VOL_DOWN_RELEASED,
	DOCK_KEY_PREV_PRESSED,
	DOCK_KEY_PREV_RELEASED,
	DOCK_KEY_PLAY_PAUSE_PRESSED,
	DOCK_KEY_PLAY_PAUSE_RELEASED,
	DOCK_KEY_NEXT_PRESSED,
	DOCK_KEY_NEXT_RELEASED,

};

#if defined(CONFIG_SEC_DUAL_MODEM)
static void fsa9485_init_detect(struct work_struct *work);
#endif

static struct fsa9485_usbsw *local_usbsw = NULL;
#ifdef CONFIG_SAMSUNG_MHL
#define CONFIG_VIDEO_MHL_V2
#endif
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
#define MHL_DEVICE 2
static int isDeskdockconnected;
extern u8 mhl_onoff_ex(bool onoff);

#endif

#if defined(CONFIG_SEC_DUAL_MODEM)
static int curr_usb_path = 0;
static int curr_uart_path = 0;
#define SWITCH_CP		1
#define SWITCH_AP	0
#define SWITCH_ESC		2
#endif
static int fsa9485_write_reg(struct i2c_client *client,        u8 reg, u8 data)
{
       int ret = 0;
       u8 buf[2];
       struct i2c_msg msg[1];

       buf[0] = reg;
       buf[1] = data;

       msg[0].addr = client->addr;
       msg[0].flags = 0;
       msg[0].len = 2;
       msg[0].buf = buf;

       ret = i2c_transfer(client->adapter, msg, 1);
       if (ret != 1) {
               printk("\n [fsa9485] i2c Write Failed (ret=%d) \n", ret);
               return -1;
       }
       
       return ret;
}

static int fsa9485_read_reg(struct i2c_client *client, u8 reg, u8 *data)
{
       int ret = 0;
       u8 buf[1];
       struct i2c_msg msg[2];

       buf[0] = reg;

        msg[0].addr = client->addr;
        msg[0].flags = 0;
        msg[0].len = 1;
        msg[0].buf = buf;

        msg[1].addr = client->addr;
        msg[1].flags = I2C_M_RD;
        msg[1].len = 1;
        msg[1].buf = buf;
		
       ret = i2c_transfer(client->adapter, msg, 2);
       if (ret != 2) {
               printk("\n [fsa9485] i2c Read Failed (ret=%d) \n", ret);
               return -1;
       }
       *data = buf[0];

       return 0;
}

static int fsa9485_read_word_reg(struct i2c_client *client, u8 reg, int *data)
{
       int ret = 0;
       u8 buf[1];
	u8 data1,data2;   
       struct i2c_msg msg[2];

       buf[0] = reg;

        msg[0].addr = client->addr;
        msg[0].flags = 0;
        msg[0].len = 1;
        msg[0].buf = buf;

        msg[1].addr = client->addr;
        msg[1].flags = I2C_M_RD;
        msg[1].len = 1;
        msg[1].buf = buf;
		
       ret = i2c_transfer(client->adapter, msg, 2);
       if (ret != 2) {
               printk("\n [fsa9485] i2c Read Failed (ret=%d) \n", ret);
               return -1;
       }

	data1 = buf[0];

	  buf[0] = reg+1;

        msg[0].addr = client->addr;
        msg[0].flags = 0;
        msg[0].len = 1;
        msg[0].buf = buf;

        msg[1].addr = client->addr;
        msg[1].flags = I2C_M_RD;
        msg[1].len = 1;
        msg[1].buf = buf;
		
       ret = i2c_transfer(client->adapter, msg, 2);
       if (ret != 2) {
               printk("\n [fsa9485] i2c Read Failed (ret=%d) \n", ret);
               return -1;
       }

	data2 = buf[0];
	*data = (int)((data2<<8) | data1);

       return 0;
}

static void DisableFSA9480Interrupts(void)
{
	struct i2c_client *client = local_usbsw->client;
	int value, ret;

	fsa9485_read_reg(client, FSA9485_REG_CTRL,&value);
	value |= 0x01;

	ret = fsa9485_write_reg(client, FSA9485_REG_CTRL, value);
	
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

}

static void EnableFSA9480Interrupts(void)
{
	struct i2c_client *client = local_usbsw->client;
	int value, ret;

	fsa9485_read_reg(client, FSA9485_REG_CTRL,&value);
	value &= 0xFE;

	ret = fsa9485_write_reg(client, FSA9485_REG_CTRL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

}

void FSA9485_CheckAndHookAudioDock(int value)
{
	struct i2c_client *client = local_usbsw->client;
	struct fsa9485_platform_data *pdata = local_usbsw->pdata;
	int ret = 0;

	if (value) {
		pr_info("FSA9485_CheckAndHookAudioDock ON\n");
		if (pdata->dock_cb)
			pdata->dock_cb(FSA9485_ATTACHED_DESK_DOCK);

		ret = fsa9485_write_reg(client,
					FSA9485_REG_MANSW1, SW_AUDIO);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n",__func__, ret);

			fsa9485_read_reg(client,FSA9485_REG_CTRL,&ret);
			if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n",__func__, ret);

		ret = fsa9485_write_reg(client,FSA9485_REG_CTRL,
					ret & ~CON_MANUAL_SW & ~CON_RAW_DATA);
			if (ret < 0)
			dev_err(&client->dev,"%s: err %d\n", __func__, ret);
	} 
	else {
		dev_info(&client->dev,"FSA9485_CheckAndHookAudioDock Off\n");

		if (pdata->dock_cb)
			pdata->dock_cb(FSA9485_DETACHED_DOCK);

			fsa9485_read_reg(client,FSA9485_REG_CTRL,&ret);
			
			if (ret < 0)
			dev_err(&client->dev,"%s: err %d\n", __func__, ret);

		ret = fsa9485_write_reg(client,FSA9485_REG_CTRL,
					ret | CON_MANUAL_SW | CON_RAW_DATA);
			if (ret < 0)
			dev_err(&client->dev,"%s: err %d\n", __func__, ret);
	}
}

static void fsa9485_reg_init(struct fsa9485_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	unsigned int ctrl = CON_MASK;
	int ret;
	
	fsa9485_write_reg(client,FSA9485_REG_INT1_MASK,0x5c);
	fsa9485_write_reg(client,FSA9485_REG_INT2_MASK,0x18);
	
	fsa9485_write_reg(client,FSA9485_REG_CK_INTMASK1,0xff);
	fsa9485_write_reg(client,FSA9485_REG_CK_INTMASK2,0x07);


	/* ADC Detect Time: 500ms */
	ret = fsa9485_write_reg(client, FSA9485_REG_TIMING1, 0x0);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	fsa9485_read_reg(client,FSA9485_REG_MANSW1,&ret);
	usbsw->mansw = ret;
	
	if (usbsw->mansw < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, usbsw->mansw);

	if (usbsw->mansw)
		ctrl &= ~CON_MANUAL_SW;	/* Manual Switching Mode */
	else
		ctrl &= ~(CON_INT_MASK);

	ret = fsa9485_write_reg(client, FSA9485_REG_CTRL, ctrl);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = fsa9485_write_reg(client, FSA9485_REG_RESERVED_20, 0x04);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	fsa9485_read_reg(client,FSA9485_REG_DEVID,&ret);
	
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	dev_info(&client->dev, " fsa9485_reg_init dev ID: 0x%x\n", ret);
}

#if defined(CONFIG_SEC_DUAL_MODEM)
void fsa9485_set_switch(const char *buf)
{
	struct fsa9485_usbsw *usbsw = local_usbsw;
	struct i2c_client *client = usbsw->client;
	unsigned int path;
	u8 value;
	int ret;

	printk("%s , buf = %s\n" , __func__ , buf);
	fsa9485_read_reg(client, FSA9485_REG_CTRL, &value);
	dev_info(&client->dev, "FSA9485_REG_CTRL: 0x%x\n", value);

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
		dev_err(&client->dev, "Wrong command\n");
		return;
	}

	usbsw->mansw = path;
	ret = fsa9485_write_reg(client, FSA9485_REG_MANSW1, path);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = fsa9485_write_reg(client, FSA9485_REG_CTRL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	/* initial cable detection */
	INIT_DELAYED_WORK(&usbsw->init_work, fsa9485_init_detect);
	schedule_delayed_work(&usbsw->init_work, msecs_to_jiffies(700));
}
#endif

static ssize_t fsa9485_show_control(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	fsa9485_read_reg(client,FSA9485_REG_CTRL,&value);

	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return snprintf(buf, 13, "CONTROL: %02x\n", value);
}

static ssize_t fsa9485_show_device_type(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	fsa9485_read_reg(client,FSA9485_REG_DEV_T1,&value);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return snprintf(buf, 11, "DEVICE_TYPE: %02x\n", value);
}

static ssize_t fsa9485_show_manualsw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	unsigned int value;

	fsa9485_read_reg(client,FSA9485_REG_MANSW1,&value);
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

static ssize_t fsa9485_set_manualsw(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	unsigned int value;
	unsigned int path = 0;
	int ret;

	fsa9485_read_reg(client,FSA9485_REG_CTRL,&value);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if ((value & ~CON_MANUAL_SW) != (CON_SWITCH_OPEN | CON_RAW_DATA | CON_WAIT))
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

	ret = fsa9485_write_reg(client, FSA9485_REG_MANSW1, path);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = fsa9485_write_reg(client, FSA9485_REG_CTRL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return count;
}

static ssize_t fsa9480_show_usb_state(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int device_type;
	unsigned char device_type1, device_type2;

	fsa9485_read_reg(client,FSA9485_REG_DEV_T1,&device_type1);
	fsa9485_read_reg(client,FSA9485_REG_DEV_T2,&device_type2);
	
	if (device_type1 & DEV_T1_USB_MASK || device_type2 & DEV_T2_USB_MASK)
		return snprintf(buf, 22, "USB_STATE_CONFIGURED\n");

	return snprintf(buf, 25, "USB_STATE_NOTCONFIGURED\n");
}

static ssize_t fsa9485_show_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	char adc;

	fsa9485_read_reg(client, FSA9485_REG_ADC,&adc);
	if (adc < 0) {
		dev_err(&client->dev,"%s: err at read adc %d\n", __func__, adc);
		return snprintf(buf, 9, "UNKNOWN\n");
	}

	return sprintf(buf,"%x\n", adc);
}

static ssize_t fsa9485_reset(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int ret;

	if (!strncmp(buf, "1", 1)) {
		dev_info(&client->dev, "fsa9480 reset after delay 1000 msec.\n");
		mdelay(1000);
		ret = fsa9485_write_reg(client,FSA9485_REG_MANUAL_OVERRIDES1, 0x01);
		if (ret < 0)
			dev_err(&client->dev,"cannot soft reset, err %d\n", ret);

		dev_info(&client->dev, "fsa9480_reset_control done!\n");
	} 
	else {
		dev_info(&client->dev,"fsa9480_reset_control, but not reset_value!\n");
	}

	fsa9485_reg_init(usbsw);

	return count;
}

#if defined(CONFIG_SEC_DUAL_MODEM)
static void fsa9485_uart_reset(void)
{
	struct i2c_client *client = local_usbsw->client;
	int ret=0;
	mdelay(1000);
	ret = fsa9485_write_reg(client,FSA9485_REG_MANUAL_OVERRIDES1, 0x01);
	if(ret<0)
	   printk("usb_switch_reset!\n");
	fsa9485_reg_init(local_usbsw);
	printk("usb_switch_reset!\n");
}
#endif



#if defined(CONFIG_SEC_DUAL_MODEM)
static ssize_t fsa9485_uart_sel_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int fd;
	char buffer[2]={0};
	int ret;

	printk(KERN_ERR "%s\n", __func__);

	mm_segment_t fs = get_fs();
	set_fs(get_ds());

	if ((fd = sys_open("/data/path/uart_sel.bin", O_RDONLY,0)) < 0) {
		printk("[FSA9480]: %s :: open failed %s ,fd=0x%x\n", __func__, "/data/path/uart_sel.bin", fd);
		return 0;
	}

	ret = sys_read(fd, buffer, 1);
	if(ret < 0) {
		printk("uart_switch_show READ FAIL!\n");
		return 0;
	}
	
	sys_close(fd);
	set_fs(fs);

	printk("curr_uart_path = %d \n", curr_uart_path);
	printk("uart_switch_show uart_sel.bin = %s \n", buffer);

	if (!strncmp(buffer, "0", 1)) {
	  return sprintf(buf, "AP");
	} else if(!strncmp(buffer, "1", 1)) {
	  return sprintf(buf, "CP");
	}  else {
	  return sprintf(buf, "AP");
	}
}
// 1 : SPRD
// 0 : BCOM
#define GPIO_UART_SEL 159
#define GPIO_CP2_USB_ON 161
static void fsa9485_uart_sel_switch_en(bool en)
{
	printk("%s : %d \n",__func__, en);
	gpio_direction_output(GPIO_UART_SEL, en);
	mdelay(10);
	printk("%s read GPIO 159: value = %d \n", __func__,gpio_get_value(GPIO_UART_SEL));	
}
static void fsa9485_cp2_usb_on_en(bool en)
{
	printk("%s : %d \n",__func__, en);
	gpio_direction_output(GPIO_CP2_USB_ON, en);
	mdelay(10);
	printk("%s read GPIO 161: value = %d \n", __func__,gpio_get_value(GPIO_CP2_USB_ON));	
}

#if defined(CONFIG_SEC_DUAL_MODEM)
#define GPIO_AP_CP_INT1 164
#define GPIO_PDA_ACTIVE 8
static void fsa9485_ap_cp_int1_en(bool en)
{
	printk("%s : %d \n",__func__, en);
	gpio_direction_output(GPIO_AP_CP_INT1, en);
	mdelay(10);
	printk("%s read GPIO 164: value = %d \n", __func__,gpio_get_value(GPIO_AP_CP_INT1));	
}

static void fsa9485_pda_active_en(bool en)
{	
	printk("%s : %d \n",__func__, en);
	gpio_direction_output(GPIO_PDA_ACTIVE, en);
	mdelay(10);
	printk("%s read GPIO 8: value = %d \n", __func__,gpio_get_value(GPIO_PDA_ACTIVE));	
}
#endif

static ssize_t fsa9485_uart_sel_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size)
{
	int fd;
	char value[50];
	char buffer[2]={0};

	printk(KERN_ERR "%s\n", __func__);

	memset(value,0x0,sizeof(value));

	if (sscanf(buf, "%49s", value) != 1) {
		pr_err("%s : Invalid value\n", __func__);
		return -EINVAL;
	}

	mm_segment_t fs = get_fs();
	set_fs(get_ds());
	
	if ((fd = sys_open("/data/path/uart_sel.bin", O_CREAT|O_RDWR, 0666)) < 0)
	{
		printk("%s :: open failed %s ,fd=0x%x\n", __func__, "/data/path/uart_sel.bin", fd);
	} else {
		printk("%s :: open success %s ,fd=0x%x\n", __func__, "/data/path/uart_sel.bin", fd);
	}

	printk("%s : value = %s \n", __func__,value);
	
	
	if (!strncmp(value,"AP",2)) {
		sprintf(buffer, "0");
		curr_uart_path = SWITCH_AP;
		fsa9485_uart_sel_switch_en(0);
	}
	 else if (!strncmp(value,"CP",2)) {
	 	sprintf(buffer, "1");
		curr_uart_path = SWITCH_CP;
		fsa9485_uart_sel_switch_en(1);
	} else {
		sprintf(buffer, "0");
		fsa9485_uart_sel_switch_en(0);
	}

	sys_write(fd, buffer, strlen(buffer));

	sys_close(fd); 
	set_fs(fs);

	return size;

}


static ssize_t fsa9485_uart_sel_factory_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int fd;
	char buffer[2]={0};
	int ret;

	printk(KERN_ERR "%s\n", __func__);

	mm_segment_t fs = get_fs();
	set_fs(get_ds());

	if ((fd = sys_open("/data/path/uart_sel.bin", O_RDONLY,0)) < 0) {
		printk("[FSA9480]: %s :: open failed %s ,fd=0x%x\n", __func__, "/data/path/uart_sel.bin", fd);
		return 0;
	}

	ret = sys_read(fd, buffer, 1);
	if(ret < 0) {
		printk("uart_switch_show READ FAIL!\n");
		return 0;
	}
	
	sys_close(fd);
	set_fs(fs);

	printk("curr_uart_path = %d \n", curr_uart_path);
	printk("uart_switch_show uart_sel.bin = %s \n", buffer);

	if (!strncmp(buffer, "0", 1))
	{
	  return sprintf(buf, "AP");
	} else if(!strncmp(buffer, "1", 1)) {
	  return sprintf(buf, "CP");
	}  else {
	  return sprintf(buf, "AP");
	}
}


//extern void extern_cmd_force_sleep(void);
extern void uas_jig_force_sleep(void);
int	force_jig_sleep = 0;

//don't save the uart path in /data/path/uart_sel.bin
static ssize_t fsa9485_uart_sel_factory_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size)
{
	int fd;
	char value[50];
	char buffer[2]={0};

	struct fsa9485_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	struct fsa9485_platform_data *pdata = usbsw->pdata;
	
	if (sscanf(buf, "%49s", value) != 1) {
		pr_err("%s : Invalid value\n", __func__);
		return -EINVAL;
	}
	
	printk("%s : value = %s \n", __func__,value);	
	printk("do not save uart path, just change \n", buffer);
	
	if (!strncmp(value,"pgmsleep",8)) {
		fsa9485_ap_cp_int1_en(0);
		fsa9485_pda_active_en(0);

		force_jig_sleep = 1;
		printk("cmd_force_sleep!\n");

		if (pdata->uart_cb)
			pdata->uart_cb(FSA9485_DETACHED);

		uart_connecting = 0;

		//uas_jig_force_sleep();
		//extern_cmd_force_sleep();

		return size;
	}
	else
	{
		if (!strncmp(value,"AP",2)) {
			fsa9485_uart_sel_switch_en(0);
		} else if (!strncmp(value,"CP",2)) {
			fsa9485_uart_sel_switch_en(1);
		} else {
			fsa9485_uart_sel_switch_en(0);
		}
	}

	return size;
}

static ssize_t fsa9485_usb_sel_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int fd;
	char buffer[2]={0};
	int ret;

	printk(KERN_ERR "%s\n", __func__);

	mm_segment_t fs = get_fs();
	set_fs(get_ds());

	if ((fd = sys_open("/data/path/usb_sel.bin", O_RDONLY,0)) < 0){	
		printk("%s :: open failed %s ,fd=0x%x\n",__func__,"/data/path/usb_sel.bin",fd);
		return 0;
	}

	ret = sys_read(fd,buffer,1);
	if(ret<0) {
		printk("usb_switch_show READ FAIL!\n");
		return 0;
	}	

	sys_close(fd);
	set_fs(fs);

	printk("curr_usb_path = %d \n", curr_usb_path);
	printk("usb_switch_show usb_sel.bin = %s \n",buffer);

	
	if (!strncmp(buffer, "1", 1)) {
		return sprintf(buf, "PDA");
	}  else if (!strncmp(buffer, "0", 1)) {
		return sprintf(buf, "MODEM");
	} else {
		return sprintf(buf, "PDA");
	}
}

static ssize_t fsa9485_usb_sel_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size)
{
	int fd;
	char value[50];
	char buffer[2]={0};

	printk(KERN_ERR "%s\n", __func__);

	memset(value,0x0,sizeof(value));

	if (sscanf(buf, "%49s", value) != 1) {
		pr_err("%s : Invalid value\n", __func__);
		return -EINVAL;
	}

	printk("%s : value = %s \n", __func__,value);

	mm_segment_t fs = get_fs();
	set_fs(get_ds());

	if ((fd = sys_open("/data/path/usb_sel.bin", O_CREAT|O_WRONLY  ,0666)) < 0)
	{ 
		printk("[FSA9480]: %s :: open failed %s ,fd=0x%x\n",__func__,"/data/path/usb_sel.bin",fd);
	} else {
		printk("[FSA9480]: %s :: open success %s ,fd=0x%x\n",__func__,"/data/path/usb_sel.bin",fd);
	}

	

    if (!strncmp(value,"PDA",3)) {
		if(curr_usb_path != SWITCH_AP){ 
			fsa9485_cp2_usb_on_en(0);
			sprintf(buffer, "1");
			fsa9485_set_switch("DHOST");
			curr_usb_path = SWITCH_AP;	
		} 		
	} else if (!strncmp(value,"MODEM",5)) {		
	 	if( curr_usb_path != SWITCH_CP) {
			fsa9485_cp2_usb_on_en(1);
			sprintf(buffer, "0");		
			fsa9485_set_switch("VAUDIO");
			curr_usb_path = SWITCH_CP;		
 		}
	} else {
		fsa9485_set_switch("AUTO");
	}

	sys_write(fd,buffer,strlen(buffer));

	sys_close(fd); 
	set_fs(fs);

	return size;

}
#endif

static DEVICE_ATTR(control, S_IRUGO, fsa9485_show_control, NULL);
static DEVICE_ATTR(device_type, S_IRUGO, fsa9485_show_device_type, NULL);
static DEVICE_ATTR(switch, S_IRUGO | S_IWUSR,
		fsa9485_show_manualsw, fsa9485_set_manualsw);
static DEVICE_ATTR(usb_state, S_IRUGO, fsa9480_show_usb_state, NULL);
static DEVICE_ATTR(adc, S_IRUGO, fsa9485_show_adc, NULL);
static DEVICE_ATTR(reset_switch, S_IWUSR | S_IWGRP, NULL, fsa9485_reset);
#if defined(CONFIG_SEC_DUAL_MODEM)
static DEVICE_ATTR(uart_sel, S_IRUGO | S_IWUSR | S_IWGRP, fsa9485_uart_sel_show, fsa9485_uart_sel_store);
static DEVICE_ATTR(uart_sel_factory, S_IRUGO | S_IWUGO, fsa9485_uart_sel_factory_show, fsa9485_uart_sel_factory_store);
static DEVICE_ATTR(usb_sel, S_IRUGO | S_IWUSR | S_IWGRP, fsa9485_usb_sel_show, fsa9485_usb_sel_store);
#endif

static struct attribute *fsa9485_attributes[] = {
	&dev_attr_control.attr,
	&dev_attr_device_type.attr,
	&dev_attr_switch.attr,
	NULL
};

static const struct attribute_group fsa9485_group = {
	.attrs = fsa9485_attributes,
};

void fsa9485_otg_detach(void)
{
	unsigned int data = 0;
	int ret;
	struct i2c_client *client = local_usbsw->client;

	if (local_usbsw->dev1 & DEV_USB_OTG) {
		dev_info(&client->dev, "%s: real device\n", __func__);

		data = 0x00;
		ret = fsa9485_write_reg(client, FSA9485_REG_MANSW2, data);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);

		data = SW_ALL_OPEN;
		ret = fsa9485_write_reg(client, FSA9485_REG_MANSW1, data);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);

		data = 0x1A;
		ret = fsa9485_write_reg(client, FSA9485_REG_CTRL, data);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
	} else
		dev_info(&client->dev, "%s: not real device\n", __func__);
}
EXPORT_SYMBOL(fsa9485_otg_detach);

//ENABLE_OTG
bool otg_status = 0;
//to check the ID status
u8 fsa9485_otg_status(void)
{
	return otg_status;
}
EXPORT_SYMBOL(fsa9485_otg_status);
//control the external OTG booster
void fsa9485_otg_vbus_en(u8 on)
{
#if defined(CONFIG_USB_OTG_AUDIODOCK)
	struct fsa9485_usbsw *usbsw = local_usbsw;

	if(usbsw->audio_dock)
		gpio_direction_output(8/*OTG_EN*/,0);//OTG_EN  keep low because charging is required
	else
#endif
	gpio_direction_output(8/*OTG_EN*/,on);//OTG_EN high
}
EXPORT_SYMBOL(fsa9485_otg_vbus_en);

int get_acc_status()
{
	struct fsa9485_usbsw *usbsw;
	
	if(!local_usbsw)
	{
		pr_info("%s fsa9485 driver is not ready\n", __func__);
		return;
	}
	usbsw = local_usbsw;	

	pr_info("%s acc_status=%d\n",__func__,usbsw->acc_status);

	return usbsw->acc_status;
	
}
EXPORT_SYMBOL(get_acc_status);
void fsa9485_manual_switching(int path)
{
	struct i2c_client *client = local_usbsw->client;
	unsigned int value;
	unsigned int data = 0;
	int ret;

	fsa9485_read_reg(client,FSA9485_REG_CTRL,&value);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if ((value & ~CON_MANUAL_SW) !=(CON_SWITCH_OPEN | CON_RAW_DATA | CON_WAIT))
		return;

	if (path == SWITCH_PORT_VAUDIO) {
		data = SW_VAUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_UART) {
		data = SW_UART;
		value &= ~CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_AUDIO) {
		data = SW_AUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_USB) {
		data = SW_DHOST;
		value &= ~CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_AUTO) {
		data = SW_AUTO;
		value |= CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_USB_OPEN) {
		data = SW_USB_OPEN;
		value &= ~CON_MANUAL_SW;
	} else if (path ==  SWITCH_PORT_ALL_OPEN) {
		data = SW_ALL_OPEN;
		value &= ~CON_MANUAL_SW;
	} else {
		pr_info("%s: wrong path (%d)\n", __func__, path);
		return;
	}

	local_usbsw->mansw = data;

	/* path for FTM sleep */
	if (path ==  SWITCH_PORT_ALL_OPEN) {
		ret = fsa9485_write_reg(client,FSA9485_REG_MANUAL_OVERRIDES1, 0x0a);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);

		ret = fsa9485_write_reg(client,FSA9485_REG_MANSW1, data);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);

		ret = fsa9485_write_reg(client,FSA9485_REG_MANSW2, data);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);

		ret = fsa9485_write_reg(client,FSA9485_REG_CTRL, value);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
	} 
	else {
		ret = fsa9485_write_reg(client,FSA9485_REG_MANSW1, data);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);

		ret = fsa9485_write_reg(client,FSA9485_REG_CTRL, value);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
	}

}
EXPORT_SYMBOL(fsa9485_manual_switching);

extern unsigned int lp_boot_mode;

void fsa9485_vbus_check(bool vbus_status)
{
	struct fsa9485_usbsw *usbsw;
	struct fsa9485_platform_data *pdata;
	
	if(!local_usbsw)
	{
		pr_info("%s fsa9485 driver is not ready\n", __func__);
		return;
	}
		
	usbsw = local_usbsw;
	pdata = local_usbsw->pdata;

	if (!usbsw || !pdata) return;

	usbsw->vbus = (int)vbus_status;

	// ephron, temporary fix for side effect of vbus handling.
	pr_info("fsa9485_vbus_check : vbus_status=%d check_vbus=%d mhl_removed=%d audio_dock=%d\n",vbus_status,usbsw->check_vbus,usbsw->mhl_removed,usbsw->audio_dock);

	if(usbsw->check_vbus ){
		if(vbus_status){
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_ATTACHED);			
		}
		else	{
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);			
		}	
	}	

	if(usbsw->mhl_removed){
		if(!vbus_status){
			usbsw->mhl_removed = 0;
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);	
			pr_info("%s mhl removed\n",__func__);
		}			
	}
	if( usbsw->audio_dock){
		if(!vbus_status){
			usbsw->audio_dock = 0;
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);	
			pr_info("%s  audio_dock removed\n",__func__);
		}			
	}
	if(!usbsw->init_det_done){
		pr_info("%s init_det_done is not ready\n", __func__);
		return;
	}

	schedule_delayed_work(&usbsw->vbus_work, msecs_to_jiffies(500));
	
}
EXPORT_SYMBOL(fsa9485_vbus_check);

static int fsa9485_read_vbus()
{
	struct i2c_client *client = local_usbsw->client;
	int vbus_status=0,ret;

	fsa9485_read_reg(client, FSA9485_REG_RESERVED_1D,&vbus_status);
	if (vbus_status < 0)
		pr_info("=====%s: err %d=======\n", __func__, vbus_status);

	if( vbus_status & VBUS_VALID)
		ret = 1;
	else
		ret = 0;
	pr_info("%s: ret=%d vbus_status=%d\n",__func__,ret,vbus_status);
	
	return ret;
}

int fsa9485_get_vbus_status()
{
	struct fsa9485_usbsw *usbsw;
	struct fsa9485_platform_data *pdata;
	
	if(!local_usbsw)
	{
		pr_info("%s fsa9485 driver is not ready\n", __func__);
		return;
	}
		
	usbsw = local_usbsw;
        printk("USBD][fsa9485_get_vbus_status] vbus=%d\n",usbsw->vbus);
        return usbsw->vbus;
}
EXPORT_SYMBOL(fsa9485_get_vbus_status);

static int fsa9485_detect_dev(struct fsa9485_usbsw *usbsw)
{
	int device_type, ret;
#if !defined(CONFIG_USB_OTG_AUDIODOCK)
	unsigned char val1, val2, adc;
#else
	unsigned int val1, val2, adc;
#endif
	u8 val;
	struct fsa9485_platform_data *pdata = usbsw->pdata;
	struct i2c_client *client = usbsw->client;
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
	u8 mhl_ret = 0;
#endif

	fsa9485_read_reg(client, FSA9485_REG_DEV_T1,&val1);
	fsa9485_read_reg(client, FSA9485_REG_DEV_T2,&val2);
	fsa9485_read_reg(client, FSA9485_REG_ADC,&adc);
#if defined(CONFIG_USB_OTG_AUDIODOCK)
	val1 &= 0xff;
	val2 &= 0xff;
	adc &= 0xff;
#endif
	dev_info(&client->dev, "dev1: 0x%x, dev2: 0x%x adc:0x%x\n", val1, val2,adc);

#if defined(CONFIG_USB_OTG_AUDIODOCK)
	if (adc == ADC_AUDIO_DOCK)
		val2 = DEV_AUDIODOCK;
#endif
	if (usbsw->dock_attached)
		pdata->dock_cb(FSA9485_DETACHED_DOCK);

#if !defined(CONFIG_USB_OTG_AUDIODOCK)
	if( adc == ADC_AUDIO_DOCK){ // AUDIO_DOCK_ATTACHED
		usbsw->check_vbus = 1;
		usbsw->audio_dock = 1;
	}
	else if( usbsw->audio_dock==1 ){ // AUDIO_DOCK_REMOVED
		usbsw->check_vbus = 0;
	}
#endif
	if (adc == 0x10)
		val2 = DEV_SMARTDOCK;

	usbsw->unnormal_TA = 0;
	/* Attached */
	if (val1 || val2) {
		usbsw->acc_status = 0;
		/* USB */
		if (val1 & DEV_USB ) {
			dev_info(&client->dev, "usb device - connect\n");

			if (pdata->usb_cb)
				pdata->usb_cb(FSA9485_ATTACHED);
			if (usbsw->mansw) {
				ret = fsa9485_write_reg(client,
				FSA9485_REG_MANSW1, usbsw->mansw);

				if (ret < 0)
					dev_err(&client->dev,"%s: err %d\n", __func__, ret);
			}
		}
		else if(val2 & DEV_JIG_USB_OFF) {
			dev_info(&client->dev, "JIG USB OFF connect\n");
			if( fsa9485_read_vbus()){
				if (pdata->charger_cb)
					pdata->charger_cb(FSA9485_ATTACHED);

			}
		}/* USB_CDP */ 
		else if (val1 & DEV_USB_CHG) {
			dev_info(&client->dev, "usb_cdp connect\n");

			if (pdata->usb_cdp_cb)
				pdata->usb_cdp_cb(FSA9485_ATTACHED);
			if (usbsw->mansw) {
				ret = fsa9485_write_reg(client,FSA9485_REG_MANSW1, usbsw->mansw);
				if (ret < 0)
					dev_err(&client->dev,"%s: err %d\n", __func__, ret);
			}
		}	/* UART */ 
		else if (val1 & DEV_T1_UART_MASK || val2 & DEV_T2_UART_MASK) {
#if defined(CONFIG_SEC_DUAL_MODEM)
			if(force_jig_sleep == 0) {
#endif
				uart_connecting = 1;
				dev_info(&client->dev, "uart connect\n");
				fsa9485_write_reg(client,FSA9485_REG_CTRL, 0x1E);
				if (pdata->uart_cb)
					pdata->uart_cb(FSA9485_ATTACHED);
#if defined(CONFIG_SEC_DUAL_MODEM)
				if( val2 & DEV_T2_UART_MASK )
				{
					fsa9485_ap_cp_int1_en(1);				
				}				
#endif
				if (usbsw->mansw) {
					ret = fsa9485_write_reg(client,
						FSA9485_REG_MANSW1, SW_UART);

					if (ret < 0)
						dev_err(&client->dev,"%s: err %d\n", __func__, ret);
				}
#if defined(CONFIG_SEC_DUAL_MODEM)
			}
#endif		
		}/* CHARGER */ 
		else if (val1 & DEV_T1_CHARGER_MASK) {
			dev_info(&client->dev, "charger connect\n");
			pr_info("charger connect\n");

			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_ATTACHED);
		
		}/* for SAMSUNG OTG */ 
		else if (val1 & DEV_USB_OTG) {
			dev_info(&client->dev, "usb host - otg connect\n");
//ENABLE_OTG
			fsa9485_write_reg(client,FSA9485_REG_MANSW1, 0x27);
			msleep(50);
			fsa9485_write_reg(client,FSA9485_REG_MANSW1, 0x27);
			fsa9485_write_reg(client,FSA9485_REG_CTRL, 0x1a);
#if defined(CONFIG_MACH_CAPRI_SS_S2VE) //Enable OTG only if the model is S2VE
			otg_status = 1;
			if (pdata->otg_cb)
				pdata->otg_cb(FSA9485_ATTACHED);
#endif				
#if defined(CONFIG_USB_OTG_AUDIODOCK)
		} /* AUDIODOCK */
		else if (val2 & DEV_AUDIODOCK) {	
				dev_info(&client->dev, "usb host - audiodock connect\n");

				fsa9485_write_reg(client,FSA9485_REG_MANSW1, 0x27);
				msleep(50);
				fsa9485_write_reg(client,FSA9485_REG_MANSW1, 0x27);
				fsa9485_write_reg(client,FSA9485_REG_CTRL, 0x1a);

				otg_status = 1;
				usbsw->audio_dock = 1;
				if (pdata->audiodock_cb)
					pdata->audiodock_cb(FSA9485_ATTACHED_AUDIO_DOCK);

				if(fsa9485_read_vbus())
					if (pdata->charger_cb)
						pdata->charger_cb(FSA9485_ATTACHED);	
#endif				
		}/* JIG */ 
		else if (val2 & DEV_T2_JIG_MASK) {
			dev_info(&client->dev, "jig connect\n");

			if (pdata->jig_cb)
				pdata->jig_cb(FSA9485_ATTACHED);
		/* Desk Dock */
		} else if (val2 & DEV_AV) {
			if ((adc & 0x1F) == ADC_DESKDOCK) {
				pr_info("FSA Deskdock Attach\n");
				FSA9485_CheckAndHookAudioDock(1);
				usbsw->deskdock = 1;
				usbsw->check_vbus= 1;
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
				isDeskdockconnected = 1;
#endif
				fsa9485_write_reg(client,FSA9485_REG_RESERVED_20, 0x08);
				if(fsa9485_read_vbus())
					if (pdata->charger_cb)
						pdata->charger_cb(FSA9485_ATTACHED);					
			} 
			else {
				pr_info("FSA MHL Attach\n");
				usbsw->acc_status = 1;
				if(fsa9485_read_vbus())
					if (pdata->charger_cb)
						pdata->charger_cb(FSA9485_ACCESSORY);	

				fsa9485_write_reg(client,FSA9485_REG_RESERVED_20, 0x08);
		#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
			DisableFSA9480Interrupts();
				if (!isDeskdockconnected){
					usbsw->check_vbus = 1;
					mhl_ret = mhl_onoff_ex(1);
				}

				if (mhl_ret != MHL_DEVICE &&(adc & 0x1F) == 0x1A) {
					FSA9485_CheckAndHookAudioDock(1);
					isDeskdockconnected = 1;
			}
			EnableFSA9480Interrupts();
		#else
				pr_info("FSA mhl attach, but not support MHL feature!\n");
		#endif
			}
		
		}/* Car Dock */ 
		else if (val2 & DEV_JIG_UART_ON) {
			if (pdata->dock_cb)
				pdata->dock_cb(FSA9485_ATTACHED_CAR_DOCK);
			pr_info("car dock connect\n");

			if(fsa9485_read_vbus())
				if (pdata->charger_cb)
					pdata->charger_cb(FSA9485_ATTACHED);					

			usbsw->check_vbus = 1;
			ret = fsa9485_write_reg(client,FSA9485_REG_MANSW1, SW_AUDIO);
			if (ret < 0)
				dev_err(&client->dev,"%s: err %d\n", __func__, ret);

			fsa9485_read_reg(client,FSA9485_REG_CTRL,&ret);
			if (ret < 0)
				dev_err(&client->dev,"%s: err %d\n", __func__, ret);

			fsa9485_write_reg(client,FSA9485_REG_CTRL, ret & ~CON_MANUAL_SW);
			usbsw->dock_attached = FSA9485_ATTACHED;
		/* SmartDock */
		} else if (val2 & DEV_SMARTDOCK) {
			usbsw->adc = (int)adc;
			dev_info(&client->dev, "smart dock connect\n");
			pr_info("smart dock connect\n");

			usbsw->mansw = SW_DHOST;
			ret = fsa9485_write_reg(client,FSA9485_REG_MANSW1, SW_DHOST);
			if (ret < 0)
				dev_err(&client->dev,"%s: err %d\n", __func__, ret);
			
			fsa9485_read_reg(client,FSA9485_REG_CTRL,&ret);
			if (ret < 0)
				dev_err(&client->dev,"%s: err %d\n", __func__, ret);
			fsa9485_write_reg(client,FSA9485_REG_CTRL, ret & ~CON_MANUAL_SW);
	
			if (pdata->smartdock_cb)
				pdata->smartdock_cb(FSA9485_ATTACHED);
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
			mhl_onoff_ex(1);
#endif
		}
		if(usbsw->ovp)
			pdata->ovp_cb(true);
	}/* Detached */
	else {
		/* USB */	
		if (usbsw->dev1 & DEV_USB) {
			dev_info(&client->dev, "usb device - disconnect\n");

			if (pdata->usb_cb)
				pdata->usb_cb(FSA9485_DETACHED);
		} else if (usbsw->dev2 & DEV_JIG_USB_OFF) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);
			pr_info("%s DEV_JIG_USB_OFF removed\n",__func__);
		} else if (usbsw->dev1 & DEV_USB_CHG) {
			if (pdata->usb_cdp_cb)
				pdata->usb_cdp_cb(FSA9485_DETACHED);

		/* UART */
		} else if (usbsw->dev1 & DEV_T1_UART_MASK ||usbsw->dev2 & DEV_T2_UART_MASK) {
			if (pdata->uart_cb)
				pdata->uart_cb(FSA9485_DETACHED);
			uart_connecting = 0;
			dev_info(&client->dev, "[FSA9485] uart disconnect\n");

#if defined(CONFIG_SEC_DUAL_MODEM)
			if( val2 & DEV_T2_UART_MASK )
			{
				fsa9485_ap_cp_int1_en(0);				
			}
#endif

		/* CHARGER */
		} else if (usbsw->dev1 & DEV_T1_CHARGER_MASK) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);
		/* for SAMSUNG OTG */
		} else if (usbsw->dev1 & DEV_USB_OTG) {
#if defined(CONFIG_MACH_CAPRI_SS_S2VE) //Enable OTG only if the model is S2VE
//DISABLE_OTG
			dev_info(&client->dev, "usb host - otg disconnect\n");
	
			otg_status = 0;
			if (pdata->otg_cb)
				pdata->otg_cb(FSA9485_DETACHED);	
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);					
#endif						
			fsa9485_write_reg(client,FSA9485_REG_CTRL, 0x1E);
#if defined(CONFIG_USB_OTG_AUDIODOCK)
		} else if (usbsw->dev2 & DEV_AUDIODOCK) {
			dev_info(&client->dev, "usb host - audiodock disconnect\n");
			otg_status = 0;
			usbsw->audio_dock = 0;
			if (pdata->audiodock_cb)
				pdata->audiodock_cb(FSA9485_DETACHED_DOCK);
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);
			fsa9485_write_reg(client,FSA9485_REG_CTRL, 0x1E);
#endif					
		/* JIG */
		} else if (usbsw->dev2 & DEV_T2_JIG_MASK) {
			if (pdata->jig_cb)
				pdata->jig_cb(FSA9485_DETACHED);
		/* Desk Dock */
		} else if (usbsw->dev2 & DEV_AV) {
			pr_info("FSA MHL Detach\n");
			fsa9485_write_reg(client,FSA9485_REG_RESERVED_20, 0x04);
			usbsw->check_vbus = 0;
			usbsw->mhl_removed = 1;
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
			if (isDeskdockconnected)
				FSA9485_CheckAndHookAudioDock(0);
#if 0//defined CONFIG_MHL_D3_SUPPORT
			mhl_onoff_ex(false);
			detached_status = 1;
#endif
			isDeskdockconnected = 0;
#else
			if (usbsw->deskdock) {
				FSA9485_CheckAndHookAudioDock(0);
				usbsw->deskdock = 0;
			} else {
				pr_info("FSA detach mhl cable, but not support MHL feature\n");
			}
#endif
		/* Car Dock */
		}else if (usbsw->dev2 & DEV_JIG_UART_ON) {
			if (pdata->dock_cb)
				pdata->dock_cb(FSA9485_DETACHED_DOCK);
			fsa9485_read_reg(client,FSA9485_REG_CTRL,&ret);
			fsa9485_write_reg(client,FSA9485_REG_CTRL,ret | CON_MANUAL_SW);
			usbsw->dock_attached = FSA9485_DETACHED;
			usbsw->check_vbus = 0;
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);					
			
		} else if (usbsw->adc == 0x10) {
			dev_info(&client->dev, "smart dock disconnect\n");

			fsa9485_read_reg(client,FSA9485_REG_CTRL,&ret);
			fsa9485_write_reg(client,FSA9485_REG_CTRL,ret | CON_MANUAL_SW);

			if (pdata->smartdock_cb)
				pdata->smartdock_cb(FSA9485_DETACHED);
			usbsw->adc = 0;
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
			mhl_onoff_ex(false);
#endif
		}

	}
	
	usbsw->dev1 = val1;
	usbsw->dev2 = val2;

	return adc;
}

static int fsa9485_check_dev(struct fsa9485_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int device_type;
	fsa9485_read_word_reg(client, FSA9485_REG_DEV_T1,&device_type);
	if (device_type < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, device_type);
		return 0;
	}
	return device_type;
}

static int fsa9485_handle_dock_vol_key(struct fsa9485_usbsw *info, int adc)
{
	struct input_dev *input = info->input;
	int pre_key = info->previous_key;
	unsigned int code;
	int state;

	if (adc == ADC_OPEN) {
		switch (pre_key) {
		case DOCK_KEY_VOL_UP_PRESSED:
			code = KEY_VOLUMEUP;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_UP_RELEASED;
			break;
		case DOCK_KEY_VOL_DOWN_PRESSED:
			code = KEY_VOLUMEDOWN;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_DOWN_RELEASED;
			break;
		case DOCK_KEY_PREV_PRESSED:
			code = KEY_PREVIOUSSONG;
			state = 0;
			info->previous_key = DOCK_KEY_PREV_RELEASED;
			break;
		case DOCK_KEY_PLAY_PAUSE_PRESSED:
			code = KEY_PLAYPAUSE;
			state = 0;
			info->previous_key = DOCK_KEY_PLAY_PAUSE_RELEASED;
			break;
		case DOCK_KEY_NEXT_PRESSED:
			code = KEY_NEXTSONG;
			state = 0;
			info->previous_key = DOCK_KEY_NEXT_RELEASED;
			break;
		default:
			return 0;
		}
		input_event(input, EV_KEY, code, state);
		input_sync(input);
		return 0;
	}

	if (pre_key == DOCK_KEY_NONE) {
		if (adc != ADC_DOCK_VOL_UP && adc != ADC_DOCK_VOL_DN
			&& adc != ADC_DOCK_PREV_KEY && adc != ADC_DOCK_NEXT_KEY
			&& adc != ADC_DOCK_PLAY_PAUSE_KEY)
			return 0;
	}

	switch (adc) {
	case ADC_DOCK_VOL_UP:
		code = KEY_VOLUMEUP;
		state = 1;
		info->previous_key = DOCK_KEY_VOL_UP_PRESSED;
		break;
	case ADC_DOCK_VOL_DN:
		code = KEY_VOLUMEDOWN;
		state = 1;
		info->previous_key = DOCK_KEY_VOL_DOWN_PRESSED;
		break;
	case ADC_DOCK_PREV_KEY-1 ... ADC_DOCK_PREV_KEY+1:
		code = KEY_PREVIOUSSONG;
		state = 1;
		info->previous_key = DOCK_KEY_PREV_PRESSED;
		break;
	case ADC_DOCK_PLAY_PAUSE_KEY-1 ... ADC_DOCK_PLAY_PAUSE_KEY+1:
		code = KEY_PLAYPAUSE;
		state = 1;
		info->previous_key = DOCK_KEY_PLAY_PAUSE_PRESSED;
		break;
	case ADC_DOCK_NEXT_KEY-1 ... ADC_DOCK_NEXT_KEY+1:
		code = KEY_NEXTSONG;
		state = 1;
		info->previous_key = DOCK_KEY_NEXT_PRESSED;
		break;
	case ADC_DESKDOCK:
		if (pre_key == DOCK_KEY_VOL_UP_PRESSED) {
			code = KEY_VOLUMEUP;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_UP_RELEASED;
		} else if (pre_key == DOCK_KEY_VOL_DOWN_PRESSED) {
			code = KEY_VOLUMEDOWN;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_DOWN_RELEASED;
		} else if (pre_key == DOCK_KEY_PREV_PRESSED) {
			code = KEY_PREVIOUSSONG;
			state = 0;
			info->previous_key = DOCK_KEY_PREV_RELEASED;
		} else if (pre_key == DOCK_KEY_PLAY_PAUSE_PRESSED) {
			code = KEY_PLAYPAUSE;
			state = 0;
			info->previous_key = DOCK_KEY_PLAY_PAUSE_RELEASED;
		} else if (pre_key == DOCK_KEY_NEXT_PRESSED) {
			code = KEY_NEXTSONG;
			state = 0;
			info->previous_key = DOCK_KEY_NEXT_RELEASED;
		} else {
			return 0;
		}
		break;
	default:
		break;
		return 0;
	}

	input_event(input, EV_KEY, code, state);
	input_sync(input);

	return 1;
}

static irqreturn_t fsa9485_irq_thread(int irq, void *data)
{
	struct fsa9485_usbsw *usbsw = data;
	//pr_info("fsa9480_irq_handler irq #: %x\n", irq);
	if (!work_pending(&usbsw->work)) {
		disable_irq_nosync(irq);
		schedule_work(&usbsw->work);
	}
	else{
		//pr_info("fsa9480_irq_handler irq pending\n");	
		LOCAL_BUG_ON;
	}

	return IRQ_HANDLED;
}

static void fsa9485_vbus_work_cb(struct delayed_work *work)
{
	struct fsa9485_usbsw *usbsw = container_of(work, struct fsa9485_usbsw, work);
	struct fsa9485_platform_data *pdata;
#if defined(CONFIG_BQ24272_CHARGER)||defined(CONFIG_SMB358_CHARGER)	
	u8 val1,val2;
#endif
		
	usbsw = local_usbsw;
	pdata = local_usbsw->pdata;	

	if (!usbsw || !pdata){
		pr_info("fsa9485_vbus_work_cb error\n");
		return;
	}
	if(usbsw->dev1==0 && usbsw->dev2==0 && usbsw->vbus==1 ){
		usbsw->unnormal_TA = 1;
		pr_info("%s unnormal_TA=%d acc_status=%d\n",__func__,usbsw->unnormal_TA,usbsw->acc_status);
	}

	if(usbsw->unnormal_TA ){
		if(usbsw->vbus){
			if( usbsw->acc_status){
				if (pdata->charger_cb)
					pdata->charger_cb(FSA9485_ACCESSORY);						
			}
			else{
				if (pdata->charger_cb)
					pdata->charger_cb(FSA9485_ATTACHED);			
			}
		}
		else	{
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);			
		}	
	}	
		
#if defined(CONFIG_BQ24272_CHARGER)||defined(CONFIG_SMB358_CHARGER)		
	if(usbsw->vbus==0){
		fsa9485_read_reg(usbsw->client, FSA9485_REG_DEV_T1,&val1);
		fsa9485_read_reg(usbsw->client, FSA9485_REG_DEV_T2,&val2);
		if( val1 || val2)
			return;
		else{
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9485_DETACHED);			
		}
	}
#endif		
		
}

static void fsa9485_work_cb(struct work_struct *work)
{
	struct fsa9485_usbsw *usbsw = container_of(work, struct fsa9485_usbsw, work);
	struct i2c_client *client = usbsw->client;
	int intr, intr2, detect,temp;

	/* FSA9485 : Read interrupt -> Read Device
	 FSA9485 : Read Device -> Read interrupt */

	/* device detection */
	mutex_lock(&usbsw->mutex);
	detect = fsa9485_detect_dev(usbsw);
	mutex_unlock(&usbsw->mutex);

	/* read and clear interrupt status bits */
	fsa9485_read_word_reg(client, FSA9485_REG_INT1,&intr);
	//fsa9485_read_reg(client, FSA9485_REG_INT2,&intr2);

	intr2 = intr >> 8;
	
	if (intr < 0) {
		msleep(100);
		fsa9485_read_word_reg(client, FSA9485_REG_INT1,&intr);
		if (intr < 0)
			dev_err(&client->dev,"%s: err at read %d\n", __func__, intr);
		fsa9485_reg_init(usbsw);
		return IRQ_HANDLED;
	} else if (intr == 0) {
		/* interrupt was fired, but no status bits were set,
		so device was reset. In this case, the registers were
		reset to defaults so they need to be reinitialised. */
		fsa9485_reg_init(usbsw);
	}
#if !defined(CONFIG_MACH_CAPRI_SS_CRATER)	
	else if(intr & 0x20) // ovp
	{
		usbsw->ovp = true;
		usbsw->pdata->ovp_cb(true);
	}
	else if(intr & 0x80 && usbsw->ovp == true)
	{
		usbsw->ovp = false;
		usbsw->pdata->ovp_cb(false);			
	}
#endif	
	/* ADC_value(key pressed) changed at AV_Dock.*/
	if (intr2) {
		if (intr2 & 0x4) { /* for adc change */
			fsa9485_handle_dock_vol_key(usbsw, detect);
			dev_info(&client->dev,"intr2: 0x%x, adc_val: %x\n",intr2, detect);
		} else if (intr2 & 0x2) { /* for smart dock */
			fsa9485_read_word_reg(client, FSA9485_REG_INT1,&temp);

		} else if (intr2 & 0x1) { /* for av change (desk dock, hdmi) */
			dev_info(&client->dev,"%s enter Av charing\n", __func__);
			fsa9485_detect_dev(usbsw);
		} else {
			dev_info(&client->dev,"%s intr2 but, nothing happend, intr2: 0x%x\n",__func__, intr2);
		}
	}

	enable_irq(client->irq);
}

static int fsa9485_irq_init(struct fsa9485_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int ret;

	pr_info("%s\n", __func__);	
	INIT_WORK(&usbsw->work, fsa9485_work_cb);
	INIT_DELAYED_WORK(&usbsw->vbus_work, fsa9485_vbus_work_cb);


	if (client->irq) {
		ret = request_threaded_irq(client->irq, NULL,
			fsa9485_irq_thread, IRQF_TRIGGER_FALLING|IRQF_NO_SUSPEND,"fsa9485 micro USB", usbsw);
		if (ret) {
			dev_err(&client->dev, "failed to reqeust IRQ\n");
			return ret;
		}

		ret = enable_irq_wake(client->irq);
		if (ret < 0)
			dev_err(&client->dev,"failed to enable wakeup src %d\n", ret);
	}

	return 0;
}

static void fsa9485_init_detect(struct work_struct *work)
{
	struct fsa9485_usbsw *usbsw = container_of(work,
			struct fsa9485_usbsw, init_work.work);
	int ret = 0;

	dev_info(&usbsw->client->dev, "%s\n", __func__);


#if 1
	disable_irq(usbsw->client->irq);
	mutex_lock(&usbsw->mutex);
	fsa9485_detect_dev(usbsw);
	mutex_unlock(&usbsw->mutex);
	enable_irq(usbsw->client->irq);
#endif
	// make sure of clearing not handled int.
	{
		int intr;
		fsa9485_read_word_reg(usbsw->client,FSA9485_REG_INT1,&intr);

		// TODO : handling ovp and other interrupt events.
	}
#if 1  // ephron, move it to probe

	ret = fsa9485_irq_init(usbsw);
	if (ret)
		dev_info(&usbsw->client->dev,"failed to enable  irq init %s\n", __func__);
#endif

	schedule_delayed_work(&usbsw->vbus_work, msecs_to_jiffies(0));
	usbsw->init_det_done = 1;

}

#if defined(CONFIG_SEC_DUAL_MODEM)
extern unsigned int lp_boot_mode;
static void sec_switch_init_work(struct work_struct *work)
{
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	struct fsa9485_wq *wq = container_of(dw, struct fsa9485_wq, work_q);
	struct fsa9485_usbsw *usbsw = wq->sdata;
	int fd_usb, fd_uart;
	int ret;
	char buffer[2]={0};

	printk("[FSA9485]: %s :: \n",__func__);	

	mm_segment_t fs = get_fs();
	set_fs(get_ds());

	if ((fd_usb = sys_open("/data/path/usb_sel.bin", O_CREAT|O_RDWR  ,0666)) < 0 ||
		(fd_uart = sys_open("/data/path/uart_sel.bin", O_CREAT|O_RDWR  ,0666)) < 0)
	{ 
		schedule_delayed_work(&wq->work_q, msecs_to_jiffies(2000));
		printk("[FSA9485]: %s :: open failed %s ,fd=0x%x\n",__func__,"/data/path/usb_sel.bin",fd_usb);

		if(fd_usb < 0)
			sys_close(fd_usb);
		if(fd_uart < 0)
			sys_close(fd_uart);
		set_fs(fs);
		return;
	} else {
		cancel_delayed_work(&wq->work_q);
		printk("[FSA9485]: %s :: open success %s ,fd=0x%x\n",__func__,"/data/path/usb_sel.bin",fd_usb);
		printk("[FSA9485]: %s :: open success %s ,fd=0x%x\n",__func__,"/data/path/uart_sel.bin",fd_uart);		
	}	

	ret = sys_read(fd_uart, buffer, 1);
	if(ret < 0) {
		printk("uart_switch_show READ FAIL!\n");
		sys_close(fd_usb);
		sys_close(fd_uart);
		set_fs(fs);		
		return 0;
	}

	printk("uart buffer : %c\n", buffer);

	if (!strcmp(buffer, "0"))
	{
		printk("sec_switch_init_work uart PDA\n");
		curr_uart_path = SWITCH_AP;
		fsa9485_uart_sel_switch_en(0);
	} else if(!strcmp(buffer, "1")){
		printk("sec_switch_init_work uart MODEM\n");
		curr_uart_path = SWITCH_CP;
		fsa9485_uart_sel_switch_en(1);
		
		if(!lp_boot_mode)
			fsa9485_uart_reset();
	} else {
		printk("sec_switch_init_work uart PDA in else\n");
		curr_uart_path = SWITCH_AP;
		fsa9485_uart_sel_switch_en(0);
	}
		
	ret = sys_read(fd_usb, buffer, 1);
	if(ret < 0) {
		printk("usb_switch_show READ FAIL!\n");
		sys_close(fd_usb);
		sys_close(fd_uart);
		set_fs(fs);		
		return 0;
	}

	printk("usb buffer : %c\n", buffer);

	if (!strcmp(buffer, "1"))
	{
		printk("sec_switch_init_work usb PDA\n");		
		fsa9485_cp2_usb_on_en(0);
		fsa9485_set_switch("DHOST");
		curr_usb_path = SWITCH_AP;		
	} else 	if(!strcmp(buffer, "0")){
		printk("sec_switch_init_work usb MODEM\n");			
		fsa9485_cp2_usb_on_en(1);
		fsa9485_set_switch("VAUDIO");
		curr_usb_path = SWITCH_CP;	
	} else {
		printk("sec_switch_init_work usb PDA in else\n");		
		fsa9485_cp2_usb_on_en(0);
		fsa9485_set_switch("DHOST");
		curr_usb_path = SWITCH_AP;	
	}

	sys_close(fd_uart);
	sys_close(fd_usb);
	set_fs(fs);

	return;
}
#endif

static int __devinit fsa9485_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct fsa9485_usbsw *usbsw;
	int ret = 0;
	struct input_dev *input;
	struct device *switch_dev;
#if defined(CONFIG_SEC_DUAL_MODEM)
	struct fsa9485_wq *wq;
#endif

	pr_info("fsa9485_probe\n");

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	input = input_allocate_device();
	usbsw = kzalloc(sizeof(struct fsa9485_usbsw), GFP_KERNEL);
	if (!usbsw || !input) {
		dev_err(&client->dev, "failed to allocate driver data\n");
		input_free_device(input);
		kfree(usbsw);
		return -ENOMEM;
	}

	usbsw->input = input;
	input->name = client->name;
	input->phys = "deskdock-key/input0";
	input->dev.parent = &client->dev;
	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0001;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	input_set_capability(input, EV_KEY, KEY_VOLUMEUP);
	input_set_capability(input, EV_KEY, KEY_VOLUMEDOWN);
	input_set_capability(input, EV_KEY, KEY_PLAYPAUSE);
	input_set_capability(input, EV_KEY, KEY_PREVIOUSSONG);
	input_set_capability(input, EV_KEY, KEY_NEXTSONG);

	ret = input_register_device(input);
	if (ret)
		dev_err(&client->dev,"input_register_device %s: err %d\n", __func__, ret);

	usbsw->client = client;
	usbsw->pdata = client->dev.platform_data;
	usbsw->init_det_done = 0;
	if (!usbsw->pdata)
		goto fail1;

	i2c_set_clientdata(client, usbsw);

	mutex_init(&usbsw->mutex);

	local_usbsw = usbsw;

	if (usbsw->pdata->cfg_gpio)
		usbsw->pdata->cfg_gpio();

	fsa9485_reg_init(usbsw);

	uart_connecting = 0;

	ret = sysfs_create_group(&client->dev.kobj, &fsa9485_group);
	if (ret) {
		dev_err(&client->dev,"failed to create fsa9485 attribute group\n");
		goto fail2;
	}

//#ifdef CONFIG_SEC_CHARGING_FEATURE
#if 0
	/* make sysfs node /sys/class/sec/switch/usb_state */
#if defined(CONFIG_SEC_DUAL_MODEM)
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
#else
	switch_dev = device_create(sec_class, NULL, 0, NULL, "fsa9485");
#endif
	if (IS_ERR(switch_dev)) {
		pr_err("[FSA9485] Failed to create device (switch_dev)!\n");
		ret = PTR_ERR(switch_dev);
		goto fail2;
	}

	ret = device_create_file(switch_dev, &dev_attr_usb_state);
	if (ret < 0) {
		pr_err("[FSA9485] Failed to create file (usb_state)!\n");
		goto err_create_file_state;
	}

	ret = device_create_file(switch_dev, &dev_attr_adc);
	if (ret < 0) {
		pr_err("[FSA9485] Failed to create file (adc)!\n");
		goto err_create_file_adc;
	}

	ret = device_create_file(switch_dev, &dev_attr_reset_switch);
	if (ret < 0) {
		pr_err("[FSA9485] Failed to create file (reset_switch)!\n");
		goto err_create_file_reset_switch;
	}

#if defined(CONFIG_SEC_DUAL_MODEM)
	ret = device_create_file(switch_dev, &dev_attr_uart_sel);
	if (ret < 0) {
		dev_err(&client->dev,
			"Failed to create device (usb_state)!\n");
		goto err_create_file_state;
	}

	ret = device_create_file(switch_dev, &dev_attr_uart_sel_factory);
	if (ret < 0) {
		dev_err(&client->dev,
			"Failed to create device (usb_state)!\n");
		goto err_create_file_state;
	}

	ret = device_create_file(switch_dev, &dev_attr_usb_sel);
	if (ret < 0) {
		dev_err(&client->dev,
			"Failed to create device (usb_state)!\n");
		goto err_create_file_state;
	}
#endif

	dev_set_drvdata(switch_dev, usbsw);
#endif
	
	/* fsa9485 dock init*/
	if (usbsw->pdata->ex_init)
		usbsw->pdata->ex_init();

	/* fsa9485 reset */
	if (usbsw->pdata->reset_cb)
		usbsw->pdata->reset_cb();

	/* set fsa9485 init flag. */
	if (usbsw->pdata->set_init_flag)
		usbsw->pdata->set_init_flag();

	msleep(1000);
	fsa9485_write_reg(usbsw->client, 0x1b, 0x01);  //reset
	msleep(100);
	fsa9485_reg_init(usbsw);
	msleep(100);

#if 0
	ret = fsa9485_irq_init(usbsw);
	if (ret)
		dev_info(&usbsw->client->dev,"failed to enable  irq init %s\n", __func__);
#endif

#if !defined(CONFIG_SEC_DUAL_MODEM)
	/* initial cable detection */
	INIT_DELAYED_WORK(&usbsw->init_work, fsa9485_init_detect);
	schedule_delayed_work(&usbsw->init_work, msecs_to_jiffies(700));
#endif

#if defined(CONFIG_SEC_DUAL_MODEM)
	/* run work queue */
	wq = kmalloc(sizeof(struct fsa9485_wq), GFP_ATOMIC);
	if (wq) {
		wq->sdata = usbsw;
		INIT_DELAYED_WORK(&wq->work_q, sec_switch_init_work);
		schedule_delayed_work(&wq->work_q, msecs_to_jiffies(100));
	} else
		return -ENOMEM;
#endif

	pr_info("fsa9485_probe end.\n");
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
	input_free_device(input);
	kfree(usbsw);
	pr_info("fsa9485_probe failed .\n");
	return ret;
}

static int __devexit fsa9485_remove(struct i2c_client *client)
{
	struct fsa9485_usbsw *usbsw = i2c_get_clientdata(client);

	cancel_delayed_work(&usbsw->init_work);
	cancel_delayed_work_sync(&usbsw->vbus_work);
	if (client->irq) {
		disable_irq_wake(client->irq);
		free_irq(client->irq, usbsw);
	}
	mutex_destroy(&usbsw->mutex);
	i2c_set_clientdata(client, NULL);

	sysfs_remove_group(&client->dev.kobj, &fsa9485_group);
	kfree(usbsw);
	return 0;
}

static int fsa9485_resume(struct i2c_client *client)
{
	int value;
	struct fsa9485_usbsw *usbsw = i2c_get_clientdata(client);

/* add for fsa9485_irq_thread i2c error during wakeup */
	fsa9485_check_dev(usbsw);

	fsa9485_read_reg(client,FSA9485_REG_INT1,&value);

	/* device detection */
	mutex_lock(&usbsw->mutex);
	fsa9485_detect_dev(usbsw);
	mutex_unlock(&usbsw->mutex);
	return 0;
}


static const struct i2c_device_id fsa9485_id[] = {
	{"fsa9485", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fsa9485_id);

static struct i2c_driver fsa9485_i2c_driver = {
	.driver = {
		.name = "fsa9485",
	},
	.probe = fsa9485_probe,
	.remove = __devexit_p(fsa9485_remove),
	.resume = fsa9485_resume,
	.id_table = fsa9485_id,
};

static int __init fsa9485_init(void)
{
	return i2c_add_driver(&fsa9485_i2c_driver);
}
module_init(fsa9485_init);

static void __exit fsa9485_exit(void)
{
	i2c_del_driver(&fsa9485_i2c_driver);
}
module_exit(fsa9485_exit);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("FSA9485 USB Switch driver");
MODULE_LICENSE("GPL");
