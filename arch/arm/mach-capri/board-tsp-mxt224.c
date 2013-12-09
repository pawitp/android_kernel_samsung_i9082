/*
 * linux/arch/arm/mach-xxxx/xxxx
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/gpio.h>
//#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT224
#include <linux/regulator/consumer.h>
#include <linux/i2c/mxt224.h>
#define TSP_IRQ_READY_DELAY 45
/*-------------MXT224  TOUCH DRIVER by Xtopher----------*/

#define MXT224_MAX_MT_FINGERS 10
/*
  Configuration for MXT224-E
*/
#define MXT224E_THRESHOLD_BATT		30 // 50 ->30 20120621
#define MXT224E_THRESHOLD_CHRG		40
#define MXT224E_CALCFG_BATT		0x72 //0x42 ->0x72  20120621
#define MXT224E_CALCFG_CHRG		0x72 //0x52 ->0x72  20120621
#define MXT224E_ATCHFRCCALTHR_NORMAL		40
#define MXT224E_ATCHFRCCALRATIO_NORMAL		55
#define MXT224E_GHRGTIME_BATT		22 //27 ->22 20120621
#define MXT224E_GHRGTIME_CHRG		22
#define MXT224E_ATCHCALST		4
#define MXT224E_ATCHCALTHR		35
#define MXT224E_BLEN_BATT		32
#define MXT224E_BLEN_CHRG		16
#define MXT224E_MOVFILTER_BATT		81 //46 ->81 20120621
#define MXT224E_MOVFILTER_CHRG		47 //46 ->47 20120621
#define MXT224E_ACTVSYNCSPERX_NORMAL		24//32->24 20120621
#define MXT224E_NEXTTCHDI_NORMAL		0

static u8 t7_config_e[] = {GEN_POWERCONFIG_T7,
				48,		/* IDLEACQINT */
				255,	/* ACTVACQINT */
				25/* ACTV2IDLETO: 25 * 200ms = 5s */};
static u8 t8_config_e[] = {GEN_ACQUISITIONCONFIG_T8,
				22, 0, 5, 1, 0, 0, 5, 35, 40, 55};

/* NEXTTCHDI added */
static u8 t9_config_e[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 19, 11, 0, 32, 50, 2, 0,
				10,
				15,	/* MOVHYSTI */
				1, 46, MXT224_MAX_MT_FINGERS, 5, 40, 10, 31, 3,
				223, 1, 10, 10, 10, 10, 143, 40, 143, 80,
				18, 15, 50, 50, 3};

