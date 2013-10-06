/*
 *  bq24272_charger.c
 *  Samsung BQ24272 Charger Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
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
#define DEBUG
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/input.h>
#include <linux/bq24272_charger.h>


struct i2c_client * global_client;

static int bq24272_write_reg(struct i2c_client *client, u8 reg, u8 data)
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
               printk("\n [bq24272] i2c Write Failed (ret=%d) \n", ret);
               return -1;
       }
       
       return ret;
}
static int bq24272_read_reg(struct i2c_client *client, u8 reg, u8 *data)
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
               printk("\n [bq24272] i2c Read Failed (ret=%d) \n", ret);
               return -1;
       }
       *data = buf[0];

       return 0;
}

static void bq24272_test_read(struct i2c_client *client)
{
	u8 data = 0,reg;
	int addr = 0;

	for (reg = 0; reg <= 0x07; reg++) {
		bq24272_read_reg(client, reg, &data);
		pr_info("bq24157 addr : 0x%02x data : 0x%02x\n", reg, data);
	}
}
void bq24272_stop_chg()
{
	struct i2c_client * client;
	u8 data=0;
	
	client = global_client;
	if( !client) return;

	pr_info("%s\n",__func__);
	
	bq24272_read_reg(client, RQ24272_CONTROL, &data);
	data |= CTRL_CHG_DISABLE;
	bq24272_write_reg(client,RQ24272_CONTROL,data);		
	
	bq24272_test_read(client);
}
void bq24272_start_chg(int type)
{
	struct i2c_client * client;
	u8 data=0;
	int temp;
	client = global_client;
	if( !client) return;

	pr_info("%s : %s\n",__func__,(type?"TA":"USB"));
	
	data = CHG_VOLTAGE_4_36V | INLIMIT_IN_2_5A;
	bq24272_write_reg(client,RQ24272_VOLTAGE,data);

	if( type ) // TA
	{
		data = CHG_CURRENT_1750mA|EOC_100MA|BASE_EOC_50MA;
		bq24272_write_reg(client,RQ24272_CHG_CURRENT,data);		
	}
	else  // USB
	{
		data = CHG_550MA|EOC_100MA|BASE_EOC_50MA;
		bq24272_write_reg(client,RQ24272_CHG_CURRENT,data);		
	
	}


	bq24272_read_reg(client, RQ24272_CONTROL, &data);
	data &= CTRL_CHG_ENABLE;
	bq24272_write_reg(client,RQ24272_CONTROL,data);		
	
	bq24272_test_read(client);
	
}



EXPORT_SYMBOL(bq24272_start_chg);

bool bq24272_chg_init(struct i2c_client *client)
{
	u8 data=0;
	
	data = CHG_VOLTAGE_4_36V | INLIMIT_IN_2_5A;
	bq24272_write_reg(client,RQ24272_VOLTAGE,data);

	data = DISABLE_SAFE_TIMERS|TS_DISABLE;
	bq24272_write_reg(client,RQ24272_SAFETY,data);

	bq24272_test_read(client);
	
	return true;
}

static int __devinit bq24272_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct sec_charger_info *charger;
	int ret=0;
	u8 reg;

	pr_info("bq24272_probe\n");
	global_client = client;
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	charger->client = client;
	charger->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, charger);

	bq24272_chg_init(charger->client);

	return ret;
}

static int __devexit bq24272_remove(struct i2c_client *client)
{

	return 0;
}

static int bq24272_resume(struct i2c_client *client)
{
	return 0;
}


static const struct i2c_device_id bq24272_id[] = {
	{"bq24272", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fsa9485_id);

static struct i2c_driver bq24272_i2c_driver = {
	.driver = {
		.name = "bq24272",
	},
	.probe = bq24272_probe,
	.remove = __devexit_p(bq24272_remove),
	.resume = bq24272_resume,
	.id_table = bq24272_id,
};

static int __init bq24272_init(void)
{
	return i2c_add_driver(&bq24272_i2c_driver);
}
module_init(bq24272_init);

static void __exit bq24272_exit(void)
{
	i2c_del_driver(&bq24272_i2c_driver);
}
module_exit(bq24272_exit);

MODULE_AUTHOR("Sehyoung Park <sh16.park@samsung.com>");
MODULE_DESCRIPTION("BQ24272 USB Switch driver");
MODULE_LICENSE("GPL");


