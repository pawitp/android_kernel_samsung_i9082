/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
*	@file	drivers/input/misc/kona_headset_multi_button.c
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

/* This should be defined before kernel.h is included */
/* #define DEBUG */

//#define CONFIG_ACI_COMP_FILTER

/*
* Driver HW resource usage info:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* GPIO MODE
* GPIO  - Accessory insertion/removal detection.
*
* COMP2 - When an accessory is connected, this is used for the acccessory
*         type detection. ie. Headset/Headphone/Open Cable.
*
* COMP1 - If the connected accesory is a headset, configured to detect
*         button press.
*
* NON-GPIO MODE
* COMP2 - Accessory insertion detection.When an accessory is connected,
*         this is used for the acccessory type detection.
*         ie. Headset/Headphone/Open Cable.
*
* COMP2_INV - Used for Accessory removal detection.
*
* COMP1 - If the connected accesory is a headset, configured to detect
*         button press
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif
#include <mach/io_map.h>
#include <mach/kona_headset_pd.h>
#include <chal/chal_util.h>
#include <mach/rdb/brcm_rdb_aci.h>
#include <mach/rdb/brcm_rdb_auxmic.h>
#include <mach/rdb/brcm_rdb_audioh.h>
#include <mach/rdb/brcm_rdb_khub_clk_mgr_reg.h>
#include <linux/wakelock.h>
#ifdef DEBUG
#include <linux/sysfs.h>
#endif
#include <plat/chal/chal_aci.h>
#include <plat/kona_mic_bias.h>
#include <linux/mfd/bcmpmu.h>

#define GPIO_DEBOUNCE_TIME	(16000) /* 16ms */
#define KEY_PRESS_REF_TIME	msecs_to_jiffies(5)
#define KEY_DETECT_DELAY	msecs_to_jiffies(50)
#define KEY_ENABLE_DELAY	(250)
#define FALSE_KEY_AVOID_DELAY (30)
#define ACCESSORY_INSERTION_SETTLE_TIME 	msecs_to_jiffies(300)
#define ACCESSORY_REMOVE_SETTLE_TIME 		msecs_to_jiffies(150)
#define ADC_LOOP_COUNT  (5)
#define ADC_CHECK_OFFSET (3)
#define ADC_READ_RETRY_DELAY	(50)
#define COMP1_THRESHOLD (730)
#define COMP2_THRESHOLD (2100)
#define WAKE_LOCK_TIME				(HZ * 5)	/* 5 sec */
#define WAKE_LOCK_TIME_IN_SENDKEY	(HZ * 2)	/* 2 sec */

/*
* After configuring the ADC, it takes different 'time' for the 
* ADC to settle depending on the HS type. The time outs are 
* in milli seconds
*/
#define DET_PLUG_CONNECTED_SETTLE		120      /* in HAL_ACC_DET_PLUG_CONNECTED */
#define DET_OPEN_CABLE_SETTLE			20      /* in HAL_ACC_DET_OPEN_CABLE */
#define DET_HEADSET_SETTLE				40      /* in HAL_ACC_DET_BASIC_CARKIT */

/*
* Accessory Detecting voltage
*
* Voltage defined in mv 
*/
#define HEADPHONE_DETECT_LEVEL_MIN	0
#ifdef CONFIG_MACH_CAPRI_SS_CRATER
#define HEADPHONE_DETECT_LEVEL_MAX      900
#elif CONFIG_SS_NEW_EARPHONE_SPEC
#define HEADPHONE_DETECT_LEVEL_MAX      720
#else
#define HEADPHONE_DETECT_LEVEL_MAX      750
#endif
#define BASIC_HEADSET_DETECT_LEVEL_MIN  HEADPHONE_DETECT_LEVEL_MAX
#define BASIC_HEADSET_DETECT_LEVEL_MAX  4096
#define OPENCABLE_DETECT_LEVEL_MIN		3000
#define OPENCABLE_DETECT_LEVEL_MAX		5000
#ifdef CONFIG_MACH_CAPRI_SS_CRATER
#define CALL_OPENCABLE_DETECT_LEVEL_MIN	BASIC_HEADSET_DETECT_LEVEL_MAX
#else 
#define CALL_OPENCABLE_DETECT_LEVEL_MIN	2000
#endif
#define CALL_OPENCABLE_DETECT_LEVEL_MAX	5000

enum hs_type {
	DISCONNECTED = 0, /* Nothing is connected  */ 
	HEADPHONE,    /* The one without MIC   */
	OPEN_CABLE,   /* Not sent to userland  */
	HEADSET,	  /* The one with MIC 	   */
	/* If more HS types are required to be added
	* add here, not below HS_TYPE_MAX
	*/
	HS_TYPE_MAX,
};

enum button_name {
	BUTTON_SEND_END = 0,
	BUTTON_VOLUME_UP,
	BUTTON_VOLUME_DOWN,
	BUTTON_NONE,
	BUTTON_NAME_MAX
};

enum button_state {
	BUTTON_RELEASED = 0,
	BUTTON_PRESSED
};

struct mic_t {
	int gpio_irq;
	int comp2_irq;
	int comp2_inv_irq;
	int comp1_irq;
	u32 auxmic_base;
	u32 aci_base;
	int hs_state;
	int hs_detecting;
	int button_state;
	int button_pressed;
	int button_suspend;
	/*
	* 1 - mic bias is ON
	* 0 - mic bias is OFF
	*/
	int mic_bias_status;
	int low_voltage_mode;
	int recheck_jack;
	CHAL_HANDLE aci_chal_hdl;
	struct clk *aci_apb_clk;
	struct clk *audioh_apb_clk;
	struct kona_headset_pd *headset_pd;
#ifdef CONFIG_SWITCH
	struct switch_dev sdev;
#endif
	struct delayed_work accessory_detect_work_deb;
	struct delayed_work accessory_detect_work;
	struct delayed_work accessory_remove_work;
	struct delayed_work button_work_deb;
	struct delayed_work button_work;
	struct input_dev *headset_button_idev;
	struct wake_lock accessory_wklock;
	struct class* audio_class;
	struct device* headset_dev;
	/* 2V9_AUD LDO */
	struct regulator *ldo;
};

static struct mic_t *mic_dev = NULL;

/*
* Default table used if the platform does not pass one
*/ 
static unsigned int button_adc_values_no_resistor [3][2] = 
{
	/* SEND/END Min, Max*/
	{0,     95},
	/* Volume Up  Min, Max*/
	{96,    195},
	/* Volue Down Min, Max*/
	{196,   500},
};

#ifdef CONFIG_ACI_COMP_FILTER
/* * Button/Hook Filter configuration */
/* = 1024 / (Filter block frequencey) = 1024 / 32768 => 31ms */
#define ACC_HW_COMP1_FILTER_WIDTH   1024
#endif

/* Accessory Hardware configuration support variables */
/* ADC */
static const CHAL_ACI_filter_config_adc_t aci_filter_adc_config = { 0, 0x0B };

#ifdef CONFIG_ACI_COMP_FILTER
static const CHAL_ACI_filter_config_comp_t comp_values_for_button_press = {
	CHAL_ACI_FILTER_MODE_INTEGRATE,
	CHAL_ACI_FILTER_RESET_FIRMWARE,
	0,			/* = S */
	0xFE,			/* = T */
	0x500,			/* = M = 1280 / 32768 => 39ms */
	ACC_HW_COMP1_FILTER_WIDTH	/* = MT */
};

/* COMP2 */
static const CHAL_ACI_filter_config_comp_t comp_values_for_type_det = {
	CHAL_ACI_FILTER_MODE_INTEGRATE,
	CHAL_ACI_FILTER_RESET_FIRMWARE,
	0,			/* = S  */
	0xFE,			/* = T  */
	0x700,			/* = M  */
	0x650			/* = MT */
};
#endif

/* MIC bias */
static CHAL_ACI_micbias_config_t aci_mic_bias = {
	CHAL_ACI_MIC_BIAS_OFF,
	CHAL_ACI_MIC_BIAS_2_1V,
	CHAL_ACI_MIC_BIAS_PRB_CYC_256MS,
	CHAL_ACI_MIC_BIAS_MSR_DLY_4MS,
	CHAL_ACI_MIC_BIAS_MSR_INTVL_64MS,
	CHAL_ACI_MIC_BIAS_1_MEASUREMENT
};

/*
* Based on the power consumptio analysis. Program the MIC BIAS probe cycle to be
* 512ms. In this the mesaurement interval is 64ms and the measurement delay
* is 16ms. This means that the duty cycle is approximately 15.6% 
*
* Mic Bias Power down control  and peridoic measurement control looks like
* this

*  --                      -------------------------------
*    | <-------------->   | <---------------------------> |
*    |     64 ms          |    512-64= 448ms              |
*     --------------------
*            -------------
*    <----->| <---------> |
*      16ms |   48ms      |
* ----------               ---------------------
* 
*/ 
static CHAL_ACI_micbias_config_t aci_init_mic_bias = {
	CHAL_ACI_MIC_BIAS_ON,
	CHAL_ACI_MIC_BIAS_2_1V,
	CHAL_ACI_MIC_BIAS_PRB_CYC_512MS,
	CHAL_ACI_MIC_BIAS_MSR_DLY_16MS,
	CHAL_ACI_MIC_BIAS_MSR_INTVL_64MS,
	CHAL_ACI_MIC_BIAS_1_MEASUREMENT
};

/* Vref */
static CHAL_ACI_vref_config_t aci_vref_config = {CHAL_ACI_VREF_OFF};

static int __headset_hw_init_micbias_on(struct mic_t *p);
static int __headset_hw_init_micbias_off(struct mic_t *p);
static int __headset_hw_init (struct mic_t *mic);
static void __handle_accessory_inserted (struct mic_t *p);
static void __handle_accessory_removed (struct mic_t *p);
static irqreturn_t gpio_isr(int irq, void *dev_id);