static u8 t15_config_e[] = {TOUCH_KEYARRAY_T15, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0};
static u8 t18_config_e[] = {SPT_COMCONFIG_T18, 0, 0};
static u8 t23_config_e[] = {TOUCH_PROXIMITY_T23, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static u8 t25_config_e[] = {SPT_SELFTEST_T25, 0, 0, 0, 0, 0, 0,
	0, 0};
static u8 t40_config_e[] = {PROCI_GRIPSUPPRESSION_T40, 0, 0,
	0, 0, 0};
static u8 t42_config_e[] = {PROCI_TOUCHSUPPRESSION_T42, 0,
	0, 0, 0, 0, 0, 0, 0};
static u8 t46_config_e[] = {SPT_CTECONFIG_T46, 0, 3, 35 /*24*/, 24, 0,
	0, 1, 0, 0};
static u8 t47_config_e[] = {PROCI_STYLUS_T47, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0};

static u8 t38_config_e[] = {SPT_USERDATA_T38, 0, 1, 15, 19, 45, 40, 0, 0};

static u8 t48_config_chrg_e[] = {PROCG_NOISESUPPRESSION_T48,
	3, 132, 0x72, 0, 0, 0, 0, 0, 10, 15,
	0, 0, 0, 6, 6, 0, 0, 64, 4, 64,
	10, 0, 9, 5, 0, 15, 0, 20, 0, 0,
	0, 0, 0, 0, 0, 40, 2,/*blen=0,threshold=50*/
	2 /*MOVHYSTI*/, 1, 47,
	MXT224_MAX_MT_FINGERS, 5, 40, 250, 250, 
	20, 20, 160, 50, 200,
	80, 18, 15, 0};

static u8 t48_config_e[] = {PROCG_NOISESUPPRESSION_T48,
	3, 132, 0x72, 25 /*27*/, 0, 0, 0, 0, 1, 2,
	0, 0, 0, 6, 6, 0, 0, 48, 4, 48,
	10, 0, 100, 5, 0, 100, 0, 5, 0, 0,
	0, 0, 0, 0, 0, 30, 2,
	2 /*MOVHYSTI*/, 1, 81,
	MXT224_MAX_MT_FINGERS, 5, 40, 250,250,
	20, 20, 160, 50, 200,
	80, 18, 15, 0};

static u8 end_config_e[] = {RESERVED_T255};

static const u8 *mxt224e_config[] = {
	t7_config_e,
	t8_config_e,
	t9_config_e,
	t15_config_e,
	t18_config_e,
	t23_config_e,
	t25_config_e,
	t40_config_e,
	t42_config_e,
	t46_config_e,
	t47_config_e,
	t48_config_e,
	t38_config_e,
	end_config_e,
};
/*
  Configuration for MXT224
*/
#define MXT224_THRESHOLD_BATT		40
#define MXT224_THRESHOLD_BATT_INIT		55
#define MXT224_THRESHOLD_CHRG		70
#define MXT224_NOISE_THRESHOLD_BATT		30
#define MXT224_NOISE_THRESHOLD_CHRG		40
#define MXT224_MOVFILTER_BATT		11
#define MXT224_MOVFILTER_CHRG		46
#define MXT224_ATCHCALST		9
#define MXT224_ATCHCALTHR		30

#if defined(CONFIG_S2VE_TSP_REV05)
#define MXT224_GPIO_TSP_INT   134
#else
#define MXT224_GPIO_TSP_INT   2
#endif

#define TSP_LDO_ON_GPIO		129

#if !defined(CONFIG_S2VE_TSP_REV05)
#define NLSX4373_EN_GPIO    14
#endif

static u8 t7_config[] = { GEN_POWERCONFIG_T7,
	48,			/* IDLEACQINT */
	255,			/* ACTVACQINT */
	25			/* ACTV2IDLETO: 25 * 200ms = 5s */
};

static u8 t8_config[] = { GEN_ACQUISITIONCONFIG_T8,
	10, 0, 5, 1, 0, 0, MXT224_ATCHCALST, MXT224_ATCHCALTHR
};				/*byte 3: 0 */

static u8 t9_config[] = { TOUCH_MULTITOUCHSCREEN_T9,
	131, 0, 0, 19, 11, 0, 32, MXT224_THRESHOLD_BATT, 2, 0,
	0,
	15,			/* MOVHYSTI */
	1, MXT224_MOVFILTER_BATT, MXT224_MAX_MT_FINGERS, 5, 40, 10, 191, 3,
	27, 2, 0, 0, 0, 0, 143, 55, 143, 90, 18
};

static u8 t18_config[] = { SPT_COMCONFIG_T18,
	0, 1
};

static u8 t20_config[] = { PROCI_GRIPFACESUPPRESSION_T20,
	7, 0, 0, 0, 0, 0, 0, 30, 20, 4, 15, 10
};

static u8 t22_config[] = { PROCG_NOISESUPPRESSION_T22,
	143, 0, 0, 0, 0, 0, 0, 3, MXT224_NOISE_THRESHOLD_BATT, 0, 0, 29, 34, 39,
	49, 58, 3
};

static u8 t28_config[] = { SPT_CTECONFIG_T28,
			   0, 0, 3, 16, 19, 60
};
static u8 end_config[] = { RESERVED_T255 };

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t18_config,
	t20_config,
	t22_config,
	t28_config,
	end_config,
};

struct mxt224_callbacks *charger_callbacks;
void tsp_charger_infom(bool en)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, en);
	pr_debug("[TSP] %s - %s\n", __func__,
		en ? "on" : "off");
}

void tsp_register_callback(struct mxt224_callbacks *cb)
{
	charger_callbacks = cb;
}

static bool is_cable_attached = false;

void tsp_read_ta_status(void *ta_status)
{
	*(bool *)ta_status = is_cable_attached;
}

static struct regulator *tsp_regulator_1_8=NULL;
#if !defined(CONFIG_S2VE_TSP_REV05)
static struct regulator *tsp_regulator_2_8=NULL;
#endif
static struct regulator *tsp_regulator_3_3=NULL;

