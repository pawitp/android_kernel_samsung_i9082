/*
 * Generic PWM backlight driver data - see drivers/video/backlight/pwm_bl.c
 */
#ifndef __LINUX_ISA1000_VIBRATOR_H
#define __LINUX_ISA1000_VIBRATOR_H

#include <linux/device.h>
#include <linux/mutex.h>

struct platform_isa1000_vibrator_data {
	int (*gpio_en) (bool) ;
	const char *pwm_name;
	unsigned int pwm_period_ns;
	unsigned int pwm_duty;
	unsigned int polarity;
	const char *regulator_id;
};

#endif