static ssize_t show_headset(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t store_headset(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count);

#define HEADSET_ATTR(_name)													\
{																				\
	.attr = { .name = #_name, .mode = 0644, },					\
	.show = show_headset,														\
}

#define HEADSET_ATTRW(_name)													\
{																				\
	.attr = { .name = #_name, .mode = 0664, },					\
	.store = store_headset,                                \
}

enum {
	STATE = 0,
	KEY_STATE,
	RECHECK,
};

static struct device_attribute headset_Attrs[] = {
	HEADSET_ATTR(state),
	HEADSET_ATTR(key_state),
	HEADSET_ATTRW(reselect_jack),
};

static ssize_t show_headset(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	const ptrdiff_t off = attr - headset_Attrs;

	switch (off)
	{
	case STATE:
		ret = scnprintf(buf, PAGE_SIZE, "%d\n", (((mic_dev->hs_state == HEADPHONE) || (mic_dev->hs_state == HEADSET)) ? 1 : 0));
		break;
	case KEY_STATE:
		ret = scnprintf(buf, PAGE_SIZE, "%d\n", mic_dev->button_state);
		break;
	default :
		break;
	}

	return ret;
}

static ssize_t store_headset(struct device *pdev, struct device_attribute *attr, const char *buf, size_t count)
{
	const ptrdiff_t off = attr - headset_Attrs;

	switch(off)
	{
	case RECHECK:
		{
#ifndef CONFIG_MACH_CAPRI_SS_CRATER			
			int recheck = 0;
			sscanf(buf, "%d\n", &recheck);
			if(recheck)
			{
				pr_info("%s: recheck headset\n", __func__);
				mic_dev->recheck_jack = 1;
				gpio_isr(0, mic_dev);
			}
#endif			
		}
		break;
	}

	return count;
}

/* Function to dump the HW regs */
static void dump_hw_regs (struct mic_t *p)
{
	int i;

	pr_info("Dumping MIC BIAS registers\r\n");
	for (i=0x0; i <=0x28; i+= 0x04){
		pr_info("Addr: 0x%x  OFFSET: 0x%x  Value:0x%x \r\n",
			p->auxmic_base + i, i, readl(p->auxmic_base + i));
	}

	pr_info("\n");
	pr_info("Dumping ACI registers \r\n");
	for (i=0x30; i <=0xD8; i+= 0x04){
		pr_info("Addr: 0x%x  OFFSET: 0x%x  Value:0x%x \r\n",
			p->aci_base + i, i, readl(p->aci_base + i));
	}

	for (i=0x400; i <=0x420; i+= 0x04){
		pr_info("Addr: 0x%x  OFFSET: 0x%x Value:0x%x \r\n",
			p->aci_base + i, i, readl(p->aci_base + i));
	}
	pr_info("\n");
}

/* Low level functions called to detect the accessory type */
static int config_adc_for_accessory_detection(int hst)
{
	int time_to_settle = 0;

	if (mic_dev == NULL) {
		pr_err("%s():Invalid mic_dev handle \r\n", __func__ );
		return  -EFAULT;
	}

	if (mic_dev->aci_chal_hdl == NULL) {
		pr_err("%s():Invalid CHAL handle \r\n", __func__);
		return -EFAULT;
	}

	switch (hst) {
	case HEADPHONE:

		/* Setup MIC bias */
		aci_mic_bias.mode = CHAL_ACI_MIC_BIAS_ON;
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
			CHAL_ACI_BLOCK_GENERIC, &aci_mic_bias);

		/* Power up Digital block */
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_ENABLE,
			CHAL_ACI_BLOCK_DIGITAL);

		/* Power up ADC */
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_DISABLE,
			CHAL_ACI_BLOCK_ADC);
		usleep_range(1000, 1200);
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_ENABLE,
			CHAL_ACI_BLOCK_ADC);
		usleep_range(1000, 1200);

		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_ADC_RANGE,
			CHAL_ACI_BLOCK_ADC,
			CHAL_ACI_BLOCK_ADC_HIGH_VOLTAGE);

		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_CONFIGURE_FILTER,
			CHAL_ACI_BLOCK_ADC, &aci_filter_adc_config);

		time_to_settle = DET_PLUG_CONNECTED_SETTLE;
		break;

	case OPEN_CABLE:
#if	0
		/* Powerup ADC */
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_DISABLE,
			CHAL_ACI_BLOCK_ADC);
		usleep_range(1000, 1200);
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_ENABLE,
			CHAL_ACI_BLOCK_ADC);
		usleep_range(1000, 1200);

		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_ADC_RANGE,
			CHAL_ACI_BLOCK_ADC,
			CHAL_ACI_BLOCK_ADC_HIGH_VOLTAGE);

		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_CONFIGURE_FILTER,
			CHAL_ACI_BLOCK_ADC, &aci_filter_adc_config);
#endif
		time_to_settle = DET_OPEN_CABLE_SETTLE;
		break;

	case HEADSET:
#if 0
		/* Power up ADC */
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_DISABLE,
			CHAL_ACI_BLOCK_ADC);
		usleep_range(1000, 1200);
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_ENABLE,
			CHAL_ACI_BLOCK_ADC);
		usleep_range(1000, 1200);

		chal_aci_block_ctrl(mic_dev->aci_chal_hdl, CHAL_ACI_BLOCK_ACTION_ADC_RANGE,
			CHAL_ACI_BLOCK_ADC, CHAL_ACI_BLOCK_ADC_LOW_VOLTAGE);

		__headset_hw_init_micbias_off(mic_dev);

		/* Turn OFF MIC Bias */
		aci_mic_bias.mode = CHAL_ACI_MIC_BIAS_GND;
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
			CHAL_ACI_BLOCK_GENERIC, &aci_mic_bias);

		chal_aci_block_ctrl(mic_dev->aci_chal_hdl, 
			CHAL_ACI_BLOCK_ACTION_MIC_POWERDOWN_HIZ_IMPEDANCE,
			CHAL_ACI_BLOCK_GENERIC, 0);
#endif

		time_to_settle = DET_HEADSET_SETTLE;
		break;

	default:
		/* TODO: Logically what does this mean ???? */
		break;
	}

	/* 
	* Wait till the ADC settles, the timings varies for different types
	* of headset. 
	*/
	/* TODO:
	* There is a settling time after which a call back is
	* invoked, does this call back trigger any other HW config or
	* its just a notification to the upper layer ????
	*/
	if (time_to_settle != 0)
		msleep(time_to_settle);

	return 0;
}

static int config_adc_for_bp_detection(void)
{
	if (mic_dev == NULL) {
		pr_err("%s(): invalid mic_dev handle \r\n", __func__);
		return  -EFAULT;
	}

	if (mic_dev->aci_chal_hdl == NULL) {
		pr_err("%s(): Invalid CHAL handle \r\n", __func__);
		return -EFAULT;
	}
	/* Set the threshold value for button press */
	/*
	* Got the feedback from the system design team that the button press
	* threshold to programmed should be 0.12V as well.
	* With this only send/end button works, so made this 600 i.e 0.6V.
	* With the threshold level set to 0.6V all the 3 button press works
	* OK.Note that this would trigger continus COMP1 Interrupts if enabled.
	* But since we don't enable COMP1 until we identify a Headset its OK.
	*/
	chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
		CHAL_ACI_BLOCK_COMP1, COMP1_THRESHOLD);

	/* Set the threshold value for accessory type detection */
	chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
		CHAL_ACI_BLOCK_COMP2, COMP2_THRESHOLD);

	/* 
	* TODO: As of now this function uses the same MIC BIAS settings
	* and filter settings as accessory detection, this might need
	* fine tuning. 
	*/

	/* Setup MIC bias */
	aci_mic_bias.mode = CHAL_ACI_MIC_BIAS_ON;
	chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
		CHAL_ACI_BLOCK_GENERIC, &aci_mic_bias);

	/* Power up Digital block */
	chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_ENABLE, 
		CHAL_ACI_BLOCK_DIGITAL);

	/* Power up the ADC */
	chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_DISABLE,
		CHAL_ACI_BLOCK_ADC);
	usleep_range(1000, 1200);
	chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_ENABLE,
		CHAL_ACI_BLOCK_ADC);
	usleep_range(1000, 1200);

	chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_ADC_RANGE, CHAL_ACI_BLOCK_ADC,
		CHAL_ACI_BLOCK_ADC_LOW_VOLTAGE);

	chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_CONFIGURE_FILTER,
		CHAL_ACI_BLOCK_ADC, &aci_filter_adc_config);

	BRCM_WRITE_REG( KONA_ACI_VA, ACI_ADC_CTRL, 0xD2);

	// comp1 debounce 20ms
	BRCM_WRITE_REG(KONA_ACI_VA, ACI_M1, 0x28E);

	/* 
	* Wait till the ADC settles, the timings might need fine tuning
	*/
	msleep(DET_HEADSET_SETTLE);

	return 0;
}

static int get_adc_data(struct mic_t *mic_dev)
{
	int adc_data;
	int adc_max = 0;
	int adc_min = 0xFFFF;
	int adc_total = 0;
	int adc_retry_cnt = 0;
	int i;

	for (i = 0; i < ADC_LOOP_COUNT; i++) {

		adc_data = chal_aci_block_read(mic_dev->aci_chal_hdl,
					CHAL_ACI_BLOCK_ADC,
					CHAL_ACI_BLOCK_ADC_RAW);

		if (adc_data < 0) {
			/* ADC overflow: -1 */
			adc_retry_cnt++;

			if (adc_retry_cnt > 10)
				return adc_data;
		}

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}
		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (ADC_LOOP_COUNT - 2);
}

