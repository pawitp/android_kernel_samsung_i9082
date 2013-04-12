/* cyttsp */
#include <linux/cyttsp4_bus.h>
#include <linux/cyttsp4_core.h>
#include <linux/cyttsp4_btn.h>
#include <linux/cyttsp4_mt.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>


#define CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_INCLUDE_FW

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_INCLUDE_FW
#include "cyttsp4_img.h"
static struct cyttsp4_touch_firmware cyttsp4_firmware = {
	.img = cyttsp4_img,
	.size = ARRAY_SIZE(cyttsp4_img),
	.ver = cyttsp4_ver,
	.vsize = ARRAY_SIZE(cyttsp4_ver),
};
#else
static struct cyttsp4_touch_firmware cyttsp4_firmware = {
	.img = NULL,
	.size = 0,
	.ver = NULL,
	.vsize = 0,
};
#endif



#define CYTTSP4_USE_I2C

#ifdef CYTTSP4_USE_I2C
#define CYTTSP4_I2C_NAME "cyttsp4_i2c_adapter"
#define CYTTSP4_I2C_TCH_ADR 0x24
#define CYTTSP4_LDR_TCH_ADR 0x24

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X_CRT
#define TMA400_GPIO_TSP_INT   8
#else
#define TMA400_GPIO_TSP_INT   2
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X_CRT
#define CYTTSP4_I2C_IRQ_GPIO 8 /* J6.9, C19, GPMC_AD14/GPIO_38 */
#else
#define CYTTSP4_I2C_IRQ_GPIO 2 /* J6.9, C19, GPMC_AD14/GPIO_38 */
#endif

#define CYTTSP4_I2C_RST_GPIO 14 /* J6.10, D18, GPMC_AD13/GPIO_37 */
#endif

#define CY_MAXX 600
#define CY_MAXY 800
#define CY_MINX 0
#define CY_MINY 0

#define CY_ABS_MIN_X CY_MINX
#define CY_ABS_MIN_Y CY_MINY
#define CY_ABS_MAX_X CY_MAXX
#define CY_ABS_MAX_Y CY_MAXY
#define CY_ABS_MIN_P 0
#define CY_ABS_MAX_P 255
#define CY_ABS_MIN_W 0
#define CY_ABS_MAX_W 255

#define CY_ABS_MIN_T 0

#define CY_ABS_MAX_T 15

#define CY_IGNORE_VALUE 0xFFFF

#define  NLSX4373_EN_GPIO    14

#define TSP_DEBUG_OPTION 0
int touch_debug_option = TSP_DEBUG_OPTION;

