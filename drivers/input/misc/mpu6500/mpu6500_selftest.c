/*
	$License:
	Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	$
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/irq.h>
#include <linux/gpio.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include <linux/mpu6500_input.h>

#define VERBOSE_OUT 1

#define X (0)
#define Y (1)
#define Z (2)

/*--- Test parameters defaults. See set_test_parameters for more details ---*/

#define DEF_MPU_ADDR             (0x68)	/* I2C address of the mpu     */

#define DEF_GYRO_FULLSCALE       (2000)	/* gyro full scale dps        */
#define DEF_GYRO_SENS            (32768 / DEF_GYRO_FULLSCALE)
    /* gyro sensitivity LSB/dps   */
#define DEF_PACKET_THRESH        (75)	/* 75 ms / (1ms / sample) OR
					   600 ms / (8ms / sample)   */
#define DEF_TOTAL_TIMING_TOL     (3)	/* 3% = 2 pkts + 1% proc tol. */

#define DEF_BIAS_THRESH_SELF     (20)	/* dps */
#define DEF_BIAS_THRESH_CAL      (20)

#define DEF_BIAS_LSB_THRESH_SELF (DEF_BIAS_THRESH_SELF*DEF_GYRO_SENS)
#define DEF_BIAS_LSB_THRESH_CAL     (DEF_BIAS_THRESH_CAL*DEF_GYRO_SENS)

/* 0.4 dps-rms in LSB-rms	  */
#define DEF_RMS_THRESH_SELF     (5)	/* dps : spec  is 0.4dps_rms */
#define DEF_RMS_LSB_THRESH_SELF (DEF_RMS_THRESH_SELF*DEF_GYRO_SENS)

#define DEF_TESTS_PER_AXIS       (1)	/* num of periods used to test
					   each axis */
#define DEF_N_ACCEL_SAMPLES      (20)	/* num of accel samples to
					   average from, if applic.   */
#define ML_INIT_CAL_LEN          (36)	/* length in bytes of
					   calibration data file      */
#define DEF_PERIOD_SELF          (75)	/* ms of time, self test */
#define DEF_PERIOD_CAL           (600)	/* ms of time, full cal */

#define DEF_SCALE_FOR_FLOAT (1000)
#define DEF_RMS_SCALE_FOR_RMS (10000)
#define DEF_SQRT_SCALE_FOR_RMS (100)

/*
    Types
*/
struct mpu6500_selftest_info {
	int gyro_sens;
	int gyro_fs;
	int packet_thresh;
	int total_timing_tol;
	int bias_thresh;
	int rms_thresh;
	unsigned int tests_per_axis;
	unsigned short accel_samples;
};

struct mpu6500_selftest {
	unsigned char pwm_mgmt[2];
	unsigned char smplrt_div;
	unsigned char user_ctrl;
	unsigned char config;
	unsigned char gyro_config;
	unsigned char int_enable;
};

/*
    Global variables
*/

static struct mpu6500_selftest mpu6500_selftest;

short mpu6500_big8_to_int16(const unsigned char *big8)
{
	short x;
	x = ((short)big8[0] << 8) | ((short)big8[1]);
	return x;
}

static int mpu6500_backup_register(struct i2c_client *client)
{
	int result = 0;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_PWR_MGMT_1,
				 2, mpu6500_selftest.pwm_mgmt);
	if (result)
		return result;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_CONFIG,
				 1, &mpu6500_selftest.config);
	if (result)
		return result;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_GYRO_CONFIG,
				 1, &mpu6500_selftest.gyro_config);
	if (result)
		return result;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_USER_CTRL,
				 1, &mpu6500_selftest.user_ctrl);
	if (result)
		return result;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_INT_ENABLE,
				 1, &mpu6500_selftest.int_enable);
	if (result)
		return result;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_SMPLRT_DIV,
				 1, &mpu6500_selftest.smplrt_div);
	if (result)
		return result;

	return result;
}

static int mpu6500_recover_register(struct i2c_client *client)
{
	int result = 0;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_1,
					 mpu6500_selftest.pwm_mgmt[0]);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_2,
					 mpu6500_selftest.pwm_mgmt[1]);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_CONFIG,
					 mpu6500_selftest.config);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_GYRO_CONFIG,
					 mpu6500_selftest.gyro_config);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_USER_CTRL,
					 mpu6500_selftest.user_ctrl);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_SMPLRT_DIV,
					 mpu6500_selftest.smplrt_div);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_INT_ENABLE,
					 mpu6500_selftest.int_enable);
	if (result)
		return result;

	return result;
}