static int read_adc_for_accessory_detection(int hst)
{
	int status = DISCONNECTED;
	int mic_level, i;
	int level_sum = 0;
	int pre_level = 0xFFFF;
	int retry_cnt = 50; // 1 sec

	if (mic_dev == NULL) {
		pr_err("aci_adc_config: invalid mic_dev handle \r\n");
		return  -EFAULT;
	}

	if (mic_dev->aci_chal_hdl == NULL) {
		pr_err("aci_adc_config: Invalid CHAL handle \r\n");
		return -EFAULT;
	}

	wake_lock(&mic_dev->accessory_wklock);
	while((gpio_get_value(irq_to_gpio(mic_dev->gpio_irq)) ^ mic_dev->headset_pd->hs_default_state)
		&& retry_cnt && !mic_dev->recheck_jack)
	{
		mic_level = get_adc_data(mic_dev);
		if(mic_level > 550 || mic_level < 30)
			break;

		retry_cnt--;
		msleep(20);
	}

	for(i = 0; i < 3;)
	{
		mic_level = get_adc_data(mic_dev);
		
#ifdef CONFIG_MACH_CAPRI_SS_CRATER	
		if (mic_level == -1 && level_sum == 0)
		{
			// exception for overflow from larger input than ADC range.
			printk(" ++ ADC overflow from read_adc_data\r\n");
			mic_level = BASIC_HEADSET_DETECT_LEVEL_MAX - 12;
		}
#endif

		if(unlikely(mic_level < 0))
		{
			/* ADC overflow: -1 */
			__headset_hw_init_micbias_off(mic_dev);
			__headset_hw_init_micbias_on(mic_dev);
			config_adc_for_accessory_detection(hst);
			continue;
		}
		
		if(mic_level >= (pre_level - ADC_CHECK_OFFSET) && mic_level <= (pre_level + ADC_CHECK_OFFSET))
		{
			level_sum += mic_level;
			i++;
		}
		else
		{
			pre_level = mic_level;
			i = 0;
			level_sum = 0;
		}
		
		if(mic_level > HEADPHONE_DETECT_LEVEL_MAX)
		{
			level_sum = mic_level;
			i = 1;
			break;
		}

		msleep(50);
	}
	
	mic_level = level_sum / i;
	wake_unlock(&mic_dev->accessory_wklock);
	wake_lock_timeout(&mic_dev->accessory_wklock, WAKE_LOCK_TIME_IN_SENDKEY);

	/*
	* What is phone_ref_offset?
	*
	* Because of the resistor in the MIC IN line the actual ground is not 0,
	* but a small offset is added to it. We call this as
	* phone_ref_offset.
	* This needs to be subtracted from the measured voltage to determine
	* the correct value. This will vary for different HW based on the
	* resistor values used. So by default this variable is 0, if no one
	* initializes it. For boards on which this resistor is present this
	* value should be passed from the board specific data structure
	*
	* In the below logic, if mic_level read is less than or equal to 0
	* then we don't do anything.
	* If the read value is greater than  phone_ref_offset then subtract this offset
	* from the value read, otherwise mic_level is zero
	*/

		pr_debug
			(" ++ read_adc_for_accessory_detection:"
			" mic_level before calc %d \r\n",
			mic_level);
		mic_level =
			mic_level <=
			0 ? mic_level : ((mic_level > mic_dev->headset_pd->phone_ref_offset)
			? (mic_level -
			mic_dev->headset_pd->phone_ref_offset) : 0);
		pr_debug
			(" ++ read_adc_for_accessory_detection:"
			" mic_level after calc %d \r\n",
			mic_level);

		switch (hst) {
		case HEADPHONE:
			if ( mic_level >= HEADPHONE_DETECT_LEVEL_MIN &&
				mic_level <= HEADPHONE_DETECT_LEVEL_MAX)
			{
				status = HEADPHONE;		
			}	
			else if(mic_level > BASIC_HEADSET_DETECT_LEVEL_MIN &&
				mic_level <= BASIC_HEADSET_DETECT_LEVEL_MAX)
			{
					if(mic_level >= CALL_OPENCABLE_DETECT_LEVEL_MIN &&
						!((gpio_get_value(irq_to_gpio(mic_dev->gpio_irq))) ^ mic_dev->headset_pd->hs_default_state))
						status = OPEN_CABLE;
					else 
						status = HEADSET;
			}

			if(mic_dev->recheck_jack == 1)
			{
				if (mic_level > CALL_OPENCABLE_DETECT_LEVEL_MIN && 
					mic_level <= CALL_OPENCABLE_DETECT_LEVEL_MAX)
					status = OPEN_CABLE;

			mic_dev->recheck_jack = 0;
			}

			break;
#if 0			
		case OPEN_CABLE:
			{
				if(mic_dev->recheck_jack == 1)
				{
					if (mic_level > CALL_OPENCABLE_DETECT_LEVEL_MIN && 
						mic_level <= CALL_OPENCABLE_DETECT_LEVEL_MAX)
						status = OPEN_CABLE;
					else
						status = HEADSET;

					mic_dev->recheck_jack = 0;
				}
				else
				{
					if(mic_level >= BASIC_HEADSET_DETECT_LEVEL_MIN && 
						mic_level <= BASIC_HEADSET_DETECT_LEVEL_MAX)
						status = HEADSET;
					else if (mic_level > OPENCABLE_DETECT_LEVEL_MIN && 
						mic_level <= OPENCABLE_DETECT_LEVEL_MAX)
						status = OPEN_CABLE;
				}
			}
			break;
		case HEADSET:
			if (mic_level >= BASIC_HEADSET_DETECT_LEVEL_MIN && 
				mic_level <= BASIC_HEADSET_DETECT_LEVEL_MAX)
				status = HEADSET;
			break;
#endif			
		default:
			/* TODO: Logically what does this mean ???? */
			break;
		}

	pr_info("status is %d mic_level is %d\n", status, mic_level);
	return status;
}

/* Function that is invoked when the user tries to read from sysfs interface */
static int detect_hs_type(struct mic_t *mic_dev)
{
#if 0
	int i;
#endif
	int type = DISCONNECTED;

	if (mic_dev == NULL) {
		pr_err("mic_dev is empty \r\n");
		return 0;
	}

#if 0
	/* 
	* Configure the ACI block of ADC for detecting a particular type of
	* accessory
	*/
	for (i=HEADPHONE;i<HEADSET;i++) {

		/* Configure the ADC to check a given HS type */
		config_adc_for_accessory_detection(i);

		/* 
		* Now, read back to check whether the given accessory is
		* present. If yes, then break the loop. If not, continue
		*/
		type = read_adc_for_accessory_detection(i);
		if(type != DISCONNECTED)
			break;
	} /* end of loop to check the devices */
#else
	config_adc_for_accessory_detection(HEADPHONE);
	type = read_adc_for_accessory_detection(HEADPHONE);
#endif

	if(type == DISCONNECTED)
		return OPEN_CABLE;
	else
		return type;
}

static int detect_button_pressed (struct mic_t *mic_dev)
{
	int i;
	int mic_level;
	int button = BUTTON_NONE;

	if (mic_dev == NULL) {
		pr_err("mic_dev is empty \r\n");
		return 0;
	}

	/*
	* What is phone_ref_offset?
	*
	* Because of the resistor in the MIC IN line the actual ground is not 0,
	* but a small offset is added to it. We call this as
	* phone_ref_offset.
	* This needs to be subtracted from the measured voltage to determine
	* the correct value. This will vary for different HW based on the
	* resistor values used. So by default this variable is 0, if no one
	* initializes it. For boards on which this resistor is present this
	* value should be passed from the board specific data structure
	*
	* In the below logic, if mic_level read is less than or equal to 0
	* then we don't do anything.
	* If the read value is greater than  phone_ref_offset then subtract this offset
	* from the value read, otherwise mic_level is zero
	*/
/*	mic_level = chal_aci_block_read(mic_dev->aci_chal_hdl,
		CHAL_ACI_BLOCK_ADC, CHAL_ACI_BLOCK_ADC_RAW);*/

	mic_level = get_adc_data(mic_dev);

	mic_level = mic_level <= 0 ? mic_level :
		((mic_level > mic_dev->headset_pd->phone_ref_offset) ?
		(mic_level - mic_dev->headset_pd->phone_ref_offset) : 0);
	pr_info("cal mic_level = %d\n", mic_level);

	/* Find out what is the button pressed */

	/* Take the table based on what is passed from the board */
	for (i=BUTTON_SEND_END;i<BUTTON_NAME_MAX;i++) {
		if((mic_level >= mic_dev->headset_pd->button_adc_values[i][0]) &
			(mic_level <= mic_dev->headset_pd->button_adc_values[i][1]))
		{
			button = i;
			break;
		}
	}

	return button; 
}

static int false_button_check(struct mic_t *p)
{	
	if (p->hs_state != HEADSET) {
		pr_err("%s: Acessory is not Headset\n", __func__);
		return 1;
	}

	if(p->hs_detecting)
	{
		pr_err("%s: Acessory is detecting\n", __func__);
		return 1;
	}

	return 0;
}

static void button_work_deb_func(struct work_struct *work)
{
	struct mic_t *p = container_of(work, struct mic_t,
		button_work_deb.work);
	cancel_delayed_work_sync(&(p->button_work));
	schedule_delayed_work(&(p->button_work), KEY_DETECT_DELAY);
}

