/*****************************************************************************
*  Copyright 2011 - 2012 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/licenses/old-license/gpl-2.0.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/

#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <linux/serial_8250.h>
#include <linux/clk.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/kona.h>
#include <mach/rdb/brcm_rdb_uartb.h>
#include <mach/irqs.h>
#include <plat/chal/chal_trace.h>
#include <trace/stm.h>
#include <asm/pmu.h>
#include <plat/pl330-pdata.h>
#include <linux/dma-mapping.h>
#include <linux/spi/spi.h>
#include <plat/spi_kona.h>
#include <plat/chal/chal_trace.h>
#include <trace/stm.h>

#ifdef CONFIG_KONA_AVS
#include <plat/kona_avs.h>
#include "capri_avs.h"
#endif

#include <linux/android_pmem.h>
#include <linux/memblock.h>
#include <linux/dma-contiguous.h>
#include <mach/clock.h>

#if defined(CONFIG_KONA_CPU_FREQ_DRV)
#include <plat/kona_cpufreq_drv.h>
#include <linux/cpufreq.h>
#include <linux/clk.h>
#include <mach/pi_mgr.h>
#endif

#ifdef CONFIG_KONA_POWER_MGR
#include <plat/pwr_mgr.h>

#define VLT_LUT_SIZE 16
#endif

#ifdef CONFIG_SENSORS_KONA
#include <linux/broadcom/kona-thermal.h>
#include <linux/csapi_adc.h>
#include <linux/mfd/bcmpmu.h>
#include <linux/broadcom/kona-tmon.h>
#endif

#include <mach/chip_pinmux.h>
#include <mach/pinmux.h>
#include <linux/usb/bcm_hsotgctrl.h>

#include <mach/rdb/brcm_rdb_kpm_clk_mgr_reg.h>
#include <mach/rdb/brcm_rdb_usbh_phy.h>

#include <mach/pi_mgr.h>
#include <plat/pi_mgr.h>

#define KONA_UART0_PA   UARTB_BASE_ADDR
#define KONA_UART1_PA   UARTB2_BASE_ADDR
#define KONA_UART2_PA   UARTB3_BASE_ADDR
#define KONA_UART3_PA   UARTB4_BASE_ADDR

#define KONA_8250PORT(name, clk, freq, private_index) \
{ \
	.membase = (void __iomem *)(KONA_##name##_VA), \
	.mapbase = (resource_size_t)(KONA_##name##_PA), \
	.irq = BCM_INT_ID_##name, \
	.uartclk = freq, \
	.regshift = 2, \
	.iotype = UPIO_MEM32, \
	.type = PORT_KONA, \
	.flags = UPF_BOOT_AUTOCONF | UPF_FIXED_TYPE | UPF_SKIP_TEST, \
	.clk_name = clk, \
	.pm = capri_8250_pm, \
	.private_data = (void *) &capri_uart_private_data[private_index], \
	.serial_out = capri_serial_out, \
	.serial_in = capri_serial_in, \
	.handle_irq = capri_serial_handle_irq, \
}

struct capri_uart_data {
	int last_lcr;
	char *peri_clk;
	struct pi_mgr_qos_node pi_qos_node;
	int initialized;
};

static struct capri_uart_data capri_uart_private_data[] = {
	[0] = {
	       .last_lcr = 0,
	       .peri_clk = "uartb_clk",
	       },
	[1] = {
	       .last_lcr = 0,
	       .peri_clk = "uartb2_clk",
	       },
	[2] = {
	       .last_lcr = 0,
	       .peri_clk = "uartb3_clk",
	       },
	[3] = {
	       .last_lcr = 0,
	       .peri_clk = "uartb4_clk",
	       },
};

static void capri_8250_pm(struct uart_port *port, unsigned int state,
			  unsigned int old_state)
{
	struct clk *clk;
	struct capri_uart_data *private_data =
	    (struct capri_uart_data *)port->private_data;

	/* Current reduction hack for UARTB4 RTS since GPS pulls RTS low */
	static struct pin_config uartb4_rts_config[2] = {
		{.name = PN_UARTB4_URTS,
		 .func = PF_UARTB4_URTS,
		 .reg.b = {
			   .drv_sth = 3,
			   .input_dis = 1,
			   .slew_rate_ctrl = 0,
			   .pull_up = 0,
			   .pull_dn = 0,
			   .hys_en = 0,
			   .sel = 0,},
		 },
		{.name = PN_UARTB4_URTS,
		 .func = PF_GPIO_079,
		 .reg.b = {
			   .drv_sth = 3,
			   .input_dis = 1,
			   .slew_rate_ctrl = 0,
			   .pull_up = 0,
			   .pull_dn = 1,
			   .hys_en = 0,
			   .sel = 3,},
		 },
	};

	pr_debug("In %s port = 0x%08X state = %d old_state = %d\n",
	       __func__, (unsigned int)port, state, old_state);

	clk = clk_get(port->dev, private_data->peri_clk);

	if (!private_data->initialized) {
		pi_mgr_qos_add_request(&private_data->pi_qos_node,
				       (char *)dev_name(port->dev),
				       PI_MGR_PI_ID_ARM_SUB_SYSTEM,
				       PI_MGR_QOS_DEFAULT_VALUE);

		private_data->initialized = 1;
	}

	switch (state) {
	case 0:
		if (port->irq == BCM_INT_ID_UART3)
			pinmux_set_pin_config(&uartb4_rts_config[0]);
		clk_enable(clk);

		pi_mgr_qos_request_update(&private_data->pi_qos_node, 0);

		serial8250_do_pm(port, state, old_state);
		break;
	case 3:
		if (port->irq == BCM_INT_ID_UART3)
			pinmux_set_pin_config(&uartb4_rts_config[1]);
		clk_disable(clk);

		pi_mgr_qos_request_update(&private_data->pi_qos_node,
					  PI_MGR_QOS_DEFAULT_VALUE);

		serial8250_do_pm(port, state, old_state);
		break;
	default:
		serial8250_do_pm(port, state, old_state);
		break;
	}

	clk_put(clk);
}

