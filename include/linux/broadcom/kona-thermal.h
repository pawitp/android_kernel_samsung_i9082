/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
*       @file   include/linux/broadcom/kona-thermal.h
*
* Unless you and Broadcom execute a separate written software license agreement
* governing use of this software, this software is licensed to you under the
* terms of the GNU General Public License version 2, available at
* http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*******************************************************************************/

/*
*
*****************************************************************************
*
* kona-thermal.h
*
* PURPOSE:
*
*
*
* NOTES:
*
* ****************************************************************************/

#ifndef __KONA_THERMAL_H__
#define __KONA_THERMAL_H__

enum sensor_type {
	SENSOR_BB_TMON,		/*Kona TMON for baseband temp monitoring */
	SENSOR_BB_MULTICORE,	/*allow for multi-core */
	SENSOR_PMU,
	SENSOR_BATTERY,
	SENSOR_CRYSTAL,		/* 32KHz, 26Mhz */
	SENSOR_PA,
	SENSOR_OTHER		/* allow for growth */
};

enum sensor_read_type {
	SENSOR_READ_DIRECT,	/* Direct register access */
	SENSOR_READ_PMU_I2C,	/* I2C access           */
	SENSOR_READ_SPI		/* SPI access           */
};

struct temp_threshold {
	unsigned temp_low;	/*undertemp  threshold */
	unsigned temp_high;	/*overtemp  threshold */
};

enum threshold_type {
	/* current enum allows two warning and one fatal alarm */
	/* threshold but this can be expanded , if necessary */
	WARNING_1,
	WARNING_2,
	FATAL
};

enum temperature_type {
	LOW,			/* undertemp threshold */
	HIGH			/* overtemp threshold */
};

enum therm_action_type {
	THERM_ACTION_NONE,
	THERM_ACTION_NOTIFY,
	THERM_ACTION_NOTIFY_SHUTDOWN,
	THERM_ACTION_SHUTDOWN
};

enum sensor_control_type {
	/*Interval located in separate field */
	SENSOR_PERIODIC_READ,
	/*Interrupt detection of temperature violation */
	SENSOR_INTERRUPT,
	/* synchronize sensor reading to DRX cycles */
	SENSOR_DRX_SYNC
};

struct thermal_sensor_config {
	unsigned thermal_id;
	char *thermal_name;
	enum sensor_type thermal_type;
	unsigned thermal_mc;
	enum sensor_read_type thermal_read;
	unsigned thermal_location;
	int long thermal_warning_lvl_1;
	int long thermal_warning_lvl_2;
	int long thermal_fatal_lvl;
	int long thermal_lowest;
	int long thermal_highest;
	enum therm_action_type thermal_warning_action;
	enum therm_action_type thermal_fatal_action;
	unsigned thermal_sensor_param;
	enum sensor_control_type thermal_control;
	unsigned (*convert_callback) (int raw);
};

struct therm_data {
	unsigned flags;
	int thermal_update_interval;
	unsigned num_sensors;
	struct thermal_sensor_config *sensors;
};
#endif

unsigned int thermal_get_adc(int adc_sig);