static void button_work_func(struct work_struct *work)
{
	struct mic_t *p = container_of(work, struct mic_t,
		button_work.work);

	wake_lock_timeout(&p->accessory_wklock, WAKE_LOCK_TIME_IN_SENDKEY);

	if(false_button_check(p))
		goto out;

	if (readl(p->aci_base + ACI_COMP_DOUT_OFFSET) & ACI_COMP_DOUT_COMP1_DOUT_CMD_ONE)
	{
		if (p->button_state != BUTTON_PRESSED) {
			int err = 0;
			int button_name, button_name2;
			int loopcnt = 5;

			do
			{
				/* Find out the type of button pressed by reading the ADC values */
				button_name = detect_button_pressed(p);
				usleep_range(10000, 12000);
				if(false_button_check(p))
					goto out;
				button_name2 = detect_button_pressed(p);
				loopcnt--;
			}while(button_name != button_name2 && loopcnt);

			msleep(FALSE_KEY_AVOID_DELAY);
				if(false_button_check(p))
					goto out;

			if(loopcnt == 0)
				button_name = BUTTON_NONE;

			/* 
			* Store which button is being pressed (KEY_VOLUMEUP, KEY_VOLUMEDOWN, KEY_SEND) 
			* in the context structure 
			*/
			switch (button_name) {
			case BUTTON_SEND_END:
				p->button_pressed = KEY_SEND;
				break;
			case BUTTON_VOLUME_UP:
				p->button_pressed = KEY_VOLUMEUP;
				break;
			case BUTTON_VOLUME_DOWN:
				p->button_pressed = KEY_VOLUMEDOWN;
				break;
			default:
				pr_err("Button type not supported\n");
				err = 1;
				break;
			}

			if (err)
				goto out;

			/* Notify the same to input sub-system */
			p->button_state = BUTTON_PRESSED;
			pr_info("Button pressed=%d\n", button_name);
			input_report_key(p->headset_button_idev, p->button_pressed, 1);
			input_sync(p->headset_button_idev);
		}

		/* 
		* Seperate interrupt will not be fired for button release,
		* Need to periodically poll to see if the button is releaesd.
		* The content of the COMP DOUT register would tell us whether
		* the button is released or not.
		*/
		schedule_delayed_work(&(p->button_work), KEY_PRESS_REF_TIME);
	} else {
		/* 
		* Find out which button is being pressed from the context structure 
		* Notify the corresponding release event to the input sub-system
		*/

		if(p->button_state == BUTTON_PRESSED)
		{
			p->button_state = BUTTON_RELEASED;
			pr_info("Button release=%d\n", p->button_pressed);
			input_report_key(p->headset_button_idev, p->button_pressed, 0);
			input_sync(p->headset_button_idev);
		}

		if(p->hs_state == HEADSET)
		{
			/* Acknowledge & clear the interrupt */
			chal_aci_block_ctrl(p->aci_chal_hdl,
				CHAL_ACI_BLOCK_ACTION_INTERRUPT_ACKNOWLEDGE,
				CHAL_ACI_BLOCK_COMP1);

			/* Re-enable COMP1 Interrupts */
			chal_aci_block_ctrl(p->aci_chal_hdl,   
				CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
				CHAL_ACI_BLOCK_COMP1);
		}   
	}   

	return ;

out:

	if(p->button_state == BUTTON_PRESSED)
	{
		p->button_state = BUTTON_RELEASED;
		pr_info("False Button release=%d\n", p->button_pressed);
		input_report_key(p->headset_button_idev, p->button_pressed, 0);
		input_sync(p->headset_button_idev);
	}

	if(p->hs_state == HEADSET)
	{
		/* Acknowledge & clear the interrupt */
		chal_aci_block_ctrl(p->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_INTERRUPT_ACKNOWLEDGE,
			CHAL_ACI_BLOCK_COMP1);

		chal_aci_block_ctrl(p->aci_chal_hdl,   
			CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
			CHAL_ACI_BLOCK_COMP1);
	}
}

static void accessory_detect_work_deb_func(struct work_struct *work)
{
	struct mic_t *p = container_of(work, struct mic_t,
		accessory_detect_work_deb.work);
	int work_delay = ACCESSORY_INSERTION_SETTLE_TIME;

	if(p->hs_state != DISCONNECTED)
		work_delay = ACCESSORY_REMOVE_SETTLE_TIME;

	cancel_delayed_work_sync(&(p->accessory_detect_work));
	schedule_delayed_work(&(p->accessory_detect_work), work_delay);
	p->hs_detecting = 1;
}

/*------------------------------------------------------------------------------
Function name   : accessory_detect_work_func
Description     : Work function that will set the state of the headset
switch dev and enable/disable the HS button interrupt
Return type     : void
------------------------------------------------------------------------------*/
static void accessory_detect_work_func(struct work_struct *work)
{
	struct mic_t *p = container_of(work, struct mic_t,
		accessory_detect_work.work);
	unsigned int accessory_inserted = ((gpio_get_value(irq_to_gpio(p->gpio_irq))) ^ p->headset_pd->hs_default_state);
	pr_info("detect GPIO %d\n", accessory_inserted);

	if (accessory_inserted == 1) {

		/* Switch ON the MIC BIAS */
		__headset_hw_init_micbias_on(p);   
		__handle_accessory_inserted(p);

		/* 
		* Enable the COMP1 interrupt for button press detection in
		* case if the inserted item is Headset
		*/
		if(p->hs_state == HEADSET)
		{
			msleep(KEY_ENABLE_DELAY);
			
			chal_aci_block_ctrl(p->aci_chal_hdl,
				CHAL_ACI_BLOCK_ACTION_INTERRUPT_ACKNOWLEDGE,
				CHAL_ACI_BLOCK_COMP);

			chal_aci_block_ctrl(p->aci_chal_hdl,	
				CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
				CHAL_ACI_BLOCK_COMP1);

			msleep(KEY_ENABLE_DELAY);
		}
		else
			__headset_hw_init_micbias_off(p);
	}
	else
	{
		int state = p->hs_state;
		__handle_accessory_removed(p);

		if(state == HEADSET)
		{
			int msleeptime = 400;
			clk_enable(mic_dev->audioh_apb_clk);
			if(!(readl(KONA_AUDIOH_VA + AUDIOH_AUDIORX_VRX1_OFFSET) & 0x1))	//using mic
				msleeptime = 1000;
			clk_disable(mic_dev->audioh_apb_clk);

			/* First inform userland about the removal.
			* A delay is necessary here to allow the
			* application to complete the routing*/
			pr_info("Micbias off delay %dms\n", msleeptime);

			msleep(msleeptime);
		}

		__headset_hw_init_micbias_off(p);
	}

	p->hs_detecting = 0;
}

static void no_gpio_accessory_insert_work_func(struct work_struct *work)
{
	struct mic_t *p = container_of(work, struct mic_t,
		accessory_detect_work.work);
	int comp2_level;
	int pre_comp2_level = CHAL_ACI_BLOCK_COMP_LINE_LOW;
	int loopcnt = ADC_LOOP_COUNT;   

	aci_init_mic_bias.mode    = CHAL_ACI_MIC_BIAS_ON;
	aci_init_mic_bias.voltage = CHAL_ACI_MIC_BIAS_2_1V;

	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
		CHAL_ACI_BLOCK_GENERIC, &aci_init_mic_bias);

	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
		CHAL_ACI_BLOCK_COMP2,
		COMP2_THRESHOLD);

	do
	{
		/* 
		* Read COMP2 level, if accessory is plugged in low level is
		* expected
		*/
		comp2_level = chal_aci_block_read(p->aci_chal_hdl,
			CHAL_ACI_BLOCK_COMP2,
			CHAL_ACI_BLOCK_COMP_RAW);

		loopcnt = (pre_comp2_level == comp2_level ? loopcnt - 1 : ADC_LOOP_COUNT);
		pre_comp2_level = comp2_level;
		msleep(20);
	}while(loopcnt != 0);

	if (comp2_level == CHAL_ACI_BLOCK_COMP_LINE_LOW) {

		__handle_accessory_inserted(p);

		aci_init_mic_bias.mode    = CHAL_ACI_MIC_BIAS_ON;
		aci_init_mic_bias.voltage = CHAL_ACI_MIC_BIAS_2_1V;

		chal_aci_block_ctrl(p->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
			CHAL_ACI_BLOCK_GENERIC, &aci_init_mic_bias);

		chal_aci_block_ctrl(p->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
			CHAL_ACI_BLOCK_COMP2,
			COMP2_THRESHOLD);

		if(p->hs_state == OPEN_CABLE)
		{
			chal_aci_block_ctrl(p->aci_chal_hdl,   
				CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
				CHAL_ACI_BLOCK_COMP2);

			return ;
		}

		/*
		* Enable COMP2_INV ISR for accessory removal 
		* and in case if the connected accessory is Headset enable
		* COMP1 ISR for button press
		*/
		chal_aci_block_ctrl(p->aci_chal_hdl,   
			CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
			CHAL_ACI_BLOCK_COMP2_INV);
		/*
		* IMPORTANT
		* ---------
		*
		* This is done to handle spurious button press interrupt.
		* Note that when the COMP2 is used for accessory insertion
		* detection. Soon after the accessory is inserted I see two
		* spurious interrupts, one for COMP2_INV and one for COMP1
		*
		* COMP1 spurious handling
		* ------------------------
		* Luckily what I observed is that the COMP1 (button press)
		* interrupt happens earlier than the COMP2 INV. So if we just
		* delay enabling the COMP1 interrupt by 100ms, then we can
		* avoid this spurious interrupt. Also since we are
		* Acknowledging all the COMP interrupts in the below code,
		* the COMP1 interrupt that had happened would get cleared
		* below. Note that this is just a workaround and will live
		* here until we figure out the root cause for this spurious
		* interrupt. There is no side effect of  this delay because
		* the notification of accessory insertion has already been
		* passed on to the  Switch class layer. If some one presses a
		* button just after insertion within 100ms delay then we'll
		* miss the button press, but its not practical for a user to
		* press a button within 100ms of accessory insertion.
		*
		* COMP2 INV spurious handling
		* ---------------------------
		* We have not done anything special here because even though
		* the COMP2 INV work gets scheduled, since there is a level
		* check happenning in the Work queue, it promptly identifies
		* this as a spurious condition.
		*/
		msleep(KEY_ENABLE_DELAY);

		if(p->hs_state == HEADSET) {
			chal_aci_block_ctrl(p->aci_chal_hdl,	
				CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
				CHAL_ACI_BLOCK_COMP1);
		}
	}
	else {
		pr_err("ACCESSORY INSERT FALSE\n");

		/* Just re-enable the COMP2 ISR for further detection */
		if(p->hs_state == DISCONNECTED)
		{
			chal_aci_block_ctrl(p->aci_chal_hdl,	
				CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
				CHAL_ACI_BLOCK_COMP2);
		}
		else
		{
			chal_aci_block_ctrl(p->aci_chal_hdl,   
				CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
				CHAL_ACI_BLOCK_COMP2_INV);
		}
	}
}

