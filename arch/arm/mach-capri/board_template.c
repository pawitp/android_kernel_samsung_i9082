/*****************************************************************************
* Copyright 2011 Broadcom Corporation.  All rights reserved.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a
* license other than the GPL, without Broadcom's express prior written
* consent.
*****************************************************************************/

/*
 * A board template for adding devices and pass their associated board
 * dependent configurations as platform_data into the drivers
 *
 * This file needs to be included by the board specific source code
 */

#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/serial_8250.h>
#include <linux/i2c.h>
#include <linux/i2c-kona.h>
#include <linux/debugfs.h>

#include <asm/memory.h>
#include <asm/sizes.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/hardware/gic.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/clock.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/kona.h>
#include <mach/dma_mmap.h>
#include <mach/sdma.h>

#if defined(CONFIG_SPI_SPIDEV) && defined(CONFIG_SPI_SSPI_KONA)
#include <linux/spi/spi.h>
#endif

#ifdef CONFIG_I2C_GPIO
#include <linux/i2c-gpio.h>
#endif

#if defined(CONFIG_BCM2079X_NFC_I2C)
#include <linux/nfc/bcm2079x.h>
#include <bcm2079x_nfc_settings.h>
#endif

#include <linux/mmc/sh_mobile_sdhi.h>
#include <linux/mmc/host.h>

#include <mach/sdio_platform.h>
#include <mach/usbh_cfg.h>

#include <sdio_settings.h>
#include <i2c_settings.h>
#include <usbh_settings.h>

#include <mach/pinmux.h>

#if defined(CONFIG_BCM_HALAUDIO)
#include <halaudio_settings.h>
#endif

#if defined(CONFIG_BCM_ALSA_SOUND)
#include <mach/caph_platform.h>
#include <caph_settings.h>
#endif

#if defined(CONFIG_BCM_RFKILL) || defined(CONFIG_BCM_RFKILL_MODULE)
#include <linux/broadcom/bcmbt_rfkill.h>
#include <bcmbt_rfkill_settings.h>
#endif

#if defined(CONFIG_BCM_BT_LPM) || defined(CONFIG_BCM_BT_LPM_MODULE)
#include <linux/broadcom/bcmbt_lpm.h>
#include <bcmbt_lpm_settings.h>
#endif

#ifdef CONFIG_BCM_BZHW
#include <linux/broadcom/bcm_bzhw.h>
#endif
#ifdef CONFIG_USB_SWITCH_TSU6721
#include <linux/power_supply.h>
#include <linux/i2c/tsu6721.h>
#endif
#ifdef CONFIG_USB_SWITCH_FSA9485
#include <linux/power_supply.h>
#include <linux/switch.h>
#include <linux/i2c/fsa9485.h>
#endif
#ifdef CONFIG_BQ24272_CHARGER
#include <linux/bq24272_charger.h>
#endif
#ifdef CONFIG_SMB358_CHARGER
#include <linux/smb358_charger.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_EGALAX_I2C) || defined(CONFIG_TOUCHSCREEN_EGALAX_I2C_MODULE)
#include <linux/i2c/egalax_i2c_ts.h>
#include <egalax_i2c_ts_settings.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_BCM915500) || defined(CONFIG_TOUCHSCREEN_BCM915500_MODULE)
#include <linux/i2c/bcm15500_i2c_ts.h>
#endif

#if defined(CONFIG_INPUT_MPU6050) || defined(CONFIG_INPUT_MPU6500)
#include <linux/mpu6k_input.h>
#endif

#if defined(CONFIG_OPTICAL_CM3663) 
#include <linux/cm3663.h>
#endif

#if  defined (CONFIG_SENSORS_GP2A030)
#include <linux/gp2ap030.h>
#endif

#if defined(CONFIG_BMP18X_I2C) || defined(CONFIG_BMP18X_I2C_MODULE)
#include <linux/bmp18x.h>
#include <bmp18x_i2c_settings.h>
#endif

#if defined(CONFIG_AL3006) || defined(CONFIG_AL3006_MODULE)
#include <linux/al3006.h>
#include <al3006_i2c_settings.h>
#endif

#if defined(CONFIG_AMI306) || defined(CONFIG_AMI306_MODULE)
#include <linux/ami306_def.h>
#include <linux/ami_sensor.h>
#include <ami306_settings.h>
#endif

#if defined(CONFIG_INPUT_HSCDTD006A) || defined(CONFIG_INPUT_HSCDTD006A_MODULE)
#include <linux/hscdtd.h>
#include <hscdtd006a_settings.h>
#endif

#if defined(CONFIG_NET_ISLAND)
#include <mach/net_platform.h>
#include <net_settings.h>
#endif

#if defined(CONFIG_LEDS_GPIO) || defined(CONFIG_LEDS_GPIO_MODULE)
#include <leds_gpio_settings.h>
#include <linux/leds.h>
#endif

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <gpio_keys_settings.h>
#endif

#if defined(CONFIG_KONA_VCHIQ) || defined(CONFIG_KONA_VCHIQ_MODULE)
#include <mach/io_map.h>
#include <mach/aram_layout.h>

#include <linux/broadcom/vchiq_platform_data_kona.h>
#include <linux/broadcom/vchiq_platform_data_memdrv_kona.h>
#include <camera_settings.h>
#endif

#if defined(CONFIG_KEYBOARD_KONA) || defined(CONFIG_KEYBOARD_KONA_MODULE)
#include <linux/kona_keypad.h>
#include <keymap_settings.h>
#endif

#if defined(CONFIG_BCM_GPS) || defined(CONFIG_BCM_GPS_MODULE)
#include <gps_settings.h>
#include <linux/broadcom/gps.h>
#endif

#if defined(CONFIG_BCM_HAPTICS) || defined(CONFIG_BCM_HAPTICS_MODULE)
#include <linux/broadcom/bcm_haptics.h>
#include <bcm_haptics_settings.h>
#endif

#if defined(CONFIG_BCM_HEADSET_SW)
#include <linux/broadcom/headset_cfg.h>
#include <headset_settings.h>
#endif

#if defined(CONFIG_KONA_HEADSET) || defined(CONFIG_KONA_HEADSET_MULTI_BUTTON)
#include <mach/kona_headset_pd.h>
#endif

#if defined(CONFIG_BCM_HDMI_DET) || defined(CONFIG_BCM_HDMI_DET_MODULE)
#include <linux/broadcom/hdmi_cfg.h>
#include <hdmi_settings.h>
#endif

#if defined(CONFIG_TFT_PANEL) || defined(CONFIG_TFT_PANEL_MODULE)
#include <linux/broadcom/tft_panel.h>
#include <tft_panel_settings.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_TANGO
#include <linux/i2c/tango_ts.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_FT5X06
#include <linux/i2c/ft5x06_ts.h>
#endif /* CONFIG_TOUCHSCREEN_FT5X06 */

#ifdef CONFIG_TOUCHSCREEN_QT602240
#include <linux/i2c/qt602240_ts.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_GT818
#include <linux/i2c/gt818-ts.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_GT9xx
#include <linux/i2c/gt9xx.h>
#endif

#ifdef CONFIG_KEYBOARD_BCM
#include <linux/input.h>
#include <mach/bcm_keypad.h>
#endif

#ifdef CONFIG_WD_TAPPER
#include <linux/broadcom/wd-tapper.h>
#endif

#ifdef CONFIG_BCM_VC_CMA
#include <linux/broadcom/vc_cma.h>
#endif

#ifdef CONFIG_BCM_VC_CAM
#include <linux/broadcom/vc_cam.h>
#endif

#if defined(CONFIG_BCM_VC_RESERVE_LOWMEM)
#include <linux/broadcom/vc_mem.h>
#endif

#if defined(CONFIG_SEC_CHARGING_FEATURE)
// Samsung charging feature
#include <linux/spa_power.h>
#endif

#include "mach.h"
#include "common.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X
#include "board-touch-cyttsp4_core.c"
#endif

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT224 
#include "board-tsp-mxt224.c"
#else

static bool is_cable_attached = false;
void tsp_charger_infom(bool en)
{
  
}
#endif
static bool is_uart_wl_active = true;

#if defined(CONFIG_LCD_LD9040)
#include "board-s2ve-oled.c"
#endif

#include <linux/mfd/bcmpmu.h>

#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCH )
#include <linux/i2c/touchkey_i2c.h>
#endif

#if defined(CONFIG_KEYBOARD_TC360L_TOUCHKEY)
#include <linux/input/tc360-touchkey.h>
#endif

#ifdef CONFIG_ISA1000_VIBRATOR
#include <linux/isa1000_vibrator.h> 
#endif
#ifdef CONFIG_DRV2603_VIBRATOR
#include <linux/drv2603_vibrator.h> 
#endif

//dh0318.lee ENABLE_OTGGTEST
#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif

#ifndef CAPRI_BOARD_ID
#error CAPRI_BOARD_ID needs to be defined in board_xxx.c
#endif

#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT

#include "board-capri-wifi.h"
extern int capri_wifi_status_register(void (*callback) (int card_present, void *dev_id),
			  void *dev_id);

#endif

#if defined(CONFIG_SENSORS_GP2A030)
static void gp2a_led_onoff(int onoff);
#endif

#define INT_SRAM_BASE             0x34040000



/*
 * Since this board template is included by each board_xxx.c. We concatenate
 * CAPRI_BOARD_ID to help debugging when multiple boards are compiled into
 * a single image
 */
#define concatenate_again(a, b) a ## b
#define concatenate(a, b) concatenate_again(a, b)

/* The SDIO index starts from 1 in RDB. Remap to start numbering from 0 */
#define PHYS_ADDR_SDIO0        SDIO1_BASE_ADDR
#define PHYS_ADDR_SDIO1        SDIO2_BASE_ADDR
#define PHYS_ADDR_SDIO2        SDIO3_BASE_ADDR
#define PHYS_ADDR_SDIO3        SDIO4_BASE_ADDR
#define SDIO_CORE_REG_SIZE     0x10000


/* number of I2C adapters (hosts/masters) */
#define MAX_I2C_ADAPS    4

/* The BSC (I2C) index starts from 1 in RDB. Remap to start at 0 */
#define PHYS_ADDR_BSC0         BSC1_BASE_ADDR
#define PHYS_ADDR_BSC1         BSC2_BASE_ADDR
#define PHYS_ADDR_BSC2         PMU_BSC_BASE_ADDR
#define PHYS_ADDR_BSC3         BSC3_BASE_ADDR

#define BSC_CORE_REG_SIZE      0x100

#define USBH_EHCI_CORE_REG_SIZE    0x90
#define USBH_OHCI_CORE_REG_SIZE    0x1000
#define USBH_DWC_REG_OFFSET        USBH_EHCI_CORE_REG_SIZE
#define USBH_DWC_BASE_ADDR         (EHCI_BASE_ADDR + USBH_DWC_REG_OFFSET)
#define USBH_DWC_CORE_REG_SIZE     0x20
#define USBH_CTRL_CORE_REG_SIZE    0x20

#define USBH_HSIC2_EHCI_CORE_REG_SIZE	0x90
#define USBH_HSIC_PHY_CORE_REG_SIZE		0x40

#define OTG_CTRL_CORE_REG_SIZE     0x100

#if defined(CONFIG_MACH_CAPRI_RAY) || defined(CONFIG_MACH_CAPRI_STONE) || defined(CONFIG_MACH_CAPRI_A01)
#define TANGO_GPIO_RESET_PIN        0
#define TANGO_GPIO_IRQ_PIN          1
#define TANGO_I2C_BUS_ID            1
#endif

#ifdef CONFIG_TOUCHSCREEN_TANGO
#define IS_MULTI_TOUCH  1
#define MAX_NUM_FINGERS 2
#endif

#ifdef CONFIG_TOUCHSCREEN_FT5X06
#define FT5X06_GPIO_RESET_PIN	8
#define FT5X06_GPIO_IRQ_PIN	11
#define FT5X06_I2C_BUS_ID	1
#endif /* CONFIG_TOUCHSCREEN_FT5X06 */

#ifdef CONFIG_TOUCHSCREEN_GT818
#define GT818_GPIO_RESET_PIN        (10)
#define GT818_GPIO_IRQ_PIN          (11)
#define GT818_I2C_BUS_ID            (10)


#define CAPRI_SCL_GPIO  (8)
#define CAPRI_SDA_GPIO  (9)
#endif

#ifdef CONFIG_TOUCHSCREEN_GT9xx
#define GT9xx_I2C_BUS_ID            1
#endif


#ifdef CONFIG_KEYBOARD_BCM
/* keypad map */
#define BCM_KEY_ROW_0  0
#define BCM_KEY_ROW_1  1
#define BCM_KEY_ROW_2  2
#define BCM_KEY_ROW_3  3
#define BCM_KEY_ROW_4  4
#define BCM_KEY_ROW_5  5
#define BCM_KEY_ROW_6  6
#define BCM_KEY_ROW_7  7

#define BCM_KEY_COL_0  0
#define BCM_KEY_COL_1  1
#define BCM_KEY_COL_2  2
#define BCM_KEY_COL_3  3
#define BCM_KEY_COL_4  4
#define BCM_KEY_COL_5  5
#define BCM_KEY_COL_6  6
#define BCM_KEY_COL_7  7
#endif

