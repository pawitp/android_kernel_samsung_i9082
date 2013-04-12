/*
* drivers/misc/lps331wp.h
*
* STMicroelectronics LPS331AP Pressure / Temperature Sensor module driver
*
* Copyright (C) 2010 STMicroelectronics- MSH - Motion Mems BU - Application Team
* Matteo Dameno (matteo.dameno@st.com)
* Carmine Iascone (carmine.iascone@st.com)
*
* Both authors are willing to be considered the contact and update points for
* the driver.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
*/

#ifndef	__LPS331AP_H__
#define	__LPS331AP_H__

#define	DEBUG	1

#define	PR_ABS_MAX	8388607	/* 24 bit 2'compl */
#define	PR_ABS_MIN	-8388608

#ifdef SHRT_MAX
#define	TEMP_MAX	SHRT_MAX
#define TEMP_MIN	SHRT_MIN
#else
#define	TEMP_MAX	SHORT_MAX
#define TEMP_MIN	SHORT_MIN
#endif

#define	LPS331AP_I2C_BUS_ID  0
#define	LPS331AP__I2C_ADDRESS 0x5C

#define	WHOAMI_LPS331AP_PRS	0xBB	/*      Expctd content for WAI  */

/*	CONTROL REGISTERS	*/
#define	REF_P_XL	0x08	/*      pressure reference      */
#define	REF_P_L		0x09	/*      pressure reference      */
#define	REF_P_H		0x0A	/*      pressure reference      */
#define	REF_T_L		0x0B	/*      temperature reference   */
#define	REF_T_H		0x0C	/*      temperature reference   */

#define	WHO_AM_I	0x0F	/*      WhoAmI register         */
#define	TP_RESOL	0x10	/*      Pres Temp resolution set */
#define	DGAIN_L		0x18	/*      Dig Gain (3 regs)       */

#define	CTRL_REG1	0x20	/*      power / ODR control reg */
#define	CTRL_REG2	0x21	/*      boot reg                */
#define	CTRL_REG3	0x22	/*      interrupt control reg   */
#define	INT_CFG_REG	0x23	/*      interrupt config reg    */
#define	INT_SRC_REG	0x24	/*      interrupt source reg    */
#define	THS_P_L		0x25	/*      pressure threshold      */
#define	THS_P_H		0x26	/*      pressure threshold      */
#define	STATUS_REG	0X27	/*      status reg              */

#define	PRESS_OUT_XL	0x28	/*      press output (3 regs)   */
#define	TEMP_OUT_L	0x2B	/*      temper output (2 regs)  */
#define	COMPENS_L	0x30	/*      compensation reg (9 regs) */
#define	DELTA_T1	0x3B	/*      deltaTemp1 reg           */
#define	DELTA_T2T3	0x3F	/*      deltaTemp2, deltaTemp3 reg */
#define CALIB_SETUP	0x1E	/*      calibrationSetup reg */
#define	CALIB_STP_MASK	0x80	/*      mask to catch calibSetup info */

/*	REGISTERS ALIASES	*/
#define	P_REF_INDATA_REG	REF_P_XL
#define	T_REF_INDATA_REG	REF_T_L
#define	P_THS_INDATA_REG	THS_P_L
#define	P_OUTDATA_REG		PRESS_OUT_XL
#define	T_OUTDATA_REG		TEMP_OUT_L
#define	OUTDATA_REG		PRESS_OUT_XL

/* */
#define	LPS331AP_PRS_ENABLE_MASK	0x80	/*  ctrl_reg1 */
#define	LPS331AP_PRS_ODR_MASK		0x70	/*  ctrl_reg1 */
#define	LPS331AP_PRS_DIFF_MASK		0x08	/*  ctrl_reg1 */
#define	LPS331AP_PRS_BDU_MASK		0x04	/*  ctrl_reg1 */
#define	LPS331AP_PRS_DELTA_EN_MASK	0x02	/*  ctrl_reg1 */
#define	LPS331AP_PRS_AUTOZ_MASK		0x02	/*  ctrl_reg2 */

#define	LPS331AP_PRS_PM_NORMAL		0x80	/* Power Normal Mode */
#define	LPS331AP_PRS_PM_OFF		0x00	/* Power Down */

#define	LPS331AP_PRS_DIFF_ON		0x08	/* En Difference circuitry */
#define	LPS331AP_PRS_DIFF_OFF		0x00	/* Dis Difference circuitry */

#define	LPS331AP_PRS_AUTOZ_ON		0x02	/* En AutoZero Function */
#define	LPS331AP_PRS_AUTOZ_OFF		0x00	/* Dis Difference Function */

