/* /linux/drivers/misc/modem_if/modem_modemctl_device_sprd8803.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
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

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <mach/pinmux.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/platform_data/modem_v2.h>
#include "modem_prj.h"

int sprd_boot_done;

static int sprd_power_on_cnt=0;

#define GPIO_UART_TX 	74
#define GPIO_SPI_TX 		94


static int configure_control_pin(int sprd_on)
{
	int ret=0;
	static int sprd_status=0xff;
	struct pin_config new_pin_config_u;
	struct pin_config new_pin_config_s;

	if(sprd_status==sprd_on)
	{
	//	pr_err("%s, return %d\n",__func__,sprd_on);
		return 0;
	}

//	pr_err("%s, Congifure Pin with sprd:%d\n",__func__,sprd_on);
	
	new_pin_config_u.name = PN_UARTB2_UTXD;
	ret = pinmux_get_pin_config(&new_pin_config_u);

	if(ret){
		printk("%s,%d,  Error pinmux_get_pin_config():%d\n",__func__,__LINE__,ret);
		return ret;
	}

	new_pin_config_s.name = PN_SSP0_TXD;
	ret = pinmux_get_pin_config(&new_pin_config_s);
	if(ret){
		printk("%s,%d,  Error pinmux_get_pin_config():%d\n",__func__,__LINE__,ret);
		return ret;
	}

	
	if(sprd_on){
		new_pin_config_u.func =PF_UARTB2_UTXD;
		new_pin_config_u.reg.b.sel =0x0;

		ret = pinmux_set_pin_config(&new_pin_config_u);
		if(ret){
			printk("%s,%d,  Error pinmux_get_pin_config():%d\n",__func__,__LINE__,ret);
			return ret;
		}

		new_pin_config_s.func =PF_SSP0_TXD;
		new_pin_config_s.reg.b.sel =0x0;

		ret = pinmux_set_pin_config(&new_pin_config_s);
		if(ret){
			printk("%s,%d,  Error pinmux_get_pin_config():%d\n",__func__,__LINE__,ret);
			return ret;
		}

	}
	else{
		new_pin_config_u.func =PF_GPIO_074;
		new_pin_config_u.reg.b.sel =0x3;

		ret = pinmux_set_pin_config(&new_pin_config_u);
		if(ret){
			printk("%s,%d,  Error pinmux_get_pin_config():%d\n",__func__,__LINE__,ret);
			return ret;
		}

		new_pin_config_s.func =PF_GPIO_094;
		new_pin_config_s.reg.b.sel =0x3;

		ret = pinmux_set_pin_config(&new_pin_config_s);
		if(ret){
			printk("%s,%d,  Error pinmux_get_pin_config():%d\n",__func__,__LINE__,ret);
			return ret;
		}

		gpio_set_value(GPIO_UART_TX, 0);
		gpio_set_value(GPIO_SPI_TX, 0);

		
	
	}
	sprd_status=sprd_on;

	return ret;
}


static int sprd8803_on(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] sprd8803_on()\n");
#if defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)
	if (!mc->gpio_cp_on || !mc->gpio_pda_active||!mc->gpio_cp_on2)
#else
	if (!mc->gpio_cp_on || !mc->gpio_pda_active)
#endif
	 {
		pr_err("[MODEM_IF] no gpio data\n");
		return -ENXIO;
	}

	//disable_irq(mc->irq_phone_active); //TBD

#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	gpio_set_value(mc->gpio_sim_sel, 0); // dual mode -sprd
	gpio_set_value(mc->gpio_cp_ctrl1, 0); // dual mode -sprd 
#endif

	if(sprd_power_on_cnt)
	{
		pr_info("[MODEM_IF] reset !!!\n");
		gpio_set_value(mc->gpio_reset_req_n, 1);
		gpio_set_value(mc->gpio_pda_active, 0);
		configure_control_pin(1);
		msleep(100);
		gpio_set_value(mc->gpio_reset_req_n, 0);
		gpio_set_value(mc->gpio_pda_active, 1);
		mc->phone_state = STATE_BOOTING;
	
	}
	else
	{
		pr_info("[MODEM_IF] cp_on ctrl !!!\n");
		sprd_power_on_cnt=1;
#if defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)
		gpio_set_value(mc->gpio_cp_on2, 1);
#endif
		gpio_set_value(mc->gpio_cp_on, 1);
		msleep(100);
		configure_control_pin(1);
		gpio_set_value(mc->gpio_pda_active, 1);
		mc->phone_state = STATE_BOOTING;
	}

	return 0;
}

static int sprd8803_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s\n", __func__);
#if defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)
	if (!mc->gpio_cp_on||!mc->gpio_cp_on2)
#else
	if (!mc->gpio_cp_on)
#endif
 	{
		mif_err("no gpio data\n");
		return -ENXIO;
	}
//	configure_control_pin(0);
	gpio_set_value(mc->gpio_cp_on, 0);
#if defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)
	gpio_set_value(mc->gpio_cp_on2, 0);
#endif
#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	gpio_set_value(mc->gpio_sim_sel, 1); // dual mode -other cp
	gpio_set_value(mc->gpio_cp_ctrl1, 1); // dual mode -other cp
#endif

	mc->phone_state = STATE_OFFLINE;

	return 0;
}

static int sprd8803_reset(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] %s\n", __func__);

	if_spi_thread_restart();

	return 0;
}

static int sprd8803_boot_on(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] sprd8803_boot_on() %d\n", sprd_boot_done);
	
	//enable_irq(mc->irq_phone_active); //TBD

	if (sprd_boot_done) {
		gpio_set_value(mc->gpio_cp_on, 0);
		sprd_power_on_cnt=0;
		pr_info("[MODEM_IF] sprd8803 boot complete\n");
		pr_info("[MODEM_IF] switch uart3 to AP_TXD(RXD)\n");
	}

	return sprd_boot_done;
}

static int sprd8803_boot_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] sprd8803_boot_off()\n");
	spi_sema_init();
	return 0;
}

static int sprd8803_dump_reset(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF] sprd8803_dump_reset()\n");

	if (!mc->gpio_ap_cp_int2)
		return -ENXIO;

	gpio_set_value(mc->gpio_ap_cp_int2, 0);

	mc->phone_state = STATE_OFFLINE;

	msleep(100);

	gpio_set_value(mc->gpio_ap_cp_int2, 1);

	pr_info("[MODEM_IF] %s set high\n", __func__);
	
	return 0;
}

static irqreturn_t phone_active_irq_handler(int irq, void *_mc)
{
	int phone_active_value = 0;
	int cp_dump_value = 0;
	int phone_state = 0;
	struct modem_ctl *mc = (struct modem_ctl *)_mc;


	if (!mc->gpio_phone_active ||
			!mc->gpio_cp_dump_int) {
		pr_err("[MODEM_IF] no gpio data\n");
		return IRQ_HANDLED;
	}

	if (!sprd_boot_done)
		return IRQ_HANDLED;

	phone_active_value = gpio_get_value(mc->gpio_phone_active);
	cp_dump_value = gpio_get_value(mc->gpio_cp_dump_int);

	pr_info("PA EVENT : pa=%d, cp_dump=%d\n",
				phone_active_value, cp_dump_value);

	if (phone_active_value)
		phone_state = STATE_ONLINE;
	else
		phone_state = STATE_OFFLINE;

	if (cp_dump_value) {
		phone_state = STATE_CRASH_EXIT;
	}

	if(phone_state ==STATE_OFFLINE){
		gpio_set_value(mc->gpio_pda_active, 0);
		configure_control_pin(0);
	}


	if (mc->iod && mc->iod->modem_state_changed)
		mc->iod->modem_state_changed(mc->iod, phone_state);

	if (mc->bootd && mc->bootd->modem_state_changed)
		mc->bootd->modem_state_changed(mc->bootd, phone_state);

	return IRQ_HANDLED;
}

static void sprd8803_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = sprd8803_on;
	mc->ops.modem_off = sprd8803_off;
	mc->ops.modem_reset = sprd8803_reset;
	mc->ops.modem_boot_on = sprd8803_boot_on;
	mc->ops.modem_boot_off = sprd8803_boot_off;
	mc->ops.modem_dump_reset = sprd8803_dump_reset;
	mc->ops.modem_force_crash_exit = sprd8803_dump_reset;
}

int sprd8803_init_modemctl_device(struct modem_ctl *mc,
			struct modem_data *pdata)
{
	int ret = 0;
	int irq_cp_dump_int;
	struct platform_device *pdev;

	pr_info("[MODEM_IF] %s\n", __func__);
#if defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)
	mc->gpio_cp_on2 = pdata->gpio_cp_on2;
#endif
	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_cp_dump_int = pdata->gpio_cp_dump_int;
	mc->gpio_ap_cp_int2 = pdata->gpio_ap_cp_int2;
	mc->gpio_uart_sel = pdata->gpio_uart_sel;

	mc->gpio_reset_req_n = pdata->gpio_reset_req_n;
	mc->gpio_cp_pwr_check = pdata->gpio_cp_pwr_check;

	
#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	mc->gpio_sim_sel = pdata->gpio_sim_sel;
	mc->gpio_cp_ctrl1 = pdata->gpio_cp_ctrl1;
#endif


	pdev = to_platform_device(mc->dev);
	mc->irq_phone_active = gpio_to_irq(mc->gpio_phone_active);
	mc->irq_cp_dump_int = gpio_to_irq(mc->gpio_cp_dump_int);

	sprd8803_get_ops(mc);


	ret = request_irq(mc->irq_phone_active,phone_active_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"phone_active", mc);
	if (ret) {
		pr_err("[MODEM_IF] %s: failed to request_irq:%d\n",
			__func__, ret);
		goto err;
	}

	 printk("%s gpio : %d    irq : %d \n",__func__,mc->gpio_phone_active, mc->irq_phone_active);

#if 0
	ret = enable_irq_wake(mc->irq_phone_active);
	if (ret) {
		pr_err("[MODEM_IF] %s: failed to enable_irq_wake:%d\n",
					__func__, ret);
		free_irq(mc->irq_phone_active, mc);
		goto err;
	}
#endif	

	ret = request_irq(mc->irq_cp_dump_int, phone_active_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"cp_dump_int", mc);
	if (ret) {
		pr_err("[MODEM_IF] %s: failed to request_irq:%d\n",
			__func__, ret);
		goto err;
	}
#if 0
	ret = enable_irq_wake(irq_cp_dump_int);
	if (ret) {
		pr_err("[MODEM_IF] %s: failed to enable_irq_wake:%d\n",
					__func__, ret);
		free_irq(irq_cp_dump_int, mc);
		goto err;
	}

#endif
err:
	return ret;
}
