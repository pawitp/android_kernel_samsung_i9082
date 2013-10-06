/*
 * bq24272_charger.h
 * Samsung BQ24272 Charger Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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



/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#define SEC_CHARGER_I2C_SLAVEADDR	0x6b
#define CHARGER_I2C_BUS_ID 3
#define GPIO_RQ24272_INT 142



#define RQ24272_STATUS_CTRL		0x00
#define RQ24272_BATT_STATUS		0x01
#define RQ24272_CONTROL			0x02
#define RQ24272_VOLTAGE			0x03
#define RQ24272_VENDER				0x04
#define RQ24272_CHG_CURRENT		0x05
#define RQ24272_VOLT_STATUS		0x06
#define RQ24272_SAFETY				0x07

#define STAT_IN_READY				(1<<4)
#define STAT_CHARGING				(3<<3)
#define STAT_CHARGE_DONE			(5<<3)
#define STAT_FAULT					(7<<3)

#define CTRL_CHG_ENABLE			(0<<1)
#define CTRL_CHG_DISABLE			(1<<1)

#define CHG_640MV						(1<<7)
#define CHG_320MV						(1<<6)
#define CHG_160MV						(1<<5)
#define CHG_80MV							(1<<4)
#define CHG_40MV							(1<<3)
#define CHG_20MV							(1<<2)
#define CHG_350MV							0
#define INLIMIT_IN_1_5A				(0<<1)
#define INLIMIT_IN_2_5A				(1<<1)

#define CHG_VOLTAGE_4_3V			(CHG_350MV|CHG_640MV|CHG_160MV)
#define CHG_VOLTAGE_4_36V			(CHG_350MV|CHG_640MV|CHG_160MV|CHG_40MV|CHG_20MV)
#define CHG_VOLTAGE_4_38V			(CHG_350MV|CHG_640MV|CHG_160MV|CHG_80MV)
#define CHG_VOLTAGE_4_4V			(CHG_VOLTAGE_4_3V|CHG_80MV|CHG_20MV)


#define CHG_1200MA						(1<<7)
#define CHG_600MA						(1<<6)
#define CHG_300MA						(1<<5)
#define CHG_150MA						(1<<4)
#define CHG_75MA							(1<<3)
#define CHG_550MA							0

#define EOC_200MA					(1<<2)
#define EOC_100MA					(1<<1)
#define EOC_50MA					(1<<0)
#define BASE_EOC_50MA					0




#define CHG_CURRENT_1000mA		(CHG_550MA|CHG_300MA|CHG_150MA)
#define CHG_CURRENT_1600mA		(CHG_CURRENT_1000mA|CHG_600MA)
#define CHG_CURRENT_1750mA		(CHG_1200MA|CHG_550MA)
#define CHG_CURRENT_2200mA		(CHG_CURRENT_1000mA|CHG_1200MA)
#define CHG_CURRENT_2275mA		(CHG_CURRENT_2200mA|CHG_75MA)
#define CHG_CURRENT_2350mA		(CHG_1200MA|CHG_600MA|CHG_550MA)

#define DISABLE_SAFE_TIMERS		(3<<5)
#define TS_DISABLE					(0<<3)


struct rq24272_platform_data {
	unsigned int irq;
	
};


struct sec_charger_info {
	struct i2c_client		*client;
	struct rq24272_platform_data *pdata;
	struct delayed_work isr_work;

	int cable_type;
	bool is_charging;

	/* register programming */
	int reg_addr;
	int reg_data;
};