static int TSP_PowerOnOff(int on)
{
	int ret;

	printk("[TSP] %s, onoff=%d\n",__func__, on);
	
	if(on)
	{
#if defined (CONFIG_TOUCHSCREEN_POWER_USING_PMULDO)

#if defined(CONFIG_S2VE_TSP_REV05)

	if(tsp_regulator_1_8 == NULL)
	{
		tsp_regulator_1_8 = regulator_get(NULL, "tsp_vdd_1.8v");
		if(IS_ERR(tsp_regulator_1_8)){
			printk("[TSP] can not get TSP VDD 1.8V\n"); /*touch, touch key power*/
			tsp_regulator_1_8 = NULL;
			return -1;
			}	
			
		ret = regulator_set_voltage(tsp_regulator_1_8,1800000,1800000);
		printk("[TSP] %s --> regulator_set_voltage ret = %d \n",__func__, ret);
	}
	msleep(2);
	if(tsp_regulator_3_3 == NULL)
	{
		tsp_regulator_3_3 = regulator_get(NULL, "gpldo6_uc"); /*touch power*/
		if(IS_ERR(tsp_regulator_3_3)){
			printk("[TSP] can not get TSP AVDD 3.3V\n");
			tsp_regulator_3_3 = NULL;
			return -1;
		}
		
		ret = regulator_set_voltage(tsp_regulator_3_3,3300000,3300000);
		printk("[TSP] %s --> regulator_set_voltage ret = %d \n",__func__, ret);
	}

	ret = regulator_enable(tsp_regulator_1_8);
	printk("[TSP] --> 1.8v regulator_enable ret = %d \n", ret);

	msleep(2);

	ret = regulator_enable(tsp_regulator_3_3);
	printk("[TSP] --> 3.3v regulator_enable ret = %d \n", ret);


#else

	if(tsp_regulator_2_8 == NULL)
		{
		tsp_regulator_2_8 = regulator_get(NULL, "gpldo5_uc");
		if(IS_ERR(tsp_regulator_2_8)){
			printk("[TSP] can not get TSP VDD 2.8V\n"); /*touch, touch key power*/
			tsp_regulator_2_8 = NULL;
			return -1;
			}	
			
		ret = regulator_set_voltage(tsp_regulator_2_8,2800000,2800000);
		printk("[TSP] %s --> regulator_set_voltage ret = %d \n",__func__, ret);
		}
	if(tsp_regulator_3_3 == NULL)
	{
		tsp_regulator_3_3 = regulator_get(NULL, "gpldo6_uc"); /*touch power*/
		if(IS_ERR(tsp_regulator_3_3)){
			printk("[TSP] can not get TSP AVDD 3.3V\n");
			tsp_regulator_3_3 = NULL;
			return -1;
		}
		
		ret = regulator_set_voltage(tsp_regulator_3_3,3300000,3300000);
		printk("[TSP] %s --> regulator_set_voltage ret = %d \n",__func__, ret);
	}
	
//		if(!regulator_is_enabled(tsp_regulator_2_8))
		{
		ret = regulator_enable(tsp_regulator_2_8);
		printk("[TSP] --> 2.8v regulator_enable ret = %d \n", ret);
		}


//		if(!regulator_is_enabled(tsp_regulator_3_3))
		{		
		ret = regulator_enable(tsp_regulator_3_3);
		printk("[TSP] --> 3.3v regulator_enable ret = %d \n", ret);
		}
		
#endif		
		
#else	
		gpio_set_value(TSP_LDO_ON_GPIO, 1);
#endif
	}		
	else
	{
#if defined (CONFIG_TOUCHSCREEN_POWER_USING_PMULDO)

#if defined(CONFIG_S2VE_TSP_REV05)
	ret = regulator_disable(tsp_regulator_1_8);
	printk("[TSP] --> 1.8v regulator_disable ret = %d \n", ret);

	ret = regulator_disable(tsp_regulator_3_3);
	printk("[TSP] --> 3.3v regulator_disable ret = %d \n", ret);
#else
		ret = regulator_disable(tsp_regulator_2_8);
		printk("[TSP] --> 2.8v regulator_disable ret = %d \n", ret);       
		msleep(1);
		ret = regulator_disable(tsp_regulator_3_3);
		printk("[TSP] --> 3.3v regulator_disable ret = %d \n", ret);       
#endif

#else		
		gpio_set_value(TSP_LDO_ON_GPIO, 0);
#endif	

	}
	msleep(10);
	return 0;
}
static unsigned int vir_addr;
static void mxt224_power_on(void)
{
	gpio_direction_input(MXT224_GPIO_TSP_INT);
	/*change bsc2 pin to pullup*/
	writel(0x18, vir_addr+0x10);
	writel(0x18, vir_addr+0x14);
	
	TSP_PowerOnOff(1);
#if !defined(CONFIG_S2VE_TSP_REV05)	
	gpio_set_value(NLSX4373_EN_GPIO, 1);
#endif
	printk(KERN_INFO"mxt224_power_on is finished\n");
}

static void mxt224_power_off(void)
{
	gpio_direction_output(MXT224_GPIO_TSP_INT, 0);
	TSP_PowerOnOff(0);

	/*change bsc2 pin to pullup disabled*/
	writel(0x8, vir_addr+0x10);
	writel(0x8, vir_addr+0x14);
	
#if !defined(CONFIG_S2VE_TSP_REV05)		
	gpio_set_value(NLSX4373_EN_GPIO, 0);
#endif
	printk(KERN_INFO"mxt224_power_off is finished\n");
}

