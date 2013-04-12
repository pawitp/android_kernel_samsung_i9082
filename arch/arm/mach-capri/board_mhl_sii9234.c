#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/sii9234.h>
/*#include "midas.h"*/


#ifdef CONFIG_SAMSUNG_MHL

static void mhl_gpio_config(void)
{
	int gpio, rc;
	
	printk(KERN_INFO "%s()\n", __func__);
	
	gpio = GPIO_MHL_WAKE_UP;
	rc = gpio_request(gpio, "MHL_WAKEUP");
	if (rc < 0) {
		printk(KERN_ERR "unable to request GPIO pin %d\n", gpio);
		return;
	}
	gpio_direction_input(gpio);
	
	gpio = GPIO_MHL_INT;
	rc = gpio_request(gpio, "MHL_INT");
	if (rc < 0) {
		printk(KERN_ERR "unable to request GPIO pin %d\n", gpio);
		return;
	}
	gpio_direction_input(gpio);
	
	
	gpio = GPIO_MHL_EN;
	rc = gpio_request(gpio, "MHL_EN");
	if (rc < 0) {
		printk(KERN_ERR "unable to request GPIO pin %d\n", gpio);
		return;
	}
	gpio_direction_output(gpio, 0);

	gpio = GPIO_MHL_RST;
	rc = gpio_request(gpio, "MHL_RST");
	if (rc < 0) {
		printk(KERN_ERR "unable to request GPIO pin %d\n", gpio);
		return;
	}
	gpio_direction_output(gpio, 0);
	

	gpio = GPIO_MHL_SEL;
	rc = gpio_request(gpio, "MHL_SEL");
	if (rc < 0) {
		printk(KERN_ERR "unable to request GPIO pin %d\n", gpio);
		return;
	}
	gpio_direction_output(gpio, 0);

	pr_info("sii9244 GPIO_MHL_SEL=%d \n",gpio);

}

static void sii9234_hw_onoff(bool on)
{
	printk(KERN_INFO "%s(%d)\n", __func__, on);

	if (on) {
		gpio_set_value(GPIO_MHL_EN, 1);
	} else {
		usleep_range(10000, 20000);
		gpio_set_value(GPIO_MHL_EN, 0);

		usleep_range(10000, 20000);
		gpio_set_value(GPIO_MHL_RST, 0);
	}
}

static void sii9234_hw_reset(void)
{
	printk(KERN_INFO "%s()\n", __func__);

	usleep_range(10000, 20000);
	gpio_set_value(GPIO_MHL_RST, 1);

	usleep_range(10000, 20000);
	gpio_set_value(GPIO_MHL_RST, 0);
	
	usleep_range(10000, 20000);
	gpio_set_value(GPIO_MHL_RST, 1);

	msleep(30);
}

static void cfg_mhl_sel(bool on)
{
	printk(KERN_INFO "sii9234 %s() [MHL] USB path change : %s\n",
	       __func__, on ? "MHL" : "USB");
	if (on == 1) {
		if (gpio_get_value(GPIO_MHL_SEL))
			printk(KERN_INFO "[MHL] GPIO_MHL_SEL : already 1\n");
		else
			gpio_set_value(GPIO_MHL_SEL, 1);
	} else {
		if (!gpio_get_value(GPIO_MHL_SEL))
			printk(KERN_INFO "[MHL] GPIO_MHL_SEL : already 0\n");
		else
			gpio_set_value(GPIO_MHL_SEL, 0);
	}
}

#ifdef CONFIG_MHL_NEW_CBUS_MSC_CMD
static void fsa9485_mhl_cb(bool attached, int mhl_charge)
{

	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_mhl_cb attached (%d), mhl_charge(%d)\n",
			attached, mhl_charge);

	fsa9485_charger_cb(attached);
}
#else
static void fsa9485_mhl_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("%s attached %d\n", __func__, attached);

	fsa9485_charger_cb(attached);
}
#endif

int get_mhl_int_irq(void)
{
	return  gpio_to_irq(GPIO_MHL_INT);
}


static struct sii9234_platform_data sii9234_pdata = {
	.get_irq = get_mhl_int_irq,
	.hw_onoff = sii9234_hw_onoff,
	.hw_reset = sii9234_hw_reset,
	.gpio = GPIO_MHL_SEL,
	.gpio_cfg = mhl_gpio_config,
#if defined(CONFIG_VIDEO_MHL_V2)
	.mhl_sel = cfg_mhl_sel,
	.vbus_present = NULL,
#endif
};

static struct i2c_board_info __initdata i2c_devs_sii9234[] = {
	{
		I2C_BOARD_INFO("sii9234_mhl_tx", 0x72>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_tpi", 0x7A>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_hdmi_rx", 0x92>>1),
		.platform_data = &sii9234_pdata,
	},
	{
		I2C_BOARD_INFO("sii9234_cbus", 0xC8>>1),
		.platform_data = &sii9234_pdata,
	},
};


#if 0
static struct i2c_board_info i2c_dev_hdmi_ddc __initdata = {
	I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
};
#endif

static void __init board_mhl_init(void)
{
	int ret;

	ret = i2c_register_board_info(I2C_BUS_ID_MHL, i2c_devs_sii9234,
			ARRAY_SIZE(i2c_devs_sii9234));

	if (ret < 0) {
		printk(KERN_ERR "[MHL] adding i2c fail - nodevice\n");
		return;
	}

#if 0
#if defined(CONFIG_MACH_S2PLUS) || defined(CONFIG_MACH_P4NOTE)
	sii9234_pdata.ddc_i2c_num = 5;
#else
	sii9234_pdata.ddc_i2c_num = (system_rev == 3 ? 16 : 5);
#endif

#ifdef CONFIG_MACH_SLP_PQ_LTE
	sii9234_pdata.ddc_i2c_num = 16;
#endif
	ret = i2c_register_board_info(sii9234_pdata.ddc_i2c_num, &i2c_dev_hdmi_ddc, 1);
	if (ret < 0) {
		printk(KERN_ERR "[MHL] adding ddc fail - nodevice\n");
		return;
	}

#endif

	return;
}

#endif
