/*****************************************************************************
*  Copyright 2001 - 2011 Broadcom Corporation.  All rights reserved.
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

int bcm_hsotgctrl_en_clock(bool on)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_en_clock);


int bcm_hsotgctrl_phy_init(bool id_device)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_init);

int bcm_hsotgctrl_phy_deinit(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_deinit);

int bcm_hsotgctrl_bc_reset(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_bc_reset);

int bcm_hsotgctrl_bc_status(unsigned long *status)
{
	*status = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_bc_status);

int bcm_hsotgctrl_bc_vdp_src_off(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_bc_vdp_src_off);

int bcm_hsotgctrl_get_clk_count(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_get_clk_count);

int bcm_hsotgctrl_handle_bus_suspend(send_core_event_cb_t suspend_core_cb,
			send_core_event_cb_t wakeup_core_cb)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_handle_bus_suspend);

static int __devinit bcm_hsotgctrl_probe(struct platform_device *pdev)
{
	return 0;
}

static int bcm_hsotgctrl_remove(struct platform_device *pdev)
{
	return 0;
}

static int bcm_hsotgctrl_runtime_suspend(struct device *dev)
{
	return 0;
}

static int bcm_hsotgctrl_runtime_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops bcm_hsotg_ctrl_pm_ops = {
	.runtime_suspend = bcm_hsotgctrl_runtime_suspend,
	.runtime_resume = bcm_hsotgctrl_runtime_resume,
};

static struct platform_driver bcm_hsotgctrl_driver = {
	.driver = {
		   .name = "bcm_hsotgctrl",
		   .owner = THIS_MODULE,
		   .pm = &bcm_hsotg_ctrl_pm_ops,
	},
	.probe = bcm_hsotgctrl_probe,
	.remove = bcm_hsotgctrl_remove,
};

static int __init bcm_hsotgctrl_init(void)
{
	pr_info("Broadcom USB HSOTGCTRL Driver\n");

	return platform_driver_register(&bcm_hsotgctrl_driver);
}
module_init(bcm_hsotgctrl_init);

static void __exit bcm_hsotgctrl_exit(void)
{
	platform_driver_unregister(&bcm_hsotgctrl_driver);
}
module_exit(bcm_hsotgctrl_exit);

int bcm_hsotgctrl_phy_set_vbus_stat(bool on)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_set_vbus_stat);

int bcm_hsotgctrl_phy_set_non_driving(bool on)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_set_non_driving);

int bcm_hsotgctrl_reset_clk_domain(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_reset_clk_domain);

int bcm_hsotgctrl_set_phy_off(bool on)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_phy_off);

int bcm_hsotgctrl_set_phy_iso(bool on)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_phy_iso);

int bcm_hsotgctrl_set_bc_iso(bool on)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_bc_iso);


int bcm_hsotgctrl_set_soft_ldo_pwrdn(bool on)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_soft_ldo_pwrdn);

int bcm_hsotgctrl_set_aldo_pdn(bool on)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_aldo_pdn);

int bcm_hsotgctrl_set_phy_resetb(bool on)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_phy_resetb);

int bcm_hsotgctrl_set_phy_clk_request(bool on)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_phy_clk_request);

int bcm_hsotgctrl_set_ldo_suspend_mask(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_set_ldo_suspend_mask);


int bcm_hsotgctrl_phy_set_id_stat(bool floating)
{
	return 0;
}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_set_id_stat);

int bcm_hsotgctrl_phy_wakeup_condition(bool set)
{
	return 0;

}
EXPORT_SYMBOL_GPL(bcm_hsotgctrl_phy_wakeup_condition);


MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("USB HSOTGCTRL driver");
MODULE_LICENSE("GPL");