static void capri_serial_out(struct uart_port *p, int offset, int value)
{
	struct capri_uart_data *d = p->private_data;

	if (offset == UART_LCR)
		d->last_lcr = value;

	offset <<= p->regshift;
	writel(value, p->membase + offset);
}

static unsigned int capri_serial_in(struct uart_port *p, int offset)
{
	offset <<= p->regshift;

	return readl(p->membase + offset);
}

/* Offset for the DesignWare's UART Status Register. */
#define UART_USR	0x1f

static int capri_serial_handle_irq(struct uart_port *p)
{
	struct capri_uart_data *d = p->private_data;
	unsigned int iir = p->serial_in(p, UART_IIR);

	if (serial8250_handle_irq(p, iir)) {
		return 1;
	} else if ((iir & UART_IIR_BUSY) == UART_IIR_BUSY) {
		/* Clear the USR and write the LCR again. */
		(void)p->serial_in(p, UART_USR);
		p->serial_out(p, d->last_lcr, UART_LCR);

		return 1;
	}

	return 0;
}

static struct plat_serial8250_port uart_data[] = {
	KONA_8250PORT(UART0, "uartb_clk", 26000000, 0),
	KONA_8250PORT(UART1, "uartb2_clk", 14747415, 1),
#ifndef CONFIG_MACH_CAPRI_FPGA
	KONA_8250PORT(UART2, "uartb3_clk", 48000000, 2),
	KONA_8250PORT(UART3, "uartb4_clk", 26000000, 3),
#endif
	{.flags = 0,},
};

static struct platform_device board_serial_device = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = uart_data,
		},
};

#ifdef CONFIG_STM_TRACE
static struct stm_platform_data stm_pdata = {
	.regs_phys_base = STM_BASE_ADDR,
	.channels_phys_base = SWSTM_BASE_ADDR,
	.id_mask = 0x0,		/* Skip ID check/match */
	.final_funnel = CHAL_TRACE_FIN_FUNNEL,
};

struct platform_device kona_stm_device = {
	.name = "stm",
	.id = -1,
	.dev = {
		.platform_data = &stm_pdata,
		},
};
#endif

