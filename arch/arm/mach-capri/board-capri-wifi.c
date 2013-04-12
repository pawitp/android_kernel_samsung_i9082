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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <linux/if.h>
#include <linux/skbuff.h>
#include <linux/wlan_plat.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/fixed.h>
#include <asm/mach/mmc.h>

#include <linux/random.h>
#include <linux/jiffies.h>

#include <mach/rdb/brcm_rdb_sysmap.h>
#include <mach/rdb/brcm_rdb_padctrlreg.h>

#include <mach/sdio_platform.h>
#include <mach/rdb/brcm_rdb_padctrlreg.h>
#include "board-capri-wifi.h"

#ifdef CONFIG_BRCM_UNIFIED_DHD_SUPPORT

#define GPIO_WLAN_PMENA		104
#define GPIO_WLAN_IRQ		2

#define ATAG_MAC	0x57464d41
/* #define ATAG_MAC_DEBUG */

#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM

#define WLAN_STATIC_SCAN_BUF0		5
#define WLAN_STATIC_SCAN_BUF1		6
#define PREALLOC_WLAN_SEC_NUM		4
#define PREALLOC_WLAN_BUF_NUM		160
#define PREALLOC_WLAN_SECTION_HEADER	24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_BUF_NUM * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_BUF_NUM * 1024)

#define DHD_SKB_HDRSIZE			336
#define DHD_SKB_1PAGE_BUFSIZE	((PAGE_SIZE*1)-DHD_SKB_HDRSIZE)
#define DHD_SKB_2PAGE_BUFSIZE	((PAGE_SIZE*2)-DHD_SKB_HDRSIZE)
#define DHD_SKB_4PAGE_BUFSIZE	((PAGE_SIZE*4)-DHD_SKB_HDRSIZE)

#define WLAN_SKB_BUF_NUM	17

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

struct wlan_mem_prealloc {
	void *mem_ptr;
	unsigned long size;
};

static struct wlan_mem_prealloc wlan_mem_array[PREALLOC_WLAN_SEC_NUM] = {
	{NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER)}
};

void *wlan_static_scan_buf0;
void *wlan_static_scan_buf1;

static void *capri_wifi_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_SEC_NUM)
		return wlan_static_skb;
	if (section == WLAN_STATIC_SCAN_BUF0)
		return wlan_static_scan_buf0;
	if (section == WLAN_STATIC_SCAN_BUF1)
		return wlan_static_scan_buf1;
	if ((section < 0) || (section > PREALLOC_WLAN_SEC_NUM))
		return NULL;

	if (wlan_mem_array[section].size < size)
		return NULL;

	return wlan_mem_array[section].mem_ptr;
}

