/* drivers/input/misc/hscdtd006a.c
 *
 * GeoMagneticField device driver (HSCDTD006A)
 *
 * Copyright (C) 2011-2012 ALPS ELECTRIC CO., LTD. All Rights Reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifdef ALPS_MAG_DEBUG
#define DEBUG 1
#endif

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/hscdtd.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#define HSCDTD_LOG_TAG		"[HSCDTD], "

#define I2C_RETRY_DELAY		5
#define I2C_RETRIES		5

#define HSCDTD_CHIP_ID		0x5445

#define HSCDTD_STBA		0x0B
#define HSCDTD_STBB		0x0C
#define HSCDTD_INFO		0x0D
#define HSCDTD_XOUT		0x10
#define HSCDTD_YOUT		0x12
#define HSCDTD_ZOUT		0x14
#define HSCDTD_XOUT_H		0x11
#define HSCDTD_XOUT_L		0x10
#define HSCDTD_YOUT_H		0x13
#define HSCDTD_YOUT_L		0x12
#define HSCDTD_ZOUT_H		0x15
#define HSCDTD_ZOUT_L		0x14

#define HSCDTD_STATUS		0x18
#define HSCDTD_CTRL1		0x1b
#define HSCDTD_CTRL2		0x1c
#define HSCDTD_CTRL3		0x1d

#define HSCDTD_TCS_TIME		10000	/* Measure temp. of every 10 sec */
#define HSCDTD_DATA_ACCESS_NUM	6
#define HSCDTD_3AXIS_NUM	3
#define HSCDTD_INITIALL_DELAY	20
#define HSCDTD_MAX_LSB		4095
#define HSCDTD_MIN_LSB		-4096


struct hscdtd_data {
	struct input_dev		*input;
	struct i2c_client		*i2c;
	struct delayed_work		work_data;
	struct mutex			lock;
	struct hscdtd_platform_data	*pdata;
	unsigned int			delay_msec;
	unsigned int			tcs_thr;
	unsigned int			tcs_cnt;
	bool				factive;
};


/*--------------------------------------------------------------------------
 * i2c read/write function
 *--------------------------------------------------------------------------*/