int configure_wifi_pullup(bool pull_up)
{
	int ret=0;
	char i;
	struct pin_config new_pin_config;

	pr_err("%s, Congifure Pin with pull_up:%d\n",__func__,pull_up);
	
	new_pin_config.name = PN_SDIO1_CMD;

	ret = pinmux_get_pin_config(&new_pin_config);
	if(ret){
		printk("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
		return ret;
	}
	
	if(pull_up){
		new_pin_config.reg.b.pull_up =PULL_UP_ON;
		new_pin_config.reg.b.pull_dn =PULL_DN_OFF;
	}
	else{
		new_pin_config.reg.b.pull_up =PULL_UP_OFF;
		new_pin_config.reg.b.pull_dn =PULL_DN_ON;
	}
		
	ret = pinmux_set_pin_config(&new_pin_config);
	if(ret){
		pr_err("%s - fail to configure mmc_cmd:%d\n",__func__,ret);
		return ret;
	}
		
	for (i = 0; i < 4; i++ ) {
		new_pin_config.name = ( PN_SDIO1_DATA_0 + i );	
		ret = pinmux_get_pin_config(&new_pin_config);
		if(ret){
			printk("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
			return ret;
		}
		
		if(pull_up){
			new_pin_config.reg.b.pull_up =PULL_UP_ON;
			new_pin_config.reg.b.pull_dn =PULL_DN_OFF;
		}
		else{
			new_pin_config.reg.b.pull_up =PULL_UP_OFF;
			new_pin_config.reg.b.pull_dn =PULL_DN_ON;
		}
		
		ret = pinmux_set_pin_config(&new_pin_config);
		if(ret){
			pr_err("%s - fail to configure mmc_cmd:%d\n",__func__,ret);
			return ret;
		}
		
	}	

	return ret;
}

//EXPORT_SYMBOL(configure_wifi_pullup);

int configure_sdmmc_pullup(bool pull_up)
{
	int ret=0;
	char i;
	struct pin_config new_pin_config;

	pr_err("%s, Congifure Pin with pull_up:%d\n",__func__,pull_up);
	
	new_pin_config.name = PN_SDIO4_CMD;

	ret = pinmux_get_pin_config(&new_pin_config);
	if(ret){
		printk("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
		return ret;
	}
	
	if(pull_up){
		new_pin_config.reg.b.pull_up =PULL_UP_ON;
		new_pin_config.reg.b.pull_dn =PULL_DN_OFF;
	}
	else{
		new_pin_config.reg.b.pull_up =PULL_UP_OFF;
		new_pin_config.reg.b.pull_dn =PULL_DN_ON;
	}
		
	ret = pinmux_set_pin_config(&new_pin_config);
	if(ret){
		pr_err("%s - fail to configure mmc_cmd:%d\n",__func__,ret);
		return ret;
	}
		
	for (i = 0; i < 4; i++ ) {
		new_pin_config.name = ( PN_SDIO4_DATA_0 + i );	
		ret = pinmux_get_pin_config(&new_pin_config);
		if(ret){
			printk("%s, Error pinmux_get_pin_config():%d\n",__func__,ret);
			return ret;
		}
		
		if(pull_up){
			new_pin_config.reg.b.pull_up =PULL_UP_ON;
			new_pin_config.reg.b.pull_dn =PULL_DN_OFF;
		}
		else{
			new_pin_config.reg.b.pull_up =PULL_UP_OFF;
			new_pin_config.reg.b.pull_dn =PULL_DN_ON;
		}
		
		ret = pinmux_set_pin_config(&new_pin_config);
		if(ret){
			pr_err("%s - fail to configure mmc_cmd:%d\n",__func__,ret);
			return ret;
		}
		
	}	

	return ret;
}


static struct resource sdio0_resource[] = {
	[0] = {
		.start = PHYS_ADDR_SDIO0,
		.end = PHYS_ADDR_SDIO0 + SDIO_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_SDIO0,
		.end = BCM_INT_ID_SDIO0,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource sdio1_resource[] = {
	[0] = {
		.start = PHYS_ADDR_SDIO1,
		.end = PHYS_ADDR_SDIO1 + SDIO_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_SDIO1,
		.end = BCM_INT_ID_SDIO1,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource sdio2_resource[] = {
        [0] = {
                .start = PHYS_ADDR_SDIO2,
                .end = PHYS_ADDR_SDIO2 + SDIO_CORE_REG_SIZE - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = BCM_INT_ID_SDIO_NAND,
                .end = BCM_INT_ID_SDIO_NAND,
                .flags = IORESOURCE_IRQ,
        },
};

static struct resource sdio3_resource[] = {
        [0] = {
                .start = PHYS_ADDR_SDIO3,
                .end = PHYS_ADDR_SDIO3 + SDIO_CORE_REG_SIZE - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = BCM_INT_ID_SDIO_MMC,
                .end = BCM_INT_ID_SDIO_MMC,
                .flags = IORESOURCE_IRQ,
        },
};


static struct sdio_platform_cfg sdio_param[] =
#ifdef HW_SDIO_PARAM
	HW_SDIO_PARAM;
#else
	{};
#endif

static struct sh_mobile_sdhi_info sdhi0_info = {
	.tmio_caps	= MMC_CAP_SD_HIGHSPEED | MMC_CAP_SDIO_IRQ,
};

static struct platform_device sdio_devices[SDIO_MAX_NUM_DEVICES] =
{
	{ /* SDIO0 */
		.name = "sdhci",
		.id = 0,
		.resource = sdio0_resource,
		.num_resources	= ARRAY_SIZE(sdio0_resource),
               	.dev	= {
        		.platform_data	= &sdhi0_info,
                },

	},
	{ /* SDIO1 */
		.name = "sdhci",
		.id = 1,
		.resource = sdio1_resource,
		.num_resources	= ARRAY_SIZE(sdio1_resource),
	},
	{/* SDIO2 */
		.name = "sdhci",
		.id = 2,
		.resource = sdio2_resource,
		.num_resources    = ARRAY_SIZE(sdio2_resource),
	},
	{/* SDIO3 */
                .name = "sdhci",
                .id = 3,
                .resource = sdio3_resource,
                .num_resources    = ARRAY_SIZE(sdio3_resource),
        },
};

#if defined(CONFIG_NET_ISLAND)
static struct island_net_hw_cfg net_data =
#ifdef HW_CFG_NET
	HW_CFG_NET;
#else
{
	.addrPhy0 = 0,
	.addrPhy1 = 1,
	.gpioPhy0 = -1,
	.gpioPhy1 = -1,
};
#endif

static struct platform_device net_device =
{
	.name = "island-net",
	.id = -1,
	.dev =
	{
		.platform_data = &net_data,
	},
};
#endif /* CONFIG_NET_ISLAND */

static struct bsc_adap_cfg i2c_adap_param[] =
#ifdef HW_I2C_ADAP_PARAM
	HW_I2C_ADAP_PARAM;
#else
	{};
#endif

static struct resource i2c0_resource[] = {
	[0] =
	{
		.start = PHYS_ADDR_BSC0,
		.end = PHYS_ADDR_BSC0 + BSC_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] =
	{
		.start = BCM_INT_ID_I2C0,
		.end = BCM_INT_ID_I2C0,
		.flags = IORESOURCE_IRQ,
	},
};

static struct resource i2c1_resource[] = {
	[0] =
	{
		.start = PHYS_ADDR_BSC1,
		.end = PHYS_ADDR_BSC1 + BSC_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] =
	{
		.start = BCM_INT_ID_I2C1,
		.end = BCM_INT_ID_I2C1,
		.flags = IORESOURCE_IRQ,
	},
};
static struct resource i2c2_resource[] = {
	[0] =
	{
		.start = PHYS_ADDR_BSC2,
		.end = PHYS_ADDR_BSC2 + BSC_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] =
	{
		.start = BCM_INT_ID_PM_I2C,
		.end = BCM_INT_ID_PM_I2C,
		.flags = IORESOURCE_IRQ,
	},
};
static struct resource i2c3_resource[] = {
	[0] =
	{
		.start = PHYS_ADDR_BSC3,
		.end = PHYS_ADDR_BSC3 + BSC_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] =
	{
		.start = BCM_INT_ID_I2C2,
		.end = BCM_INT_ID_I2C2,
		.flags = IORESOURCE_IRQ,
	},
};



static struct platform_device i2c_adap_devices[MAX_I2C_ADAPS] =
{
	{  /* for BSC0 */
		.name = "bsc-i2c",
		.id = 0,
		.resource = i2c0_resource,
		.num_resources	= ARRAY_SIZE(i2c0_resource),
	},
	{  /* for BSC1 */
		.name = "bsc-i2c",
		.id = 1,
		.resource = i2c1_resource,
		.num_resources	= ARRAY_SIZE(i2c1_resource),
	},
	{  /* for BSC2 */
		.name = "bsc-i2c",
		.id = 2,
		.resource = i2c2_resource,
		.num_resources	= ARRAY_SIZE(i2c2_resource),
	},
	{  /* for PMU BSC */
		.name = "bsc-i2c",
		.id = 3,
		.resource = i2c3_resource,
		.num_resources	= ARRAY_SIZE(i2c3_resource),
	},
};

#ifdef CONFIG_BCM_HEADSET_SW

#define board_headsetdet_data concatenate(CAPRI_BOARD_ID, _headsetdet_data)
static struct headset_hw_cfg board_headsetdet_data =
#ifdef HW_CFG_HEADSET
	HW_CFG_HEADSET;
#else
{
	.gpio_headset_det = -1,
	.gpio_headset_active_low = 0,
	.gpio_mic_det = -1,
	.gpio_mic_active_low = 0,
};
#endif

#define board_headsetdet_device concatenate(CAPRI_BOARD_ID, _headsetdet_device)
static struct platform_device board_headsetdet_device =
{
	.name = "bcm-headset-det",
	.id = -1,
	.dev =
	{
		.platform_data = &board_headsetdet_data,
	},
};

#define board_add_headsetdet_device concatenate(CAPRI_BOARD_ID, _add_headsetdet_device)
static void __init board_add_headsetdet_device(void)
{
	platform_device_register(&board_headsetdet_device);
}

#endif /* CONFIG_BCM_HEADSET_SW */

#if defined(CONFIG_KONA_HEADSET) || defined(CONFIG_KONA_HEADSET_MULTI_BUTTON)
#define HS_IRQ		gpio_to_irq(12)
#define HSB_IRQ		BCM_INT_ID_AUXMIC_COMP1
#define HSI_IRQ		BCM_INT_ID_AUXMIC_COMP2
#define HSR_IRQ 	BCM_INT_ID_AUXMIC_COMP2_INV
#ifdef CONFIG_KONA_GPIO_HEADSET_SW_EN
#define GPIO_HEADSET_SW_EN	101 /* GPIO101 */
#else
#define GPIO_HEADSET_SW_EN	-1
#endif

/*
 * Default table used if the platform does not pass one
 */ 

#if defined(CONFIG_SS_NEW_EARPHONE_SPEC)
static unsigned int  capriss_button_adc_values [3][2] = 
{
	/* SEND/END Min, Max*/
        {0,     116},
	/* Volume Up  Min, Max*/
        {117,   248},
	/* Volue Down Min, Max*/
        {249,   999},
};
#elif defined(CONFIG_MACH_CAPRI_SS_CRATER)
static unsigned int  capriss_button_adc_values [3][2] = 
{
	/* SEND/END Min, Max*/
        {0,     150},
	/* Volume Up  Min, Max*/
        {151,    300},
	/* Volue Down Min, Max*/
        {301,   700},
};
#else
static unsigned int  capriss_button_adc_values [3][2] = 
{
	/* SEND/END Min, Max*/
        {0,     99},
	/* Volume Up  Min, Max*/
        {100,    240},
	/* Volue Down Min, Max*/
        {241,   550},
};
#endif

#if defined(CONFIG_MACH_CAPRI_M500)
static unsigned int caprim500_button_adc_values[3][2] = {
    /* SEND/END Min, Max*/
    {0, 30},
    /* Volume Up  Min, Max*/
    {30, 50},
    /* Volue Down Min, Max*/
    {50, 680},
};

static unsigned int caprim500_button_adc_values_2_1[3][2] = {
    /* SEND/END Min, Max*/
    {0,     170},
    /* Volume Up  Min, Max*/
    {200,   340},
    /* Volue Down Min, Max*/
    {400,   680},
};
#endif

static struct kona_headset_pd headset_data = {
	/* GPIO state read is 1 on HS insert and 0 for
	 * HS remove
	 */
#if defined(CONFIG_MACH_CAPRI_SS) || defined(CONFIG_MACH_CAPRI_SS_S2VE) ||defined(CONFIG_MACH_CAPRI_M500)||defined(CONFIG_MACH_CAPRI_SS_BAFFIN)||defined(CONFIG_MACH_CAPRI_SS_CRATER)
	.hs_default_state = 1,
#else
	.hs_default_state = 0,
#endif
	/*
	 * Because of the presence of the resistor in the MIC_IN line.
	 * The actual ground is not 0, but a small offset is added to it.
	 * This needs to be subtracted from the measured voltage to determine the
	 * correct value. This will vary for different HW based on the resistor
	 * values used.
	 *
	 * What this means to Rhearay?
	 * From the schematics looks like there is no such resistor put on
	 * Rhearay. That means technically there is no need to subtract any extra load
	 * from the read Voltages. On other HW, if there is a resistor present
	 * on this line, please measure the load value and put it here.
	 */
	.phone_ref_offset = 0,
	.gpio_for_accessory_detection = 1,
	.aci_clk_name = "aci_apb_clk",
	.gpio_headset_sw_en = GPIO_HEADSET_SW_EN,

#if defined(CONFIG_MACH_CAPRI_M500)
	/*
	 * Pass the board specific button detection range
	 */
	.button_adc_values_low = caprim500_button_adc_values,

	/*
	 * Pass the board specific button detection range
	 */
	.button_adc_values_high = caprim500_button_adc_values_2_1,
#else
	.button_adc_values = capriss_button_adc_values,
	/* ldo for MICBIAS */
#endif
	/* ldo for MICBIAS */
	.ldo_id = "audldo_uc",
};

static struct resource board_headset_resource[] = {
	{	/* For AUXMIC */
		.start = AUXMIC_BASE_ADDR,
		.end = AUXMIC_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{	/* For ACI */
		.start = ACI_BASE_ADDR,
		.end = ACI_BASE_ADDR + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	{	/* For GPIO Headset IRQ */
		.start = HS_IRQ,
		.end = HS_IRQ,
		.flags = IORESOURCE_IRQ,
	},
	{	/* For Headset insert IRQ */
		.start = HSI_IRQ,
		.end = HSI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
	{	/* For Headset remove IRQ */
		.start = HSR_IRQ,
		.end = HSR_IRQ,
		.flags = IORESOURCE_IRQ,
	},
	{	/* COMP1 for Button*/
		.start = HSB_IRQ,
		.end = HSB_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device headset_device = {
	.name = "konaaciheadset",
	.id = -1,
	.resource = board_headset_resource,
	.num_resources	= ARRAY_SIZE(board_headset_resource),
	.dev	=	{
		.platform_data = &headset_data,
	},
};
#endif /* CONFIG_KONA_HEADSET || CONFIG_KONA_HEADSET_MULTI_BUTTON */

#if defined(CONFIG_SPI_SPIDEV) && defined(CONFIG_SPI_SSPI_KONA)
/*
 * SPI board info for the slaves
 */
static struct spi_board_info spidev_board_info[] __initdata = {
	{
		.modalias = "spidev",  /* use spidev generic driver */
		.max_speed_hz = 13000000,      /* use max speed */
		.bus_num = 0,          /* framework bus number */
		.chip_select = 0,      /* for each slave */
		.platform_data = NULL, /* no spi_driver specific */
		.irq = 0,              /* IRQ for this device */
		.mode = SPI_LOOP,      /* SPI mode 0 */
		},
#ifdef CONFIG_SPI_KONA_SSP3_TEST
	{
		.modalias = "spidev",  /* use spidev generic driver */
		.max_speed_hz = 13000000,      /* use max speed */
		.bus_num = 2,          /* framework bus number */
		.chip_select = 0,      /* for each slave */
		.platform_data = NULL, /* no spi_driver specific */
		.irq = 0,              /* IRQ for this device */
		.mode = SPI_LOOP,      /* SPI mode 0 */
	},
#endif /* CONFIG_SPI_KONA_SSP3_TEST */
};
#endif

#if defined(CONFIG_BCM_HDMI_DET) || defined(CONFIG_BCM_HDMI_DET_MODULE)

#define board_hdmidet_data concatenate(CAPRI_BOARD_ID, _hdmidet_data)
static struct hdmi_hw_cfg board_hdmidet_data =
#ifdef HW_CFG_HDMI
	HW_CFG_HDMI;
#else
{
	.gpio_hdmi_det = -1,
};
#endif

#define board_hdmidet_device concatenate(CAPRI_BOARD_ID, _hdmidet_device)
static struct platform_device board_hdmidet_device =
{
	.name = "hdmi-detect",
	.id = -1,
	.dev =
	{
		.platform_data = &board_hdmidet_data,
	},
};

#define board_add_hdmidet_device concatenate(CAPRI_BOARD_ID, _add_hdmidet_device)
static void __init board_add_hdmidet_device(void)
{
	platform_device_register(&board_hdmidet_device);
}

#endif /* #if defined(CONFIG_BCM_HDMI_DET) || defined(CONFIG_BCM_HDMI_DET_MODULE) */

#if defined(CONFIG_TFT_PANEL) || defined(CONFIG_TFT_PANEL_MODULE)
#define board_tft_panel_data concatenate(CAPRI_BOARD_ID, _tft_panel_data)
static struct tft_panel_platform_data board_tft_panel_data = TFT_PANEL_SETTINGS;

#define board_tft_panel_device concatenate(CAPRI_BOARD_ID, _tft_panel_device)
static struct platform_device board_tft_panel_device =
{
	.name = TFT_PANEL_DRIVER_NAME,
	.id = -1,
	.dev =
	{
		.platform_data = &board_tft_panel_data,
	},
};

#define board_add_tft_panel_device concatenate(CAPRI_BOARD_ID, _add_tft_panel_device)
static void __init board_add_tft_panel_device(void)
{
	platform_device_register(&board_tft_panel_device);
}
#endif

#if defined(CONFIG_USB_BCM_CAPRI)
static struct usbh_cfg usbh_param = HW_USBH_PARAM;

static struct resource usbh_resource[] = {
	[0] = {
		.start = USBH_PHY_BASE_ADDR,
		.end = USBH_PHY_BASE_ADDR + USBH_CTRL_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device usbh_device =
{
	.name = "usbh",
	.resource = usbh_resource,
	.num_resources = ARRAY_SIZE(usbh_resource),
	.dev = {
		.platform_data = &usbh_param,
	},
};
#endif

#if defined(CONFIG_USB_BCM_CAPRI_HSIC)
static struct usbh_cfg usbh_hsic_param = HW_USBH_HSIC_PARAM;

static struct resource usbh_hsic_resource[] = {
	[0] = {
		.start = HSIC_PHY_BASE_ADDR,
		.end = HSIC_PHY_BASE_ADDR + USBH_HSIC_PHY_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device usbh_hsic_device = {
	.name = "usbhsic",
	.resource = usbh_hsic_resource,
	.num_resources = ARRAY_SIZE(usbh_hsic_resource),
	.dev = {
		.platform_data = &usbh_hsic_param,
	},
};
#endif

#if defined(CONFIG_USB_EHCI_BCM)
static u64 ehci_dmamask = DMA_BIT_MASK(32);

static struct resource usbh_ehci_resource[] = {
	[0] = {
		.start = EHCI_BASE_ADDR,
		.end = EHCI_BASE_ADDR + USBH_EHCI_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_ULPI_EHCI,
		.end = BCM_INT_ID_ULPI_EHCI,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device usbh_ehci_device =
{
	.name = "bcm-ehci",
	.id = 0,
	.resource = usbh_ehci_resource,
	.num_resources = ARRAY_SIZE(usbh_ehci_resource),
	.dev = {
		.dma_mask = &ehci_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};
#endif

#if defined(CONFIG_USB_EHCI_BCM) && defined(CONFIG_USB_BCM_CAPRI_HSIC)
static u64 hsic2_ehci_dmamask = DMA_BIT_MASK(32);

static struct resource usbh_hsic2_ehci_resource[] = {
	[0] = {
		.start = HSIC2_EHCI_BASE_ADDR,
		.end = HSIC2_EHCI_BASE_ADDR + USBH_HSIC2_EHCI_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_HSIC2_EHCI,
		.end = BCM_INT_ID_HSIC2_EHCI,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device usbh_hsic2_ehci_device = {
	.name = "bcm-ehci",
	.id = 1,
	.resource = usbh_hsic2_ehci_resource,
	.num_resources = ARRAY_SIZE(usbh_hsic2_ehci_resource),
	.dev = {
		.dma_mask = &hsic2_ehci_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};
#endif

#if defined(CONFIG_USB_OHCI_BCM)

static u64 ohci_dmamask = DMA_BIT_MASK(32);

static struct resource usbh_ohci_resource[] = {
	[0] = {
		.start = OHCI_BASE_ADDR,
		.end = OHCI_BASE_ADDR + USBH_OHCI_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_ULPI_OHCI,
		.end = BCM_INT_ID_ULPI_OHCI,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device usbh_ohci_device =
{
	.name = "bcm-ohci",
	.id = 0,
	.resource = usbh_ohci_resource,
	.num_resources = ARRAY_SIZE(usbh_ohci_resource),
	.dev = {
		.dma_mask = &ohci_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};
#endif

#if defined(CONFIG_TOUCHSCREEN_EGALAX_I2C) || defined(CONFIG_TOUCHSCREEN_EGALAX_I2C_MODULE)
static struct egalax_i2c_ts_cfg egalax_i2c_param =
{
	.id = -1,
	.gpio = {
		.reset = -1,
		.event = -1,
	},
   .reset_time = 0,
   .reset_level = 0,
	.supply = "5v_lcd",
};

static struct i2c_board_info egalax_i2c_boardinfo[] =
{
	{
		.type = "egalax_i2c",
		.addr = 0x04,
		.platform_data = &egalax_i2c_param,
	},
};
#endif

#ifdef CONFIG_BQ24272_CHARGER
static struct i2c_board_info  __initdata charger_i2c_devices_info[]  = {
	{
		I2C_BOARD_INFO("bq24272", 0x6B),
	},
};

static struct rq24272_platform_data bq24272_info = {
	.irq = GPIO_RQ24272_INT,
}; 

struct platform_device bq24272_charger =  {
	.name		= "bq24272",
	.id		= CHARGER_I2C_BUS_ID, 
	.dev		= {
		.platform_data = &bq24272_info,
	},	
};

#endif

#ifdef CONFIG_SMB358_CHARGER
static struct i2c_board_info  __initdata charger_i2c_devices_info[]  = {
	{
		I2C_BOARD_INFO("smb358", SMB358_SLAVE_ADDR),
	},
};

static struct smb358_platform_data smb358_info = {
	.irq = GPIO_SMB358_INT,
}; 

struct platform_device smb358_charger =  {
	.name		= "smb358",
	.id		= CHARGER_I2C_BUS_ID, 
	.dev		= {
		.platform_data = &smb358_info,
	},	
};
#endif

#if defined(CONFIG_USB_SWITCH_TSU6721)

enum cable_type_t{
	CABLE_TYPE_USB,
	CABLE_TYPE_AC,
	CABLE_TYPE_ACCESSORY,
	CABLE_TYPE_NONE
};


extern int musb_info_handler(struct notifier_block *nb, unsigned long event, void *para);
void set_usb_connection_status(void *para);
void send_usb_insert_event(enum bcmpmu_event_t event, void *para);
void send_USB_remove_event();
void send_chrgr_insert_event(enum bcmpmu_event_t event, void *para);
void send_uart_jig_insert_event(bool attached);
static enum cable_type_t set_cable_status;

static void tsu6721_usb_cb(bool attached)
{
	enum bcmpmu_chrgr_type_t chrgr_type;
	enum bcmpmu_usb_type_t usb_type;
#if defined(CONFIG_SEC_CHARGING_FEATURE)
	int spa_data=0;
#endif	

	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;

	switch (set_cable_status) {
	case CABLE_TYPE_USB:
		usb_type = PMU_USB_TYPE_SDP;
		chrgr_type = PMU_CHRGR_TYPE_SDP;
#if defined(CONFIG_SEC_CHARGING_FEATURE)
		spa_data = POWER_SUPPLY_TYPE_USB;
#endif
		pr_info("%s USB attached\n",__func__);
		send_usb_insert_event(BCMPMU_USB_EVENT_USB_DETECTION,
				      &usb_type);
		break;		

	case CABLE_TYPE_NONE:
		usb_type = PMU_USB_TYPE_NONE;
		chrgr_type = PMU_CHRGR_TYPE_NONE;		
#if defined(CONFIG_SEC_CHARGING_FEATURE)
		spa_data = POWER_SUPPLY_TYPE_BATTERY;
		pr_info("%s USB removed\n",__func__);
		set_usb_connection_status(&usb_type); // for unplug, we only set status, but not send event
#else
		send_usb_insert_event(BCMPMU_USB_EVENT_USB_DETECTION,
				      &usb_type);
#endif
		break;
	}	
	send_chrgr_insert_event(BCMPMU_CHRGR_EVENT_CHGR_DETECTION,&chrgr_type);
#if defined(CONFIG_SEC_CHARGING_FEATURE)
	spa_event_handler(SPA_EVT_CHARGER, spa_data);
#endif
	return;
}

static void tsu6721_charger_cb(bool attached)
{
	enum bcmpmu_chrgr_type_t chrgr_type;
	enum bcmpmu_usb_type_t usb_type;
#if defined(CONFIG_SEC_CHARGING_FEATURE)	
	int spa_data=0;
#endif
	
	set_cable_status = attached ? CABLE_TYPE_AC : CABLE_TYPE_NONE;

	switch (set_cable_status) {
	case CABLE_TYPE_AC:
		chrgr_type = PMU_CHRGR_TYPE_DCP;		
#if defined(CONFIG_SEC_CHARGING_FEATURE)	
		if( attached == CABLE_TYPE_ACCESSORY)
			spa_data = POWER_SUPPLY_TYPE_USB_ACA;
		else
			spa_data = POWER_SUPPLY_TYPE_USB_DCP;
#endif
		is_cable_attached = 1;
		tsp_charger_infom(1);
		pr_info("%s TA attached\n",__func__);
		break;
	case CABLE_TYPE_NONE:
		chrgr_type = PMU_CHRGR_TYPE_NONE;
#if defined(CONFIG_SEC_CHARGING_FEATURE)				
		spa_data = POWER_SUPPLY_TYPE_BATTERY;
#endif
		is_cable_attached = 0;
		tsp_charger_infom(0);
		pr_info("%s TA removed\n",__func__);
		break;
	}
	send_chrgr_insert_event(BCMPMU_CHRGR_EVENT_CHGR_DETECTION,&chrgr_type);
#if defined(CONFIG_SEC_CHARGING_FEATURE)
	spa_event_handler(SPA_EVT_CHARGER, spa_data);
#endif
}

static void tsu6721_jig_cb(bool attached)
{
	pr_info("%s attached %d\n", __func__,attached);
}

static struct wake_lock jig_uart_wl;

static int uas_jiguart_wl_init(void)
{
	wake_lock_init(&jig_uart_wl, WAKE_LOCK_SUSPEND, "jig_uart_wake");
}
void uas_jig_force_sleep(void)
{
	if(wake_lock_active(&jig_uart_wl))
	{
		wake_unlock(&jig_uart_wl);
		pr_info("Force unlock jig_uart_wl\n");
	}
}

int jig_attach_info = SPA_ACC_NONE;
EXPORT_SYMBOL(jig_attach_info);

static void tsu6721_uart_cb(bool attached)
{
	SPA_ACC_INFO_T acc_info;

	if(attached==true)
	{
		acc_info=SPA_ACC_JIG_UART;
		musb_info_handler(NULL, 0, 1);
	}
	else
	{
		acc_info=SPA_ACC_NONE;
		musb_info_handler(NULL, 0, 0);
	}
      jig_attach_info = acc_info;
	send_uart_jig_insert_event(attached);

	if (is_uart_wl_active) {
		if (attached == true) {
			if (wake_lock_active(&jig_uart_wl) == 0)
				wake_lock(&jig_uart_wl);
		} else {
			if (wake_lock_active(&jig_uart_wl))
				wake_unlock(&jig_uart_wl);
		}
	}
}

void send_otg_insert_event();

static void tsu6721_ovp_cb(bool attached)
{
	pr_info("%s:%s\n",__func__,(attached?"TRUE":"FALSE")); 
#ifdef CONFIG_CHARGER_BCMPMU_SPA
	spa_event_handler(SPA_EVT_OVP, (int)attached);
#endif
}


static struct tsu6721_platform_data tsu6721_pdata = {
        .usb_cb = tsu6721_usb_cb,
        .charger_cb = tsu6721_charger_cb,
        .jig_cb = tsu6721_jig_cb,
        .uart_cb = tsu6721_uart_cb,
        .ovp_cb = tsu6721_ovp_cb,

};

static struct i2c_board_info  __initdata micro_usb_i2c_devices_info[]  = {
	{
                I2C_BOARD_INFO("tsu6721", 0x4A >> 1),
                .platform_data = &tsu6721_pdata,
                .irq = gpio_to_irq(GPIO_USB_INT),
	},
};

static struct i2c_gpio_platform_data mUSB_i2c_gpio_data={
        .sda_pin        = GPIO_USB_I2C_SDA,
                .scl_pin= GPIO_USB_I2C_SCL,
	.udelay	 = 2,
};

static struct platform_device mUSB_i2c_gpio_device = {
	.name			= "i2c-gpio",
        .id                     = TSU6721_I2C_BUS_ID,
	.dev			={
                .platform_data  = &mUSB_i2c_gpio_data,
	},
};
static struct platform_device *mUSB_i2c_devices[] __initdata = {
        &mUSB_i2c_gpio_device,
};

#endif

#ifdef CONFIG_USB_SWITCH_FSA9485

enum cable_type_t{
	CABLE_TYPE_USB,
	CABLE_TYPE_AC,
	CABLE_TYPE_ACCESSORY,
	CABLE_TYPE_NONE
};


#define FSA9485_I2C_BUS_ID 8
#define GPIO_FSA9485_I2C_SDA 131
#define GPIO_FSA9485_I2C_SCL 132
#define GPIO_FSA9485_INT 100

extern int musb_info_handler(struct notifier_block *nb, unsigned long event, void *para);
void set_usb_connection_status(void *para);
void send_usb_insert_event(enum bcmpmu_event_t event, void *para);
void send_USB_remove_event();
void send_chrgr_insert_event(enum bcmpmu_event_t event, void *para);
static enum cable_type_t set_cable_status;

static void fsa9485_usb_cb(bool attached)
{
	enum bcmpmu_chrgr_type_t chrgr_type;
	enum bcmpmu_usb_type_t usb_type;
#if defined(CONFIG_SEC_CHARGING_FEATURE)
	int spa_data=0;
#endif	
	pr_info("fsa9485_usb_cb attached %d\n", attached);

	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;

	switch (set_cable_status) {
	case CABLE_TYPE_USB:
		usb_type = PMU_USB_TYPE_SDP;
		chrgr_type = PMU_CHRGR_TYPE_SDP;
		spa_data = POWER_SUPPLY_TYPE_USB;
		pr_info("%s USB attached\n",__func__);
		send_usb_insert_event(BCMPMU_USB_EVENT_USB_DETECTION, &usb_type);
		break;		
	case CABLE_TYPE_NONE:
		usb_type = PMU_USB_TYPE_NONE;
		chrgr_type = PMU_CHRGR_TYPE_NONE;		
		spa_data = POWER_SUPPLY_TYPE_BATTERY;
		pr_info("%s USB removed\n",__func__);
		set_usb_connection_status(&usb_type); // for unplug, we only set status, but not send event
		break;
	}	
	send_chrgr_insert_event(BCMPMU_CHRGR_EVENT_CHGR_DETECTION,&chrgr_type);
	spa_event_handler(SPA_EVT_CHARGER, spa_data);
}

static void fsa9485_charger_cb(unsigned int attached)
{
	enum bcmpmu_chrgr_type_t chrgr_type;
	enum bcmpmu_usb_type_t usb_type;
#if defined(CONFIG_SEC_CHARGING_FEATURE)	
	int spa_data=0;
#endif
	
	pr_info("fsa9480_charger_cb attached %d\n", attached);

	set_cable_status = attached ? CABLE_TYPE_AC : CABLE_TYPE_NONE;

	switch (set_cable_status) {
	case CABLE_TYPE_AC:
		chrgr_type = PMU_CHRGR_TYPE_DCP;		
#if defined(CONFIG_SEC_CHARGING_FEATURE)	
		if( attached == CABLE_TYPE_ACCESSORY)
			spa_data = POWER_SUPPLY_TYPE_USB_ACA;
		else
			spa_data = POWER_SUPPLY_TYPE_USB_DCP;
#endif
		is_cable_attached = 1;
		tsp_charger_infom(1);
		pr_info("%s TA attached\n",__func__);
		break;
	case CABLE_TYPE_NONE:
		chrgr_type = PMU_CHRGR_TYPE_NONE;
#if defined(CONFIG_SEC_CHARGING_FEATURE)				
		spa_data = POWER_SUPPLY_TYPE_BATTERY;
#endif
		is_cable_attached = 0;
		tsp_charger_infom(0);
		pr_info("%s TA removed\n",__func__);
		break;
	}
	send_chrgr_insert_event(BCMPMU_CHRGR_EVENT_CHGR_DETECTION,&chrgr_type);
#if defined(CONFIG_SEC_CHARGING_FEATURE)
	spa_event_handler(SPA_EVT_CHARGER, spa_data);
#endif
}

static struct switch_dev switch_dock = {
	.name = "dock",
};

static void fsa9485_jig_cb(bool attached)
{
	pr_info("fsa9480_jig_cb attached %d\n", attached);
}

static struct wake_lock jig_uart_wl;
static int uas_jiguart_wl_init(void)
{
	wake_lock_init(&jig_uart_wl, WAKE_LOCK_SUSPEND, "jig_uart_wake");
}
void uas_jig_force_sleep(void)
{
	if(wake_lock_active(&jig_uart_wl))
	{
		wake_unlock(&jig_uart_wl);
		pr_info("Force unlock jig_uart_wl\n");
	}
}

static void fsa9485_uart_cb(bool attached)
{
	SPA_ACC_INFO_T acc_info;
	pr_info("fsa9485_uart_cb attached %d\n", attached);

	if(attached==true)
	{
		acc_info=SPA_ACC_JIG_UART;
		musb_info_handler(NULL, 0, 1);
	}
	else
	{
		acc_info=SPA_ACC_NONE;
		musb_info_handler(NULL, 0, 0);
	}
	//spa_event_handler(SPA_EVT_ACC_INFO, (void *)acc_info);

	if(attached==true)
	{
		if(wake_lock_active(&jig_uart_wl) == 0)
			wake_lock(&jig_uart_wl);
	}
	else
	{
		if(wake_lock_active(&jig_uart_wl))
			wake_unlock(&jig_uart_wl);
	}
}

void send_otg_insert_event();
static void fsa9485_otg_cb(bool attached);

static void fsa9485_ovp_cb(bool attached)
{
	pr_info("%s:%s\n",__func__,(attached?"TRUE":"FALSE")); 
	spa_event_handler(SPA_EVT_OVP, (int)attached);
}

static void fsa9485_dock_cb(int attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("%s attached %d\n", __func__, attached);
	switch_set_state(&switch_dock, attached);
}

static void fsa9485_usb_cdp_cb(bool attached)
{
	pr_info("fsa9485_usb_cdp_cb attached %d\n", attached);
}
static void fsa9485_smartdock_cb(bool attached)
{
	pr_info("fsa9485_smartdock_cb attached %d\n", attached);
}

#if defined(CONFIG_USB_OTG_AUDIODOCK)
static void fsa9485_audiodock_cb(int attached)
{
	pr_info("fsa9485_audiodock_cb attached %d\n", attached);
	switch_set_state(&switch_dock, attached);

	send_otg_insert_event();
}
#endif

static int fsa9485_dock_init(void)
{
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);
	if (ret < 0) {
		pr_err("Failed to register dock switch. %d\n", ret);
		return ret;
	}
	return 0;
}

static int fsa9485_ex_init(void)
{
	fsa9485_dock_init();
	uas_jiguart_wl_init();
}
static struct fsa9485_platform_data fsa9485_pdata = {
	.usb_cb = fsa9485_usb_cb,
	.charger_cb = fsa9485_charger_cb,
	.jig_cb = fsa9485_jig_cb,
	.uart_cb = fsa9485_uart_cb,
	.otg_cb = fsa9485_otg_cb,	
	.ovp_cb = fsa9485_ovp_cb,
	.dock_cb = fsa9485_dock_cb,
	.ex_init = fsa9485_ex_init,
	.usb_cdp_cb = fsa9485_usb_cb, /*fix:Process USB when it is connected as a CDP type*/
	.smartdock_cb = fsa9485_smartdock_cb,
#if defined(CONFIG_USB_OTG_AUDIODOCK)
	.audiodock_cb = fsa9485_audiodock_cb,
#endif
};

static struct i2c_board_info  __initdata micro_usb_i2c_devices_info[]  = {
	{
		I2C_BOARD_INFO("fsa9485", 0x4A >> 1),
		.platform_data = &fsa9485_pdata,
		.irq = gpio_to_irq(GPIO_FSA9485_INT),
	},
};

static struct i2c_gpio_platform_data fsa_i2c_gpio_data={
	.sda_pin = GPIO_FSA9485_I2C_SDA,
	.scl_pin = GPIO_FSA9485_I2C_SCL,
	.udelay	 = 2,
};

static struct platform_device fsa_i2c_gpio_device = {
	.name			= "i2c-gpio",
	.id			= FSA9485_I2C_BUS_ID,
	.dev			={
		.platform_data	= &fsa_i2c_gpio_data,
	},
};

static struct platform_device *mUSB_i2c_devices[] __initdata = {
	&fsa_i2c_gpio_device,
};


#endif

#ifdef CONFIG_SAMSUNG_MHL
#include "board_mhl_sii9234.c"
#endif

#if defined(CONFIG_TOUCHSCREEN_BCM915500) || defined(CONFIG_TOUCHSCREEN_BCM915500_MODULE)
static struct bcmtch_platform_data bcm915500_i2c_platform_data =
{
	.i2c_bus_id             = BCMTCH_HW_I2C_BUS_ID,
	.i2c_addr_sys           = BCMTCH_HW_I2C_ADDR_SYS,

	.gpio_interrupt_pin     = BCMTCH_HW_GPIO_INTERRUPT_PIN,
	.gpio_interrupt_trigger = BCMTCH_HW_GPIO_INTERRUPT_TRIGGER,
	.gpio_reset_pin         = BCMTCH_HW_GPIO_RESET_PIN,
	.gpio_reset_polarity    = BCMTCH_HW_GPIO_RESET_POLARITY,
	.gpio_reset_time_ms     = BCMTCH_HW_GPIO_RESET_TIME_MS,
};

static struct i2c_board_info __initdata bcm915500_i2c_boardinfo[] =
{
	{
		I2C_BOARD_INFO(BCM15500_TSC_NAME, BCMTCH_HW_I2C_ADDR_SPM),
		.platform_data  = &bcm915500_i2c_platform_data,
		.irq            = gpio_to_irq(BCMTCH_HW_GPIO_INTERRUPT_PIN),
	},
};
#endif

#if defined(CONFIG_BCM2079X_NFC_I2C)
static struct bcm2079x_platform_data bcm2079x_pdata = {
	.irq_gpio = BCM_NFC_IRQ_GPIO,
	.en_gpio = BCM_NFC_EN_GPIO,
	.wake_gpio = BCM_NFC_WAKE_GPIO,
};

static struct i2c_board_info __initdata i2c_devs_nfc[] = {
	{
	 I2C_BOARD_INFO("bcm2079x-i2c", BCM_NFC_ADDR),
	 .platform_data = (void *)&bcm2079x_pdata,
	 .irq = gpio_to_irq(BCM_NFC_IRQ_GPIO),
	 },

};

static struct i2c_gpio_platform_data nfc_i2c_gpio_data = {
	.sda_pin    = BCM_NFC_SDA_GPIO,
	.scl_pin    = BCM_NFC_SCL_GPIO,
	.udelay  = 3, 
	.timeout = 100,
};

static struct platform_device nfc_i2c_gpio_device = {
        .name   = "i2c-gpio",
        .id     = BCM_NFC_BUSID,
        .dev        = {
		.platform_data  = &nfc_i2c_gpio_data,
        },
};

#endif

#if defined(CONFIG_I2C_GPIO)
#if defined(CONFIG_TOUCHSCREEN_GT818)
/* Adding the device here as this device is board specific */
struct i2c_gpio_platform_data capri_i2c_gpio_data = {
	.sda_pin               = CAPRI_SDA_GPIO,
	.scl_pin               = CAPRI_SCL_GPIO,
	.udelay = 1,
	.sda_is_open_drain     = 0,
	.scl_is_open_drain     = 0,
 /*       .scl_is_open_drain     = 1,*/
 };
struct platform_device capri_i2c_gpio_device =  {
	.name   = "i2c-gpio",
	.id     = 0x0a,
	.dev    = {
		.platform_data = &capri_i2c_gpio_data,
	},
};
#endif

#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCH )

#define GPIO_3_TOUCH_LDO_EN 129
#define GPIO_3_TOUCH_INT 4
#define GPIO_3_TOUCH_SCL 1
#define GPIO_3_TOUCH_SDA 0

static struct i2c_gpio_platform_data touch_i2c_gpio_data = {
        .sda_pin    = GPIO_3_TOUCH_SDA,
        .scl_pin    = GPIO_3_TOUCH_SCL,
        .udelay  = 3,  //// brian :3
        .timeout = 100,
};

static struct platform_device touch_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 4,
        .dev        = {
            .platform_data  = &touch_i2c_gpio_data,
        },
};


static struct i2c_board_info __initdata i2c_gpio_emul_driver[] = {
	{
		I2C_BOARD_INFO("sec_touchkey", 0x20),
		.irq = gpio_to_irq(GPIO_3_TOUCH_INT),
	},
};

#endif

#if defined(CONFIG_KEYBOARD_TC360L_TOUCHKEY)

#define GPIO_3_TOUCH_LDO_EN 130
#define GPIO_3_TOUCH_INT 4
#define GPIO_3_TOUCH_SCL 1
#define GPIO_3_TOUCH_SDA 0

int tc360_keycodes[] = {KEY_MENU, KEY_BACK};

static struct regulator *touchkey_regulator_2_8=NULL;
bool touchscreen_is_pressed = false;
static int IsTouchkeyPowerOn;

static int tc360_setup_power(struct device *dev, bool setup)
{

	int ret;
	int min_uV, max_uV;
	
	if(setup)
	{
		min_uV = max_uV = 2800000;
	
//		touchkey_regulator_2_8 = regulator_get(dev, "gpldo5_uc");
		touchkey_regulator_2_8 = regulator_get(dev, "tsp_vdd_2.8v");	
		if(IS_ERR(touchkey_regulator_2_8)){
			ret = PTR_ERR(touchkey_regulator_2_8);
			printk("[TouchKey] can not get TSP VDD 2.8V\n");
			return ret;
			}	
			
		ret = regulator_set_voltage(touchkey_regulator_2_8,min_uV,max_uV);

		if (ret < 0) {
			printk(KERN_ERR "%s: fail to set tc360_regulator_vdd to"
			       " %d, %d (%d)\n", __func__, min_uV, max_uV,
			       ret);
			goto err_set_vdd_voltage;
		}
	}
	else
	{
		ret = regulator_force_disable(touchkey_regulator_2_8);
		regulator_put(touchkey_regulator_2_8);		
		printk("[TouchKey] --> 2.8v regulator_disable ret = %d \n", ret);
	}

	return 0;

err_set_vdd_voltage:
	regulator_put(touchkey_regulator_2_8);

	return ret;

}

static void tc360_power(bool on)
{
	int ret;

	if(!touchkey_regulator_2_8) {
		printk(KERN_ERR "%s: No regulator. \n", __func__);
		return;
	}

	if (on)
	{
		ret = regulator_enable(touchkey_regulator_2_8);
		IsTouchkeyPowerOn=1;
	}
	else
	{
		ret = regulator_force_disable(touchkey_regulator_2_8);
		IsTouchkeyPowerOn=0;		
	}

	printk(KERN_INFO "%s: %s (%d)\n", __func__, (on) ? "on" : "off", ret);
}


struct tc360_platform_data tc360_pdata = {
	.gpio_scl = GPIO_3_TOUCH_SCL,
	.gpio_sda = GPIO_3_TOUCH_SDA,
	.gpio_int = GPIO_3_TOUCH_INT,
	.gpio_en = GPIO_3_TOUCH_LDO_EN,
	.udelay = 6,
	.num_key = ARRAY_SIZE(tc360_keycodes),
	.keycodes = tc360_keycodes,
	.suspend_type = TC360_SUSPEND_WITH_POWER_OFF,
	.setup_power = tc360_setup_power,
	.power = tc360_power,
	.touchscreen_is_pressed= &touchscreen_is_pressed,
};

static struct i2c_board_info __initdata touchkey_gpio_i2c_devices[] = {
	{
		I2C_BOARD_INFO(TC360_DEVICE, 0x20),
		.platform_data	= &tc360_pdata,
		.irq = gpio_to_irq(GPIO_3_TOUCH_INT),
	},
};

static struct i2c_gpio_platform_data touch_i2c_gpio_data = {
        .sda_pin    = GPIO_3_TOUCH_SDA,
        .scl_pin    = GPIO_3_TOUCH_SCL,
        .udelay  = 3,  //// brian :3
        .timeout = 100,
};


static struct platform_device touch_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 4,
        .dev        = {
            .platform_data  = &touch_i2c_gpio_data,
        },
};

#endif

#ifdef CONFIG_SAMSUNG_MHL

static struct i2c_gpio_platform_data mhl_i2c_gpio_data = {
        .sda_pin    = GPIO_MHL_SDA_1_8V,
        .scl_pin    = GPIO_MHL_SCL_1_8V,
		//.sda_is_open_drain = true,
		//.scl_is_open_drain = true,
        .udelay  = 3, 
        .timeout = 100,
};

static struct platform_device mhl_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = I2C_BUS_ID_MHL,
        .dev        = {
            .platform_data  = &mhl_i2c_gpio_data,
        },
};
#endif

#define GPIO_SENSOR_SCL 115
#define GPIO_SENSOR_SDA 116

static struct i2c_gpio_platform_data sensor1_i2c_gpio_data = {
        .sda_pin    = GPIO_SENSOR_SDA,
        .scl_pin    = GPIO_SENSOR_SCL,
        .udelay  = 3,  //// brian :3
        .timeout = 100,
};

static struct platform_device sensor1_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 5,
        .dev        = {
            .platform_data  = &sensor1_i2c_gpio_data,
        },
};

#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN) || defined(CONFIG_MACH_CAPRI_SS_CRATER)
#define GPIO_SENSOR2_SCL 179
#define GPIO_SENSOR2_SDA 137

static struct i2c_gpio_platform_data sensor2_i2c_gpio_data = {
        .sda_pin    = GPIO_SENSOR2_SDA,
        .scl_pin    = GPIO_SENSOR2_SCL,
        .udelay  = 2,//3,  //// brian :3
        .timeout = 100,
};

static struct platform_device sensor2_i2c_gpio_device = {
        .name       = "i2c-gpio",
        .id     = 6,
        .dev        = {
            .platform_data  = &sensor2_i2c_gpio_data,
        },
};
#endif

static struct platform_device *gpio_i2c_devices[] __initdata = {
	
#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCH ) || defined(CONFIG_KEYBOARD_TC360L_TOUCHKEY)
	&touch_i2c_gpio_device,
#endif
#ifdef CONFIG_SAMSUNG_MHL
	&mhl_i2c_gpio_device,
#endif
	&sensor1_i2c_gpio_device,
#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN) || defined(CONFIG_MACH_CAPRI_SS_CRATER)
	&sensor2_i2c_gpio_device,
#endif
#ifdef CONFIG_BCM2079X_NFC_I2C
	&nfc_i2c_gpio_device,
#endif 
};
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X
static struct i2c_board_info __initdata cyttsp4_i2c_info[]  = {
	{
		I2C_BOARD_INFO(CYTTSP4_I2C_NAME, CYTTSP4_I2C_TCH_ADR),
		.irq = gpio_to_irq(TMA400_GPIO_TSP_INT),
		.platform_data = CYTTSP4_I2C_NAME,
	},
};
#endif

#ifdef CONFIG_TOUCHSCREEN_ZINITIX_BT532
static struct i2c_board_info __initdata zinitix_i2c_info[]  = {
	{
		I2C_BOARD_INFO("zinitix_touch", 0x20),
#if defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)		
		.irq = gpio_to_irq(183),
#else	
		.irq = gpio_to_irq(8),
#endif

		.platform_data = "zinitix_touch",
	},
};
#endif

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT
#include <linux/i2c/atmel_mxt_ts.h>

#define MXT224_INT_GPIO_PIN      2 /* skip expander chip */

static int mxt224_platform_init_hw(void)
{
	int rc;
	rc = gpio_request(MXT224_INT_GPIO_PIN, "ts_mxt224");
	if (rc < 0)
	{
		printk(KERN_ERR "unable to request GPIO pin %d\n", MXT224_INT_GPIO_PIN);
		return rc;
	}
	gpio_direction_input(MXT224_INT_GPIO_PIN);

	return 0;
}

static void mxt224_platform_exit_hw(void)
{
	gpio_free(MXT224_INT_GPIO_PIN);
}

static struct mxt_platform_data mxt224_platform_data = {
	.x_line		= 19,
	.y_line		= 11,
	.x_size		= 480,
	.y_size		= 800,
	.blen		= 11,
	.threshold	= 40,
	.voltage	= 2800000,              /* 2.8V */
	.orient		= MXT_DIAGONAL,
	//.init_platform_hw = mxt224_platform_init_hw,
	//.exit_platform_hw = mxt224_platform_exit_hw,
};


static struct i2c_board_info __initdata mxt224_info[] = {
	{
		I2C_BOARD_INFO("mXT224", 0x4a),
		.platform_data = &mxt224_platform_data,
		.irq = gpio_to_irq(MXT224_INT_GPIO_PIN),
	},
};


#endif

#ifdef CONFIG_TOUCHSCREEN_FT5X06
static struct ft5x06_platform_data ft5x06_plat_data = {
	.gpio_irq_pin = FT5X06_GPIO_IRQ_PIN,
	.gpio_reset_pin = FT5X06_GPIO_RESET_PIN,
	.x_max_value = FT5X06_MAX_X - 1,
	.y_max_value = FT5X06_MAX_Y - 1,
	.layout = FT5X06_LAYOUT,
	.is_multi_touch = FT5X06_IS_MULTI_TOUCH,
	.is_resetable = 1,
	.max_finger_val = FT5X06_MAX_NUM_FINGERS,
	.press_max = FT5X06_PRESS_MAX,
	.press_value = FT5X06_PRESS,
};

static struct i2c_board_info __initdata ft5x06_info[] = {
	{
	 I2C_BOARD_INFO(I2C_FT5X06_DRIVER_NAME, FT5X06_SLAVE_ADDR),
	 .platform_data = &ft5x06_plat_data,
	 .irq = gpio_to_irq(FT5X06_GPIO_IRQ_PIN),
	 },
};
#endif /* CONFIG_TOUCHSCREEN_FT5X06 */

#ifdef CONFIG_TOUCHSCREEN_GT818

static struct goodix_i2c_rmi_platform_data capri_gt818_pdata = {
        .scr_x_min = 0,
        .scr_x_max = 240,
        .scr_y_min = 0,
        .scr_y_max = 320,
        .int_port = GT818_GPIO_IRQ_PIN,
	.tp_rst   = GT818_GPIO_RESET_PIN,
      /*  .init_hw = gt818_init_platform_hw,*/
        
};
static struct i2c_board_info __initdata gt818_info[] =
{
        {       /* New touch screen i2c slave address. */
                I2C_BOARD_INFO("gt818-ts", 0x5D),
                .platform_data = &capri_gt818_pdata,
                .irq = gpio_to_irq(GT818_GPIO_IRQ_PIN),
        },
};
#endif

#ifdef CONFIG_TOUCHSCREEN_GT9xx
static struct i2c_board_info __initdata gt9xx_info[] =
{
        {       /* goodix gt9xx touch screen i2c slave address. */
                I2C_BOARD_INFO("Goodix-TS", 0x5D),
        },
};

#endif

#if defined(CONFIG_LEDS_GPIO) || defined(CONFIG_LEDS_GPIO_MODULE)
#define board_gpio_leds concatenate(CAPRI_BOARD_ID, _board_gpio_leds)
static struct gpio_led board_gpio_leds[] = GPIO_LEDS_SETTINGS;

#define leds_gpio_data concatenate(CAPRI_BOARD_ID, _leds_gpio_data)
static struct gpio_led_platform_data leds_gpio_data =
{
	.num_leds = ARRAY_SIZE(board_gpio_leds),
	.leds = board_gpio_leds,
};

#define board_leds_gpio_device concatenate(CAPRI_BOARD_ID, _leds_gpio_device)
static struct platform_device board_leds_gpio_device = {
	.name = "leds-gpio",
	.id = -1,
	.dev = {
		.platform_data = &leds_gpio_data,
	},
};
#endif

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
#define board_gpio_keys concatenate(CAPRI_BOARD_ID, _board_gpio_keys)
static struct gpio_keys_button board_gpio_keys[] = GPIO_KEYS_SETTINGS;

#define gpio_keys_data concatenate(CAPRI_BOARD_ID, _gpio_keys_data)
static struct gpio_keys_platform_data gpio_keys_data =
{
	.nbuttons = ARRAY_SIZE(board_gpio_keys),
	.buttons = board_gpio_keys,
};

#define board_gpio_keys_device concatenate(CAPRI_BOARD_ID, _gpio_keys_device)
static struct platform_device board_gpio_keys_device = {
	.name = "gpio-keys",
	.id = -1,
	.dev = {
		.platform_data = &gpio_keys_data,
	},
};
#endif

#if defined(CONFIG_KEYBOARD_KONA) || defined(CONFIG_KEYBOARD_KONA_MODULE)

#define board_keypad_keymap concatenate(CAPRI_BOARD_ID, _keypad_keymap)
static struct KEYMAP board_keypad_keymap[] = HW_DEFAULT_KEYMAP;

#define board_keypad_pwroff concatenate(CAPRI_BOARD_ID, _keypad_pwroff)
static unsigned int board_keypad_pwroff[] = HW_DEFAULT_POWEROFF;

#define board_keypad_param concatenate(CAPRI_BOARD_ID, _keypad_param)
static struct KEYPAD_DATA board_keypad_param =
{
	.active_mode = 0,
	.keymap      = board_keypad_keymap,
	.keymap_cnt  = ARRAY_SIZE(board_keypad_keymap),
	.pwroff      = board_keypad_pwroff,
	.pwroff_cnt  = ARRAY_SIZE(board_keypad_pwroff),
	.clock       = GPIOKP_APB_BUS_CLK_NAME_STR,
};

#define board_keypad_device_resource concatenate(CAPRI_BOARD_ID, _keypad_device_resource)
static struct resource board_keypad_device_resource[] = {
	[0] = {
		.start = KEYPAD_BASE_ADDR,
		.end   = KEYPAD_BASE_ADDR + 0xD0,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_KEYPAD,
		.end   = BCM_INT_ID_KEYPAD,
		.flags = IORESOURCE_IRQ,
	},
};

#define board_keypad_device concatenate(CAPRI_BOARD_ID, _keypad_device)
static struct platform_device board_keypad_device =
{
	.name          = "kona_keypad",
	.id            = -1,
	.resource      = board_keypad_device_resource,
	.num_resources = ARRAY_SIZE(board_keypad_device_resource),
	.dev = {
		.platform_data = &board_keypad_param,
	},
};
#endif

#ifdef CONFIG_KEYBOARD_BCM
/*!
 * The keyboard definition structure.
 */
struct platform_device bcm_kp_device = {
	.name = "bcm_keypad",
	.id = -1,
};

#if defined(CONFIG_MACH_CAPRI_RAY)
static struct bcm_keymap newKeymap[] = {
	{BCM_KEY_ROW_0, BCM_KEY_COL_0, "Search Key", KEY_SEARCH},
	{BCM_KEY_ROW_0, BCM_KEY_COL_1, "Menu-Key", KEY_MENU},
	{BCM_KEY_ROW_0, BCM_KEY_COL_2, "Home-Key", KEY_HOME},
	{BCM_KEY_ROW_0, BCM_KEY_COL_3, "Back Key", KEY_BACK},
	{BCM_KEY_ROW_0, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_0, "Volumeup Key", KEY_VOLUMEUP},
	{BCM_KEY_ROW_1, BCM_KEY_COL_1, "Volumedown Key", KEY_VOLUMEDOWN},
	{BCM_KEY_ROW_1, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_7, "unused", 0},
};

static struct bcm_keypad_platform_info bcm_keypad_data = {
	.row_num = 2,
	.col_num = 4,
	.keymap = newKeymap,
	.bcm_keypad_base = (void *)__iomem HW_IO_PHYS_TO_VIRT(KEYPAD_BASE_ADDR),
};
#elif defined(CONFIG_MACH_CAPRI_SS) || defined(CONFIG_MACH_CAPRI_SS_S2VE)
static struct bcm_keymap newKeymap[] = {
	{BCM_KEY_ROW_0, BCM_KEY_COL_0, "Home-Key", KEY_HOME},
	{BCM_KEY_ROW_0, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_0, "Volumeup Key", KEY_VOLUMEUP},
	{BCM_KEY_ROW_1, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_0, "Volumedn Key", KEY_VOLUMEDOWN},
	{BCM_KEY_ROW_2, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_7, "unused", 0},
};

static struct bcm_keypad_platform_info bcm_keypad_data = {
	.row_num = 3,
	.col_num = 1,
	.keymap = newKeymap,
	.bcm_keypad_base = (void *)__iomem HW_IO_PHYS_TO_VIRT(KEYPAD_BASE_ADDR),
};
#elif defined(CONFIG_MACH_CAPRI_M500)
static struct bcm_keymap newKeymap[] = {
	{BCM_KEY_ROW_0, BCM_KEY_COL_0, "Volumeup Key", KEY_VOLUMEUP},
	{BCM_KEY_ROW_0, BCM_KEY_COL_1, "Volumedown Key", KEY_VOLUMEDOWN},
	{BCM_KEY_ROW_0, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_7, "unused", 0},
};

static struct bcm_keypad_platform_info bcm_keypad_data = {
	.row_num = 1,
	.col_num = 2,
	.keymap = newKeymap,
	.bcm_keypad_base = (void *)__iomem HW_IO_PHYS_TO_VIRT(KEYPAD_BASE_ADDR),
};

#elif defined(CONFIG_MACH_CAPRI_A01)
static struct bcm_keymap newKeymap[] = {
        {BCM_KEY_ROW_0, BCM_KEY_COL_0, "Volumeup Key", KEY_VOLUMEUP},
        {BCM_KEY_ROW_0, BCM_KEY_COL_1, "Volumedown Key", KEY_VOLUMEDOWN},
        {BCM_KEY_ROW_0, BCM_KEY_COL_2, "Home Key", KEY_HOME},
        {BCM_KEY_ROW_0, BCM_KEY_COL_3, "unused", 0},
        {BCM_KEY_ROW_0, BCM_KEY_COL_4, "unused", 0},
        {BCM_KEY_ROW_0, BCM_KEY_COL_5, "unused", 0},
        {BCM_KEY_ROW_0, BCM_KEY_COL_6, "unused", 0},
        {BCM_KEY_ROW_0, BCM_KEY_COL_7, "unused", 0},
        {BCM_KEY_ROW_1, BCM_KEY_COL_0, "unused", 0},
        {BCM_KEY_ROW_1, BCM_KEY_COL_1, "unused", 0},
        {BCM_KEY_ROW_1, BCM_KEY_COL_2, "Hot Key", KEY_F1},
        {BCM_KEY_ROW_1, BCM_KEY_COL_3, "unused", 0},
        {BCM_KEY_ROW_1, BCM_KEY_COL_4, "unused", 0},
        {BCM_KEY_ROW_1, BCM_KEY_COL_5, "unused", 0},
        {BCM_KEY_ROW_1, BCM_KEY_COL_6, "unused", 0},
        {BCM_KEY_ROW_1, BCM_KEY_COL_7, "unused", 0},
        {BCM_KEY_ROW_2, BCM_KEY_COL_0, "unused", 0},
        {BCM_KEY_ROW_2, BCM_KEY_COL_1, "unused", 0},
        {BCM_KEY_ROW_2, BCM_KEY_COL_2, "unused", 0},
        {BCM_KEY_ROW_2, BCM_KEY_COL_3, "unused", 0},
        {BCM_KEY_ROW_2, BCM_KEY_COL_4, "unused", 0},
        {BCM_KEY_ROW_2, BCM_KEY_COL_5, "unused", 0},
        {BCM_KEY_ROW_2, BCM_KEY_COL_6, "unused", 0},
        {BCM_KEY_ROW_2, BCM_KEY_COL_7, "unused", 0},
        {BCM_KEY_ROW_3, BCM_KEY_COL_0, "unused", 0},
        {BCM_KEY_ROW_3, BCM_KEY_COL_1, "unused", 0},
        {BCM_KEY_ROW_3, BCM_KEY_COL_2, "unused", 0},
        {BCM_KEY_ROW_3, BCM_KEY_COL_3, "unused", 0},
        {BCM_KEY_ROW_3, BCM_KEY_COL_4, "unused", 0},
        {BCM_KEY_ROW_3, BCM_KEY_COL_5, "unused", 0},
        {BCM_KEY_ROW_3, BCM_KEY_COL_6, "unused", 0},
        {BCM_KEY_ROW_3, BCM_KEY_COL_7, "unused", 0},
        {BCM_KEY_ROW_4, BCM_KEY_COL_0, "unused", 0},
        {BCM_KEY_ROW_4, BCM_KEY_COL_1, "unused", 0},
        {BCM_KEY_ROW_4, BCM_KEY_COL_2, "unused", 0},
        {BCM_KEY_ROW_4, BCM_KEY_COL_3, "unused", 0},
        {BCM_KEY_ROW_4, BCM_KEY_COL_4, "unused", 0},
        {BCM_KEY_ROW_4, BCM_KEY_COL_5, "unused", 0},
        {BCM_KEY_ROW_4, BCM_KEY_COL_6, "unused", 0},
        {BCM_KEY_ROW_4, BCM_KEY_COL_7, "unused", 0},
        {BCM_KEY_ROW_5, BCM_KEY_COL_0, "unused", 0},
        {BCM_KEY_ROW_5, BCM_KEY_COL_1, "unused", 0},
        {BCM_KEY_ROW_5, BCM_KEY_COL_2, "unused", 0},
        {BCM_KEY_ROW_5, BCM_KEY_COL_3, "unused", 0},
        {BCM_KEY_ROW_5, BCM_KEY_COL_4, "unused", 0},
        {BCM_KEY_ROW_5, BCM_KEY_COL_5, "unused", 0},
        {BCM_KEY_ROW_5, BCM_KEY_COL_6, "unused", 0},
        {BCM_KEY_ROW_5, BCM_KEY_COL_7, "unused", 0},
        {BCM_KEY_ROW_6, BCM_KEY_COL_0, "unused", 0},
        {BCM_KEY_ROW_6, BCM_KEY_COL_1, "unused", 0},
        {BCM_KEY_ROW_6, BCM_KEY_COL_2, "unused", 0},
        {BCM_KEY_ROW_6, BCM_KEY_COL_3, "unused", 0},
        {BCM_KEY_ROW_6, BCM_KEY_COL_4, "unused", 0},
        {BCM_KEY_ROW_6, BCM_KEY_COL_5, "unused", 0},
        {BCM_KEY_ROW_6, BCM_KEY_COL_6, "unused", 0},
        {BCM_KEY_ROW_6, BCM_KEY_COL_7, "unused", 0},
        {BCM_KEY_ROW_7, BCM_KEY_COL_0, "unused", 0},
        {BCM_KEY_ROW_7, BCM_KEY_COL_1, "unused", 0},
        {BCM_KEY_ROW_7, BCM_KEY_COL_2, "unused", 0},
        {BCM_KEY_ROW_7, BCM_KEY_COL_3, "unused", 0},
        {BCM_KEY_ROW_7, BCM_KEY_COL_4, "unused", 0},
        {BCM_KEY_ROW_7, BCM_KEY_COL_5, "unused", 0},
        {BCM_KEY_ROW_7, BCM_KEY_COL_6, "unused", 0},
        {BCM_KEY_ROW_7, BCM_KEY_COL_7, "unused", 0},
};

static struct bcm_keypad_platform_info bcm_keypad_data = {
        .row_num = 2,
        .col_num = 3,
        .keymap = newKeymap,
        .bcm_keypad_base = (void *)__iomem HW_IO_PHYS_TO_VIRT(KEYPAD_BASE_ADDR),
};

#elif defined(CONFIG_MACH_CAPRI_SS_BAFFIN)
static struct bcm_keymap newKeymap[] = {
	{BCM_KEY_ROW_0, BCM_KEY_COL_0, "Home-Key", KEY_HOME},
	{BCM_KEY_ROW_0, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_0, "Volumeup Key", KEY_VOLUMEUP},
	{BCM_KEY_ROW_1, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_0, "Volumedn Key", KEY_VOLUMEDOWN},
	{BCM_KEY_ROW_2, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_7, "unused", 0},
};

static struct bcm_keypad_platform_info bcm_keypad_data = {
        .row_num = 2,
        .col_num = 3,
        .keymap = newKeymap,
        .bcm_keypad_base = (void *)__iomem HW_IO_PHYS_TO_VIRT(KEYPAD_BASE_ADDR),
};

#elif defined(CONFIG_MACH_CAPRI_SS_ARUBA)
static struct bcm_keymap newKeymap[] = {
	{BCM_KEY_ROW_0, BCM_KEY_COL_0, "Home-Key", KEY_HOME},
	{BCM_KEY_ROW_0, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_0, "Volumeup Key", KEY_VOLUMEUP},
	{BCM_KEY_ROW_1, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_0, "Volumedn Key", KEY_VOLUMEDOWN},
	{BCM_KEY_ROW_2, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_7, "unused", 0},
};

#elif defined(CONFIG_MACH_CAPRI_GARNET)
static struct bcm_keymap newKeymap[] = {
	{BCM_KEY_ROW_0, BCM_KEY_COL_0, "Volumedown Key", KEY_VOLUMEDOWN},
	{BCM_KEY_ROW_0, BCM_KEY_COL_1, "Volumeup Key", KEY_VOLUMEUP},
	{BCM_KEY_ROW_0, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_0, "Camera Focus Key", KEY_CAMERA_FOCUS},
	{BCM_KEY_ROW_1, BCM_KEY_COL_1, "Camera Key", KEY_CAMERA},
	{BCM_KEY_ROW_1, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_7, "unused", 0},
};

static struct bcm_keypad_platform_info bcm_keypad_data = {
	.row_num = 2,
	.col_num = 3,
	.keymap = newKeymap,
	.bcm_keypad_base = (void *)__iomem HW_IO_PHYS_TO_VIRT(KEYPAD_BASE_ADDR),
};

#else 
static struct bcm_keymap newKeymap[] = {
	{BCM_KEY_ROW_0, BCM_KEY_COL_0, "Volumedown Key", KEY_VOLUMEDOWN},
	{BCM_KEY_ROW_0, BCM_KEY_COL_1, "Volumeup Key", KEY_VOLUMEUP},
	{BCM_KEY_ROW_0, BCM_KEY_COL_2, "Search Key", KEY_SEARCH},
	{BCM_KEY_ROW_0, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_0, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_0, "Home-Key", KEY_HOME},
	{BCM_KEY_ROW_1, BCM_KEY_COL_1, "Back Key", KEY_BACK},
	{BCM_KEY_ROW_1, BCM_KEY_COL_2, "Menu-Key", KEY_MENU},
	{BCM_KEY_ROW_1, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_1, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_2, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_3, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_4, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_5, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_6, BCM_KEY_COL_7, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_0, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_1, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_2, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_3, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_4, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_5, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_6, "unused", 0},
	{BCM_KEY_ROW_7, BCM_KEY_COL_7, "unused", 0},
};

static struct bcm_keypad_platform_info bcm_keypad_data = {
	.row_num = 2,
	.col_num = 3,
	.keymap = newKeymap,
	.bcm_keypad_base = (void *)__iomem HW_IO_PHYS_TO_VIRT(KEYPAD_BASE_ADDR),
};
#endif

#endif

#if defined(CONFIG_BCM_WFD) || defined(CONFIG_BCM_WFD_MODULE)
static struct platform_device board_bcm_wfd_device = {
	.name = "bcm-wfd",
	.id = -1,
};
#endif

#if defined(CONFIG_BCM_GPS) || defined(CONFIG_BCM_GPS_MODULE)
#define board_hana_gps_info concatenate(CAPRI_BOARD_ID, _board_hana_gps_info)
static struct gps_platform_data board_hana_gps_info = GPS_PLATFORM_DATA_SETTINGS;

#define platform_device_gps concatenate(CAPRI_BOARD_ID, _platform_device_gps)
static struct platform_device platform_device_gps =
{
	.name = "gps",
	.id = -1,
	.dev = {
		.platform_data = &board_hana_gps_info,
	},
};
#endif

#if defined(CONFIG_BCM_HAPTICS) || defined(CONFIG_BCM_HAPTICS_MODULE)
#define board_bcm_haptics_device concatenate(CAPRI_BOARD_ID, _bcm_haptics_device)

#define board_bcm_haptics_data concatenate(CAPRI_BOARD_ID, _board_bcm_haptics_data)
static struct bcm_haptics_data board_bcm_haptics_data = BCM_HAPTICS_SETTINGS;

static struct platform_device board_bcm_haptics_device = {
	.name = BCM_HAPTICS_DRIVER_NAME,
	.id = -1,
	.dev = {
		.platform_data = &board_bcm_haptics_data,
	},
};
#endif

#if defined(CONFIG_KONA_OTG_CP) || defined(CONFIG_KONA_OTG_CP_MODULE)
static struct resource otg_cp_resource[] = {
	[0] = {
		.start = HSOTG_CTRL_BASE_ADDR,
		.end = HSOTG_CTRL_BASE_ADDR + OTG_CTRL_CORE_REG_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BCM_INT_ID_USB_OTG_DRV_VBUS,
		.end = BCM_INT_ID_USB_OTG_DRV_VBUS,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device otg_cp_device =
{
	.name = "kona-otg-cp",
	.id = -1,
	.resource = otg_cp_resource,
	.num_resources = ARRAY_SIZE(otg_cp_resource),
};
#endif

#if defined(CONFIG_KONA_VCHIQ) || defined(CONFIG_KONA_VCHIQ_MODULE)

/****************************************************************************
*
*   VCHIQ display device
*
***************************************************************************/
#if defined( CONFIG_KONA_VCHIQ_MEMDRV ) || defined( CONFIG_KONA_VCHIQ_MEMDRV_MODULE )

/*
 * Internal videocore using the vchiq_arm stack
 */

#define  VCHIQ_DISPLAY_DEVICE

#define IPC_SHARED_CHANNEL_VIRT     ( KONA_INT_SRAM_BASE + BCMHANA_ARAM_VC_OFFSET )
#define IPC_SHARED_CHANNEL_PHYS     ( INT_SRAM_BASE + BCMHANA_ARAM_VC_OFFSET )

static VCHIQ_PLATFORM_MEMDRV_KONA_DATA_GPIO_T vchiq_display_data_memdrv_kona_gpio_config[] = {
#ifndef CONFIG_KONA_ATAG_DT
	HW_CFG_CAMERA_GPIO
#endif
};

static VCHIQ_PLATFORM_DATA_MEMDRV_KONA_T vchiq_display_data_memdrv_kona = {
    .memdrv = {
        .common = {
            .instance_name = "display",
            .dev_type      = VCHIQ_DEVICE_TYPE_SHARED_MEM,
        },
        .sharedMemVirt  = (void *)(IPC_SHARED_CHANNEL_VIRT),
        .sharedMemPhys  = IPC_SHARED_CHANNEL_PHYS,
    },
    .ipcIrq                =  BCM_INT_ID_IPC_OPEN,
    .num_gpio_configs      =  ARRAY_SIZE( vchiq_display_data_memdrv_kona_gpio_config ),
    .gpio_config           = vchiq_display_data_memdrv_kona_gpio_config,
};

static struct platform_device vchiq_display_device = {
    .name = "vchiq_memdrv_kona",
    .id = 0,
    .dev = {
        .platform_data = &vchiq_display_data_memdrv_kona,
    },
};

#elif defined( CONFIG_VC_VCHIQ_BUSDRV_SHAREDMEM ) || defined( CONFIG_VC_VCHIQ_BUSDRV_SHAREDMEM_MODULE )

/*
 * Internal videocore using the vchiq stack.
 */

#define  VCHIQ_DISPLAY_DEVICE

static VCHIQ_PLATFORM_DATA_KONA_T vchiq_display_data_shared_mem = {
    .common = {
        .instance_name  = "display",
        .dev_type       = VCHIQ_DEVICE_TYPE_HOST_PORT,
    },
};

static struct platform_device vchiq_display_device = {
    .name = "vchiq_busdrv_sharedmem",
    .id = 0,
    .dev = {
        .platform_data = &vchiq_display_data_shared_mem,
    },
};

#endif

struct platform_device * vchiq_devices[] __initdata =
{
#if defined( VCHIQ_DISPLAY_DEVICE )
	&vchiq_display_device,
#endif

#ifdef CONFIG_KEYBOARD_BCM
	&bcm_kp_device,
#endif
};

#endif  /* CONFIG_KONA_VCHIQ */

#if defined(CONFIG_INPUT_MPU6050) || defined(CONFIG_INPUT_MPU6500)
#define GYRO_INT_GPIO_PIN   	(121)
static void sensors_regulator_on(bool onoff)
{

}

#if defined(CONFIG_MACH_CAPRI_SS_CRATER)

static struct mpu6k_input_platform_data mpu6k_pdata = {
	.power_on = sensors_regulator_on,
	.orientation = {
	1, 0, 0,
	0, -1, 0,
	0, 0, -1
	},	
	.acc_cal_path = "/efs/calibration_data",
	.gyro_cal_path = "/efs/gyro_cal_data",
};

static struct i2c_board_info __initdata inv_mpu_i2c0_boardinfo[] =
{
	{
		I2C_BOARD_INFO("mpu6050_input", 0x68),
		.platform_data = &mpu6k_pdata,
		.irq = gpio_to_irq(GYRO_INT_GPIO_PIN),
	},
};

#elif defined(CONFIG_MACH_CAPRI_SS_BAFFIN)

static struct mpu6k_input_platform_data mpu6k_pdata = {
	.power_on = sensors_regulator_on,
	.orientation = {
	0, 1, 0,
	1, 0, 0,
	0, 0, -1
	},	
	.acc_cal_path = "/efs/calibration_data",
	.gyro_cal_path = "/efs/gyro_cal_data",
};

static struct i2c_board_info __initdata inv_mpu_i2c0_boardinfo[] =
{
	{
		I2C_BOARD_INFO("mpu6050_input", 0x68),
		.platform_data = &mpu6k_pdata,
		.irq = gpio_to_irq(GYRO_INT_GPIO_PIN),
	},
};

#else

static struct mpu6k_input_platform_data mpu6k_pdata = {
	.power_on = sensors_regulator_on,
	.orientation = {
	-1, 0, 0,
	0, -1, 0,
        0, 0, 1
	},	
	.acc_cal_path = "/efs/calibration_data",
	.gyro_cal_path = "/efs/gyro_cal_data",
};

static struct i2c_board_info __initdata inv_mpu_i2c0_boardinfo[] =
{
	{
		I2C_BOARD_INFO("mpu6050_input", 0x68),
		.platform_data = &mpu6k_pdata,
		.irq = gpio_to_irq(GYRO_INT_GPIO_PIN),
	},

#ifdef CONFIG_INPUT_YAS_MAGNETOMETER
	{
		I2C_BOARD_INFO("geomagnetic", 0x2e),
	},
#endif        

};

#endif

#endif //defined(CONFIG_INPUT_MPU6050) || defined(CONFIG_INPUT_MPU6500)

#if defined(CONFIG_INPUT_YAS_ORIENTATION)
static struct platform_device yas532_orient_device = {
	.name = "orientation",
};
#endif

#if defined(CONFIG_OPTICAL_CM3663) 

#define GPIO_PS_ALS_INT 59
#define GPIO_PROXI_EN 122

static int cm3663_ldo(bool on)
{
        if(on)
            gpio_direction_output(GPIO_PROXI_EN, 1);
        else
            gpio_direction_output(GPIO_PROXI_EN, 0);
        
	return 0;
}

static struct cm3663_platform_data cm3663_pdata = {
    	.power_gpio = GPIO_PROXI_EN,
	.proximity_power = cm3663_ldo,
};
#endif

#if defined (CONFIG_SENSORS_GP2A030)
enum {
	GP2AP020 = 0,
	GP2AP030,
};
   
#define GPIO_PS_ALS_INT 59
#define GPIO_PROXI_EN 122

static struct gp2ap030_pdata gp2a_data = {
	.p_out = GPIO_PS_ALS_INT,
    	.power_gpio = GPIO_PROXI_EN,        
    	.led_on	= gp2a_led_onoff,
	.version = GP2AP030,
	.prox_cal_path = "/efs/prox_cal"
};
#endif

static struct i2c_board_info __initdata sensor1_i2c_gpio_board_info[] = {

#if defined(CONFIG_OPTICAL_CM3663) 
	{
		I2C_BOARD_INFO("cm3663", 0x20),
		.irq = GPIO_PS_ALS_INT,
		.platform_data = &cm3663_pdata,
	},
#endif

#if defined (CONFIG_SENSORS_GP2A030)
	{
		I2C_BOARD_INFO("gp2a030", 0x72>>1),
		.platform_data = &gp2a_data,
	},
#endif

};


#if defined(CONFIG_SENSORS_GP2A030)
static void gp2a_led_onoff(int onoff)
{
	if (onoff) {	
		gpio_direction_output(GPIO_PROXI_EN , 1 );
	} else {
		gpio_direction_output(GPIO_PROXI_EN , 0 );
	}
}
#endif


#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN) || defined(CONFIG_MACH_CAPRI_SS_CRATER)

static struct i2c_board_info __initdata sensor2_i2c_gpio_board_info[] = {

#ifdef CONFIG_INPUT_YAS_MAGNETOMETER
	{
		I2C_BOARD_INFO("geomagnetic", 0x2e),
	},
#endif

};

#endif

#if defined(CONFIG_AL3006) || defined(CONFIG_AL3006_MODULE)
static struct al3006_platform_data al3006_pdata = {
#ifdef AL3006_IRQ_GPIO
	.irq_gpio = AL3006_IRQ_GPIO,
#else
	.irq_gpio = -1,
#endif
};

static struct i2c_board_info __initdata i2c_al3006_info[] = {
	{
		I2C_BOARD_INFO("al3006", AL3006_I2C_ADDRESS),
		.platform_data = &al3006_pdata,
	},
};
#endif

#if defined(CONFIG_INPUT_HSCDTD006A) || defined(CONFIG_INPUT_HSCDTD006A_MODULE)
static struct hscdtd_platform_data hscdtd006a_data = HSCDTD006A_PLATFORM_DATA;
static struct i2c_board_info __initdata i2c_hscdtd006a_info[] = {
	{
	 I2C_BOARD_INFO(HSCDTD_DRIVER_NAME, HSCDTD006A_I2C_ADDRESS),
	 .platform_data = &hscdtd006a_data,
	 },
};
#endif

#if defined(CONFIG_BCM_RFKILL) || defined(CONFIG_BCM_RFKILL_MODULE)
#define board_bcmbt_rfkill_cfg concatenate(CAPRI_BOARD_ID, _bcmbt_rfkill_cfg)
#define GPIO_BT_RESET 95
static struct bcmbt_rfkill_platform_data board_bcmbt_rfkill_cfg =
{
#if defined(BCMBT_VREG_GPIO)
	.vreg_gpio = BCMBT_VREG_GPIO,
#else
	.vreg_gpio = -1,
#endif
#if defined(CONFIG_MACH_CAPRI_SS_S2VE)
	.n_reset_gpio = GPIO_BT_RESET,
#else
	.n_reset_gpio = -1,
#endif
	.aux0_gpio = -1,
	.aux1_gpio = -1
};
#define board_bcmbt_rfkill_device concatenate(CAPRI_BOARD_ID, _bcmbt_rfkill_device)
static struct platform_device board_bcmbt_rfkill_device =
{
	.name = "bcmbt-rfkill",
	.id = 1,
	.dev =
	{
		.platform_data = &board_bcmbt_rfkill_cfg,
	},
};

static void __init board_add_bcmbt_rfkill_device(void)
{
	platform_device_register(&board_bcmbt_rfkill_device);
}
#endif


#ifdef CONFIG_BCM_BZHW
#define GPIO_BT_WAKE 97
#define GPIO_HOST_WAKE 96
#define board_bcm_bzhw_data concatenate(CAPRI_BOARD_ID, _bcm_bzhw_data)
static struct bcm_bzhw_platform_data bcm_bzhw_data = {
	.gpio_bt_wake = GPIO_BT_WAKE,
	.gpio_host_wake = GPIO_HOST_WAKE,
};

#define board_bcm_bzhw_device concatenate(CAPRI_BOARD_ID, _bcm_bzhw_device)
static struct platform_device board_bcm_bzhw_device = {
	.name = "bcm_bzhw",
	.id = -1,
	.dev = {
		.platform_data = &bcm_bzhw_data,
		},
};

static void __init board_add_bcm_bzhw_device(void)
{
	platform_device_register(&board_bcm_bzhw_device);
}
#endif


#if defined(CONFIG_BCM_BT_LPM) || defined(CONFIG_BCM_BT_LPM_MODULE)
#define GPIO_BT_WAKE 97
#define GPIO_HOST_WAKE 96
#define board_bcmbt_lpm_cfg concatenate(CAPRI_BOARD_ID, _bcmbt_lpm_cfg)
static struct bcmbt_platform_data board_bcmbt_lpm_cfg = {
	.bt_wake_gpio = GPIO_BT_WAKE,
	.host_wake_gpio = GPIO_HOST_WAKE,
	.bt_uart_port = 1,
};
#define board_bcmbt_lpm_device concatenate(CAPRI_BOARD_ID, _bcmbt_lpm_device)
static struct platform_device board_bcmbt_lpm_device =
{
	.name = "bcmbt-lpm",
	.id = -1,
	.dev =
	{
		.platform_data = &board_bcmbt_lpm_cfg,
	},
};

static void __init board_add_bcmbt_lpm_device(void)
{
	platform_device_register(&board_bcmbt_lpm_device);
}
#endif

static void __init add_sdio_device(void)
{
	unsigned int i, id, num_devices;

	num_devices = ARRAY_SIZE(sdio_param);
	if (num_devices > SDIO_MAX_NUM_DEVICES)
		num_devices = SDIO_MAX_NUM_DEVICES;

	/*
	 * Need to register eMMC as the first SDIO device so it grabs mmcblk0 when
	 * it's installed. This required for rootfs to be mounted properly
	 *
	 * Ask Darwin for why we need to do this
	 */
	for (i = 0; i < num_devices; i++)
	{
		id = sdio_param[i].id;
		if (id < SDIO_MAX_NUM_DEVICES)
		{
			if (sdio_param[i].devtype == SDIO_DEV_TYPE_EMMC)
			{
				sdio_devices[id].dev.platform_data = &sdio_param[i];
				platform_device_register(&sdio_devices[id]);
			}
		}
	}

	for (i = 0; i < num_devices; i++)
	{
		id = sdio_param[i].id;

		/* skip eMMC as it has been registered */
		if (sdio_param[i].devtype == SDIO_DEV_TYPE_EMMC)
			continue;

		if (id < SDIO_MAX_NUM_DEVICES)
		{
			if (sdio_param[i].devtype == SDIO_DEV_TYPE_WIFI)
			{
				struct sdio_wifi_gpio_cfg *wifi_gpio =
					&sdio_param[i].wifi_gpio;

#ifdef HW_WLAN_GPIO_RESET_PIN
				wifi_gpio->reset = HW_WLAN_GPIO_RESET_PIN;
#else
				wifi_gpio->reset = -1;
#endif
#ifdef HW_WLAN_GPIO_SHUTDOWN_PIN
				wifi_gpio->shutdown = HW_WLAN_GPIO_SHUTDOWN_PIN;
#else
				wifi_gpio->shutdown = -1;
#endif
#ifdef HW_WLAN_GPIO_REG_PIN
				wifi_gpio->reg = HW_WLAN_GPIO_REG_PIN;
#else
				wifi_gpio->reg = -1;
#endif
#ifdef HW_WLAN_GPIO_HOST_WAKE_PIN
				wifi_gpio->host_wake = HW_WLAN_GPIO_HOST_WAKE_PIN;
#else
				wifi_gpio->host_wake = -1;
#endif
			}
			sdio_devices[id].dev.platform_data = &sdio_param[i];
			platform_device_register(&sdio_devices[id]);
		}
	}
}

static void __init add_i2c_device(void)
{
	unsigned int i, num_devices;

	num_devices = ARRAY_SIZE(i2c_adap_param);
	if (num_devices == 0)
		return;
	if (num_devices > MAX_I2C_ADAPS)
		num_devices = MAX_I2C_ADAPS;

	for (i = 0; i < num_devices; i++) {
		/* DO NOT register the I2C device if it is disabled */
		if (i2c_adap_param[i].disable == 1)
			continue;

		i2c_adap_devices[i].dev.platform_data = &i2c_adap_param[i];
		platform_device_register(&i2c_adap_devices[i]);
	}


#if defined(CONFIG_TOUCHSCREEN_BCM915500) || defined(CONFIG_TOUCHSCREEN_BCM915500_MODULE)
	printk(KERN_INFO "PPTEST %s() bcm915500_i2c_boardinfo[0].irq: %d\n",
		__func__, bcm915500_i2c_boardinfo[0].irq);

	i2c_register_board_info(bcm915500_i2c_platform_data.i2c_bus_id,
				bcm915500_i2c_boardinfo,
				ARRAY_SIZE(bcm915500_i2c_boardinfo));
#endif

#ifdef CONFIG_TOUCHSCREEN_FT5X06
	i2c_register_board_info(FT5X06_I2C_BUS_ID, ft5x06_info,
				ARRAY_SIZE(ft5x06_info));
#endif /* CONFIG_TOUCHSCREEN_FT5X06 */

#if defined(CONFIG_USB_SWITCH_TSU6721)
       pr_info("tsu6721\n");
	uas_jiguart_wl_init();
       i2c_register_board_info(TSU6721_I2C_BUS_ID, micro_usb_i2c_devices_info,ARRAY_SIZE(micro_usb_i2c_devices_info));
#endif
#ifdef CONFIG_USB_SWITCH_FSA9485
	pr_info("fsa9485\n");
	i2c_register_board_info(FSA9485_I2C_BUS_ID, micro_usb_i2c_devices_info,ARRAY_SIZE(micro_usb_i2c_devices_info));
#endif
#if defined(CONFIG_BQ24272_CHARGER) || defined(CONFIG_SMB358_CHARGER)
	pr_info("charger_i2c_devices_info\n");
	i2c_register_board_info(CHARGER_I2C_BUS_ID, charger_i2c_devices_info,ARRAY_SIZE(charger_i2c_devices_info));
#endif

#ifdef CONFIG_TOUCHSCREEN_GT818
        i2c_register_board_info(GT818_I2C_BUS_ID,
                gt818_info,
                ARRAY_SIZE(gt818_info));
#endif

#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCH )
	i2c_register_board_info(4, i2c_gpio_emul_driver, ARRAY_SIZE(i2c_gpio_emul_driver));
#endif

#if defined(CONFIG_KEYBOARD_TC360L_TOUCHKEY)
	i2c_register_board_info(4, touchkey_gpio_i2c_devices, ARRAY_SIZE(touchkey_gpio_i2c_devices));
#endif

#if defined(CONFIG_OPTICAL_CM3663) || defined (CONFIG_SENSORS_GP2A030)
	i2c_register_board_info(5, sensor1_i2c_gpio_board_info, ARRAY_SIZE(sensor1_i2c_gpio_board_info));
#endif

#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN) || defined(CONFIG_MACH_CAPRI_SS_CRATER)
	i2c_register_board_info(6, sensor2_i2c_gpio_board_info, ARRAY_SIZE(sensor2_i2c_gpio_board_info));
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X
	i2c_register_board_info(1, cyttsp4_i2c_info,
		ARRAY_SIZE(cyttsp4_i2c_info));
#endif

#ifdef CONFIG_TOUCHSCREEN_ZINITIX_BT532
	i2c_register_board_info(1, zinitix_i2c_info,
		ARRAY_SIZE(zinitix_i2c_info));
#endif

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT
i2c_register_board_info(1,
			mxt224_info,
			ARRAY_SIZE(mxt224_info));
#endif

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT224
	board_tsp_init();
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_TMA46X
       board_tsp_init();
#endif

#ifdef CONFIG_SAMSUNG_MHL
	board_mhl_init();
#endif

#if defined(CONFIG_INPUT_MPU6050)
	i2c_register_board_info(0, inv_mpu_i2c0_boardinfo, ARRAY_SIZE(inv_mpu_i2c0_boardinfo));
#endif

#if defined(CONFIG_BMP18X_I2C) || defined(CONFIG_BMP18X_I2C_MODULE)
	i2c_register_board_info(
#ifdef BMP18X_I2C_BUS_ID
			BMP18X_I2C_BUS_ID,
#else
			-1,
#endif
			i2c_bmp18x_info, ARRAY_SIZE(i2c_bmp18x_info));
#endif

#if defined(CONFIG_AL3006) || defined(CONFIG_AL3006_MODULE)
#ifdef AL3006_IRQ
	i2c_al3006_info[0].irq = gpio_to_irq(AL3006_IRQ_GPIO);
#else
	i2c_al3006_info[0].irq = -1;
#endif
	i2c_register_board_info(
#ifdef AL3006_I2C_BUS_ID
		AL3006_I2C_BUS_ID,
#else
		-1,
#endif
		i2c_al3006_info, ARRAY_SIZE(i2c_al3006_info));
#endif /* CONFIG_AL3006 */

#if defined(CONFIG_AMI306) || defined(CONFIG_AMI306_MODULE)
	i2c_register_board_info (
#ifdef AMI_I2C_BUS_NUM
		AMI_I2C_BUS_NUM,
#else
		-1,
#endif
		i2c_ami306_info, ARRAY_SIZE(i2c_ami306_info));
#endif /* CONFIG_AMI306 */

#if defined(CONFIG_INPUT_HSCDTD006A) || defined(CONFIG_INPUT_HSCDTD006A_MODULE)
	i2c_register_board_info(HSCDTD006A_I2C_BUS_ID, i2c_hscdtd006a_info, ARRAY_SIZE(i2c_hscdtd006a_info));
#endif


#if defined(CONFIG_BCM2079X_NFC_I2C)
	i2c_register_board_info(BCM_NFC_BUSID, i2c_devs_nfc, ARRAY_SIZE(i2c_devs_nfc));
#endif

}

#if defined(CONFIG_LEDS_GPIO) || defined(CONFIG_LEDS_GPIO_MODULE)
#define board_add_led_device concatenate(CAPRI_BOARD_ID, _add_led_device)
static void __init board_add_led_device(void)
{
	platform_device_register(&board_leds_gpio_device);
}
#endif

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
#define board_add_keys_device concatenate(CAPRI_BOARD_ID, _add_keyboard_device)
static void __init board_add_keys_device(void)
{
	platform_device_register(&board_gpio_keys_device);
}
#endif

#if defined(CONFIG_KEYBOARD_KONA) || defined(CONFIG_KEYBOARD_KONA_MODULE)
#define board_add_keyboard_kona concatenate(CAPRI_BOARD_ID, _add_keyboard_kona)
static void __init board_add_keyboard_kona(void)
{
	platform_device_register(&board_keypad_device);
}
#endif


static void __init add_usbh_device(void)
{
	/*
	 * Always register the low level USB host device before EHCI/OHCI
	 * devices. Also, always add EHCI device before OHCI
	 */
#if defined(CONFIG_USB_BCM_CAPRI)
	platform_device_register(&usbh_device);
#endif
#if defined(CONFIG_USB_BCM_CAPRI_HSIC)
	platform_device_register(&usbh_hsic_device);
#endif
#if defined(CONFIG_USB_EHCI_BCM)
	platform_device_register(&usbh_ehci_device);
#endif
#if defined(CONFIG_USB_OHCI_BCM)
	platform_device_register(&usbh_ohci_device);
#endif
#if defined(CONFIG_USB_EHCI_BCM) && defined(CONFIG_USB_BCM_CAPRI_HSIC)
	platform_device_register(&usbh_hsic2_ehci_device);
#endif
}

#define GPIO_OTG_EN 8
//ENABLE_OTGGTEST
#ifdef CONFIG_USB_HOST_NOTIFY
static void otg_accessory_power(int enable)
{
	u8 on = (u8)!!enable;
	printk("USBD] otg_accessory_power dis\n");
	/* max77693 otg power control */
	/*
	otg_control(enable);

	gpio_request(GPIO_OTG_EN, "USB_OTG_EN");
	gpio_direction_output(GPIO_OTG_EN, on);
	gpio_free(GPIO_OTG_EN);
	pr_info("%s: otg accessory power = %d\n", __func__, on);*/
}

static void otg_accessory_powered_booster(int enable)
{
	u8 on = (u8)!!enable;
	printk("dh0318.lee] otg_accessory_powered_booster\n");
	/* max77693 powered otg power control */
/*	powered_otg_control(enable);
	pr_info("%s: otg accessory power = %d\n", __func__, on);*/
}

static struct host_notifier_platform_data host_notifier_pdata = {
	.ndev.name	= "usb_otg",
	.booster	= otg_accessory_power,
	.powered_booster = otg_accessory_powered_booster,
	.thread_enable	= 0,
};

struct platform_device host_notifier_device = {
	.name = "host_notifier",
	.dev.platform_data = &host_notifier_pdata,
};
#endif

#ifdef CONFIG_USB_SWITCH_FSA9485
static void fsa9485_otg_cb(bool attached)
{
	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;
	switch (set_cable_status) {
	case CABLE_TYPE_USB:
#ifdef CONFIG_USB_HOST_NOTIFY		
		host_state_notify(&host_notifier_pdata.ndev, NOTIFY_HOST_ADD);
#endif
		send_otg_insert_event();
		break;
	case CABLE_TYPE_NONE:
#ifdef CONFIG_USB_HOST_NOTIFY		
		host_state_notify(&host_notifier_pdata.ndev, NOTIFY_HOST_REMOVE);
#endif
                send_otg_insert_event();
		break;
	}	

}
#endif
static void __init add_usb_otg_device(void)
{
#if defined(CONFIG_KONA_OTG_CP) || defined(CONFIG_KONA_OTG_CP_MODULE)
	platform_device_register(&otg_cp_device);
#endif

#if defined(CONFIG_BCM_WFD) || defined(CONFIG_BCM_WFD_MODULE)
	platform_device_register(&board_bcm_wfd_device);
#endif

//dh0318.lee ENABLE_OTGGTEST
#ifdef CONFIG_USB_HOST_NOTIFY
	platform_device_register(&host_notifier_device);
#endif
}

#if defined(CONFIG_BCM_HALAUDIO)
#define board_halaudio_dev_list concatenate(CAPRI_BOARD_ID, _halaudio_dev_list)
static HALAUDIO_DEV_CFG board_halaudio_dev_list[] =
#ifdef HALAUDIO_DEV_LIST
	HALAUDIO_DEV_LIST;
#else
	NULL;
#endif

#define board_halaudio_cfg concatenate(CAPRI_BOARD_ID, _halaudio_cfg)
static HALAUDIO_CFG board_halaudio_cfg;

#define board_halaudio_device concatenate(CAPRI_BOARD_ID, _halaudio_device)
static struct platform_device board_halaudio_device =
{
	.name = "bcm-halaudio",
	.id = -1, /* to indicate there's only one such device */
	.dev =
	{
		.platform_data = &board_halaudio_cfg,
	},
};

#define board_add_halaudio_device concatenate(CAPRI_BOARD_ID, _add_halaudio_device)
static void __init board_add_halaudio_device(void)
{
	board_halaudio_cfg.numdev = ARRAY_SIZE(board_halaudio_dev_list);
	board_halaudio_cfg.devlist = board_halaudio_dev_list;
	platform_device_register(&board_halaudio_device);
}
#endif /* CONFIG_BCM_HALAUDIO */

#ifdef CONFIG_SEC_CHARGING_FEATURE
#ifdef CONFIG_WD_TAPPER
static struct wd_tapper_platform_data wd_tapper_data = {
  /* Set the count to the time equivalent to the time-out in seconds
   * required to pet the PMU watchdog to overcome the problem of reset in
   * suspend*/
  .count = 300,
  .lowbattcount = 120,
  .verylowbattcount = 5,
  .ch_num = 1,
  .name = "aon-timer",
};

static struct platform_device wd_tapper = {
  .name = "wd_tapper",
  .id = 0,
  .dev = {
    .platform_data = &wd_tapper_data,
  },
};
#endif
#endif

#if defined(CONFIG_BCM_ALSA_SOUND)
#define board_caph_platform_cfg concatenate(CAPRI_BOARD_ID, _caph_platform_cfg)
static struct caph_platform_cfg board_caph_platform_cfg =
#ifdef HW_CFG_CAPH
	HW_CFG_CAPH;
#else
{
	.aud_ctrl_plat_cfg = {
		.ext_aud_plat_cfg = {
			.ihf_ext_amp_gpio = -1,
			.dock_aud_route_gpio = -1,
			.mic_sel_aud_route_gpio = -1,
#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN_CMCC)||defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)		
			.mode_sel_aud_route_gpio = -1,
			.bt_sel_aud_route_gpio = -1,
#endif	

		}
	}
};
#endif

#define board_caph_device concatenate(CAPRI_BOARD_ID, _caph_device)
static struct platform_device board_caph_device = {
	.name = "brcm_caph_device",
	.id = -1, /*Indicates only one device */
	.dev = {
		.platform_data = &board_caph_platform_cfg,
	},
};
#endif /* CONFIG_BCM_ALSA_SOUND */

#if defined(CONFIG_SEC_CHARGING_FEATURE)
// Samsung charging feature
// +++ for board files, it may contain changeable values
#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN)
struct spa_temp_tb batt_temp_tb[]=
{
	{869, -300},            /* -30 */
	{754, -200},			/* -20 */
	{709, -150},			/* -15 */
	{644, -100},                    /* -10 */
	{577, -50},						/* -5 */
	{510,   0},                    /* 0   */
	{445,   50},                    /* 0   */
	{383,  100},                    /* 10  */
	{329,  150},                    /* 15  */
	{279,  200},                    /* 20  */
	{234,  250},                    /* 25  */
	{197,  300},                    /* 30  */
	{164,  350},                    /* 30  */
	{137,  400},                    /* 40  */
	{115,  450},                    /* 40  */
	{96 ,  500},                    /* 50  */
	{81 ,  550},                    /* 50  */
	{68 ,  600},                    /* 60  */
	{57 ,  650},                    /* 65  */
	{48 ,  700},            /* 70  */
	{34 ,  800},            /* 80  */
};
#elif defined(CONFIG_MACH_CAPRI_SS_CRATER)
struct spa_temp_tb batt_temp_tb[]=
{
	{869, -300},            /* -30 */
	{754, -200},			/* -20 */
	{639, -100},                    /* -10 */
	{566, -50},			/* -5 */
	{492,   0},                    /* 0   */
	{440,   50},                    /* 5   */
	{380,  100},                    /* 10  */
	{323,  150},                    /* 10  */
	{267,  200},                    /* 20  */
	{229,  250},                    /* 25  */
	{194,  300},                    /* 30  */
	{163,  350},                    /* 30  */
	{132,  400},                    /* 40  */
	{114,  450},                    /* 45  */
	{97 ,  500},                    /* 50  */
	{81 ,  550},                    /* 50  */
	{68 ,  600},                    /* 60  */
	{57 ,  650},                    /* 65  */
	{48 ,  700},            /* 70  */
	{41 ,  750},           	/* 75  */	
	{35 ,  800},            	/* 80  */
	{30 ,  850},           	/* 85  */	
	{25 ,  900},            	/* 90  */	
};
#else
struct spa_temp_tb batt_temp_tb[]=
{
	{869, -300},            /* -30 */
	{754, -200},			/* -20 */
	{646, -100},                    /* -10 */
	{578, -50},						/* -5 */
	{511,   0},                    /* 0   */
	{445,   50},                    /* 0   */
	{384,  100},                    /* 10  */
	{328,  150},                    /* 10  */
	{278,  200},                    /* 20  */
	{234,  250},                    /* 25  */
	{197,  300},                    /* 30  */
	{164,  350},                    /* 30  */
	{137,  400},                    /* 40  */
	{114,  450},                    /* 40  */
	{96 ,  500},                    /* 50  */
	{81 ,  550},                    /* 50  */
	{68 ,  600},                    /* 60  */
	{57 ,  650},                    /* 65  */
	{48 ,  700},            /* 70  */
	{34 ,  800},            /* 80  */
};
#endif

struct spa_power_data spa_power_pdata=
{
#if defined(CONFIG_MFD_BCM59054)
	.charger_name = "bcm59054_charger",
#else
	.charger_name = "bcm59056_charger",
#endif
#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN)
	.eoc_current=160,
	.recharge_voltage=4300,
	.charging_cur_usb=500,
	.charging_cur_wall=1200,
#if defined(CONFIG_SPA_SUPPLEMENTARY_CHARGING)
	.backcharging_time = 40, //mins
	.recharging_eoc = 40, // mA
#endif
#elif defined(CONFIG_MACH_CAPRI_SS_CRATER)
	.eoc_current=200,
	.recharge_voltage=4300,
	.charging_cur_usb=450,
	.charging_cur_wall=2000,
#if defined(CONFIG_SPA_SUPPLEMENTARY_CHARGING)
	.backcharging_time = 40, //mins
	.recharging_eoc = 100, // mA
#endif
#else	
#if defined(CONFIG_SPA_SUPPLEMENTARY_CHARGING)
	.backcharging_time = 18, //mins
	.recharging_eoc = 100, // mA
	.eoc_current=150,
#else
	.eoc_current=100,
#endif
	.recharge_voltage=4150,
	.charging_cur_usb=500,
	.charging_cur_wall=850,
#endif	
	.suspend_temp_hot=600,
	.recovery_temp_hot=400,
	.suspend_temp_cold=-50,
	.recovery_temp_cold=0,
	.charge_timer_limit=CHARGE_TIMER_6HOUR,
	.batt_temp_tb=&batt_temp_tb[0],
	.batt_temp_tb_len=ARRAY_SIZE(batt_temp_tb),
};
EXPORT_SYMBOL(spa_power_pdata);

static struct platform_device spa_power_device=
{
	.name = "spa_power",
	.id=-1,
	.dev.platform_data = &spa_power_pdata,
};
static struct platform_device spa_ps_device=
{
	.name = "spa_ps",
	.id=-1,
};
// --- for board files
#endif

#ifdef CONFIG_BCM_SS_VIBRA
struct platform_device bcm_vibrator_device = {     
	.name = "vibrator",     
	.id = 0,     
	.dev = {     
		.platform_data="vibldo_uc",     
	},     
};     
#endif  

static void __init add_devices(void)
{
#if defined(HW_I2C_ADAP_PARAM)
	add_i2c_device();
#endif

#if defined(HW_SDIO_PARAM)
	add_sdio_device();
#endif

#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT
	printk(KERN_ERR "Calling WLAN_INIT!\n");
	capri_wlan_init();
	printk(KERN_ERR "DONE WLAN_INIT!\n");
#endif

#if defined(CONFIG_LEDS_GPIO) || defined(CONFIG_LEDS_GPIO_MODULE)
	//board_add_led_device();
#endif

#if defined(CONFIG_BCM_HEADSET_SW)
	board_add_headsetdet_device();
#endif

#if defined(CONFIG_KONA_HEADSET) || defined(CONFIG_KONA_HEADSET_MULTI_BUTTON)
	platform_device_register(&headset_device);
#endif

#if defined(CONFIG_KEYBOARD_GPIO) || defined(CONFIG_KEYBOARD_GPIO_MODULE)
	board_add_keys_device();
#endif

#if defined(CONFIG_KEYBOARD_KONA) || defined(CONFIG_KEYBOARD_KONA_MODULE)
	board_add_keyboard_kona();
#endif

#ifdef CONFIG_KEYBOARD_BCM
	bcm_kp_device.dev.platform_data = &bcm_keypad_data;
#endif

#if defined(CONFIG_BCM_RFKILL) || defined(CONFIG_BCM_RFKILL_MODULE)
        board_add_bcmbt_rfkill_device();
#endif

#ifdef CONFIG_BCM_BZHW
	board_add_bcm_bzhw_device();
#endif

#if defined(CONFIG_BCM_BT_LPM) || defined(CONFIG_BCM_BT_LPM_MODULE)
        board_add_bcmbt_lpm_device();
#endif

#if defined(CONFIG_BCM_HAPTICS) || defined(CONFIG_BCM_HAPTICS_MODULE)
	platform_device_register(&board_bcm_haptics_device);
#endif

	add_usbh_device();
	add_usb_otg_device();

#if defined(CONFIG_BCM_HDMI_DET) || defined(CONFIG_BCM_HDMI_DET_MODULE)
	board_add_hdmidet_device();
#endif

#if defined(CONFIG_TFT_PANEL) || defined(CONFIG_TFT_PANEL_MODULE)
	board_add_tft_panel_device();
#endif

#if defined(CONFIG_BCM_HALAUDIO)
	board_add_halaudio_device();
#endif

#if defined(CONFIG_BCM_ALSA_SOUND)
	platform_device_register(&board_caph_device);
#endif

#if defined(CONFIG_I2C_GPIO)
#if defined(CONFIG_TOUCHSCREEN_GT818)
       platform_device_register(&capri_i2c_gpio_device);
#endif
	platform_add_devices(gpio_i2c_devices, ARRAY_SIZE(gpio_i2c_devices));
#endif

#ifdef CONFIG_USB_SWITCH_FSA9485
	pr_info("fsa9485 mUSB_i2c_devices\n");
	platform_add_devices(mUSB_i2c_devices, ARRAY_SIZE(mUSB_i2c_devices));
#endif
#if defined(CONFIG_USB_SWITCH_TSU6721)
        pr_info("tsu6721 mUSB_i2c_devices\n");
	platform_add_devices(mUSB_i2c_devices, ARRAY_SIZE(mUSB_i2c_devices));
#endif
#ifdef CONFIG_BQ24272_CHARGER
	pr_info("bq24272 charger_i2c_devices_info\n");
	platform_device_register(&bq24272_charger);
#endif
#ifdef CONFIG_SMB358_CHARGER
	pr_info("smb358 charger_i2c_devices_info\n");
	platform_device_register(&smb358_charger);
#endif
#if defined(CONFIG_NET_ISLAND)
	platform_device_register(&net_device);
#endif

#if defined(CONFIG_BCM_GPS) || defined(CONFIG_BCM_GPS_MODULE)
	platform_device_register(&platform_device_gps);
#endif

#if defined(CONFIG_KONA_VCHIQ) || defined(CONFIG_KONA_VCHIQ_MODULE)
   platform_add_devices( vchiq_devices, ARRAY_SIZE( vchiq_devices ) );
#endif

#ifdef CONFIG_SEC_CHARGING_FEATURE
#if defined(CONFIG_WD_TAPPER)
	platform_device_register(&wd_tapper);
#endif
#endif
#ifdef CONFIG_BCM_SS_VIBRA                                                                                                         
	platform_device_register( &bcm_vibrator_device);
#endif

#if defined(CONFIG_SENSORS_KIONIX_KXTIK) || defined(CONFIG_SENSORS_KIONIX_KXTIK_MODULE)
	kxtik_init_platform_hw();
#endif /* CONFIG_SENSORS_KIONIX_KXTIK */

#if defined(CONFIG_SENSORS_AK8963) || defined(CONFIG_SENSORS_AK8963_MODULE)
	akm8963_init_platform_hw();
#endif /* CONFIG_SENSORS_AK8963 */
#if defined(CONFIG_SPI_SPIDEV) && defined(CONFIG_SPI_SSPI_KONA)
	spi_register_board_info(spidev_board_info,
			ARRAY_SIZE(spidev_board_info));
#endif

#if defined(CONFIG_SEC_CHARGING_FEATURE)
// Samsung charging feature
	platform_device_register(&spa_power_device);
	platform_device_register(&spa_ps_device);
#endif

#if defined(CONFIG_INPUT_YAS_ORIENTATION)
        platform_device_register(&yas532_orient_device);    
#endif

}

static void capri_poweroff(void)
{
#ifdef CONFIG_MFD_BCMPMU
        bcmpmu_client_power_off();
#endif

	while(1);
}

#if !defined(CONFIG_LCD_LD9040)
struct platform_ktd259b_backlight_data {
      	unsigned int max_brightness;
     	unsigned int dft_brightness;
     	unsigned int ctrl_pin;
};

static struct platform_ktd259b_backlight_data bcm_ktd259b_backlight_data = {
	.max_brightness = 255,
	.dft_brightness = 143,
	.ctrl_pin = 95,
};


static struct platform_device bcm_backlight_devices = {
    .name           = "panel",
	.id 		= -1,
	.dev 	= {
		.platform_data  =       &bcm_ktd259b_backlight_data,
	},
	
};
	
static void __init backlight_init(void)
{
	platform_device_register(&bcm_backlight_devices);
}

 static struct platform_device touchkeyled_device = {
 	.name 		= "touchkey-led",
 	.id 		= -1,
 };

static void __init touchkeyled_init(void)
{
	platform_device_register(&touchkeyled_device);
}
#endif

#if defined(CONFIG_LDI_MDNIE_SUPPORT)
struct platform_ldi_mdnie_data {
      	unsigned int mdnie_info;

};

static struct platform_ldi_mdnie_data ldi_mdnie_data = {
	.mdnie_info = 100,
};


static struct platform_device ldi_mdnie_devices = {
      .name           = "mdnie",
	.id 		= -1,
	.dev 	= {
		.platform_data  =       &ldi_mdnie_data,
	},
	
};
	
static void __init ldi_mdnie_init(void)
{
	platform_device_register(&ldi_mdnie_devices);
}
#endif

#if defined(CONFIG_ISA1000_VIBRATOR)
 
/******************************************************/
/*
 * ISA1000 HAPTIC MOTOR Driver IC.
 * MOTOR Resonance frequency: 205HZ. 
 * Input PWM Frequency: 205 * 128 = 26240 HZ. 
 * PWM_period_ns = 1000000000/26240 = 38109. 
 * PWM Enable GPIO number = 189. 
*/
/******************************************************/

#define GPIO_MOTOR_EN	189

static int isa1000_enable(bool en)
{
	return gpio_direction_output(GPIO_MOTOR_EN, en);
}

static struct platform_isa1000_vibrator_data isa1000_vibrator_data =
{
	.gpio_en	= isa1000_enable,
	.pwm_name	= "kona_pwmc:4",
	.pwm_duty	= 37728, //37000,
	.pwm_period_ns	= 38109,
	.polarity 	= 1,
	.regulator_id 	= "vibldo_uc",
};

static struct platform_device isa1000_device =
{
	.name     = "isa1000-vibrator",
	.id       = -1,
	.dev      =
		{
		.platform_data = &isa1000_vibrator_data,
	},
};

static void isa1000_gpio_init(void)
{
	int gpio, ret;

	printk("%s start \n",__func__);
	gpio = GPIO_MOTOR_EN;
	gpio_request(gpio, "MOTOR_EN");
	gpio_direction_output(gpio, 1);
	gpio_export(gpio, 0);
	printk("%s end \n",__func__);
}

static void __init isa1000_vibrator_init(void)
{
	isa1000_gpio_init();
	platform_device_register(&isa1000_device);
}
#endif
#if defined(CONFIG_DRV2603_VIBRATOR)
 
/******************************************************/
/*
 * DRV2603 HAPTIC MOTOR Driver IC.
 * MOTOR Resonance frequency: 205HZ. 
 * Input PWM Frequency: 205 * 128 = 26240 HZ. 
 * PWM_period_ns = 1000000000/26240 = 38109. 
 * PWM Enable GPIO number = 189. 
*/
/******************************************************/

#define GPIO_MOTOR_EN	189

static int drv2603_enable(bool en)
{
	return gpio_direction_output(GPIO_MOTOR_EN, en);
}

static struct platform_drv2603_vibrator_data drv2603_vibrator_data =
{
	.gpio_en	= drv2603_enable,
	.pwm_name	= "kona_pwmc:4",
	.pwm_duty	= 37728, //37000,
	.pwm_period_ns	= 38109,
	.polarity 	= 1,
	.regulator_id 	= "vibldo_uc",
};

static struct platform_device drv2603_device =
{
	.name     = "drv2603-vibrator",
	.id       = -1,
	.dev      =
		{
		.platform_data = &drv2603_vibrator_data,
	},
};

static void drv2603_gpio_init(void)
{
	int gpio, ret;

	printk("%s start \n",__func__);
	gpio = GPIO_MOTOR_EN;
	gpio_request(gpio, "MOTOR_EN");
	gpio_direction_output(gpio, 0);
	gpio_export(gpio, 0);
	printk("%s end \n",__func__);
}

static void __init drv2603_vibrator_init(void)
{
	drv2603_gpio_init();
	platform_device_register(&drv2603_device);
}
#endif
#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN_CMCC)||defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)		

#define GPIO_SIM_SEL 158

// 1 : BCOM
// 0 : SPRD
static int sim_sel_switch_en(bool en)
{
	printk("%s : %d \n",__func__, en);
	return gpio_direction_output(GPIO_SIM_SEL, en);
}

// default 0 : TD 1st.
// Temporary  1 : BCom 1st.
static int sim_sel_gpio_init(void)
{
	int gpio, ret;

	printk("%s start \n",__func__);
	gpio = GPIO_SIM_SEL;
	gpio_request(gpio, "SIM_SEL");
	gpio_direction_output(gpio, 1);
	gpio_export(gpio, 0);
	printk("%s end \n",__func__);
}

static void __init sim_sel_switch_init(void)
{
	sim_sel_gpio_init();
	//platform_device_register(&drv2603_device);
}


#define GPIO_UART_SEL 159

// 1 : SPRD
// 0 : BCOM
static int uart_sel_switch_en(bool en)
{
	printk("%s : %d \n",__func__, en);
	return gpio_direction_output(GPIO_UART_SEL, en);
}

static int uart_sel_gpio_init(void)
{
	int gpio, ret;

	printk("%s start \n",__func__);
	gpio = GPIO_UART_SEL;
	gpio_request(gpio, "UART_SEL");
	gpio_direction_output(gpio, 0);
	gpio_export(gpio, 0);
	printk("%s end \n",__func__);
}

static void __init uart_sel_switch_init(void)
{
	uart_sel_gpio_init();
	//platform_device_register(&drv2603_device);
}

#define GPIO_MODE_SEL 157

// 0 : BCOM
// 1 : SPRD
static int mode_sel_switch_en(bool en)
{
	printk("%s : %d \n",__func__, en);
	return gpio_direction_output(GPIO_MODE_SEL, en);
}

static int mode_sel_gpio_init(void)
{
	int gpio, ret;

	printk("%s start \n",__func__);
	gpio = GPIO_MODE_SEL;
	gpio_request(gpio, "MODE_SEL");
	gpio_direction_output(gpio, 0);
	gpio_set_value(gpio, 0);
	gpio_free(gpio);
	printk("%s end \n",__func__);
}

static void __init mode_sel_switch_init(void)
{
	mode_sel_gpio_init();
}

#define GPIO_CP_CTRL1 149

// 1 : BCOM
// 0 : SPRD
static int cp_ctrl1_en(bool en)
{
	printk("%s : %d \n",__func__, en);
	return gpio_direction_output(GPIO_CP_CTRL1, en);
}

static int cp_ctrl1_gpio_init(void)
{
	int gpio, ret;

	printk("%s start \n",__func__);
	gpio = GPIO_CP_CTRL1;
	gpio_request(gpio, "CP_CTRL1");
	gpio_direction_output(gpio, 1);
	gpio_export(gpio, 0);
	printk("%s end \n",__func__);
}

static void __init cp_ctrl1_switch_init(void)
{
	cp_ctrl1_gpio_init();
}

#define GPIO_MIC_SEL 146

// 0 : BCOM
// 1 : SPRD
static int mic_sel_switch_en(bool en)
{
	printk("%s : %d \n",__func__, en);
	return gpio_direction_output(GPIO_MIC_SEL, en);
}

static int mic_sel_gpio_init(void)
{
	int gpio, ret;

	printk("%s start \n",__func__);
	gpio = GPIO_MIC_SEL;
	gpio_request(gpio, "MIC_SEL");
	gpio_direction_output(gpio, 0);
	gpio_set_value(gpio, 0);
	gpio_free(gpio);
	printk("%s end \n",__func__);
}

static void __init mic_sel_switch_init(void)
{
	mic_sel_gpio_init();
}

#define GPIO_EAR_DOCK_SEL 134

// 0 : EAR
// 1 : DOCK
static int ear_dock_sel_switch_en(bool en)
{
	printk("%s : %d \n",__func__, en);
	return gpio_direction_output(GPIO_EAR_DOCK_SEL, en);
}

static int ear_dock_sel_gpio_init(void)
{
	int gpio, ret;

	printk("%s start \n",__func__);
	gpio = GPIO_EAR_DOCK_SEL;
	gpio_request(gpio, "AUDIO_DOCK_ROUTE");
	gpio_direction_output(gpio, 0);
	gpio_set_value(gpio, 0);
	gpio_free(gpio);
	printk("%s end \n",__func__);
}

static void __init ear_dock_sel_switch_init(void)
{
	ear_dock_sel_gpio_init();
}
#endif

static void __init board_init(void)
{
#if defined(CONFIG_MFD_BCMPMU)
	pm_power_off = capri_poweroff;
#endif
	
#if defined(CONFIG_MAP_DMA_MMAP)
	dma_mmap_init();
#endif
#if defined(CONFIG_MAP_SDMA)
	sdma_init();
#endif

#if defined(CONFIG_BCM_VC_CMA)
	vc_cma_early_init();
#endif

#if defined(CONFIG_BCM_VC_CAM)
	vc_cam_early_init();
#endif

	/*
	 * Add common platform devices that do not have board dependent HW
	 * configurations
	 */
	board_add_common_devices();

	/* add devices with board dependent HW configurations */
	add_devices();
#if defined(CONFIG_LCD_LD9040)
	oled_lcd_init();
#else
      backlight_init();
      touchkeyled_init();
#endif

#if defined(CONFIG_LDI_MDNIE_SUPPORT)
     ldi_mdnie_init();
#endif

#if defined(CONFIG_ISA1000_VIBRATOR)
	isa1000_vibrator_init();
#endif
#if defined(CONFIG_DRV2603_VIBRATOR)
	drv2603_vibrator_init();
#endif
#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN_CMCC)||defined(CONFIG_MACH_CAPRI_SS_CRATER_CMCC)		
    sim_sel_switch_init();
    uart_sel_switch_init();
    //mode_sel_switch_init();
    //mic_sel_switch_init();	
    //ear_dock_sel_switch_init();	
    cp_ctrl1_switch_init();
#endif

}

int disable_uart_jigbox_wakelock(void *data, u64 clk_idle)
{
	is_uart_wl_active = false;
#if defined(CONFIG_MACH_CAPRI_SS_CRATER)
	uas_jig_force_sleep();
#endif
}

DEFINE_SIMPLE_ATTRIBUTE(set_uart_sleep_fops,
		NULL, disable_uart_jigbox_wakelock, "%llu\n");


static struct dentry *dent_board_template_root_dir;
int __init board_template_debug_init(void)
{
	dent_board_template_root_dir =
		debugfs_create_dir("board_template", 0);

	if (!dent_board_template_root_dir)
		return -ENOMEM;

	if (!debugfs_create_file("allow_uart_sleep",
		S_IWUSR | S_IRUSR,
		dent_board_template_root_dir,
		NULL,
		&set_uart_sleep_fops))
		return -ENOMEM;

	return 0;
}

late_initcall(board_template_debug_init);

static void __init board_reserve(void)
{

	board_common_reserve();

#if defined(CONFIG_BCM_VC_CMA)
	vc_cma_reserve();
#endif

#if defined(CONFIG_BCM_VC_RESERVE_LOWMEM)
	vc_lowmem_reserve();
#endif
}

/*
 * Template used by board-xxx.c to create new board instance
 */
#define CREATE_BOARD_INSTANCE(id,name) \
MACHINE_START(id, name) \
	.map_io = capri_map_io, \
	.init_irq = kona_init_irq, \
	.timer  = &kona_timer, \
	.init_machine = board_init, \
	.reserve = board_reserve, \
MACHINE_END