int __init capri_init_wifi_mem(void)
{
	int i;
	int j;

	for (i = 0; i < 8; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_1PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	for (; i < 16; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_2PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_4PAGE_BUFSIZE);
	if (!wlan_static_skb[i])
		goto err_skb_alloc;

	for (i = 0; i < PREALLOC_WLAN_SEC_NUM; i++) {
		wlan_mem_array[i].mem_ptr =
		    kmalloc(wlan_mem_array[i].size, GFP_KERNEL);

		if (!wlan_mem_array[i].mem_ptr)
			goto err_mem_alloc;
	}
	wlan_static_scan_buf0 = kmalloc(65536, GFP_KERNEL);
	if (!wlan_static_scan_buf0)
		goto err_mem_alloc;
	wlan_static_scan_buf1 = kmalloc(65536, GFP_KERNEL);
	if (!wlan_static_scan_buf1)
		goto err_mem_alloc;

	printk(KERN_ERR "%s: WIFI MEM Allocated\n", __func__);
	return 0;

err_mem_alloc:
	pr_err("Failed to mem_alloc for WLAN\n");
	for (j = 0; j < i; j++)
		kfree(wlan_mem_array[j].mem_ptr);

	i = WLAN_SKB_BUF_NUM;

err_skb_alloc:
	pr_err("Failed to skb_alloc for WLAN\n");
	for (j = 0; j < i; j++)
		dev_kfree_skb(wlan_static_skb[j]);

	return -ENOMEM;
}
#endif /* CONFIG_BROADCOM_WIFI_RESERVED_MEM */

extern int bcm_sdiowl_init(int onoff);
extern int bcm_sdiowl_term(void);

int
capri_wifi_status_register(void (*callback) (int card_present, void *dev_id),
			   void *dev_id);

EXPORT_SYMBOL(capri_wifi_status_register);

static int capri_wifi_cd = 0;	/* WIFI virtual 'card detect' status */
static void (*wifi_status_cb) (int card_present, void *dev_id);
static void *wifi_status_cb_devid;

int
capri_wifi_status_register(void (*callback) (int card_present, void *dev_id),
			   void *dev_id)
{
	printk(KERN_ERR " %s ENTRY\n", __func__);

	if (wifi_status_cb) {
		printk(KERN_ERR "%s wifi_status_cb set\n", __func__);
		return -EAGAIN;
	}
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

static unsigned int capri_wifi_status(struct device *dev)
{
	return capri_wifi_cd;
}

struct mmc_platform_data capri_wifi_data = {
	.ocr_mask = MMC_VDD_165_195 | MMC_VDD_20_21,
	.built_in = 1,
	.status = capri_wifi_status,
	.card_present = 0,
	.register_status_notify = capri_wifi_status_register,
};

static int capri_wifi_set_carddetect(int val)
{
	pr_debug("%s: %d\n", __func__, val);
	printk(KERN_ERR "%s: ENTRY val=%d\n", __func__, val);
	capri_wifi_cd = val;

	if (wifi_status_cb) {
		printk(KERN_ERR "%s: CALLBACK NOT NULL\n", __func__);
		wifi_status_cb(val, wifi_status_cb_devid);
		printk(KERN_ERR "%s: CALLBACK COMPLETE\n", __func__);
	} else
		pr_warning("%s: Nobody to notify\n", __func__);

	return 0;
}

static int capri_wifi_power_state;

static int capri_wifi_power(int onoff)
{
	printk(KERN_ERR "%s ENTRY onoff=%d\n", __func__, onoff);

	if (onoff) {
		printk(KERN_ERR "%s calling bcm_sdiowl_init\n", __func__);
		bcm_sdiowl_init(onoff);
	} else {
		printk(KERN_ERR "%s calling bcm_sdiowl_term\n", __func__);
		bcm_sdiowl_term();
	}

	capri_wifi_power_state = onoff;

	return 0;
}

static int capri_wifi_reset_state;

static int capri_wifi_reset(int on)
{
	pr_debug("%s: do nothing\n", __func__);
	printk(KERN_ERR " %s INSIDE capri_wifi_reset\n", __func__);
	capri_wifi_reset_state = on;
	return 0;
}

static unsigned char capri_mac_addr[IFHWADDRLEN] = { 0, 0x90, 0x4c, 0, 0, 0 };

static int __init capri_mac_addr_setup(char *str)
{
	char macstr[IFHWADDRLEN * 3];
	char *macptr = macstr;
	char *token;
	int i = 0;

	if (!str)
		return 0;
	pr_debug("wlan MAC = %s\n", str);
	if (strlen(str) >= sizeof(macstr))
		return 0;
	strcpy(macstr, str);

	while ((token = strsep(&macptr, ":")) != NULL) {
		unsigned long val;
		int res;

		if (i >= IFHWADDRLEN)
			break;
		res = strict_strtoul(token, 0x10, &val);
		if (res < 0)
			return 0;
		capri_mac_addr[i++] = (u8)val;
	}

	return 1;
}

__setup("androidboot.macaddr=", capri_mac_addr_setup);

static int capri_wifi_get_mac_addr(unsigned char *buf)
{
	uint rand_mac;

	if ((capri_mac_addr[4] == 0) && (capri_mac_addr[5] == 0)) {
		srandom32((uint) jiffies);
		rand_mac = random32();
		capri_mac_addr[3] = (unsigned char)rand_mac;
		capri_mac_addr[4] = (unsigned char)(rand_mac >> 8);
		capri_mac_addr[5] = (unsigned char)(rand_mac >> 16);
	}
	memcpy(buf, capri_mac_addr, IFHWADDRLEN);
	return 0;
}

/* Customized Locale table : OPTIONAL feature */
#define WLC_CNTRY_BUF_SZ	4
typedef struct cntry_locales_custom {
	char iso_abbrev[WLC_CNTRY_BUF_SZ];
	char custom_locale[WLC_CNTRY_BUF_SZ];
	int custom_locale_rev;
} cntry_locales_custom_t;

static cntry_locales_custom_t capri_wifi_translate_custom_table[] = {
/* Table should be filled out based on custom platform regulatory requirement */
	{"", "XY", 4},		/* universal */
	{"US", "US", 69},	/* input ISO "US" to : US regrev 69 */
	{"CA", "US", 69},	/* input ISO "CA" to : US regrev 69 */
	{"EU", "EU", 5},	/* European union countries */
	{"AT", "EU", 5},
	{"BE", "EU", 5},
	{"BG", "EU", 5},
	{"CY", "EU", 5},
	{"CZ", "EU", 5},
	{"DK", "EU", 5},
	{"EE", "EU", 5},
	{"FI", "EU", 5},
	{"FR", "EU", 5},
	{"DE", "EU", 5},
	{"GR", "EU", 5},
	{"HU", "EU", 5},
	{"IE", "EU", 5},
	{"IT", "EU", 5},
	{"LV", "EU", 5},
	{"LI", "EU", 5},
	{"LT", "EU", 5},
	{"LU", "EU", 5},
	{"MT", "EU", 5},
	{"NL", "EU", 5},
	{"PL", "EU", 5},
	{"PT", "EU", 5},
	{"RO", "EU", 5},
	{"SK", "EU", 5},
	{"SI", "EU", 5},
	{"ES", "EU", 5},
	{"SE", "EU", 5},
	{"GB", "EU", 5},	/* input ISO "GB" to : EU regrev 05 */
	{"IL", "IL", 0},
	{"CH", "CH", 0},
	{"TR", "TR", 0},
	{"NO", "NO", 0},
	{"KR", "XY", 3},
	{"AU", "XY", 3},
	{"CN", "XY", 3},	/* input ISO "CN" to : XY regrev 03 */
	{"TW", "XY", 3},
	{"AR", "XY", 3},
	{"MX", "XY", 3}
};

static void *capri_wifi_get_country_code(char *ccode)
{
	int size = ARRAY_SIZE(capri_wifi_translate_custom_table);
	int i;

	if (!ccode)
		return NULL;

	for (i = 0; i < size; i++)
		if (strcmp
		    (ccode,
		     capri_wifi_translate_custom_table[i].iso_abbrev) == 0)
			return &capri_wifi_translate_custom_table[i];
	return &capri_wifi_translate_custom_table[0];
}

static struct resource capri_wifi_resources[] = {
	[0] = {
	       .name = "bcmdhd_wlan_irq",
	       .start = gpio_to_irq(102),	//HW_WLAN_GPIO_HOST_WAKE_PIN(GPIO102)
	       .end =  gpio_to_irq(102),	//HW_WLAN_GPIO_HOST_WAKE_PIN(GPIO102)
	       .flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE
	       /* IORESOURCE_IRQ_HIGHLEVEL */  | IORESOURCE_IRQ_SHAREABLE,
	       },
};

static struct wifi_platform_data capri_wifi_control = {
	.set_power = capri_wifi_power,
	.set_reset = capri_wifi_reset,
	.set_carddetect = capri_wifi_set_carddetect,
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	.mem_prealloc = capri_wifi_mem_prealloc,
#endif
	.get_mac_addr = capri_wifi_get_mac_addr,
	.get_country_code = capri_wifi_get_country_code,
};

static struct platform_device capri_wifi_device = {
	.name = "bcmdhd_wlan",
	.id = 2,
	.resource = capri_wifi_resources,
	.num_resources = ARRAY_SIZE(capri_wifi_resources),
	.dev = {
		.platform_data = &capri_wifi_control,
		},
};

static void __init capri_wlan_gpio(void)
{
	pr_debug("%s: start\n", __func__);
#if 1

/* Need to make decision shall we unlock the
 * pad_ctrl and changes the values of pins ?
 */

/* IMPORTANT
 *      PIN_CFG(MMC1DAT0, MMC1DAT0, 0, OFF, ON, 0, 0, 16MA);
 *      PIN_CFG(MMC1DAT1, MMC1DAT1, 0, OFF, ON, 0, 0, 16MA);
 *      PIN_CFG(MMC1DAT2, MMC1DAT2, 0, OFF, ON, 0, 0, 16MA);
 *      PIN_CFG(MMC1DAT3, MMC1DAT3, 0, OFF, ON, 0, 0, 16MA);
 *
 *      PIN_CFG(MMC1CK, MMC1CK, 0, OFF, ON, 0, 0, 16MA);
 *      PIN_CFG(MMC1CMD, MMC1CMD, 0, OFF, ON, 0, 0, 16MA);
 *      PIN_CFG(MMC1RST, GPIO70, 0, OFF, ON, 0, 0, 16MA);
 */
#endif

}

int __init capri_wlan_init(void)
{
	pr_debug("%s: start\n", __func__);
	printk(KERN_ERR " %s Calling GPIO INIT!\n", __func__);

	capri_wlan_gpio();
	printk(KERN_ERR " %s Calling GPIO INIT DONE !\n", __func__);
#ifdef CONFIG_BROADCOM_WIFI_RESERVED_MEM
	capri_init_wifi_mem();
#endif
	printk(KERN_ERR " %s Calling MEM INIT DONE !\n", __func__);

	return platform_device_register(&capri_wifi_device);

}

#endif
