/****************************************************************************
*
* Copyright 2010 --2011 Broadcom Corporation.
*
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2, available at
* http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
*****************************************************************************/
#include <linux/module.h>
#include <linux/spinlock.h>
#include <mach/pinmux.h>
#include <mach/pinmux_pm.h>

#define MAX_SDIO_DEV_NUM 4

static DEFINE_SPINLOCK(pinmux_lock);

static const enum PIN_NAME sdio0_pins[6] = {
	PN_SDIO1_CLK,
	PN_SDIO1_CMD,
	PN_SDIO1_DATA_0,
	PN_SDIO1_DATA_1,
	PN_SDIO1_DATA_2,
	PN_SDIO1_DATA_3,
};

int capri_pm_sdio_pinmux_ctrl(unsigned int sdio_dev, int enable_pulldn)
{
	unsigned int i;
	unsigned long flags;

	if (sdio_dev > MAX_SDIO_DEV_NUM)
		return -EINVAL;

	spin_lock_irqsave(&pinmux_lock, flags);

	switch (sdio_dev) {
		/* only support SDIO0 (WiFi) for now */
	case 0:
		for (i = 0; i < ARRAY_SIZE(sdio0_pins); i++) {
			struct pin_config pin_config = { 0 };

			pin_config.name = sdio0_pins[i];
			pinmux_get_pin_config(&pin_config);

			if (enable_pulldn) {
				pin_config.reg.b.pull_dn = 1;
				pin_config.reg.b.pull_up = 0;
			} else {
				pin_config.reg.b.pull_dn = 0;
				pin_config.reg.b.pull_up = 1;
			}

			pinmux_set_pin_config(&pin_config);
		}
		break;

	default:
		spin_unlock_irqrestore(&pinmux_lock, flags);
		return -EINVAL;
	}

	spin_unlock_irqrestore(&pinmux_lock, flags);
	return 0;
}