#define	CAPRI_TSP_DEBUG(fmt, args...)	\
	if (touch_debug_option)	\
		printk("[TSP %s:%4d] " fmt, \
		__func__, __LINE__, ## args);

#ifndef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X_CRT
static struct regulator *tsp_regulator_2_8=NULL;
#endif
static struct regulator *tsp_regulator_3_3=NULL;

int cyttsp4_hw_power(int on, int use_irq, int irq_gpio)
{
	int ret = 0;

	CAPRI_TSP_DEBUG(" %s set %d \n", __func__, on);

#ifndef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X_CRT
	if (tsp_regulator_2_8 == NULL) {
		tsp_regulator_2_8 = regulator_get(NULL, "gpldo5_uc");

		if (IS_ERR(tsp_regulator_2_8)) {
			ret = PTR_ERR(tsp_regulator_2_8);
			pr_err("can not get TSP VDD 2.8V, ret=%d\n", ret); /*touch, touch key power*/
			tsp_regulator_2_8 = NULL;
			goto exit;
		}

		ret = regulator_set_voltage(tsp_regulator_2_8,2800000,2800000);
		CAPRI_TSP_DEBUG(" --> regulator_set_voltage ret = %d \n", ret);
		if (ret) {
			pr_err("can not set voltage TSP VDD 2.8V, ret=%d\n", ret);
			regulator_put(tsp_regulator_2_8);
			tsp_regulator_2_8 = NULL;
			goto exit;
		}
	}
#endif

	if (tsp_regulator_3_3 == NULL){
		tsp_regulator_3_3 = regulator_get(NULL, "gpldo6_uc");

		if (IS_ERR(tsp_regulator_3_3)) {
			ret = PTR_ERR(tsp_regulator_3_3);
			pr_err("can not get TSP AVDD 3.3V, ret=%d\n", ret);
			tsp_regulator_3_3 = NULL;
			goto exit;
		}

		ret = regulator_set_voltage(tsp_regulator_3_3,3300000,3300000);
		CAPRI_TSP_DEBUG("--> regulator_set_voltage ret = %d \n", ret);
		if (ret) {
			pr_err("can not set voltage TSP AVDD 3.3V, ret=%d\n", ret);
			regulator_put(tsp_regulator_3_3);
			tsp_regulator_3_3 = NULL;
			goto exit;
		}
	}

	if (on) {
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X_CRT
		if (!regulator_is_enabled(tsp_regulator_2_8)) {
			ret = regulator_enable(tsp_regulator_2_8);
			CAPRI_TSP_DEBUG("--> 2.8v regulator_enable ret = %d \n", ret);
			if (ret) {
				pr_err("can not enable TSP VDD 2.8V, ret=%d\n", ret);
				goto exit;
			}
		}
#endif
		if (!regulator_is_enabled(tsp_regulator_3_3)) {
			ret = regulator_enable(tsp_regulator_3_3);
			CAPRI_TSP_DEBUG(" --> 3.3v regulator_enable ret = %d \n", ret);
			if (ret) {
				pr_err("can not enable TSP AVDD 3.3V, ret=%d\n", ret);
				goto exit;
			}
		}

		/* Enable the IRQ */
		if (use_irq) {
			enable_irq(gpio_to_irq(irq_gpio));
			pr_debug("Enabled IRQ %d for TSP\n", gpio_to_irq(irq_gpio));
		}
	} else {
		/* Disable the IRQ */
		if (use_irq) {
			pr_debug("Disabling IRQ %d for TSP\n", gpio_to_irq(irq_gpio));
			disable_irq_nosync(gpio_to_irq(irq_gpio));
		}
#ifndef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X_CRT
		if (regulator_is_enabled(tsp_regulator_2_8)) {
			ret = regulator_disable(tsp_regulator_2_8);
			CAPRI_TSP_DEBUG(" --> 2.8v regulator_disable ret = %d \n", ret);
			if (ret) {
				pr_err("can not disable TSP VDD 2.8V, ret=%d\n", ret);
				goto exit;
			}
		}
#endif
		if (regulator_is_enabled(tsp_regulator_3_3)) {
			ret = regulator_disable(tsp_regulator_3_3);
			CAPRI_TSP_DEBUG(" --> 3.3v regulator_disable ret = %d \n", ret);
			if (ret) {
				pr_err("can not disable TSP AVDD 3.3V, ret=%d\n", ret);
				goto exit;
			}
		}
	}

exit:
	return ret;
}

static int cyttsp4_xres(struct cyttsp4_core_platform_data *pdata,
		struct device *dev)
{
	int irq_gpio = pdata->irq_gpio;
	int rc = 0;

	cyttsp4_hw_power(0, true, irq_gpio);

	/* Delay for 10 msec */
	msleep(10); 

	cyttsp4_hw_power(1, true, irq_gpio);
		
	return rc;
}

static int cyttsp4_init(struct cyttsp4_core_platform_data *pdata,
		int on, struct device *dev)
{
	int irq_gpio = pdata->irq_gpio;
	int rc = 0;

	if (on) {
		rc = gpio_request(irq_gpio, NULL);
		if (rc < 0) {
			gpio_free(irq_gpio);
			rc = gpio_request(irq_gpio, NULL);
		}
		if (rc < 0)
  			dev_err(dev, "%s: Fail request gpio=%d\n",
 					__func__, irq_gpio);
		else
			gpio_direction_input(irq_gpio);

		cyttsp4_hw_power(1, false, 0);
	} else {
		cyttsp4_hw_power(0, false, 0);
		gpio_free(irq_gpio);
	}

	dev_info(dev, "%s: INIT CYTTSP IRQ gpio=%d r=%d\n",
			__func__, irq_gpio, rc);

	return rc;
}

static int cyttsp4_wakeup(struct cyttsp4_core_platform_data *pdata,
		struct device *dev, atomic_t *ignore_irq)
{
	int irq_gpio = pdata->irq_gpio;
      
	return cyttsp4_hw_power(1, true, irq_gpio);
}

static int cyttsp4_sleep(struct cyttsp4_core_platform_data *pdata,
		struct device *dev, atomic_t *ignore_irq)
{
	int irq_gpio = pdata->irq_gpio;

	return cyttsp4_hw_power(0, true, irq_gpio);
}

static int cyttsp4_power(struct cyttsp4_core_platform_data *pdata,
		int on, struct device *dev, atomic_t *ignore_irq)
{
	if (on)
		return cyttsp4_wakeup(pdata, dev, ignore_irq);

	return cyttsp4_sleep(pdata, dev, ignore_irq);
}

static int cyttsp4_irq_stat(struct cyttsp4_core_platform_data *pdata,
		struct device *dev)
{

	return gpio_get_value(pdata->irq_gpio);
}

/* Button to keycode conversion */
static u16 cyttsp4_btn_keys[] = {
	/* use this table to map buttons to keycodes (see input.h) */
	KEY_MENU,		/* 139 */
	KEY_BACK,		/* 158 */
};

static struct touch_settings cyttsp4_sett_btn_keys = {
	.data = (uint8_t *)&cyttsp4_btn_keys[0],
	.size = ARRAY_SIZE(cyttsp4_btn_keys),
	.tag = 0,
};

static struct cyttsp4_core_platform_data _cyttsp4_core_platform_data = {
	.irq_gpio = CYTTSP4_I2C_IRQ_GPIO,
	.rst_gpio = CYTTSP4_I2C_RST_GPIO,
	.xres = cyttsp4_xres,
	.init = cyttsp4_init,
	.power = cyttsp4_power,
	.irq_stat = cyttsp4_irq_stat,
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Cypress Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		NULL, /* &cyttsp4_sett_param_regs, */
		NULL, /* &cyttsp4_sett_param_size, */
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		NULL, /* &cyttsp4_sett_ddata, *//* Design Data Record */
		NULL, /* &cyttsp4_sett_mdata, *//* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		&cyttsp4_sett_btn_keys,	/* button-to-keycode table */
	},
	.fw = &cyttsp4_firmware,
};