u32 mpu6500_selftest_sqrt(u32 sqsum)
{
	u32 sq_rt;

	int g0, g1, g2, g3, g4;
	int seed;
	int next;
	int step;

	g4 = sqsum / 100000000;
	g3 = (sqsum - g4 * 100000000) / 1000000;
	g2 = (sqsum - g4 * 100000000 - g3 * 1000000) / 10000;
	g1 = (sqsum - g4 * 100000000 - g3 * 1000000 - g2 * 10000) / 100;
	g0 = (sqsum - g4 * 100000000 - g3 * 1000000 - g2 * 10000 - g1 * 100);

	next = g4;
	step = 0;
	seed = 0;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = seed * 10000;
	next = (next - (seed * step)) * 100 + g3;

	step = 0;
	seed = 2 * seed * 10;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 1000;
	next = (next - seed * step) * 100 + g2;
	seed = (seed + step) * 10;
	step = 0;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 100;
	next = (next - seed * step) * 100 + g1;
	seed = (seed + step) * 10;
	step = 0;

	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 10;
	next = (next - seed * step) * 100 + g0;
	seed = (seed + step) * 10;
	step = 0;

	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step;

	return sq_rt;
}

int mpu6500_selftest_run(struct i2c_client *client,
			 int packet_cnt[3],
			 int gyro_bias[3],
			 int gyro_rms[3], int gyro_lsb_bias[3])
{
	int ret_val = 0;
	int result;
	int total_count = 0;
	int total_count_axis[3] = { 0, 0, 0
	};
	int packet_count;
	short x[DEF_PERIOD_CAL * DEF_TESTS_PER_AXIS / 8 * 4] = { 0
	};
	short y[DEF_PERIOD_CAL * DEF_TESTS_PER_AXIS / 8 * 4] = { 0
	};
	short z[DEF_PERIOD_CAL * DEF_TESTS_PER_AXIS / 8 * 4] = { 0
	};
	int temperature = 0;
	long avg[3];
	long rms[3];

	int i, j, tmp;
	char tmpStr[200];
	unsigned char regs[7] = { 0
	};
	unsigned char dataout[20];
	int perform_full_test = 0;
	struct mpu6500_selftest_info test_setup = {
		DEF_GYRO_SENS, DEF_GYRO_FULLSCALE, DEF_PACKET_THRESH,
		DEF_TOTAL_TIMING_TOL, (int)DEF_BIAS_THRESH_SELF,
		DEF_RMS_LSB_THRESH_SELF * DEF_RMS_LSB_THRESH_SELF,
		/* now obsolete - has no effect */
		DEF_TESTS_PER_AXIS, DEF_N_ACCEL_SAMPLES
	};

	char a_name[3][2] = { "X", "Y", "Z" };

	/*backup registers */
	result = mpu6500_backup_register(client);
	if (result) {
		printk(KERN_ERR "register backup error=%d", result);
		return result;
	}

	if (mpu6500_selftest.pwm_mgmt[0] & 0x40) {
		result =
		    mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_1,
						 0x00);
		if (result) {
			printk(KERN_INFO "init PWR_MGMT error=%d", result);
			return result;
		}
	}

	regs[0] =
	    mpu6500_selftest.
	    pwm_mgmt[1] & ~(BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);
	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_2, regs[0]);

	/* make sure the DMP is disabled first */
	result = mpu6500_i2c_write_single_reg(client, MPUREG_USER_CTRL, 0x00);
	if (result) {
		printk(KERN_INFO "DMP disable error=%d", result);
		return result;
	}

	/* reset the gyro offset values */
	regs[0] = MPUREG_XG_OFFS_USRH;
	result = mpu6500_i2c_write(client, 6, regs);
	if (result)
		return result;

	/* sample rate */
	if (perform_full_test) {

		/* = 8ms */
		result =
		    mpu6500_i2c_write_single_reg(client, MPUREG_SMPLRT_DIV,
						 0x07);
		test_setup.bias_thresh = DEF_BIAS_LSB_THRESH_CAL;
	} else {

		/* = 1ms */
		result =
		    mpu6500_i2c_write_single_reg(client, MPUREG_SMPLRT_DIV,
						 0x00);
		test_setup.bias_thresh = DEF_BIAS_LSB_THRESH_SELF;
	}
	if (result)
		return result;

	regs[0] = 0x03;		/* filter = 42Hz, analog_sample rate = 1 KHz */
	switch (test_setup.gyro_fs) {
	case 2000:
		regs[0] |= 0x18;
		break;
	case 1000:
		regs[0] |= 0x10;
		break;
	case 500:
		regs[0] |= 0x08;
		break;
	case 250:
	default:
		regs[0] |= 0x00;
		break;
	}
	result = mpu6500_i2c_write_single_reg(client, MPUREG_CONFIG, regs[0]);
	if (result)
		return result;

	switch (test_setup.gyro_fs) {
	case 2000:
		regs[0] = 0x03;
		break;
	case 1000:
		regs[0] = 0x02;
		break;
	case 500:
		regs[0] = 0x01;
		break;
	case 250:
	default:
		regs[0] = 0x00;
		break;
	}
	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_GYRO_CONFIG,
					 regs[0] << 3);
	if (result)
		return result;
	result = mpu6500_i2c_write_single_reg(client, MPUREG_INT_ENABLE, 0x00);
	if (result)
		return result;

	/* 1st, timing test */
	for (j = 0; j < 3; j++) {
		printk(KERN_INFO "Collecting gyro data from %s gyro PLL\n",
		       a_name[j]);

		/* turn on all gyros, use gyro X for clocking
		   Set to Y and Z for 2nd and 3rd iteration */
		result =
		    mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_1,
						 j + 1);
		if (result)
			return result;

		/* wait for 50 ms after switching clock source */
		mpu6500_msleep(250);

		/* enable & reset FIFO */
		result =
		    mpu6500_i2c_write_single_reg(client, MPUREG_USER_CTRL,
						 BIT_FIFO_EN | BIT_FIFO_RST);
		if (result)
			return result;

		tmp = test_setup.tests_per_axis;
		while (tmp-- > 0) {
			const unsigned char fifo_en_reg = MPUREG_FIFO_EN;

			/* enable XYZ gyro in FIFO and nothing else */
			result =
			    mpu6500_i2c_write_single_reg(client, fifo_en_reg,
							 BIT_GYRO_XOUT |
							 BIT_GYRO_YOUT |
							 BIT_GYRO_ZOUT);
			if (result)
				return result;

			/* wait one period for data */
			if (perform_full_test)
				mpu6500_msleep(DEF_PERIOD_CAL);

			else
				mpu6500_msleep(DEF_PERIOD_SELF);

			/* stop storing gyro in the FIFO */
			result =
			    mpu6500_i2c_write_single_reg(client,
							 fifo_en_reg, 0x00);
			if (result)
				return result;

			/* Getting number of bytes in FIFO */
			result =
			    mpu6500_i2c_read_reg(client, MPUREG_FIFO_COUNTH,
						 2, dataout);
			if (result)
				return result;

			/* number of 6 B packets in the FIFO */
			packet_count = mpu6500_big8_to_int16(dataout) / 6;
			sprintf(tmpStr, "Packet Count: %d - ", packet_count);
			if ((packet_count - test_setup.packet_thresh) > 0 ||
				(abs(packet_count - test_setup.packet_thresh)
			    * 100 <=
			    /* Within total_timing_tol % range, rounded up */
			    (int)(test_setup.total_timing_tol *
				  test_setup.packet_thresh + 1))) {
				for (i = 0; i < packet_count; i++) {
					/* getting FIFO data */
					result =
					    mpu6500_i2c_read_fifo(client, 6,
								  dataout);
					if (result)
						return result;
					x[total_count + i] =
					    mpu6500_big8_to_int16(&dataout[0]);
					y[total_count + i] =
					    mpu6500_big8_to_int16(&dataout[2]);
					z[total_count + i] =
					    mpu6500_big8_to_int16(&dataout[4]);
				}
				total_count += packet_count;
				total_count_axis[j] += packet_count;
				packet_cnt[j] = packet_count;
				sprintf(tmpStr, "%sOK", tmpStr);
			} else {
				ret_val |= 1 << j;
				sprintf(tmpStr, "%sNOK - samples ignored",
					tmpStr);
			}
			printk(KERN_INFO "%s\n", tmpStr);
		}

		/* remove gyros from FIFO */
		result =
		    mpu6500_i2c_write_single_reg(client, MPUREG_FIFO_EN, 0x00);
		if (result)
			return result;

		/* Read Temperature */
		result =
		    mpu6500_i2c_read_reg(client, MPUREG_TEMP_OUT_H, 2, dataout);
		if (result)
			return result;
		temperature += (short)mpu6500_big8_to_int16(dataout);
	}

	printk(KERN_INFO "\n");
	printk(KERN_INFO "Total %d samples\n", total_count);

	/* 2nd, check bias from X, Y, and Z PLL clock source */
	tmp = total_count != 0 ? total_count : 1;
	for (i = 0, avg[X] = 0, avg[Y] = 0, avg[Z] = 0; i < total_count; i++) {
		avg[X] += x[i];
		avg[Y] += y[i];
		avg[Z] += z[i];
	}
	avg[X] /= tmp;
	avg[Y] /= tmp;
	avg[Z] /= tmp;

	printk(KERN_INFO "bias : %+8ld %+8ld %+8ld (LSB)\n", avg[X],
	       avg[Y], avg[Z]);

	gyro_bias[X] = (avg[X] * DEF_SCALE_FOR_FLOAT) / DEF_GYRO_SENS;
	gyro_bias[Y] = (avg[Y] * DEF_SCALE_FOR_FLOAT) / DEF_GYRO_SENS;
	gyro_bias[Z] = (avg[Z] * DEF_SCALE_FOR_FLOAT) / DEF_GYRO_SENS;
	gyro_lsb_bias[X] = avg[X];
	gyro_lsb_bias[Y] = avg[Y];
	gyro_lsb_bias[Z] = avg[Z];

	if (VERBOSE_OUT) {
		printk(KERN_INFO
		       "abs bias : %+8d.%03d   %+8d.%03d  %+8d.%03d (dps)\n",
		       (int)abs(gyro_bias[X]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_bias[X]) % DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_bias[Y]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_bias[Y]) % DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_bias[Z]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_bias[Z]) % DEF_SCALE_FOR_FLOAT);
	}

	for (j = 0; j < 3; j++) {
		if (abs(avg[j]) > test_setup.bias_thresh) {
			printk(KERN_INFO
			       "%s-Gyro bias (%ld) exceeded threshold "
			       "(threshold = %d LSB)\n", a_name[j], avg[j],
			       test_setup.bias_thresh);
			ret_val |= 1 << (3 + j);
		}
	}

	/* 3rd, check RMS for dead gyros
	   If any of the RMS noise value returns zero,
	   then we might have dead gyro or FIFO/register failure,
	   the part is sleeping, or the part is not responsive */
	for (i = 0, rms[X] = 0, rms[Y] = 0, rms[Z] = 0; i < total_count; i++) {
		rms[X] += (long)(x[i] - avg[X]) * (x[i] - avg[X]);
		rms[Y] += (long)(y[i] - avg[Y]) * (y[i] - avg[Y]);
		rms[Z] += (long)(z[i] - avg[Z]) * (z[i] - avg[Z]);
	}

	if (rms[X] == 0 || rms[Y] == 0 || rms[Z] == 0)
		ret_val |= 1 << 6;

	if (VERBOSE_OUT) {
		printk(KERN_INFO "RMS ^ 2 : %+8ld %+8ld %+8ld\n",
		       (long)rms[X] / total_count,
		       (long)rms[Y] / total_count, (long)rms[Z] / total_count);
	}

	{
		int dps_rms[3] = { 0 };
		u32 tmp;
		int i = 0;

		for (j = 0; j < 3; j++) {
			if (rms[j] / total_count > test_setup.rms_thresh) {
				printk(KERN_INFO
				       "%s-Gyro rms (%ld) exceeded threshold "
				       "(threshold = %d LSB)\n", a_name[j],
				       rms[j] / total_count,
				       test_setup.rms_thresh);
				ret_val |= 1 << (7 + j);
			}
		}

		for (i = 0; i < 3; i++) {
			if (rms[i] > 10000) {
				tmp =
				    ((u32) (rms[i] / total_count)) *
				    DEF_RMS_SCALE_FOR_RMS;
			} else {
				tmp =
				    ((u32) (rms[i] * DEF_RMS_SCALE_FOR_RMS)) /
				    total_count;
			}

			if (rms[i] < 0)
				tmp = 1 << 31;

			dps_rms[i] = mpu6500_selftest_sqrt(tmp) / DEF_GYRO_SENS;

			gyro_rms[i] =
			    dps_rms[i] * DEF_SCALE_FOR_FLOAT /
			    DEF_SQRT_SCALE_FOR_RMS;
		}

		printk(KERN_INFO
		       "RMS : %+8d.%03d	 %+8d.%03d  %+8d.%03d (dps)\n",
		       (int)abs(gyro_rms[X]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_rms[X]) % DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_rms[Y]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_rms[Y]) % DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_rms[Z]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_rms[Z]) % DEF_SCALE_FOR_FLOAT);
	}

	/* 4th, temperature average */
	temperature /= 3;

	/*recover registers */
	result = mpu6500_recover_register(client);
	if (result) {
		printk(KERN_ERR "register recovering error=%d", result);
		return result;
	}

	return ret_val;
}