static struct mxt224_platform_data mxt224_data = {
	.max_finger_touches = MXT224_MAX_MT_FINGERS,
	.gpio_read_done = MXT224_GPIO_TSP_INT,
	.config = mxt224_config,
	.config_e = mxt224e_config,
	.t48_config_batt_e = t48_config_e,
	.t48_config_chrg_e = t48_config_chrg_e,
	.min_x = 0,
	.max_x = 480,
	.min_y = 0,
	.max_y = 800,
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 30,
	.atchcalst = MXT224_ATCHCALST,
	.atchcalsthr = MXT224_ATCHCALTHR,
	.tchthr_batt = MXT224_THRESHOLD_BATT,
	.tchthr_batt_init = MXT224_THRESHOLD_BATT_INIT,
	.tchthr_charging = MXT224_THRESHOLD_CHRG,
	.noisethr_batt = MXT224_NOISE_THRESHOLD_BATT,
	.noisethr_charging = MXT224_NOISE_THRESHOLD_CHRG,
	.movfilter_batt = MXT224_MOVFILTER_BATT,
	.movfilter_charging = MXT224_MOVFILTER_CHRG,
	.atchcalst_e = MXT224E_ATCHCALST,
	.atchcalsthr_e = MXT224E_ATCHCALTHR,
	.tchthr_batt_e = MXT224E_THRESHOLD_BATT,
	.tchthr_charging_e = MXT224E_THRESHOLD_CHRG,
	.calcfg_batt_e = MXT224E_CALCFG_BATT,
	.calcfg_charging_e = MXT224E_CALCFG_CHRG,
	.atchfrccalthr_e = MXT224E_ATCHFRCCALTHR_NORMAL,
	.atchfrccalratio_e = MXT224E_ATCHFRCCALRATIO_NORMAL,
	.chrgtime_batt_e = MXT224E_GHRGTIME_BATT,
	.chrgtime_charging_e = MXT224E_GHRGTIME_CHRG,
	.blen_batt_e = MXT224E_BLEN_BATT,
	.blen_charging_e = MXT224E_BLEN_CHRG,
	.movfilter_batt_e = MXT224E_MOVFILTER_BATT,
	.movfilter_charging_e = MXT224E_MOVFILTER_CHRG,
	.actvsyncsperx_e = MXT224E_ACTVSYNCSPERX_NORMAL,
	.nexttchdi_e = MXT224E_NEXTTCHDI_NORMAL,
	.power_on = mxt224_power_on,
	.power_off = mxt224_power_off,
	.register_cb = tsp_register_callback,
	.read_ta_status = tsp_read_ta_status,
};

static struct i2c_board_info i2c_devs3[] __initdata = {
	{
		I2C_BOARD_INFO(MXT224_DEV_NAME, 0x4A),
		.platform_data = &mxt224_data
	}
};

void __init board_tsp_init(void)
{
	int gpio;
	int rc;

	
	printk(KERN_INFO"[TSP] naples_tsp_init() is called\n");
	/*turn LDO on */
#if !defined (CONFIG_TOUCHSCREEN_POWER_USING_PMULDO)	
	gpio = TSP_LDO_ON_GPIO;
	rc = gpio_request(gpio, "tsp_ldo");
	if (rc < 0) {
		printk(KERN_ERR "unable to request GPIO pin %d for tsp_ldo\n", gpio);
		return;
	}
	gpio_direction_output(gpio, 1);
#endif	
	
#if !defined(CONFIG_S2VE_TSP_REV05)	
	/*turn on NLSX4373*/
	gpio = NLSX4373_EN_GPIO;
	rc = gpio_request(gpio, "tsp_nlsx4373");
	if (rc < 0) {
		printk(KERN_ERR "unable to request GPIO pin %d for tsp_nlsx4373\n", gpio);
		return;
	}
	gpio_direction_output(gpio, 1);
#endif

	/* TSP_INT: */
	gpio = MXT224_GPIO_TSP_INT;
	
	rc = gpio_request(gpio, "ts_mxt224");
	if (rc < 0) {
		printk(KERN_ERR "unable to request GPIO pin %d\n", gpio);
		return;
	}
	gpio_direction_input(gpio);
	
	i2c_devs3[0].irq = gpio_to_irq(gpio);
	i2c_register_board_info(1, i2c_devs3,
		ARRAY_SIZE(i2c_devs3));
	vir_addr = (unsigned int)ioremap(0x35004800, 4096);
}
#endif