static void no_gpio_accessory_remove_work_func(struct work_struct *work)
{
	struct mic_t *p = container_of(work, struct mic_t,
		accessory_remove_work.work);
	int comp2_level;
	int pre_comp2_level = CHAL_ACI_BLOCK_COMP_LINE_HIGH;
	int loopcnt = ADC_LOOP_COUNT;

	aci_init_mic_bias.mode    = CHAL_ACI_MIC_BIAS_ON;
	aci_init_mic_bias.voltage = CHAL_ACI_MIC_BIAS_2_1V;

	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
		CHAL_ACI_BLOCK_GENERIC, &aci_init_mic_bias);

	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
		CHAL_ACI_BLOCK_COMP2,
		COMP2_THRESHOLD);

	do
	{
		/* 
		* Read COMP2 level, if accessory is plugged out high level is
		* expected
		*/
		comp2_level = chal_aci_block_read(p->aci_chal_hdl,
			CHAL_ACI_BLOCK_COMP2,
			CHAL_ACI_BLOCK_COMP_RAW);

		loopcnt = (pre_comp2_level == comp2_level ? loopcnt - 1 : ADC_LOOP_COUNT);
		pre_comp2_level = comp2_level;
		msleep(20);
	}while(loopcnt != 0);

	if (comp2_level == CHAL_ACI_BLOCK_COMP_LINE_HIGH) {

		__handle_accessory_removed(p);

		aci_init_mic_bias.mode    = CHAL_ACI_MIC_BIAS_ON;
		aci_init_mic_bias.voltage = CHAL_ACI_MIC_BIAS_2_1V;

		chal_aci_block_ctrl(p->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
			CHAL_ACI_BLOCK_GENERIC, &aci_init_mic_bias);

		chal_aci_block_ctrl(p->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
			CHAL_ACI_BLOCK_COMP2,
			COMP2_THRESHOLD);

		/* Just keep the COMP2 enabled for next accessory detection. */
		chal_aci_block_ctrl(p->aci_chal_hdl,	
			CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
			CHAL_ACI_BLOCK_COMP2);

		chal_aci_block_ctrl(p->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_INTERRUPT_ACKNOWLEDGE,
			CHAL_ACI_BLOCK_COMP);
	} else {
		pr_err("ACCESSORY REMOVE FALSE\n");
		/* Renable the COMP2 INV for accessory removal */

		if(p->hs_state == DISCONNECTED)
		{
			chal_aci_block_ctrl(p->aci_chal_hdl,	
				CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
				CHAL_ACI_BLOCK_COMP2);
		}
		else
		{
			chal_aci_block_ctrl(p->aci_chal_hdl,	
				CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
				CHAL_ACI_BLOCK_COMP2_INV);

			if(p->hs_state == HEADSET) {
				msleep(KEY_ENABLE_DELAY);            
				chal_aci_block_ctrl(p->aci_chal_hdl,	
					CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
					CHAL_ACI_BLOCK_COMP1);
			}

			return ;
		}
	}

}

static void __handle_accessory_inserted (struct mic_t *p)
{
	int type;
	int pre_type =	p->hs_state;
	pr_info("ACCESSORY INSERTED\n");

	if (p->headset_pd->gpio_headset_sw_en != -1)
		gpio_set_value(p->headset_pd->gpio_headset_sw_en, 1);

	p->hs_state = detect_hs_type(p);

	if(pre_type == p->hs_state)
	{
		pr_info("Duplicated type=%d, skip type update\n", p->hs_state);
		return ;
	}
	else if((pre_type == HEADSET || pre_type == HEADPHONE) &&
		(p->hs_state == HEADSET || p->hs_state == HEADPHONE))
	{
		pre_type = p->hs_state;
		__handle_accessory_removed(p);
		p->hs_state = pre_type;
	}

	switch(p->hs_state) {

	case OPEN_CABLE:
		pr_info("ACCESSORY OPEN\n");
		__handle_accessory_removed(p);   
		break;

	case HEADSET:
		/* Put back the aci interface to be able to detect the
		* button press. Especially this functions puts the
		* COMP2 and MIC BIAS values to be able to detect
		* button press
		*/
		p->button_state = BUTTON_RELEASED;

		/* Configure the ADC to read button press values */
		config_adc_for_bp_detection();

		/* Fall through to send the update to userland */
	case HEADPHONE:

		/* No need to enable the button press/release irq */ 
		/* prevent suspend to allow user space to respond to switch */
		wake_lock_timeout(&p->accessory_wklock, WAKE_LOCK_TIME);
#ifdef CONFIG_SWITCH
		/*
		* While notifying this to the androind world we need to pass
		* the accessory typ as Androind understands. The Android
		* 'state' variable is a bitmap as defined below.
		* BIT 0 : 1 - Headset (with MIC) connected
		* BIT 1 : 1 - Headphone (the one without MIC) is connected
		*/
		type = p->hs_state;
		if(type == HEADSET)
		{
			switch_set_state(&(p->sdev), 1);
			pr_info("ACCESSORY 4 POLE\n");
		}
		else if(type == HEADPHONE)
		{
			switch_set_state(&(p->sdev), 2);
			pr_info("ACCESSORY 3 POLE\n");
		}
		else
			pr_err("ACCESSORY %d\n", type);
#endif
		break;
	default:
		pr_err("%s():Unknown accessory type %d \r\n",__func__, p->hs_state);
		break;
	}
}

static void __handle_accessory_removed (struct mic_t *p)
{
	if(p->hs_state != DISCONNECTED)
	{
		if (p->headset_pd->gpio_headset_sw_en != -1)
			gpio_set_value(p->headset_pd->gpio_headset_sw_en, 0);
		if(p->button_state == BUTTON_PRESSED)
			msleep(30);

		/* Inform userland about accessory removal */ 
		p->hs_state = DISCONNECTED; 
		p->button_state = BUTTON_RELEASED;

#ifdef CONFIG_SWITCH
		switch_set_state(&(p->sdev),p->hs_state);
#endif
		pr_info("ACCESSORY REMOVED\n");
	}
}

/*------------------------------------------------------------------------------
Function name   : hs_unregswitchdev
Description     : Unregister input device for headset
Return type     : int
------------------------------------------------------------------------------*/
int hs_unregswitchdev(struct mic_t *p)
{
#ifdef CONFIG_SWITCH
	cancel_delayed_work_sync(&p->accessory_detect_work);
	if (p->headset_pd->gpio_for_accessory_detection != 1)
		cancel_delayed_work_sync(&p->accessory_remove_work);
	switch_dev_unregister(&p->sdev);
#endif
	return 0;
}

/*------------------------------------------------------------------------------
Function name   : hs_switchinit
Description     : Register sysfs device for headset
It uses class switch from kernel/common/driver/switch
Return type     : int
------------------------------------------------------------------------------*/
#ifdef CONFIG_SWITCH
int hs_switchinit(struct mic_t *p)
{
	int result;
	p->sdev.name = "h2w";
	p->sdev.state = 0;

	result = switch_dev_register(&p->sdev);
	if (result < 0)
		return result;

	if (p->headset_pd->gpio_for_accessory_detection == 1)
	{
		INIT_DELAYED_WORK(&(p->accessory_detect_work_deb), accessory_detect_work_deb_func);
		INIT_DELAYED_WORK(&(p->accessory_detect_work), accessory_detect_work_func);
	}
	else
	{
		INIT_DELAYED_WORK(&(p->accessory_detect_work), no_gpio_accessory_insert_work_func);
		INIT_DELAYED_WORK(&(p->accessory_remove_work), no_gpio_accessory_remove_work_func);
	}
	return 0;
}
#endif

/*------------------------------------------------------------------------------
Function name   : hs_unreginputdev
Description     : unregister and free the input device for headset button
Return type     : int
------------------------------------------------------------------------------*/
static int hs_unreginputdev(struct mic_t *p)
{
	cancel_delayed_work_sync(&p->button_work);
	input_unregister_device(p->headset_button_idev);
	input_free_device(p->headset_button_idev);
	return 0;
}

/*------------------------------------------------------------------------------
Function name   : hs_inputdev
Description     : Create and Register input device for headset button
Return type     : int
------------------------------------------------------------------------------*/
static int hs_inputdev(struct mic_t *p)
{
	int result = 0;

	/* Allocate struct for input device */
	p->headset_button_idev = input_allocate_device();
	if ((p->headset_button_idev) == NULL) {
		pr_err("%s: Not enough memory\n", __func__);
		result = -EINVAL;
		goto inputdev_err;
	}

	/* specify key event type and value for it -
	* Since we have only one button on headset,value KEY_SEND is sent */
	set_bit(EV_KEY, p->headset_button_idev->evbit);
	set_bit(KEY_SEND, p->headset_button_idev->keybit);
	set_bit(KEY_VOLUMEDOWN, p->headset_button_idev->keybit);
	set_bit(KEY_VOLUMEUP, p->headset_button_idev->keybit);

	p->headset_button_idev->name = "bcm_headset";
	p->headset_button_idev->phys = "headset/input0";
	p->headset_button_idev->id.bustype = BUS_HOST;
	p->headset_button_idev->id.vendor = 0x0001;
	p->headset_button_idev->id.product = 0x0100;

	/* Register input device for headset */
	result = input_register_device(p->headset_button_idev);
	if (result) {
		pr_err("%s: Failed to register device\n", __func__);
		goto inputdev_err;
	}

	INIT_DELAYED_WORK(&(p->button_work_deb), button_work_deb_func);
	INIT_DELAYED_WORK(&(p->button_work), button_work_func);

inputdev_err:
	return result;
}

/*------------------------------------------------------------------------------
Function name   : gpio_isr
Description     : interrupt handler
Return type     : irqreturn_t
------------------------------------------------------------------------------*/
static irqreturn_t gpio_isr(int irq, void *dev_id)
{
	struct mic_t *p = (struct mic_t *)dev_id;

	p->hs_detecting = 1;

	pr_info("%s", __func__);

	wake_lock_timeout(&p->accessory_wklock, WAKE_LOCK_TIME);

	schedule_delayed_work(&(p->accessory_detect_work_deb), msecs_to_jiffies(0));

	return IRQ_HANDLED;
}

/*------------------------------------------------------------------------------
Function name   : comp1_isr
Description     : interrupt handler
Return type     : irqreturn_t
------------------------------------------------------------------------------*/
static irqreturn_t comp1_isr(int irq, void *dev_id)
{
	struct mic_t *p = (struct mic_t *)dev_id;
	unsigned int int_val = readl(p->aci_base + ACI_INT_OFFSET);

	pr_info("%s %x\n", __func__, int_val);

	if ( (int_val & 0x01) != 0x01) {
		pr_err("%s : Spurious comp1 isr\n", __func__);
		return IRQ_HANDLED;
	}

	wake_lock_timeout(&p->accessory_wklock, WAKE_LOCK_TIME_IN_SENDKEY);

	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_DISABLE,
		CHAL_ACI_BLOCK_COMP1);

	schedule_delayed_work(&(p->button_work_deb), msecs_to_jiffies(0));

	return IRQ_HANDLED;
}

