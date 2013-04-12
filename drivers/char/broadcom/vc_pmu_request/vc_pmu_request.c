/*****************************************************************************
 *
 * VideoCore PMU handler
 *
 * Copyright 2012 Broadcom Corporation.  All rights reserved.
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
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/broadcom/vc_dt.h>

#include <mach/vc_pmu_request.h>

#define VC_PMU_REG_COUNT 4

# undef ENABLE_LOG_DBG
#define ENABLE_LOG_DBG
#ifdef ENABLE_LOG_DBG
#define LOG_DBG(fmt, arg...)  printk(KERN_INFO "[D] " fmt "\n", ##arg)
#else
#define LOG_DBG(fmt, arg...)
#endif
#define LOG_INFO(fmt, arg...)  printk(KERN_INFO "[I] " fmt "\n", ##arg)
#define LOG_ERR(fmt, arg...)  printk(KERN_ERR  "[E] " fmt "\n", ##arg)

/*============================================================================*/

static int vc_pmu_req_probe(struct platform_device *p_dev);
static int __devexit vc_pmu_req_remove(struct platform_device *p_dev);

/*============================================================================*/

static struct platform_driver vc_pmu_req_driver = {
	.probe      = vc_pmu_req_probe,
	.remove     = __devexit_p(vc_pmu_req_remove),
	.driver = {
		   .name = "vc-pmu-request"}
};

struct vc_pmu_req_state {
	struct platform_device *p_dev;
	int got_regulators;
	int regulator_count;
	char  *names;
	struct regulator_bulk_data regulators[VC_PMU_REG_COUNT];
};

static struct vc_pmu_req_state *vc_pmu_req_stt;

/*============================================================================*/
static int vc_pmu_req_parse(struct vc_pmu_req_state *s)
{
	int in_separator = 1;
	char *reg_name = NULL;
	size_t reg_idx = 0;
	size_t len = strlen(s->names);

	size_t name_idx;

	/* The name list is a comma and whitespace separated list. */
	for (name_idx = 0; name_idx <= len; name_idx++) {
		if (s->names[name_idx] == ','  ||
		    s->names[name_idx] == ' '  ||
		    s->names[name_idx] == '\t' ||
		    s->names[name_idx] == '\0') {
			/* Terminate entry. */
			s->names[name_idx] = '\0';
			in_separator = 1;

			if (reg_name) {
				LOG_INFO("%s : got regulator %s",
					__func__, reg_name);
				s->regulators[reg_idx++].supply = reg_name;
				reg_name = NULL;
			}
		} else if (in_separator) {
			/* Start of new entry. */
			in_separator = 0;
			reg_name = s->names + name_idx;
		}

		if (reg_idx == VC_PMU_REG_COUNT) {
			if (name_idx < len) {
				LOG_ERR("%s : regulator list is full",
					__func__);
			}
			break;
		}
	}

	return reg_idx;
}

static void vc_pmu_req_get_regulators(
	struct vc_pmu_req_state *s,
	struct platform_device *p_dev)
{
	int result;

	if (!s->got_regulators) {
		const char *dt_list = vc_dt_get_pmu_config();

		if (dt_list) {
			size_t total_len = strlen(dt_list);

			s->names = kzalloc(total_len + 1, GFP_KERNEL);

			if (!s->names) {
				LOG_ERR("%s : allocation fail", __func__);
			} else {
				strcpy(s->names, dt_list);
				s->names[total_len] = 0;
				s->regulator_count = vc_pmu_req_parse(s);
			}

			LOG_INFO("%s : got %d regulators", __func__,
				s->regulator_count);
		} else {
			s->names = NULL;
			s->regulator_count = 0;
		}

		if (s->regulator_count) {
			result = regulator_bulk_get(&p_dev->dev,
				s->regulator_count, s->regulators);
			if (result) {
				LOG_ERR("%s : regulator_bulk_get fail %d",
					__func__, result);
				s->regulator_count = 0;
			}
		}

		s->got_regulators = 1;
	}
}

static int vc_pmu_req_probe(struct platform_device *p_dev)
{
	int ret = 0;
	LOG_INFO("%s: start", __func__);
	vc_pmu_req_stt = kzalloc(sizeof(struct vc_pmu_req_state), GFP_KERNEL);
	if (!vc_pmu_req_stt) {
		ret = -ENOMEM;
	} else {
		vc_pmu_req_stt->p_dev = p_dev;
		vc_pmu_req_get_regulators(vc_pmu_req_stt, p_dev);

		/* We start up powered on, so call resume. */
		vc_pmu_req_resume();
	}

	LOG_INFO("%s: end, ret=%d", __func__, ret);
	return ret;
}


static int __devexit vc_pmu_req_remove(struct platform_device *p_dev)
{
	if (!vc_pmu_req_stt)
		return 0;

	if (vc_pmu_req_stt->regulator_count)
		regulator_bulk_free(vc_pmu_req_stt->regulator_count,
			vc_pmu_req_stt->regulators);

	kfree(vc_pmu_req_stt->names);
	kfree(vc_pmu_req_stt);
	vc_pmu_req_stt = NULL;

	return 0;
}

static int __init vc_pmu_req_init(void)
{
	int ret;
	LOG_INFO("vc-pmu-request Videocore PMU request driver");

	ret = platform_driver_register(&vc_pmu_req_driver);
	if (ret) {
		LOG_ERR("%s : Unable to register VC PMU request driver (%d)",
				__func__, ret);
	} else {
		LOG_INFO("%s : Registered Videocore PMU request driver",
				__func__);
	}

	return ret;
}

static void __exit vc_pmu_req_exit(void)
{
	LOG_DBG("%s: start", __func__);

	platform_driver_unregister(&vc_pmu_req_driver);

	LOG_DBG("%s: end", __func__);
}

/*============================================================================*/

int vc_pmu_req_suspend(void)
{
	struct vc_pmu_req_state *s = vc_pmu_req_stt;

	LOG_DBG("%s: start", __func__);
#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN) || defined(CONFIG_MACH_CAPRI_SS_CRATER)
	gpio_request(3,"lcd_1v8");
	gpio_direction_output(3,0);
	gpio_free(3);
#endif

	if (!s)
		return -EINVAL;

	if (s->regulator_count) {
		LOG_DBG("%s: calling regulator_bulk_disable", __func__);
		return regulator_bulk_disable(s->regulator_count,
			s->regulators);
	} else {
		LOG_DBG("%s: no regulators", __func__);
		return 0;
	}
}

/*----------------------------------------------------------------------------*/

int vc_pmu_req_resume(void)
{
	struct vc_pmu_req_state *s = vc_pmu_req_stt;

	LOG_DBG("%s: start", __func__);

#if defined(CONFIG_MACH_CAPRI_SS_BAFFIN) || defined(CONFIG_MACH_CAPRI_SS_CRATER)
        gpio_request(3,"lcd_1v8");
        gpio_direction_output(3,1);
        gpio_free(3);
#endif

	if (!s)
		return -EINVAL;

	if (s->regulator_count) {
		LOG_DBG("%s: calling regulator_bulk_enable", __func__);
		return regulator_bulk_enable(s->regulator_count, s->regulators);
	} else {
		LOG_DBG("%s: no regulators", __func__);
		return 0;
	}
}

/*============================================================================*/

late_initcall(vc_pmu_req_init);
module_exit(vc_pmu_req_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("VC PMU Request Driver");
MODULE_LICENSE("GPL");