/*
static struct cyttsp4_core cyttsp4_core_device = {
	.name = CYTTSP4_CORE_NAME,
	.id = "main_ttsp_core",
	.adap_id = CYTTSP4_I2C_NAME,
	.dev = {
		.platform_data = &_cyttsp4_core_platform_data,
	},
};
*/

static struct cyttsp4_core_info cyttsp4_core_info __initdata= {
	.name = CYTTSP4_CORE_NAME,
	.id = "main_ttsp_core",
	.adap_id = CYTTSP4_I2C_NAME,
       .platform_data = &_cyttsp4_core_platform_data,
};
 



static const uint16_t cyttsp4_abs[] = {
	ABS_MT_POSITION_X, CY_ABS_MIN_X, CY_ABS_MAX_X, 0, 0,
	ABS_MT_POSITION_Y, CY_ABS_MIN_Y, CY_ABS_MAX_Y, 0, 0,
	ABS_MT_PRESSURE, CY_ABS_MIN_P, CY_ABS_MAX_P, 0, 0,
	CY_IGNORE_VALUE, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
	ABS_MT_TRACKING_ID, CY_ABS_MIN_T, CY_ABS_MAX_T, 0, 0,
	ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0,
	ABS_MT_TOUCH_MINOR, 0, 255, 0, 0,
	ABS_MT_ORIENTATION, -128, 127, 0, 0,
};