#define	LPS331AP_PRS_BDU_ON		0x04	/* En BDU Block Data Upd */
#define	LPS331AP_PRS_DELTA_EN_ON	0x02	/* En Delta Press registers */

#define	LPS331AP_PRS_RES_AVGTEMP_064	0X60
#define	LPS331AP_PRS_RES_AVGTEMP_128	0X70
#define	LPS331AP_PRS_RES_AVGPRES_512	0X0A

#define	LPS331AP_PRS_RES_MAX		(LPS331AP_PRS_RES_AVGTEMP_128  | \
						LPS331AP_PRS_RES_AVGPRES_512)
						/* Max Resol. for 1/7/12,5Hz */

#define	LPS331AP_PRS_RES_25HZ		(LPS331AP_PRS_RES_AVGTEMP_064  | \
						LPS331AP_PRS_RES_AVGPRES_512)
						/* Max Resol. for 25Hz */

#define	FUZZ			0
#define	FLAT			0

#define	I2C_AUTO_INCREMENT	0x80

/* RESUME STATE INDICES */
#define	LPS331AP_RES_REF_P_XL		0
#define	LPS331AP_RES_REF_P_L		1
#define	LPS331AP_RES_REF_P_H		2
#define	LPS331AP_RES_REF_T_L		3
#define	LPS331AP_RES_REF_T_H		4
#define	LPS331AP_RES_TP_RESOL		5
#define	LPS331AP_RES_CTRL_REG1		6
#define	LPS331AP_RES_CTRL_REG2		7
#define	LPS331AP_RES_CTRL_REG3		8
#define	LPS331AP_RES_INT_CFG_REG	9
#define	LPS331AP_RES_THS_P_L		10
#define	LPS331AP_RES_THS_P_H		11

#define	RESUME_ENTRIES			12
/* end RESUME STATE INDICES */

/* Pressure Sensor Operating Mode */
#define	LPS331AP_PRS_DIFF_ENABLE	1
#define LPS331AP_PRS_DIFF_DISABLE	0
#define	LPS331AP_PRS_AUTOZ_ENABLE	1
#define	LPS331AP_PRS_AUTOZ_DISABLE	0

#define LPS331AP_PRS_MIN_POLL_PERIOD_MS	1

#define	SAD0L				0x00
#define	SAD0H				0x01
#define	LPS331AP_PRS_I2C_SADROOT	0x2E
#define	LPS331AP_PRS_I2C_SAD_L		((LPS331AP_PRS_I2C_SADROOT<<1)|SAD0L)
#define	LPS331AP_PRS_I2C_SAD_H		((LPS331AP_PRS_I2C_SADROOT<<1)|SAD0H)
#define	LPS331AP_PRS_DEV_NAME		"lps331ap_prs"

/* input define mappings */
#define ABS_PR		ABS_PRESSURE
#define ABS_TEMP	ABS_GAS
#define ABS_DLTPR	ABS_MISC

/* Barometer and Termometer output data rate ODR */
#define	LPS331AP_PRS_ODR_ONESH	0x00	/* one shot both                */
#define	LPS331AP_PRS_ODR_1_1	0x10	/*  1  Hz baro,  1  Hz term ODR */
#define	LPS331AP_PRS_ODR_7_7	0x50	/*  7  Hz baro,  7  Hz term ODR */
#define	LPS331AP_PRS_ODR_12_12	0x60	/* 12.5Hz baro, 12.5Hz term ODR */
#define	LPS331AP_PRS_ODR_25_25	0x70	/* 25  Hz baro, 25  Hz term ODR */

/*	Pressure section defines		*/
/*	Pressure Sensor Operating Mode		*/
#define	LPS331AP_PRS_ENABLE		0x01
#define	LPS331AP_PRS_DISABLE		0x00

/*	Output conversion factors		*/
#define	SENSITIVITY_T		480	/* =    480 LSB/degrC   */
#define	SENSITIVITY_P		4096	/* =    LSB/mbar        */
#define	SENSITIVITY_P_SHIFT	12	/* =    4096 LSB/mbar   */
#define	TEMPERATURE_OFFSET	42.5f	/* =    42.5 degrC      */

#ifdef __KERNEL__
struct lps331ap_prs_platform_data {
	int (*init) (void);
	void (*exit) (void);
	int (*power_on) (void);
	int (*power_off) (void);

	unsigned int poll_interval;
	unsigned int min_interval;
};

#endif /* __KERNEL__ */

#endif /* __LPS331AP_H__ */