/*------------------------------------------------------------------------------
Function name   : comp2_isr
Description     : interrupt handler
Return type     : irqreturn_t
------------------------------------------------------------------------------*/
static irqreturn_t comp2_isr(int irq, void *dev_id)
{
	struct mic_t *p = (struct mic_t *)dev_id;

	pr_info("%s\n", __func__);

	if ( (readl(p->aci_base + ACI_INT_OFFSET) & 0x02) != 0x02) {
		pr_err("%s : Spurious comp2 isr\n", __func__);
		return IRQ_HANDLED;
	}

	wake_lock_timeout(&p->accessory_wklock, WAKE_LOCK_TIME);

	/*
	* Note that we are clearing all the interrupts here instead of just
	* COMP2. There is a reason for it.
	*
	* What we observed is that, with the existing configuration where
	* accessory detection happens via COMP2 and not GPIO, when an
	* accessory is plugged in we get a COMP1 interrupt as well.
	*
	* We have to root cause the same and fix it. In the interim as a SW
	* workaround, when we get a COMP2 interrupt (which can happen only
	* when an accessory is plugged in) we clear all the pending
	* interrupts because they are spurious (there cannot be an button
	* press when we are plugging in the accessory). But since the GIC
	* would any way raise the COMP1 or COMP2 INV interrupt, in the
	* respective handles we check whether the respective bits are ON in
	* the ACI_INT register and if not, we'll just return.
	*/ 
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_DISABLE,
		CHAL_ACI_BLOCK_COMP);

	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_ACKNOWLEDGE,
		CHAL_ACI_BLOCK_COMP);

	schedule_delayed_work(&(p->accessory_detect_work), ACCESSORY_INSERTION_SETTLE_TIME);

	return IRQ_HANDLED;
}

/*------------------------------------------------------------------------------
Function name   : comp2_inv_isr
Description     : interrupt handler
Return type     : irqreturn_t
------------------------------------------------------------------------------*/
irqreturn_t comp2_inv_isr(int irq, void *dev_id)
{
	struct mic_t *p = (struct mic_t *)dev_id;

	pr_info("%s\n", __func__);

	if ( (readl(p->aci_base + ACI_INT_OFFSET) & 0x04) != 0x04) {
		pr_err("%s: Spurious comp2 inv isr \n", __func__);
		return IRQ_HANDLED;
	}

	wake_lock_timeout(&p->accessory_wklock, WAKE_LOCK_TIME);

	/* Disable comparator interrupts */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_DISABLE,
		CHAL_ACI_BLOCK_COMP);

	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_ACKNOWLEDGE,
		CHAL_ACI_BLOCK_COMP);

	schedule_delayed_work(&(p->accessory_remove_work), ACCESSORY_INSERTION_SETTLE_TIME);

	return IRQ_HANDLED;
}

static int __headset_hw_init_micbias_off(struct mic_t *p)
{
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_DISABLE,
		CHAL_ACI_BLOCK_COMP);  

	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_ACKNOWLEDGE,
		CHAL_ACI_BLOCK_COMP);

	/*
	* Connect P_MIC_DATA_IN to P_MIC_OUT  and P_MIC_OUT to COMP2
	* Note that one API can do this.
	*/
	chal_aci_set_mic_route(p->aci_chal_hdl, CHAL_ACI_MIC_ROUTE_MIC);

	/* Power down the COMP blocks */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_DISABLE,
		CHAL_ACI_BLOCK_COMP);

	aci_vref_config.mode = CHAL_ACI_VREF_OFF;
	chal_aci_block_ctrl(p->aci_chal_hdl, CHAL_ACI_BLOCK_ACTION_VREF,
		CHAL_ACI_BLOCK_GENERIC, &aci_vref_config);

	aci_init_mic_bias.mode = CHAL_ACI_MIC_BIAS_GND;
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
		CHAL_ACI_BLOCK_GENERIC, &aci_init_mic_bias);

	/* Switch OFF Mic BIAS only if its not already OFF */
	if (p->mic_bias_status == 1) {
		kona_mic_bias_off();
		p->mic_bias_status = 0;
	}

	/* Turning MICBIAS LDO OPMODE to OFF in DSM */
	if (p->ldo) {
		pr_info("\n HS_DEBUG: Set mode to OFF here \n");
		if (regulator_disable(p->ldo))
				pr_info("%s: Can not disable micbias LDO, \
						still proceeding\n", __func__);
		regulator_put(p->ldo);
		p->ldo = NULL;
	}

	return 0;
}

static int __headset_hw_init_micbias_on(struct mic_t *p)
{
        /* Set MICBIAS LDO to LPM */
	if (!p->headset_pd->ldo_id)
		pr_info("%s: No LDO id has been passed\n", __func__);
	else {
		pr_info("HS_DEBUG: Set LPM mode\r\n");
		p->ldo = regulator_get(NULL, p->headset_pd->ldo_id);
		if (IS_ERR_OR_NULL(p->ldo)) {
			pr_info("%s: Can not get MICBIAS LDO\n", __func__);
			return -ENOENT;
		}
		regulator_enable(p->ldo);
	}

	/* Power up the COMP blocks */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_ENABLE,
		CHAL_ACI_BLOCK_COMP);

	/* First disable all the interrupts */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_DISABLE,
		CHAL_ACI_BLOCK_COMP);

	/* Clear pending interrupts if any */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_ACKNOWLEDGE,
		CHAL_ACI_BLOCK_COMP);

	/* Turn ON only if its not already ON */
	if (p->mic_bias_status == 0) {
		kona_mic_bias_on();
		printk
			("=== __headset_hw_init_micbias_on:"
			"called kona_mic_bias_on\r\n");
		p->mic_bias_status = 1;

		/* Fast power up the Vref of ADC block */
		aci_vref_config.mode = CHAL_ACI_VREF_FAST_ON;
		chal_aci_block_ctrl(p->aci_chal_hdl, CHAL_ACI_BLOCK_ACTION_VREF,
			CHAL_ACI_BLOCK_GENERIC, &aci_vref_config);
	}

	aci_init_mic_bias.mode = CHAL_ACI_MIC_BIAS_ON;
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
		CHAL_ACI_BLOCK_GENERIC, &aci_init_mic_bias);

#ifdef CONFIG_ACI_COMP_FILTER
	/* Configure comparator 1 for button press */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_CONFIGURE_FILTER,
		CHAL_ACI_BLOCK_COMP1,
		&comp_values_for_button_press);

	/* Configure the comparator 2 for accessory detection */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_CONFIGURE_FILTER,
		CHAL_ACI_BLOCK_COMP2, &comp_values_for_type_det);
#endif

	/*
	* Connect P_MIC_DATA_IN to P_MIC_OUT  and P_MIC_OUT to COMP2
	* Note that one API can do this.
	*/
	chal_aci_set_mic_route(p->aci_chal_hdl, CHAL_ACI_MIC_ROUTE_MIC);

	/* This value is arrived at by experiments made with
	* the available multi-button headset.This should take care of
	* most multi-button headsets.But this is in no way a universal
	* value for  all singe/multi-button headsets */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
		CHAL_ACI_BLOCK_COMP1, COMP1_THRESHOLD);

	/* Set the threshold value for accessory type detection */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
		CHAL_ACI_BLOCK_COMP2, COMP2_THRESHOLD);

	return 0;
}

static int __headset_hw_init (struct mic_t *p)
{
	if (p == NULL)
		return -1;

	/*
	* IMPORTANT
	* ---------
	* Configuring these AUDIOH MIC registers was required to get ACI interrupts
	* for button press. Really not sure about the connection.
	* But this logic was taken from the BLTS code and if this is not
	* done then we were not getting the ACI interrupts for button press.
	*
	* Looks like if the Audio driver init happens this is not required, in
	* case if Audio driver is not included in the build then this macro should
	* be included to get headset working.
	*
	* Ideally if a macro is used to control brcm audio driver inclusion that does
	* AUDIOH init, then we	 don't need another macro here, it can be something
	* like #ifndef CONFING_BRCM_AUDIOH
	*
	*/
#ifdef CONFIG_HS_PERFORM_AUDIOH_SETTINGS 
	/* AUDIO settings */
	writel(AUDIOH_AUDIORX_VRX1_AUDIORX_VRX_SEL_MIC1B_MIC2_MASK,
		KONA_AUDIOH_VA + AUDIOH_AUDIORX_VRX1_OFFSET);
	writel(0x0, KONA_AUDIOH_VA + AUDIOH_AUDIORX_VRX2_OFFSET);
	writel((AUDIOH_AUDIORX_VREF_AUDIORX_VREF_POWERCYCLE_MASK |
		AUDIOH_AUDIORX_VREF_AUDIORX_VREF_FASTSETTLE_MASK),
		KONA_AUDIOH_VA + AUDIOH_AUDIORX_VREF_OFFSET);
	writel((AUDIOH_AUDIORX_VMIC_AUDIORX_VMIC_CTRL_MASK |
		AUDIOH_AUDIORX_VMIC_AUDIORX_MIC_EN_MASK),
		KONA_AUDIOH_VA + AUDIOH_AUDIORX_VMIC_OFFSET);
	writel(AUDIOH_AUDIORX_BIAS_AUDIORX_BIAS_PWRUP_MASK,
		KONA_AUDIOH_VA + AUDIOH_AUDIORX_BIAS_OFFSET);
#endif

	/* First disable all the interrupts */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_DISABLE,
		CHAL_ACI_BLOCK_COMP);	

	/* Clear pending interrupts if any */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_ACKNOWLEDGE,
		CHAL_ACI_BLOCK_COMP);

	/* Turn ON the MIC BIAS and put it in continuous mode */ 
	/*
	* NOTE:
	* This chal call was failing becuase internally this call
	* was configuring AUDIOH registers as well. We have commmented
	* configuring AUDIOH reigsrs in CHAL and it works OK
	*/

	aci_init_mic_bias.mode = CHAL_ACI_MIC_BIAS_ON;

	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
		CHAL_ACI_BLOCK_GENERIC, &aci_init_mic_bias);

#ifdef CONFIG_ACI_COMP_FILTER
	/* Configure comparator 1 for button press */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_CONFIGURE_FILTER,
		CHAL_ACI_BLOCK_COMP1,
		&comp_values_for_button_press);

	/* Configure the comparator 2 for accessory detection */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_CONFIGURE_FILTER,
		CHAL_ACI_BLOCK_COMP2, &comp_values_for_type_det);
