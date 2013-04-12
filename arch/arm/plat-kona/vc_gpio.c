/*****************************************************************************
 *
 * VideoCore GPIO handler
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

#include <linux/gpio.h>

#include <mach/gpio.h>
#include <mach/pinmux.h>
#include <mach/vc_gpio.h>

#define pr_vcgpio	pr_debug

#define NUM_OF_GPIOS_TO_STORE 128

/*============================================================================*/

typedef struct {
	struct pin_config pin_config;
	unsigned int pin_num;
	unsigned int pin_value;
} GPIO_STORE_T;

static GPIO_STORE_T stored_gpio[NUM_OF_GPIOS_TO_STORE];

static int store_index = 0;

/*============================================================================*/

/* http://mobmm.eu.broadcom.com/jira/browse/SW-8300 */

int vc_gpio_suspend(void)
{
	enum PIN_NAME pin_name;
	struct pin_config pin_config =
		{0};
	unsigned int pin_num, alt_num, flags, is_vc;
	int pin_value;

	for (pin_name = 0; pin_name < PN_MAX; pin_name++) {
		pin_config.name = pin_name;
		pinmux_get_pin_config(&pin_config);

		/* Work out alt number */
		alt_num = 0;
		while ((pin_config.func
			!= g_chip_pin_desc.desc_tbl[pin_name].f_tbl[alt_num])
			&& (alt_num < MAX_ALT_FUNC)) {
			alt_num++;
		}

		flags = g_chip_pin_desc.desc_tbl[pin_name].flags;
		is_vc = flags & (1 << alt_num);

		if (is_vc) {
			pin_num = g_chip_pin_desc.desc_tbl[pin_name].gpio_num;
			if (pin_num != PIN_DESC_GPIO_NUM_NONE) {
				pin_value = gpio_get_value(pin_num);
			}
			else {
				pin_value = -1;
			}

			if (store_index < NUM_OF_GPIOS_TO_STORE) {
				pr_vcgpio(
					"%s: pin_config.name = %d, pin_config.func = %d, pin_config.reg.val = 0x%X, "
						"pin_num = %d, pin_value = 0x%X",
					__FUNCTION__, pin_config.name,
					pin_config.func, pin_config.reg.val,
					pin_num, pin_value);

				/* Store values */
				stored_gpio[store_index].pin_config =
					pin_config;
				stored_gpio[store_index].pin_num = pin_num;
				stored_gpio[store_index].pin_value = pin_value;

				/* Remove pull resistor (if enabled) */
				if (flags & PIN_DESC_FLAGS_I2C) {
					pin_config.reg.b_i2c.pup_2_0 = 0;
				}
				else {
					pin_config.reg.b.pull_dn = 0;
					pin_config.reg.b.pull_up = 0;
				}
				pinmux_set_pin_config(&pin_config);

				/* Set to input */
				if (pin_num != PIN_DESC_GPIO_NUM_NONE) {
					pin_value |= 0x1;
					gpio_set_value(pin_num, pin_value);
				}

				store_index++;
			}
			else {
				printk(
					KERN_ERR "%s: out of space to store pin config",
					__FUNCTION__);
			} /* if */
		} /* if */
	} /* for */

	return 0;
} /* vc_gpio_suspend() */

/*----------------------------------------------------------------------------*/

int vc_gpio_resume(void)
{
	GPIO_STORE_T *stored_gpio_ptr;

	/* Restore pin settings */
	for (store_index--; store_index >= 0; store_index--) {
		stored_gpio_ptr = &stored_gpio[store_index];

		pr_vcgpio(
			"%s: pin_config.name = %d, pin_config.func = %d, pin_config.reg.val = 0x%X, "
				"pin_num = %d, pin_value = 0x%X", __FUNCTION__,
			stored_gpio_ptr->pin_config.name,
			stored_gpio_ptr->pin_config.func,
			stored_gpio_ptr->pin_config.reg.val,
			stored_gpio_ptr->pin_num, stored_gpio_ptr->pin_value);

		/* Set input/output */
		if (stored_gpio_ptr->pin_num != PIN_DESC_GPIO_NUM_NONE) {
			gpio_set_value(stored_gpio_ptr->pin_num,
				stored_gpio_ptr->pin_value);
		}
		/* Set pull resistor (if previously enabled) */
		pinmux_set_pin_config(&stored_gpio_ptr->pin_config);
	}
	store_index = 0;

	return 0;
} /* vc_gpio_resume() */

/*============================================================================*/