static int hscdtd_i2c_read(struct i2c_client *i2c, u8 *rxData, int length)
{
	int err;
	int tries = 0;

	struct i2c_msg msgs[] = {
		{
			.addr	= i2c->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxData,
		},
		{
			.addr	= i2c->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxData,
		 },
	};

	do {
		err = i2c_transfer(i2c->adapter, msgs, ARRAY_SIZE(msgs));
	} while ((err != ARRAY_SIZE(msgs)) && (++tries < I2C_RETRIES));

	if (err != ARRAY_SIZE(msgs)) {
		dev_err(&i2c->adapter->dev, "read transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

static int hscdtd_i2c_write(struct i2c_client *i2c, u8 *txData, int length)
{
	int err;
	int tries = 0;

	struct i2c_msg msgs[] = {
		{
			.addr	= i2c->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txData,
		},
	};

	do {
		err = i2c_transfer(i2c->adapter, msgs, ARRAY_SIZE(msgs));
	} while ((err != ARRAY_SIZE(msgs)) && (++tries < I2C_RETRIES));

	if (err != ARRAY_SIZE(msgs)) {
		dev_err(&i2c->adapter->dev, "write transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}


/*--------------------------------------------------------------------------
 * hscdtd function
 *--------------------------------------------------------------------------*/
static int hscdtd_get_magnetic_field_data(struct hscdtd_data *hscdtd,
					int *xyz)
{
	int err = -1;
	int i;
	u8  sx[HSCDTD_DATA_ACCESS_NUM];

	sx[0] = HSCDTD_XOUT;
	err = hscdtd_i2c_read(hscdtd->i2c, sx, HSCDTD_DATA_ACCESS_NUM);
	if (err < 0)
		return err;
	for (i = 0; i < HSCDTD_3AXIS_NUM; i++)
		xyz[i] = (int) ((short)((sx[2*i+1] << 8) | (sx[2*i])));

	dev_dbg(&hscdtd->i2c->adapter->dev,
		HSCDTD_LOG_TAG "x:%d y:%d z:%d\n", xyz[0], xyz[1], xyz[2]);

	return err;
}

static int hscdtd_tcs_setup(struct hscdtd_data *hscdtd)
{
	int rc;
	u8 buf[2];

	buf[0] = HSCDTD_CTRL3;
	buf[1] = 0x02;
	rc = hscdtd_i2c_write(hscdtd->i2c, buf, 2);
	usleep_range(1700, 1700);
	hscdtd->tcs_thr = HSCDTD_TCS_TIME / hscdtd->delay_msec;
	hscdtd->tcs_cnt = 0;

	return rc;
}

static int hscdtd_force_setup(struct hscdtd_data *hscdtd)
{
	u8 buf[2];

	buf[0] = HSCDTD_CTRL3;
	buf[1] = 0x40;
	return hscdtd_i2c_write(hscdtd->i2c, buf, 2);
}

static void hscdtd_power_on(struct hscdtd_data *hscdtd)
{
	if (hscdtd->pdata->power_mode)
		hscdtd->pdata->power_mode(1);
}

static void hscdtd_measure_setup(struct hscdtd_data *hscdtd, bool en)
{
	u8 buf[2];

	if (en) {
		buf[0] = HSCDTD_CTRL1;
		buf[1] = 0xE2;
		hscdtd_i2c_write(hscdtd->i2c, buf, 2);
		usleep_range(100, 100);

		hscdtd_tcs_setup(hscdtd);
	} else {
		buf[0] = HSCDTD_CTRL1;
		buf[1] = 0x22;
		hscdtd_i2c_write(hscdtd->i2c, buf, 2);
	}
}

static void hscdtd_measure_start(struct hscdtd_data *hscdtd)
{
	dev_dbg(&hscdtd->i2c->adapter->dev,
		HSCDTD_LOG_TAG "%s\n", __func__);

	hscdtd_measure_setup(hscdtd, true);
	hscdtd_force_setup(hscdtd);

	schedule_delayed_work(&hscdtd->work_data,
		msecs_to_jiffies(hscdtd->delay_msec));
	hscdtd->factive = true;
}

static void hscdtd_power_off(struct hscdtd_data *hscdtd)
{
	if (hscdtd->pdata->power_mode)
		hscdtd->pdata->power_mode(0);
}

static void hscdtd_measure_stop(struct hscdtd_data *hscdtd)
{
	dev_dbg(&hscdtd->i2c->adapter->dev,
		HSCDTD_LOG_TAG "%s\n", __func__);

	hscdtd_measure_setup(hscdtd, false);
	hscdtd->factive = false;
	cancel_delayed_work(&hscdtd->work_data);
}

static void hscdtd_get_hardware_data(struct hscdtd_data *hscdtd, int *xyz)
{
	hscdtd_measure_setup(hscdtd, true);
	hscdtd_force_setup(hscdtd);
	usleep_range(5000, 5000);
	hscdtd_get_magnetic_field_data(hscdtd, xyz);
	hscdtd_measure_setup(hscdtd, false);
}

static int hscdtd_register_init(struct hscdtd_data *hscdtd)
{
	int v[3], ret = 0;
	u8  buf[2];
	u16 chip_info;

	dev_dbg(&hscdtd->i2c->adapter->dev,
		HSCDTD_LOG_TAG "%s\n", __func__);

	buf[0] = HSCDTD_INFO;
	ret = hscdtd_i2c_read(hscdtd->i2c, buf, 2);
	if (ret < 0)
		return ret;

	chip_info = (u16)((buf[1]<<8) | buf[0]);
	dev_dbg(&hscdtd->i2c->adapter->dev,
		HSCDTD_LOG_TAG "chip_info, 0x%04X\n", chip_info);
	if (chip_info != HSCDTD_CHIP_ID) {
		dev_err(&hscdtd->i2c->adapter->dev,
			HSCDTD_LOG_TAG "chipID error(0x%04X).\n", chip_info);
		return -1;
	}

	mutex_lock(&hscdtd->lock);
	buf[0] = HSCDTD_CTRL3;
	buf[1] = 0x80;
	hscdtd_i2c_write(hscdtd->i2c, buf, 2);
	usleep_range(10000, 10000);

	hscdtd_get_hardware_data(hscdtd, v);
	dev_dbg(&hscdtd->i2c->adapter->dev,
		HSCDTD_LOG_TAG "x:%d y:%d z:%d\n", v[0], v[1], v[2]);
	mutex_unlock(&hscdtd->lock);

	return 0;
}

/*--------------------------------------------------------------------------
 * sysfs
 *--------------------------------------------------------------------------*/
static ssize_t hscdtd_rate_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int err;
	long new_rate;
	struct hscdtd_data *hscdtd = dev_get_drvdata(dev);

	err = strict_strtol(buf, 10, &new_rate);
	if (err < 0)
		return err;

	mutex_lock(&hscdtd->lock);
	if (new_rate < 10)
		new_rate = 10;
	else if (new_rate > 200)
		new_rate = 200;
	hscdtd->tcs_cnt = hscdtd->tcs_cnt * hscdtd->delay_msec / (int)new_rate;
	hscdtd->tcs_thr = HSCDTD_TCS_TIME / (int)new_rate;
	hscdtd->delay_msec = (int)new_rate;
	mutex_unlock(&hscdtd->lock);

	dev_dbg(&hscdtd->i2c->adapter->dev,
		HSCDTD_LOG_TAG "%s, rate = %d (msec)\n",
		__func__, hscdtd->delay_msec);

	return size;
}

static DEVICE_ATTR(rate, 0664, NULL, hscdtd_rate_store);

static ssize_t hscdtd_power_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct hscdtd_data *hscdtd = dev_get_drvdata(dev);
	unsigned long val;

	if ((strict_strtoul(buf, 10, &val) < 0) || (val > 1))
		return -EINVAL;

	mutex_lock(&hscdtd->lock);
	if(val)
	{
		hscdtd_power_on(hscdtd);
		hscdtd_measure_start(hscdtd);
	}
	else
	{
		hscdtd_power_off(hscdtd);
		hscdtd_measure_stop(hscdtd);
	}
	mutex_unlock(&hscdtd->lock);

	return count;
}

static DEVICE_ATTR(power_state, 0664, NULL, hscdtd_power_store);

static struct attribute *hscdtd_android_attributes[] = {
	&dev_attr_power_state.attr,
	&dev_attr_rate.attr,
	NULL
};

static const struct attribute_group hscdtd_android_attr_group = {
	.attrs = hscdtd_android_attributes,
};

/*--------------------------------------------------------------------------
 * suspend/resume function
 *--------------------------------------------------------------------------*/
#if defined(CONFIG_PM)
static int hscdtd_suspend(struct device *dev)
{
	struct hscdtd_data *hscdtd = dev_get_drvdata(dev);
	mutex_lock(&hscdtd->lock);
	hscdtd_measure_stop(hscdtd);
	hscdtd_power_off(hscdtd);
	mutex_unlock(&hscdtd->lock);
	return 0;
}

static int hscdtd_resume(struct device *dev)
{
	struct hscdtd_data *hscdtd = dev_get_drvdata(dev);
	mutex_lock(&hscdtd->lock);
	hscdtd_power_on(hscdtd);
	mutex_unlock(&hscdtd->lock);
	return 0;
}
#endif


/*--------------------------------------------------------------------------
 * work function
 *--------------------------------------------------------------------------*/
static void hscdtd_polling(struct work_struct *work)
{
	int xyz[3];
	struct hscdtd_data *hscdtd = container_of(work, struct hscdtd_data, work_data.work);
	struct hscdtd_platform_data *pdata = hscdtd->pdata;

	dev_dbg(&hscdtd->i2c->adapter->dev,
		HSCDTD_LOG_TAG "%s, tcs_cnt = %d\n", __func__, hscdtd->tcs_cnt);

	mutex_lock(&hscdtd->lock);
	if (hscdtd->factive) {
		if (hscdtd_get_magnetic_field_data(hscdtd, xyz) == 0) {
			input_report_abs(hscdtd->input, ABS_X, xyz[pdata->x_indx]*pdata->x_dir);
			input_report_abs(hscdtd->input, ABS_Y, xyz[pdata->y_indx]*pdata->y_dir);
			input_report_abs(hscdtd->input, ABS_Z, xyz[pdata->z_indx]*pdata->z_dir);
			input_sync(hscdtd->input);
		}
		if (++hscdtd->tcs_cnt > hscdtd->tcs_thr)
			hscdtd_tcs_setup(hscdtd);
		hscdtd_force_setup(hscdtd);	/* For next "hscdtd_polling" */
		schedule_delayed_work(&hscdtd->work_data,
			msecs_to_jiffies(hscdtd->delay_msec));
	}
	mutex_unlock(&hscdtd->lock);
}


/*--------------------------------------------------------------------------
 * input device
 *--------------------------------------------------------------------------*/
static int hscdtd_open(struct input_dev *dev)
{
	struct hscdtd_data *hscdtd = input_get_drvdata(dev);

	dev_dbg(&hscdtd->i2c->adapter->dev,
		HSCDTD_LOG_TAG "%s\n", __func__);

	mutex_lock(&hscdtd->lock);
	hscdtd_power_on(hscdtd);
	hscdtd_measure_start(hscdtd);
	mutex_unlock(&hscdtd->lock);

	return 0;
}

static void hscdtd_close(struct input_dev *dev)
{
	struct hscdtd_data *hscdtd = input_get_drvdata(dev);

	dev_dbg(&hscdtd->i2c->adapter->dev,
		HSCDTD_LOG_TAG "%s\n", __func__);

	mutex_lock(&hscdtd->lock);
	hscdtd_power_off(hscdtd);
	hscdtd_measure_stop(hscdtd);
	mutex_unlock(&hscdtd->lock);
}


/*--------------------------------------------------------------------------
 * i2c device
 *--------------------------------------------------------------------------*/
static int verify_paltform_data(struct i2c_client *client, struct hscdtd_platform_data *pdata)
{
	uint8_t indx[3];
	int8_t dir[3];
	char xyz[3] = {'x','y','z'};
	int i;

	indx[0] = pdata->x_indx;
	indx[1] = pdata->y_indx;
	indx[2] = pdata->z_indx;

	dir[0] = pdata->x_dir;
	dir[1] = pdata->y_dir;
	dir[2] = pdata->z_dir;

	for(i=0; i<3; i++)
	{
		switch(indx[i]) {
		case HSCDTD_X_INDX:
		case HSCDTD_Y_INDX:
		case HSCDTD_Z_INDX:
			break;
		default:
			return -1;
		}

		switch(dir[i]) {
		case HSCDTD_DIR_NORMAL:
		case HSCDTD_DIR_INVERT:
			break;
		default:
			return -1;
		}
	}
	dev_info(&client->adapter->dev,	HSCDTD_LOG_TAG "X:%c%c Y:%c%c Z:%c%c\n",
		pdata->x_dir==HSCDTD_DIR_NORMAL?' ':'-', xyz[pdata->x_indx],
		pdata->y_dir==HSCDTD_DIR_NORMAL?' ':'-', xyz[pdata->y_indx],
		pdata->z_dir==HSCDTD_DIR_NORMAL?' ':'-', xyz[pdata->z_indx]);

	return 0;
}

static int hscdtd_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	int rc;
	struct hscdtd_data *hscdtd;
	struct hscdtd_platform_data *pdata = dev_get_platdata(&client->dev);

	dev_dbg(&client->adapter->dev,
		HSCDTD_LOG_TAG "%s\n", __func__);

	if(!pdata)
	{
		dev_err(&client->adapter->dev, "%s: no platform data\n", __func__);
		return -ENOENT;
	}
	
	if(verify_paltform_data(client, pdata))
	{
		dev_err(&client->adapter->dev, "%s: wrong platform data\n", __func__);
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->adapter->dev, "client not i2c capable\n");
		rc = -ENODEV;
		goto out_region;
	}

	hscdtd = kzalloc(sizeof(struct hscdtd_data), GFP_KERNEL);
	if (!hscdtd) {
		dev_err(&client->adapter->dev,
			"failed to allocate memory for module data\n");
		rc = -ENOMEM;
		goto out_region;
	}
	hscdtd->i2c = client;
	hscdtd->pdata = pdata;
	i2c_set_clientdata(client, hscdtd);

	mutex_init(&hscdtd->lock);

	hscdtd->delay_msec = HSCDTD_INITIALL_DELAY;
	hscdtd->tcs_thr = HSCDTD_TCS_TIME / hscdtd->delay_msec;
	hscdtd->tcs_cnt = 0;

	hscdtd_power_on(hscdtd);
	rc = hscdtd_register_init(hscdtd);
	if (rc) {
		rc = -ENOMEM;
		dev_err(&client->adapter->dev, "hscdtd_register_init\n");
		goto out_kzalloc;
	}
#if defined(CONFIG_PM)
	hscdtd_power_off(hscdtd);
#endif
	dev_dbg(&client->adapter->dev,
		"initialize %s sensor\n", HSCDTD_DRIVER_NAME);

	hscdtd->input = input_allocate_device();
	if (!hscdtd->input) {
		rc = -ENOMEM;
		dev_err(&client->adapter->dev, "input_allocate_device\n");
		goto out_kzalloc;
	}
	dev_dbg(&client->adapter->dev, "input_allocate_device\n");

	input_set_drvdata(hscdtd->input, hscdtd);

	hscdtd->input->name		= HSCDTD_DRIVER_NAME;
	hscdtd->input->open		= hscdtd_open;
	hscdtd->input->close		= hscdtd_close;
	hscdtd->input->id.bustype	= BUS_I2C;
	hscdtd->input->evbit[0]		= BIT_MASK(EV_ABS);

	input_set_abs_params(hscdtd->input, ABS_X,
			HSCDTD_MIN_LSB, HSCDTD_MAX_LSB, 0, 0);
	input_set_abs_params(hscdtd->input, ABS_Y,
			HSCDTD_MIN_LSB, HSCDTD_MAX_LSB, 0, 0);
	input_set_abs_params(hscdtd->input, ABS_Z,
			HSCDTD_MIN_LSB, HSCDTD_MAX_LSB, 0, 0);

	rc = input_register_device(hscdtd->input);
	if (rc) {
		rc = -ENOMEM;
		dev_err(&client->adapter->dev, "input_register_device\n");
		goto out_idev_allc;
	}
	dev_dbg(&client->adapter->dev, "input_register_device\n");

	INIT_DELAYED_WORK(&hscdtd->work_data, hscdtd_polling);


	rc = sysfs_create_group(&hscdtd->input->dev.kobj, &hscdtd_android_attr_group);
	if (rc) {
		rc = -ENOMEM;
		dev_err(&client->adapter->dev, "sysfs_create_group\n");
		goto out_idev_reg;
	}
	dev_dbg(&client->adapter->dev, "sysfs_create_group\n");

	hscdtd->factive = false;
	dev_info(&client->adapter->dev,
		HSCDTD_LOG_TAG "detected %s geomagnetic field sensor\n",
		HSCDTD_DRIVER_NAME);

	return 0;

out_idev_reg:
	input_unregister_device(hscdtd->input);
out_idev_allc:
	input_free_device(hscdtd->input);
out_kzalloc:
	kfree(hscdtd);
out_region:

	return rc;
}

static int hscdtd_remove(struct i2c_client *client)
{
	struct hscdtd_data *hscdtd = i2c_get_clientdata(client);

	dev_dbg(&client->adapter->dev, "%s\n", __func__);
	sysfs_remove_group(&hscdtd->input->dev.kobj, &hscdtd_android_attr_group);
	input_unregister_device(hscdtd->input);
	input_free_device(hscdtd->input);
	hscdtd_measure_stop(hscdtd);
	hscdtd_power_off(hscdtd);
	kfree(hscdtd);
	return 0;
}

static void hscdtd_shutdown(struct i2c_client *client)
{
}


static const struct i2c_device_id HSCDTD_id[] = {
	{ HSCDTD_DRIVER_NAME, 0 },
	{ }
};

#if defined(CONFIG_PM)
static const struct dev_pm_ops hscdtd_pm_ops = {
	.suspend	= hscdtd_suspend,
	.resume		= hscdtd_resume,
};
#endif

static struct i2c_driver hscdtd_driver = {
	.probe		= hscdtd_probe,
	.remove		= hscdtd_remove,
	.shutdown	= hscdtd_shutdown,
	.id_table	= HSCDTD_id,
	.driver		= {
		.name = HSCDTD_DRIVER_NAME,
#if defined(CONFIG_PM)
		.pm = &hscdtd_pm_ops,
#endif
	},
};

static int __init hscdtd_init(void)
{
	return i2c_add_driver(&hscdtd_driver);
}

static void __exit hscdtd_exit(void)
{
	i2c_del_driver(&hscdtd_driver);
}

module_init(hscdtd_init);
module_exit(hscdtd_exit);

MODULE_DESCRIPTION("Alps Geomagnetict Device");
MODULE_AUTHOR("ALPS ELECTRIC CO., LTD.");
MODULE_LICENSE("GPL v2");