#endif    

	/* 
	* Connect P_MIC_DATA_IN to P_MIC_OUT  and P_MIC_OUT to COMP2
	* Note that one API can do this.
	*/
	chal_aci_set_mic_route (p->aci_chal_hdl, CHAL_ACI_MIC_ROUTE_MIC);

	/* Fast power up the Vref of ADC block */
	/*
	* NOTE:
	* This chal call was failing becuase internally this call
	* was configuring AUDIOH registers as well. We have commmented
	* configuring AUDIOH reigsrs in CHAL and it works OK
	*/
	aci_vref_config.mode = CHAL_ACI_VREF_FAST_ON;
	chal_aci_block_ctrl(p->aci_chal_hdl, CHAL_ACI_BLOCK_ACTION_VREF,
		CHAL_ACI_BLOCK_GENERIC, &aci_vref_config);

	/* Power down the MIC Bias and put in HIZ */ 
	chal_aci_block_ctrl(p->aci_chal_hdl, 
		CHAL_ACI_BLOCK_ACTION_MIC_POWERDOWN_HIZ_IMPEDANCE,
		CHAL_ACI_BLOCK_GENERIC, TRUE);

	/* Set the threshold value for button press */
	/* 
	* Got the feedback from the system design team that the button press 
	* threshold to programmed should be 0.12V as well.
	* With this only send/end button works, so made this 600 i.e 0.6V.
	* With the threshold level set to 0.6V all the 3 button press works
	* OK.Note that this would trigger continus COMP1 Interrupts if enabled.
	* But since we don't enable COMP1 until we identify a Headset its OK.
	*/

	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
		CHAL_ACI_BLOCK_COMP1, COMP1_THRESHOLD);

	/* Set the threshold value for accessory type detection */
	/* COMP2 used for accessory insertion/removal detection */
	chal_aci_block_ctrl(p->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
		CHAL_ACI_BLOCK_COMP2,
		COMP2_THRESHOLD);

	return 0;
}

/*------------------------------------------------------------------------------
Function name   : headset_hw_init
Description     : Hardware initialization sequence
Return type     : int
------------------------------------------------------------------------------*/
static int headset_hw_init(struct mic_t *mic)
{
	int status = 0;

	if( mic->headset_pd->gpio_headset_sw_en != -1){
		/* Request GPIO for HEADSET_SW_EN */
		unsigned int hs_sw_en_gpio = mic->headset_pd->gpio_headset_sw_en;
		pr_info("\nheadset_hw_init hs_sw_en_gpio=%d\n", hs_sw_en_gpio);
		status = gpio_request(hs_sw_en_gpio, "hs_sw_en");
		if (status < 0) {
			pr_err("%s: gpio hs_sw_en request failed \r\n", __func__);
			return status;
		}

		/* Set the GPIO direction output */
		status = gpio_direction_output(hs_sw_en_gpio, 0);
		if (status < 0) {
			pr_err("%s: gpio set direction output failed\n", __func__);
			return status;
		}
	}

	/* Configure AUDIOH CCU for clock policy */
	/* 
	* Remove the entire Audio CCU config policy settings and AUDIOH
	* initialization sequence once they are being done as a part of PMU and
	* audio driver settings
	*/
#ifdef CONFIG_HS_PERFORM_AUDIOH_SETTINGS 
	writel(0xa5a501, KONA_HUB_CLK_VA);
	writel(0x6060606,
		KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_POLICY_FREQ_OFFSET);
	writel(0x7fffffff,
		KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_POLICY0_MASK1_OFFSET);
	writel(0x7fffffff,
		KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_POLICY1_MASK1_OFFSET);
	writel(0x7fffffff,
		KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_POLICY2_MASK1_OFFSET);
	writel(0x7fffffff,
		KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_POLICY3_MASK1_OFFSET);
	writel(0x7fffffff,
		KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_POLICY0_MASK2_OFFSET);
	writel(0x7fffffff,
		KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_POLICY1_MASK2_OFFSET);
	writel(0x7fffffff,
		KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_POLICY2_MASK2_OFFSET);
	writel(0x7fffffff,
		KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_POLICY3_MASK2_OFFSET);
	writel(0x3, KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_POLICY_CTL_OFFSET);
	writel(0x1ff, KONA_HUB_CLK_VA + KHUB_CLK_MGR_REG_AUDIOH_CLKGATE_OFFSET);
	writel(0x100A00, KONA_HUB_CLK_VA + 0x104);
	writel(0x40000100, KONA_HUB_CLK_VA + 0x124);
#endif

	__headset_hw_init (mic);

	/* 
	* If the platform uses GPIO for insetion/removal detection configure
	* the same. Otherwise use COMP2 for insertion and COMP2 INV for
	* removal detection
	*/
	if (mic->headset_pd->gpio_for_accessory_detection == 1) {
		unsigned int hs_gpio = irq_to_gpio(mic->gpio_irq);

		/* Request the gpio 
		* Note that this is an optional call for setting direction/debounce
		* values. But set debounce will throw out warning messages if we 
		* call gpio_set_debounce without calling gpio_request. 
		* Note that it just throws out Warning messages and proceeds
		* to auto request the same. We are adding this call here to 
		* suppress the warning message.
		*/
		status = gpio_request (hs_gpio, "hs_detect");
		if (status < 0) {
			pr_err("%s: gpio request failed \r\n", __func__);
			return status;
		}

#ifdef CONFIG_MACH_CAPRI_SS_CRATER
		/* Set the GPIO debounce */
		status = gpio_set_debounce(hs_gpio, GPIO_DEBOUNCE_TIME);
		if (status < 0) {
			pr_err("%s: gpio set debounce failed\n", __func__);
			return status;
		}
#endif

		/* Set the GPIO direction input */
		status = gpio_direction_input(hs_gpio);
		if (status < 0) {
			pr_err("%s: gpio set direction input failed\n", __func__);
			return status;
		}

		/*
		* Assume that mic bias is ON, so that while initialization we can
		* turn this OFF and put it in known state.
		*/
		mic->mic_bias_status = 1;

		__headset_hw_init_micbias_off(mic);   
	} else {
		/* 
		* This platform does not have GPIO for accessory
		* insertion/removal detection, use COMP2 for accessory
		* insertion detection.
		*/
		chal_aci_block_ctrl(mic->aci_chal_hdl,	
			CHAL_ACI_BLOCK_ACTION_INTERRUPT_ENABLE,
			CHAL_ACI_BLOCK_COMP2);
	}

	return status;
}

int switch_bias_voltage(int mic_status)
{
	/* Force high bias for platforms not
	* supporting 0.45V mode
	*/
	if (!mic_dev->low_voltage_mode)
		mic_status = 1;


	switch (mic_status) {
	case 1:
		/*Mic will be used. Boost voltage */
		/* Set the threshold value for button press */
		pr_info("Setting Bias to 2.1V\r\n");
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
			CHAL_ACI_BLOCK_GENERIC,
			&aci_init_mic_bias);

		/* Power up Digital block */
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_ENABLE,
			CHAL_ACI_BLOCK_DIGITAL);

		/* Power up ADC */
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_DISABLE,
			CHAL_ACI_BLOCK_ADC);
		usleep_range(1000, 1200);
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_ENABLE,
			CHAL_ACI_BLOCK_ADC);
		usleep_range(1000, 1200);

		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_ADC_RANGE, CHAL_ACI_BLOCK_ADC,
			CHAL_ACI_BLOCK_ADC_HIGH_VOLTAGE);

		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_CONFIGURE_FILTER,
			CHAL_ACI_BLOCK_ADC, &aci_filter_adc_config);

		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
			CHAL_ACI_BLOCK_COMP1, COMP1_THRESHOLD);

		break;
		// Always using 2.1V MIC BIAS
#if 0		
	case 0:
		pr_info("Setting Bias to 0.45V \r\n");

		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_MIC_BIAS,
			CHAL_ACI_BLOCK_GENERIC,
			&aci_init_mic_bias_low);
		__low_power_mode_config();
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_ADC_RANGE,
			CHAL_ACI_BLOCK_ADC,
			CHAL_ACI_BLOCK_ADC_LOW_VOLTAGE);
		chal_aci_block_ctrl(mic_dev->aci_chal_hdl,
			CHAL_ACI_BLOCK_ACTION_COMP_THRESHOLD,
			CHAL_ACI_BLOCK_COMP1,
			80);
		button_adc_values =
			mic_dev->headset_pd->button_adc_values_low;
		break;
#endif
	default:
		break;
	}


	return 0;
}

static int hs_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mic_t *mic = platform_get_drvdata(pdev);

	chal_aci_block_ctrl(mic->aci_chal_hdl,
		CHAL_ACI_BLOCK_ACTION_INTERRUPT_ACKNOWLEDGE,
		CHAL_ACI_BLOCK_COMP);

	return 0;
}

static int hs_resume(struct platform_device *pdev)
{
	return 0;
}

static int hs_remove(struct platform_device *pdev)
{
	struct mic_t *mic;

	mic = platform_get_drvdata(pdev);

	hs_unreginputdev(mic);
	hs_unregswitchdev(mic);
	wake_lock_destroy(&mic->accessory_wklock);
	
	device_destroy(mic->audio_class, pdev->dev.devt);
	class_destroy(mic->audio_class);

	free_irq(mic->comp1_irq, mic);
	if (mic->headset_pd->gpio_for_accessory_detection == 1) {
		free_irq(mic->gpio_irq, mic);
	}
	else  {
		free_irq(mic->comp2_irq, mic);
		free_irq(mic->comp2_inv_irq, mic);
	}

	kfree(mic);
	return 0;
}