struct touch_framework cyttsp4_framework = {
	.abs = (uint16_t *)&cyttsp4_abs[0],
	.size = ARRAY_SIZE(cyttsp4_abs),
	.enable_vkeys = 0,
};

static struct cyttsp4_mt_platform_data _cyttsp4_mt_platform_data = {
	.frmwrk = &cyttsp4_framework,
	.flags = 0x00,
	.inp_dev_name = CYTTSP4_MT_NAME,
};

/*
struct cyttsp4_device cyttsp4_mt_device = {
	.name = CYTTSP4_MT_NAME,
	.core_id = "main_ttsp_core",
	.dev = {
		.platform_data = &_cyttsp4_mt_platform_data,
	}
};
*/

struct cyttsp4_device_info cyttsp4_mt_info __initdata = {
	.name = CYTTSP4_MT_NAME,
	.core_id = "main_ttsp_core",
      .platform_data = &_cyttsp4_mt_platform_data,
};

static struct cyttsp4_btn_platform_data _cyttsp4_btn_platform_data = {
	.inp_dev_name = CYTTSP4_BTN_NAME,
};

/*
struct cyttsp4_device cyttsp4_btn_device = {
	.name = CYTTSP4_BTN_NAME,
	.core_id = "main_ttsp_core",
	.dev = {
		.platform_data = &_cyttsp4_btn_platform_data,
	}
};
*/

struct cyttsp4_device_info cyttsp4_btn_info __initdata = {
	.name = CYTTSP4_BTN_NAME,
	.core_id = "main_ttsp_core",
	.platform_data = &_cyttsp4_btn_platform_data,
};

#ifdef CYTTSP4_VIRTUAL_KEYS
static ssize_t cyttps4_virtualkeys_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		__stringify(EV_KEY) ":"
		__stringify(KEY_BACK) ":1360:90:160:180"
		":" __stringify(EV_KEY) ":"
		__stringify(KEY_MENU) ":1360:270:160:180"
		":" __stringify(EV_KEY) ":"
		__stringify(KEY_HOME) ":1360:450:160:180"
		":" __stringify(EV_KEY) ":"
		__stringify(KEY_SEARCH) ":1360:630:160:180"
		"\n");
}

static struct kobj_attribute cyttsp4_virtualkeys_attr = {
	.attr = {
		.name = "virtualkeys.cyttsp4_mt",
		.mode = S_IRUGO,
	},
	.show = &cyttps4_virtualkeys_show,
};

static struct attribute *cyttsp4_properties_attrs[] = {
	&cyttsp4_virtualkeys_attr.attr,
	NULL
};

static struct attribute_group cyttsp4_properties_attr_group = {
	.attrs = cyttsp4_properties_attrs,
};
#endif

void __init board_tsp_init(void)
{
	int gpio;
	int rc;

	printk("[TSP] board_tsp_init\n");
	/*turn on NLSX4373*/
    #if 0
	gpio = NLSX4373_EN_GPIO;
	rc = gpio_request(gpio, "tsp_nlsx4373");
	if (rc < 0) {
		printk(KERN_ERR "unable to request GPIO pin %d for tsp_nlsx4373\n", gpio);
		return;
	}
	gpio_direction_output(gpio, 1);
    #endif
	
	printk(KERN_INFO"[TSP] naples_tsp_init() is called\n");

	cyttsp4_register_core_device(&cyttsp4_core_info);
	cyttsp4_register_device(&cyttsp4_mt_info);
	cyttsp4_register_device(&cyttsp4_btn_info);
}