#if defined(CONFIG_HW_RANDOM_KONA)
static struct resource rng_device_resource[] = {
	[0] = {
	       .start = SEC_RNG_BASE_ADDR,
	       .end = SEC_RNG_BASE_ADDR + 0x14,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = BCM_INT_ID_SECURE_TRAP1,
	       .end = BCM_INT_ID_SECURE_TRAP1,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct platform_device rng_device = {
	.name = "kona_rng",
	.id = -1,
	.resource = rng_device_resource,
	.num_resources = ARRAY_SIZE(rng_device_resource),
};
#endif

#if defined(CONFIG_KONA_PWMC)
static struct resource pwm_device_resource[] = {
	[0] = {
	       .start = PWM_BASE_ADDR,
	       .end = PWM_BASE_ADDR + 0x10,
	       .flags = IORESOURCE_MEM,
	       },
};

#define PWM_PIN_SLEEP_CNFG_UNDEFINED \
	PIN_CFG(MAX, MAX, 0, OFF, OFF, 0, 0, 2MA)

static struct pin_config pwm_pin_sleep_cnfg_info[] = {

	PWM_PIN_SLEEP_CNFG_UNDEFINED,
	PWM_PIN_SLEEP_CNFG_UNDEFINED,
	PIN_CFG(GPIO06, GPIO_006, 0, ON, OFF, 0, 0, 8MA),
	PWM_PIN_SLEEP_CNFG_UNDEFINED,
	PWM_PIN_SLEEP_CNFG_UNDEFINED,
	PWM_PIN_SLEEP_CNFG_UNDEFINED,
};

static struct platform_device pwm_device = {
	.name = "kona_pwmc",
	.id = -1,
	.resource = pwm_device_resource,
	.num_resources = ARRAY_SIZE(pwm_device_resource),
	.dev = {
		.platform_data = &pwm_pin_sleep_cnfg_info,
		},
};
#endif

#if defined(CONFIG_MPCORE_WATCHDOG)
static struct resource wdt_device_resource[] = {
	[0] = {
	       .start = PTIM_BASE_ADDR,
	       .end = PTIM_BASE_ADDR + 0x34,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = BCM_INT_ID_PPI14,
	       .end = BCM_INT_ID_PPI14,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct platform_device wdt_device = {
	.name = "mpcore_wdt",
	.id = -1,
	.resource = wdt_device_resource,
	.num_resources = ARRAY_SIZE(wdt_device_resource),
	.dev = {
		.platform_data = "arm_periph_clk",
		},
};
#endif

#if defined(CONFIG_MTD_BCMNAND)
static struct platform_device nand_device = {
	.name = "bcmnand",
	.id = -1,
};
#endif

#if defined(CONFIG_RTC_DRV_ISLAND)
static struct resource rtc_device_resource[] = {
	[0] = {
	       .start = RTC_APB_BASE_ADDR,
	       .end = RTC_APB_BASE_ADDR + 0x24,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = BCM_INT_ID_BBL1,
	       .end = BCM_INT_ID_BBL1,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = BCM_INT_ID_BBL2,
	       .end = BCM_INT_ID_BBL2,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct platform_device rtc_device = {
	.name = "island-rtc",
	.id = -1,
	.resource = rtc_device_resource,
	.num_resources = ARRAY_SIZE(rtc_device_resource),
	.dev = {
		.platform_data = "bbl_apb_clk",
		},
};
#endif

/* ARM performance monitor unit */
static struct resource pmu_resource = {
	.start = BCM_INT_ID_PMU_IRQ0,
	.end = BCM_INT_ID_PMU_IRQ0,
	.flags = IORESOURCE_IRQ,
};

static struct platform_device pmu_device = {
	.name = "arm-pmu",
	.id = ARM_PMU_DEVICE_CPU,
	.resource = &pmu_resource,
	.num_resources = 1,
};

/* SPI configuration */
static struct resource kona_sspi_spi0_resource[] = {
	[0] = {
	       .start = SSP0_BASE_ADDR,
	       .end = SSP0_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = BCM_INT_ID_SSP0,
	       .end = BCM_INT_ID_SSP0,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct spi_kona_platform_data sspi_spi0_info = {
	.enable_dma = 1,
	.cs_line = 1,
	.mode = SPI_LOOP | SPI_MODE_3 | SPI_3WIRE,
};

static struct platform_device kona_sspi_spi0_device = {
	.dev = {
		.platform_data = &sspi_spi0_info,
		},
	.name = "kona_sspi_spi",
	.id = 0,
	.resource = kona_sspi_spi0_resource,
	.num_resources = ARRAY_SIZE(kona_sspi_spi0_resource),
};

#ifdef CONFIG_SPI_KONA_SSP3_SUPPORT
static struct resource kona_sspi_spi3_resource[] = {
	[0] = {
		.start = SSP3_BASE_ADDR,
		.end = SSP3_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
		},
	[1] = {
		.start = BCM_INT_ID_SSP3_ERR,
		.end = BCM_INT_ID_SSP3_ERR,
		.flags = IORESOURCE_IRQ,
		},
};

static struct spi_kona_platform_data sspi_spi3_info = {
	.enable_dma = 1,
	.cs_line = 1,
	.mode = SPI_LOOP | SPI_MODE_3 | SPI_3WIRE,
};

static struct platform_device kona_sspi_spi3_device = {
	.dev = {
		.platform_data = &sspi_spi3_info,
		},
	.name = "kona_sspi_spi",
	.id = 2,
	.resource = kona_sspi_spi3_resource,
	.num_resources = ARRAY_SIZE(kona_sspi_spi3_resource),
};
#endif /* CONFIG_SPI_KONA_SSP3_SUPPORT */

#ifdef CONFIG_USB_DWC_OTG
static struct resource kona_hsotgctrl_platform_resource[] = {
	[0] = {
	       .start = HSOTG_CTRL_BASE_ADDR,
	       .end = HSOTG_CTRL_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = CHIPREGS_BASE_ADDR,
	       .end = CHIPREGS_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[2] = {
	       .start = HUB_CLK_BASE_ADDR,
	       .end = HUB_CLK_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[3] = {
	       .start = BCM_INT_ID_USB_OTG_WAKEUP,
	       .end = BCM_INT_ID_USB_OTG_WAKEUP,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct bcm_hsotgctrl_platform_data hsotgctrl_plat_data = {
	.hsotgctrl_virtual_mem_base = (unsigned long)
	    KONA_USB_HSOTG_CTRL_VA,
	.chipreg_virtual_mem_base = (unsigned long)KONA_CHIPREG_VA,
	.irq = BCM_INT_ID_USB_OTG_WAKEUP,
	.usb_ahb_clk_name = USB_OTG_AHB_BUS_CLK_NAME_STR,
	.mdio_mstr_clk_name = MDIOMASTER_PERI_CLK_NAME_STR,
};

static struct platform_device board_kona_hsotgctrl_platform_device = {
	.name = "bcm_hsotgctrl",
	.id = -1,
	.resource = kona_hsotgctrl_platform_resource,
	.num_resources = ARRAY_SIZE(kona_hsotgctrl_platform_resource),
	.dev = {
		.platform_data = &hsotgctrl_plat_data,
		},
};

static struct resource kona_usb_phy_platform_resource[] = {
	[0] = {
	       .start = HSOTG_CTRL_BASE_ADDR,
	       .end = HSOTG_CTRL_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = CHIPREGS_BASE_ADDR,
	       .end = CHIPREGS_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
};

static struct platform_device board_kona_usb_phy_platform_device = {
	.name = "bcm_hsotgctrl_phy_mdio",
	.id = -1,
	.resource = kona_usb_phy_platform_resource,
	.num_resources = ARRAY_SIZE(kona_usb_phy_platform_resource),
	.dev = {
		.platform_data = &hsotgctrl_plat_data,
		},

};

static struct resource kona_otg_platform_resource[] = {
	[0] = {			/* Keep HSOTG_BASE_ADDR as first IORESOURCE_MEM */
	       /* to be compatible with legacy code */
	       .start = HSOTG_BASE_ADDR,
	       .end = HSOTG_BASE_ADDR + SZ_64K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = BCM_INT_ID_USB_HSOTG,
	       .end = BCM_INT_ID_USB_HSOTG,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct platform_device board_kona_otg_platform_device = {
	.name = "dwc_otg",
	.id = -1,
	.resource = kona_otg_platform_resource,
	.num_resources = ARRAY_SIZE(kona_otg_platform_resource),
};
#endif

#ifdef CONFIG_SENSORS_KONA
static struct resource board_tmon_resource[] = {
	{			/* For Current Temperature */
	 .start = TMON_BASE_ADDR,
	 .end = TMON_BASE_ADDR + SZ_4K - 1,
	 .flags = IORESOURCE_MEM,
	 },
	{			/* For Temperature IRQ */
	 .start = BCM_INT_ID_TEMP_MON,
	 .end = BCM_INT_ID_TEMP_MON,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct tmon_data tmon_pdata = {
	.critical_temp = 105000,
	.warning_temp_thresholds = {
		[0] = 85000,
		[1] = 90000,
		[2] = 95000,
		[3] = 100000,
	},
	.threshold_enable = {
		[0] = THRES_DISABLE,
		[1] = THRES_DISABLE,
		[2] = THRES_DISABLE,
		[3] = THRES_DISABLE,
	},
	.max_cpufreq = {
		[0] = 800000,
		[1] = 800000,
		[2] = 800000,
		[3] = 312000,
	},
	.polling_interval_ms = 500,
	.temp_hysteresis = 500,
};

struct platform_device tmon_device = {
	.name = "kona-tmon",
	.id = -1,
	.resource = board_tmon_resource,
	.num_resources = ARRAY_SIZE(board_tmon_resource),
	.dev = {
		.platform_data = &tmon_pdata,
		},
};

static struct resource board_thermal_resource[] = {
	{			/* For Current Temperature */
	 .start = TMON_BASE_ADDR,
	 .end = TMON_BASE_ADDR + SZ_4K - 1,
	 .flags = IORESOURCE_MEM,
	 },
	{			/* For Temperature IRQ */
	 .start = BCM_INT_ID_TEMP_MON,
	 .end = BCM_INT_ID_TEMP_MON,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct thermal_sensor_config sensor_data[] = {
	{			/*TMON sensor */
	 .thermal_id = 1,
	 .thermal_name = "tmon",
	 .thermal_type = SENSOR_BB_TMON,
	 .thermal_mc = 0,
	 .thermal_read = SENSOR_READ_DIRECT,
	 .thermal_location = 1,
	 .thermal_warning_lvl_1 = 100000,
	 .thermal_warning_lvl_2 = 110000,
	 .thermal_fatal_lvl = 120000,
	 .thermal_lowest = 200000,
	 .thermal_highest = 0,
	 .thermal_warning_action = THERM_ACTION_NOTIFY,
	 .thermal_fatal_action = THERM_ACTION_NOTIFY_SHUTDOWN,
	 .thermal_sensor_param = 0,
	 .thermal_control = SENSOR_INTERRUPT,
	 .convert_callback = NULL,
	 },
	{			/*NTC (battery) sensor */
	 .thermal_id = 2,
	 .thermal_name = "battery",
	 .thermal_type = SENSOR_BATTERY,
	 .thermal_mc = 0,
	 .thermal_read = SENSOR_READ_PMU_I2C,
	 .thermal_location = 2,
	 .thermal_warning_lvl_1 = 105000,
	 .thermal_warning_lvl_2 = 115000,
	 .thermal_fatal_lvl = 125000,
	 .thermal_lowest = 200000,
	 .thermal_highest = 0,
	 .thermal_warning_action = THERM_ACTION_NOTIFY,
	 .thermal_fatal_action = THERM_ACTION_NOTIFY_SHUTDOWN,
#ifdef CONFIG_MFD_BCM_PMU590XX
	 .thermal_sensor_param = ADC_NTC_CHANNEL,
#else
	 .thermal_sensor_param = PMU_ADC_NTC,
#endif
	 .thermal_control = SENSOR_PERIODIC_READ,
	 .convert_callback = NULL,
	 },
	{			/*32kHz crystal sensor */
	 .thermal_id = 3,
	 .thermal_name = "32k",
	 .thermal_type = SENSOR_CRYSTAL,
	 .thermal_mc = 0,
	 .thermal_read = SENSOR_READ_PMU_I2C,
	 .thermal_location = 3,
	 .thermal_warning_lvl_1 = 106000,
	 .thermal_warning_lvl_2 = 116000,
	 .thermal_fatal_lvl = 126000,
	 .thermal_lowest = 200000,
	 .thermal_highest = 0,
	 .thermal_warning_action = THERM_ACTION_NOTIFY,
	 .thermal_fatal_action = THERM_ACTION_NOTIFY_SHUTDOWN,
#ifdef CONFIG_MFD_BCM_PMU590XX
	 .thermal_sensor_param = ADC_32KTEMP_CHANNEL,
#else
	 .thermal_sensor_param = PMU_ADC_32KTEMP,
#endif
	 .thermal_control = SENSOR_PERIODIC_READ,
	 .convert_callback = NULL,
	 },
	{			/*PA sensor */
	 .thermal_id = 4,
	 .thermal_name = "PA",
	 .thermal_type = SENSOR_PA,
	 .thermal_mc = 0,
	 .thermal_read = SENSOR_READ_PMU_I2C,
	 .thermal_location = 4,
	 .thermal_warning_lvl_1 = 107000,
	 .thermal_warning_lvl_2 = 117000,
	 .thermal_fatal_lvl = 127000,
	 .thermal_lowest = 200000,
	 .thermal_highest = 0,
	 .thermal_warning_action = THERM_ACTION_NOTIFY,
	 .thermal_fatal_action = THERM_ACTION_NOTIFY_SHUTDOWN,
#ifdef CONFIG_MFD_BCM_PMU590XX
	 .thermal_sensor_param = ADC_PATEMP_CHANNEL,
#else
	 .thermal_sensor_param = PMU_ADC_PATEMP,
#endif
	 .thermal_control = SENSOR_PERIODIC_READ,
	 .convert_callback = NULL,
	 }
};

static struct therm_data thermal_pdata = {
	.flags = 0,
	.thermal_update_interval = 0,
	.num_sensors = 4,
	.sensors = sensor_data,
};

struct platform_device thermal_device = {
	.name = "kona-thermal",
	.id = -1,
	.resource = board_thermal_resource,
	.num_resources = ARRAY_SIZE(board_thermal_resource),
	.dev = {
		.platform_data = &thermal_pdata,
		},
};
#endif

#ifdef CONFIG_KONA_CPU_FREQ_DRV
struct kona_freq_tbl kona_freq_tbl[] = {
#ifdef CONFIG_CAPRI_156M
	FTBL_INIT(156000, PI_PROC_OPP_ECONOMY),
	FTBL_INIT(312000, PI_PROC_OPP_ECONOMY1),
	FTBL_INIT(600000, PI_PROC_OPP_NORMAL),
	FTBL_INIT(800000, PI_PROC_OPP_TURBO1),
	FTBL_INIT(1200000, PI_PROC_OPP_TURBO),
#else
#ifdef CONFIG_CAPRI_PM_A1
	FTBL_INIT(312000, PI_PROC_OPP_ECONOMY1),
	FTBL_INIT(600000, PI_PROC_OPP_NORMAL),
	FTBL_INIT(800000, PI_PROC_OPP_TURBO1),
	FTBL_INIT(1200000, PI_PROC_OPP_TURBO),
#else
	FTBL_INIT(312000, PI_PROC_OPP_ECONOMY1),
	FTBL_INIT(550000, PI_PROC_OPP_NORMAL),
	FTBL_INIT(733333, PI_PROC_OPP_TURBO1),
	FTBL_INIT(1100000, PI_PROC_OPP_TURBO),
#endif
#endif
};

void capri_cpufreq_init(void)
{
	struct clk *a9_pll_chnl0;
	struct clk *a9_pll_chnl1;
	a9_pll_chnl0 = clk_get(NULL, A9_PLL_CHNL0_CLK_NAME_STR);
	a9_pll_chnl1 = clk_get(NULL, A9_PLL_CHNL1_CLK_NAME_STR);

	BUG_ON(IS_ERR(a9_pll_chnl0) || IS_ERR(a9_pll_chnl1));

	/*Update DVFS freq table based on PLL settings done by the loader */
#ifdef CONFIG_CAPRI_156M
	kona_freq_tbl[2].cpu_freq = 3 * clk_get_rate(a9_pll_chnl0) / 1000 / 4;
	kona_freq_tbl[3].cpu_freq = clk_get_rate(a9_pll_chnl0) / 1000;
	kona_freq_tbl[4].cpu_freq = clk_get_rate(a9_pll_chnl1) / 1000;

	pr_info("%s a9_pll_chnl0 freq = %dKhz a9_pll_chnl1 freq = %dKhz\n",
		__func__, kona_freq_tbl[3].cpu_freq, kona_freq_tbl[4].cpu_freq);
#else
	kona_freq_tbl[1].cpu_freq = 3 * clk_get_rate(a9_pll_chnl0) / 1000 / 4;
	kona_freq_tbl[2].cpu_freq = clk_get_rate(a9_pll_chnl0) / 1000;
	kona_freq_tbl[3].cpu_freq = clk_get_rate(a9_pll_chnl1) / 1000;

	pr_info("%s a9_pll_chnl0 freq = %dKhz a9_pll_chnl1 freq = %dKhz\n",
		__func__, kona_freq_tbl[2].cpu_freq, kona_freq_tbl[3].cpu_freq);
#endif
}

struct kona_cpufreq_drv_pdata kona_cpufreq_drv_pdata = {
	.flags = KONA_CPUFREQ_UPDATE_LPJ,
	.freq_tbl = kona_freq_tbl,
	.num_freqs = ARRAY_SIZE(kona_freq_tbl),
	/*FIX ME: To be changed according to the cpu latency */
	.latency = 10 * 1000,
	.pi_id = PI_MGR_PI_ID_ARM_CORE,
	.cpufreq_init = capri_cpufreq_init,
};

static struct platform_device kona_cpufreq_device = {
	.name = "kona-cpufreq-drv",
	.id = -1,
	.dev = {
		.platform_data = &kona_cpufreq_drv_pdata,
		},
};
#endif /*CONFIG_KONA_CPU_FREQ_DRV */

#ifdef CONFIG_KONA_AVS
#define SILICON_NUM_BIN  5

void avs_silicon_type_notify(u32 silicon_type_csr,
			     u32 silicon_type_msr, u32 silicon_type_vsr,
			     int freq_id)
{
	pr_info("%s:\n", __func__);
	pr_info("    silicon_type_csr = %d\n", silicon_type_csr);
	pr_info("    silicon_type_msr = %d\n", silicon_type_msr);
	pr_info("    silicon_type_vsr = %d\n", silicon_type_vsr);
	pr_info("    OTPed freq_id    = %d\n", freq_id);

	pm_init_pmu_sr_vlt_map_table(silicon_type_csr,
				     silicon_type_msr, silicon_type_vsr,
				     freq_id);
}

static struct kona_avs_lut_entry avs_lut_msr[] = {
	{95, 131, 160, 160, 160},
	{156, 202, 245, 245, 245},
	{94, 125, 155, 155, 155},
	{128, 168, 195, 195, 195},
};

static struct kona_avs_lut_entry avs_lut_vsr[] = {
	{95, 131, 160, 160, 160},
	{156, 202, 245, 245, 245},
	{94, 125, 155, 155, 155},
	{128, 168, 195, 195, 195},
};

static struct kona_avs_lut_entry avs_lut_irdrop = {
	470, 490, 510, 530, 530
};

/* index = ATE_AVS_BIN[3:0]*/
static struct kona_ate_lut_entry_csr ate_lut_csr[] = {
	{A9_FREQ_1_GHZ, SILICON_SS},	/* 0 */
	{A9_FREQ_1_GHZ, SILICON_FF},	/* 1 */
	{A9_FREQ_1_GHZ, SILICON_TF},	/* 2 */
	{A9_FREQ_1_GHZ, SILICON_TT},	/* 3 */
	{A9_FREQ_1_GHZ, SILICON_TS},	/* 4 */
	{A9_FREQ_1_GHZ, SILICON_SS},	/* 5 */
	{A9_FREQ_1200_MHZ, SILICON_FF},	/* 6  */
	{A9_FREQ_1200_MHZ, SILICON_TF},	/* 7  */
	{A9_FREQ_1200_MHZ, SILICON_TT},	/* 8  */
	{A9_FREQ_1200_MHZ, SILICON_TS},	/* 9  */
	{A9_FREQ_1200_MHZ, SILICON_SS},	/* 10 */
	{A9_FREQ_1400_MHZ, SILICON_FF},	/* 11 */
	{A9_FREQ_1400_MHZ, SILICON_TF},	/* 12  */
	{A9_FREQ_1400_MHZ, SILICON_TT},	/* 13  */
	{A9_FREQ_1400_MHZ, SILICON_TS},	/* 14 */
	{A9_FREQ_1400_MHZ, SILICON_SS},	/* 15 */
};

static u32 ate_lut_msr[] = {
	SILICON_SS,		/* 0 */
	SILICON_FF,		/* 1 */
	SILICON_TF,		/* 2 */
	SILICON_TT,		/* 3 */
	SILICON_TS,		/* 4 */
	SILICON_SS,		/* 5 */
	ATE_FIELD_RESERVED,	/* 6  */
	ATE_FIELD_RESERVED,	/* 7  */
	ATE_FIELD_RESERVED,	/* 8  */
	ATE_FIELD_RESERVED,	/* 9  */
	ATE_FIELD_RESERVED,	/* 10 */
	ATE_FIELD_RESERVED,	/* 11 */
	ATE_FIELD_RESERVED,	/* 12  */
	ATE_FIELD_RESERVED,	/* 13  */
	ATE_FIELD_RESERVED,	/* 14 */
	ATE_FIELD_RESERVED,	/* 15 */
};

static u32 ate_lut_vsr[] = {
	SILICON_SS,		/* 0 */
	SILICON_FF,		/* 1 */
	SILICON_TF,		/* 2 */
	SILICON_TT,		/* 3 */
	SILICON_TS,		/* 4 */
	SILICON_SS,		/* 5 */
	ATE_FIELD_RESERVED,	/* 6 */
	ATE_FIELD_RESERVED,	/* 7  */
	ATE_FIELD_RESERVED,	/* 8  */
	ATE_FIELD_RESERVED,	/* 9  */
	ATE_FIELD_RESERVED,	/* 10 */
	ATE_FIELD_RESERVED,	/* 11 */
	ATE_FIELD_RESERVED,	/* 12  */
	ATE_FIELD_RESERVED,	/* 13  */
	ATE_FIELD_RESERVED,	/* 14 */
	ATE_FIELD_RESERVED,	/* 15 */
};

static struct kona_avs_pdata avs_pdata = {
	.flags = AVS_TYPE_OPEN | AVS_READ_FROM_MEM | AVS_ATE_FEATURE_ENABLE,
	.avs_mon_addr = 0x34053FA8,
	/* Mem addr where ATE value is copied by ABI */
	.avs_ate_addr = 0x34053FA0,

	.ate_default_silicon_type = SILICON_TS,
	.ate_default_cpu_freq = A9_FREQ_1200_MHZ,

	.avs_lut_irdrop = &avs_lut_irdrop,
	.avs_lut_msr = avs_lut_msr,
	.avs_lut_vsr = avs_lut_vsr,

	.ate_lut_csr = ate_lut_csr,
	.ate_lut_msr = ate_lut_msr,
	.ate_lut_vsr = ate_lut_vsr,

	.silicon_type_notify = avs_silicon_type_notify,
};

struct platform_device kona_avs_device = {
	.name = "kona-avs",
	.id = -1,
	.dev = {
		.platform_data = &avs_pdata,
		},
};

#endif

atomic_t nohz_pause = ATOMIC_INIT(0);

#ifdef CONFIG_DMAC_PL330
static struct kona_pl330_data capri_pl330_pdata = {
	/* Non Secure DMAC virtual base address */
	.dmac_ns_base = KONA_DMAC_NS_VA,
	/* Secure DMAC virtual base address */
	.dmac_s_base = KONA_DMAC_S_VA,
	/* # of PL330 dmac channels 'configurable' */
	.num_pl330_chans = 8,
	/* irq number to use */
	.irq_base = BCM_INT_ID_RESERVED184,
	/* # of PL330 Interrupt lines connected to GIC */
	.irq_line_count = 8,
};

static struct platform_device pl330_dmac_device = {
	.name = "kona-dmac-pl330",
	.id = 0,
	.dev = {
		.platform_data = &capri_pl330_pdata,
		.coherent_dma_mask = DMA_BIT_MASK(64),
		},
};
#endif

#if defined(CONFIG_CRYPTO_DEV_BRCM_SPUM_HASH)
static struct resource board_spum_resource[] = {
	[0] = {
	       .start = SEC_SPUM_NS_APB_BASE_ADDR,
	       .end = SEC_SPUM_NS_APB_BASE_ADDR + SZ_64K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = SPUM_NS_BASE_ADDR,
	       .end = SPUM_NS_BASE_ADDR + SZ_64K - 1,
	       .flags = IORESOURCE_MEM,
	       }
};

static struct platform_device board_spum_device = {
	.name = "brcm-spum",
	.id = 0,
	.resource = board_spum_resource,
	.num_resources = ARRAY_SIZE(board_spum_resource),
};
#endif

#if defined(CONFIG_CRYPTO_DEV_BRCM_SPUM_AES)
static struct resource board_spum_aes_resource[] = {
	[0] = {
	       .start = SEC_SPUM_NS_APB_BASE_ADDR,
	       .end = SEC_SPUM_NS_APB_BASE_ADDR + SZ_64K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = SPUM_NS_BASE_ADDR,
	       .end = SPUM_NS_BASE_ADDR + SZ_64K - 1,
	       .flags = IORESOURCE_MEM,
	       }
};

static struct platform_device board_spum_aes_device = {
	.name = "brcm-spum-aes",
	.id = 0,
	.resource = board_spum_aes_resource,
	.num_resources = ARRAY_SIZE(board_spum_aes_resource),
};
#endif

#ifdef CONFIG_PM_WAKELOCK_HELPER
static struct platform_device kpwh_device = {
	.name = "kpwh",
	.id = -1,
};
#endif

#ifdef CONFIG_BCM_VC_SYSMAN_REMOTE
static struct platform_device vc_sysman_remote_device = {
	.name = "vc-sysman-remote",
	.id = -1,
};
#endif

#ifdef CONFIG_BCM_VC_WATCHDOG
static struct platform_device vc_watchdog_device = {
	.name = "vc-watchdog",
	.id   = -1,
};
#endif

#ifdef CONFIG_BCM_VC_PMU_REQUEST
static struct platform_device vc_pmu_request_device = {
	.name = "vc-pmu-request",
	.id = -1,
};
#endif

#ifdef CONFIG_BCM_VC_DISPLAY
static struct platform_device vc_display_device = {
	.name = "vc-display",
	.id = -1,
};
#endif

/* Allocate the top 24M of the DRAM for the pmem. */
static struct android_pmem_platform_data android_pmem_data = {
	.name = "pmem",
	.cmasize = 0,
	.carveout_base = 0,
	.carveout_size = 0,
};

static struct platform_device android_pmem = {
	.name = "android_pmem",
	.id = 0,
	.dev = {
		.platform_data = &android_pmem_data,
		},
};

static int __init setup_pmem_pages(char *str)
{
	char *endp = NULL;
	if (str) {
		android_pmem_data.cmasize = memparse((const char *)str, &endp);
		printk(KERN_INFO "PMEM size is 0x%08x Bytes\n",
		       (unsigned int)android_pmem_data.cmasize);
	} else {
		printk(KERN_INFO "\"pmem=\" option is not set!!!\n");
		printk(KERN_INFO
		       "Unable to determine the memory region for pmem!!!\n");
	}
	return 0;
}

early_param("pmem", setup_pmem_pages);

static int __init setup_pmem_carveout_pages(char *str)
{
	char *endp = NULL;
	phys_addr_t carveout_size = 0;
	if (str) {
		carveout_size = memparse((const char *)str, &endp);
		if (carveout_size & (PAGE_SIZE - 1)) {
			printk(KERN_INFO
			       "carveout size is not aligned to 0x%08x\n",
			       (1 << MAX_ORDER));
			carveout_size = ALIGN(carveout_size, PAGE_SIZE);
			printk(KERN_INFO "Aligned carveout size is 0x%08x\n",
			       carveout_size);
		}
		printk(KERN_INFO "PMEM: Carveout Mem (0x%08x)\n",
		       carveout_size);
	} else {
		printk(KERN_INFO "PMEM: Invalid \"carveout=\" value.\n");
	}

	if (carveout_size)
		android_pmem_data.carveout_size = carveout_size;

	return 0;
}

early_param("carveout", setup_pmem_carveout_pages);

void __init board_common_reserve(void)
{
	int err;
	phys_addr_t carveout_size, carveout_base;
	unsigned long cmasize;

	carveout_size = android_pmem_data.carveout_size;
	cmasize = android_pmem_data.cmasize;

	printk(KERN_INFO "PMEM: carveout_size:%d\n", carveout_size);

	if (carveout_size) {
		carveout_base = memblock_alloc(carveout_size, 1);
		memblock_free(carveout_base, carveout_size);
		err = memblock_remove(carveout_base, carveout_size);
		if (!err) {
			printk(KERN_INFO
			       "PMEM: Carve memory from (%08x-%08x)\n",
			       carveout_base, carveout_base + carveout_size);
			android_pmem_data.carveout_base = carveout_base;
		} else {
			printk(KERN_INFO "PMEM: Carve out memory failed\n");
		}
	}
#if 0
	if (dma_declare_contiguous(&android_pmem.dev, cmasize, 0, 0)) {
		printk(KERN_INFO "PMEM: Failed to reserve CMA region\n");
		android_pmem_data.cmasize = 0;
	}
#endif
}

/* Common devices among all Island boards */
static struct platform_device *board_common_plat_devices[] __initdata = {
	&board_serial_device,
	&kona_sspi_spi0_device,
#if defined(CONFIG_SPI_KONA_SSP3_SUPPORT)
	&kona_sspi_spi3_device,
#endif /* CONFIG_SPI_KONA_SSP3_SUPPORT */
#if defined(CONFIG_MPCORE_WATCHDOG)
	&wdt_device,
#endif
#if defined(CONFIG_HW_RANDOM_KONA)
	&rng_device,
#endif
#if defined(CONFIG_RTC_DRV_ISLAND)
	&rtc_device,
#endif
#if defined(CONFIG_MTD_BCMNAND)
	&nand_device,
#endif
#if defined(CONFIG_KONA_PWMC)
	&pwm_device,
#endif

#ifdef CONFIG_STM_TRACE
	&kona_stm_device,
#endif
	&pmu_device,

#ifdef CONFIG_USB_DWC_OTG
	&board_kona_hsotgctrl_platform_device,
	&board_kona_usb_phy_platform_device,
	&board_kona_otg_platform_device,
#endif

#ifdef CONFIG_KONA_AVS
	&kona_avs_device,
#endif

#ifdef CONFIG_KONA_CPU_FREQ_DRV
	&kona_cpufreq_device,
#endif

#ifdef CONFIG_SENSORS_KONA
	&tmon_device,
	&thermal_device,
#endif

#ifdef CONFIG_DMAC_PL330
	&pl330_dmac_device,
#endif

#ifdef CONFIG_CRYPTO_DEV_BRCM_SPUM_HASH
	&board_spum_device,
#endif

#ifdef CONFIG_CRYPTO_DEV_BRCM_SPUM_AES
	&board_spum_aes_device,
#endif

#ifdef CONFIG_PM_WAKELOCK_HELPER
	&kpwh_device,
#endif
#ifdef CONFIG_BCM_VC_SYSMAN_REMOTE
	&vc_sysman_remote_device,
#endif
#ifdef CONFIG_BCM_VC_WATCHDOG
	&vc_watchdog_device,
#endif

#ifdef CONFIG_BCM_VC_PMU_REQUEST
	&vc_pmu_request_device,
#endif

#ifdef CONFIG_BCM_VC_DISPLAY
	&vc_display_device,
#endif
};

void __init board_add_common_devices(void)
{

	/* fix for Jira HWCAPRI-1760 */
	writel(0xA5A501, KONA_KPM_CLK_VA);
	writel(KPM_CLK_MGR_REG_USBH2_CLKGATE_USBH2_AHB_HW_SW_GATING_SEL_MASK |
	       KPM_CLK_MGR_REG_USBH2_CLKGATE_USBH2_AHB_CLK_EN_MASK,
	       KONA_KPM_CLK_VA + KPM_CLK_MGR_REG_USBH2_CLKGATE_OFFSET);
	mdelay(2);
	writel(readl(KONA_USB_HOST_CTRL_VA + USBH_PHY_AFE_CTRL_OFFSET)
	       | USBH_PHY_AFE_CTRL_SW_AFE_LDO_PWRDWNB_1_MASK |
	       USBH_PHY_AFE_CTRL_SW_AFE_LDOBG_PWRDWNB_MASK,
	       KONA_USB_HOST_CTRL_VA + USBH_PHY_AFE_CTRL_OFFSET);
	mdelay(2);
	writel(readl(KONA_USB_HOST_CTRL_VA + USBH_PHY_PHY_CTRL_OFFSET)
	       & ~USBH_PHY_PHY_CTRL_SW_PHY_ISO_MASK,
	       KONA_USB_HOST_CTRL_VA + USBH_PHY_PHY_CTRL_OFFSET);
	mdelay(2);
	writel(readl(KONA_USB_HOST_CTRL_VA + USBH_PHY_PHY_CTRL_OFFSET)
	       | USBH_PHY_PHY_CTRL_SW_PHY_ISO_MASK,
	       KONA_USB_HOST_CTRL_VA + USBH_PHY_PHY_CTRL_OFFSET);
	writel(readl(KONA_USB_HOST_CTRL_VA + USBH_PHY_AFE_CTRL_OFFSET)
	       & ~(USBH_PHY_AFE_CTRL_SW_AFE_LDO_PWRDWNB_1_MASK |
		   USBH_PHY_AFE_CTRL_SW_AFE_LDOBG_PWRDWNB_MASK),
	       KONA_USB_HOST_CTRL_VA + USBH_PHY_AFE_CTRL_OFFSET);
	writel(KPM_CLK_MGR_REG_USBH2_CLKGATE_USBH2_AHB_HW_SW_GATING_SEL_MASK,
	       KONA_KPM_CLK_VA + KPM_CLK_MGR_REG_USBH2_CLKGATE_OFFSET);
	pr_info("USBH Power Down\n");

	platform_add_devices(board_common_plat_devices,
			     ARRAY_SIZE(board_common_plat_devices));

	platform_device_register(&android_pmem);
}