static int __init hs_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *mem_resource;
	struct mic_t *mic;
	int irq_resource_num = 0;

	mic = kzalloc(sizeof(struct mic_t), GFP_KERNEL);
	if (!mic)
		return -ENOMEM;

	mic_dev = mic;
	wake_lock_init(&mic_dev->accessory_wklock, WAKE_LOCK_SUSPEND, "HS_det_wklock");

	if (pdev->dev.platform_data)
		mic->headset_pd = pdev->dev.platform_data;
	else {
		/* The driver depends on the platform data (board specific)
		* information to know two things
		* 1) The GPIO state that determines accessory insertion (HIGH or LOW)
		* 2) The resistor value put on the MIC_IN line.
		*
		* So if the platform data is not present, do not proceed.
		*/
		pr_err("hs_probe: Platform data not present, could not proceed \r\n");
		ret = EINVAL;
		goto err2;
	}

	/* Initialize the switch dev for headset */
#ifdef CONFIG_SWITCH
	ret = hs_switchinit(mic);
	if (ret < 0)
		goto err2;
#endif

	/* Initialize a input device for the headset button */
	ret = hs_inputdev(mic);
	if (ret < 0) {
		hs_unregswitchdev(mic);
		goto err2;
	}

	mic->audio_class = class_create(THIS_MODULE, "audio");
	mic->headset_dev = device_create(mic->audio_class, NULL, pdev->dev.devt, NULL, "earjack");
	device_create_file(mic->headset_dev, &headset_Attrs[STATE]);
	device_create_file(mic->headset_dev, &headset_Attrs[KEY_STATE]);
	device_create_file(mic->headset_dev, &headset_Attrs[RECHECK]);

	/* setting ldo to NULL till we call regulator_get() */
	mic->ldo = NULL;
	mic_dev->aci_apb_clk = clk_get(&pdev->dev, mic->headset_pd->aci_clk_name);
	mic_dev->audioh_apb_clk = clk_get(&pdev->dev, "audioh_apb_clk");
	clk_enable(mic_dev->aci_apb_clk);

	/*
	* If on the given platform GPIO is used for accessory
	* insertion/removal detection get the GPIO IRQ to be
	* used for the same.
	*/
	if (mic->headset_pd->gpio_for_accessory_detection == 1) {
		/* Insertion detection irq */
		mic->gpio_irq = platform_get_irq(pdev, irq_resource_num);
		if (!mic->gpio_irq) {
			ret = -EINVAL;
			goto err1;
		}
		pr_info("HS GPIO irq %d\n", mic->gpio_irq);
	}
	irq_resource_num++;

	mic_dev->low_voltage_mode = 0;
	if (mic->headset_pd->button_adc_values == NULL) {
		mic->headset_pd->button_adc_values = button_adc_values_no_resistor;
	}

	/* COMP2 irq */
	mic->comp2_irq = platform_get_irq(pdev, irq_resource_num);
	if (!mic->comp2_irq) {
		ret = -EINVAL;
		goto err1;
	}
	irq_resource_num++;

	/* COMP2 INV irq */
	mic->comp2_inv_irq = platform_get_irq(pdev, irq_resource_num);
	if (!mic->comp2_inv_irq) {
		ret = -EINVAL;
		goto err1;
	}
	irq_resource_num++;

	/* Get COMP1 IRQ */
	mic->comp1_irq = platform_get_irq(pdev,irq_resource_num);
	if (!mic->comp1_irq) {
		ret = -EINVAL;
		goto err1;
	}

	/* Get the base address for AUXMIC and ACI control registers */
	mem_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_resource) {
		ret = -ENXIO;
		goto err1;
	}
	mic->auxmic_base = HW_IO_PHYS_TO_VIRT(mem_resource->start);

	mem_resource = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!mem_resource) {
		ret = -ENXIO;
		goto err1;
	}
	mic->aci_base = HW_IO_PHYS_TO_VIRT(mem_resource->start);

	/* Perform CHAL initialization */
	mic->aci_chal_hdl = chal_aci_init(mic->aci_base);

	/* Hardware initialization */
	ret = headset_hw_init(mic);
	if (ret < 0)
		goto err1;

#ifdef DEBUG
	dump_hw_regs(mic);
#endif

	mic->recheck_jack = 0;
	mic->hs_state = DISCONNECTED;
	mic->button_state = BUTTON_RELEASED;
	mic->hs_detecting = 0;

	/* Store the mic structure data as private driver data for later use */
	platform_set_drvdata(pdev, mic);

	/*
	* Please note that all the HS accessory interrupts 
	* should be requested with _NO_SUSPEND option because even if
	* the system goes to suspend we want this interrupts to be active
	*/

	/* Request for COMP1 IRQ */
	ret =
		request_threaded_irq(mic->comp1_irq, NULL, comp1_isr,
		(IRQF_NO_SUSPEND | IRQF_ONESHOT), "COMP1", mic);
	if (ret < 0) {
		pr_err("%s(): request_irq() failed for headset %s: %d\n",
			__func__, "irq", ret);
		goto err1;
	}

	if (mic->headset_pd->gpio_for_accessory_detection == 1) {
		/* Request the IRQ for accessory insertion detection */
		ret =
			request_threaded_irq(mic->gpio_irq, NULL,   gpio_isr,
			(IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_ONESHOT | IRQF_NO_SUSPEND),
			"aci_accessory_detect", mic);
		if (ret < 0) {
			pr_err("%s(): request_irq() failed for headset %s: %d\n",
				__func__, "irq", ret);

			free_irq(mic->comp1_irq, mic);
			goto err1;
		}
	} else {
		/* Request for COMP2 IRQ */
		ret =
			request_irq(mic->comp2_irq, comp2_isr, (IRQF_NO_SUSPEND),
			"COMP2", mic);
		if (ret < 0) {
			pr_err("%s(): request_irq() failed for headset %s: %d\n",
				__func__, "button press", ret);
			free_irq(mic->comp1_irq, mic);
			goto err1;
		}

		/* 
		* If GPIO is not used for accessory insertion/removal
		* detection COMP2 INV IRQ is needed for accessory removal
		* detection.
		*/
		ret =
			request_irq(mic->comp2_inv_irq, comp2_inv_isr, (IRQF_NO_SUSPEND),
			"COMP2 INV", mic);
		if (ret < 0) {
			pr_err("%s(): request_irq() failed for headset %s: %d\n",
				__func__, "button release", ret);
			free_irq(mic->comp1_irq, mic);
			free_irq(mic->comp2_irq, mic);
			goto err1;
		}     
	}

	schedule_delayed_work(&(mic->accessory_detect_work), ACCESSORY_INSERTION_SETTLE_TIME);   

#ifdef DEBUG
	dump_hw_regs(mic);
#endif

	pr_info("%s(): Headset probe done.\n", __func__);

	return ret;
err1:
	pr_err("%s(): Error driver not loaded \r\n", __func__);
	wake_lock_destroy(&mic->accessory_wklock);
	device_destroy(mic->audio_class, pdev->dev.devt);
	class_destroy(mic->audio_class);   
	hs_unregswitchdev(mic);
	hs_unreginputdev(mic);   
err2:
	clk_disable(mic_dev->aci_apb_clk);
	kfree(mic);
	return ret;
}

/*
* Note that there is a __refdata added to the headset_driver platform driver
* structure. What is the meaning for it and why its required.
*
* The probe function: 
* From the platform driver documentation its advisable to keep the probe
* function of a driver in the __init section if the device is NOT hot pluggable. 
* Note that in headset case even though the headset is hot pluggable, the driver 
* is not. That is a new node will not be created and the probe will not be called 
* again. So it makes sense to keep the hs_probe in __init section so as to 
* reduce the driver's run time foot print.
* 
* The Warning message:
* But since the functions address (reference) is stored in a structure that
* will be available even after init (in case of remove, suspend etc) there
* is a Warning message from the compiler
*
* The __refdata keyword can be used to suppress this warning message. Tells the
* compiler not to throw out this warning. And in this scenario even though
* we store the function pointer from __init section to the platform driver
* structure that lives after __init, we wont be referring the probe function
* in the life time until headset_driver lives, so its OK to suppress.
*/
static struct platform_driver __refdata headset_driver = {
	.probe = hs_probe,
	.remove = hs_remove,
	.suspend = hs_suspend,
	.resume = hs_resume,
	.driver = {
		.name = "konaaciheadset",
		.owner = THIS_MODULE,
	},
};

#ifdef DEBUG

struct kobject *hs_kobj;

static ssize_t
	hs_regdump_func(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t n)
{
	dump_hw_regs(mic_dev);
	return n;
}

static ssize_t
	hs_regwrite_func(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t n)
{
	unsigned int reg_off;
	unsigned int val;

	if (sscanf(buf,"%x %x", &reg_off, &val) != 2) {
		printk("Usage: echo reg_offset value > /sys/hs_debug/hs_regwrite \r\n");
		return n;
	}
	printk("Writing 0x%x to Address 0x%x \r\n", val, mic_dev->aci_base + reg_off);
	writel(val, mic_dev->aci_base + reg_off);
	return n;
}

static DEVICE_ATTR(hs_regdump, 0666, NULL, hs_regdump_func);
static DEVICE_ATTR(hs_regwrite, 0666, NULL, hs_regwrite_func);

static struct attribute *hs_attrs[] = {
	&dev_attr_hs_regdump.attr,
	&dev_attr_hs_regwrite.attr,
	NULL,
};

static struct attribute_group hs_attr_group = {
	.attrs = hs_attrs,
};

static int __init hs_sysfs_init(void)
{
	hs_kobj = kobject_create_and_add("hs_debug", NULL);
	if (!hs_kobj)
		return -ENOMEM;
	return sysfs_create_group(hs_kobj, &hs_attr_group);
}

static void __exit hs_sysfs_exit(void)
{
	sysfs_remove_group(hs_kobj, &hs_attr_group);
}
#endif


/*------------------------------------------------------------------------------
Function name   : kona_hs_module_init
Description     : Initialize the driver
Return type     : int
------------------------------------------------------------------------------*/
int __init kona_aci_hs_module_init(void)
{
#ifdef DEBUG
	hs_sysfs_init();
#endif
	return platform_driver_register(&headset_driver);
}

/*------------------------------------------------------------------------------
Function name   : kona_hs_module_exit
Description     : clean up
Return type     : int
------------------------------------------------------------------------------*/
void __exit kona_aci_hs_module_exit(void)
{
#ifdef DEBUG
	hs_sysfs_exit();
#endif
	return platform_driver_unregister(&headset_driver);
}

module_init(kona_aci_hs_module_init);
module_exit(kona_aci_hs_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Headset plug and button detection");
