/*****************************************************************************
* Copyright 2006 - 2012 Broadcom Corporation.  All rights reserved.
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

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>

#include <mach/pinmux.h>
#include <mach/usbh_cfg.h>

#include <mach/rdb/brcm_rdb_usbh_phy.h>

#ifdef DEBUG
#define dbg_printk(fmt, args...) printk(KERN_INFO "%s: " fmt, __func__, ## args)
#else
#define dbg_printk(fmt, args...)
#endif

struct usbh_ctrl_regs {
	u32 clkrst_ctrl;
	u32 core_strap_ctrl;
	u32 ss_fladj_val_host_i;
	u32 ss_fladj_val[USBH_NUM_PORTS];
	u32 afe_ctrl;
	u32 phy_ctrl;
	u32 phy_p1ctrl;
	u32 phy_p2ctrl;
	u32 pll_ctrl;
	u32 tp_sel;
	u32 tp_out;
	u32 suspend_wrap_ctrl;
};

struct usbh_priv {
	struct device *dev;
	struct usbh_cfg hw_cfg;
	struct clk *peri_clk;
	struct clk *ahb_clk;
	struct clk *opt_clk;
	struct usbh_ctrl_regs __iomem *ctrl_regs;
};

static struct usbh_priv usbh_data;

static int usbh_clk_ctrl(struct usbh_priv *drv_data, int enable)
{
	int ret;
	struct usbh_cfg *hw_cfg = &drv_data->hw_cfg;

	if (enable) {
		/* peripheral clock */
		if (hw_cfg->peri_clk_name) {
			drv_data->peri_clk =
			    clk_get(drv_data->dev, hw_cfg->peri_clk_name);
			if (IS_ERR(drv_data->peri_clk))
				return PTR_ERR(drv_data->peri_clk);
			ret = clk_enable(drv_data->peri_clk);
			if (ret)
				goto err_put_peri_clk;
		}

		/* AHB clock */
		if (hw_cfg->ahb_clk_name) {
			drv_data->ahb_clk =
			    clk_get(drv_data->dev, hw_cfg->ahb_clk_name);
			if (IS_ERR(drv_data->ahb_clk)) {
				ret = PTR_ERR(drv_data->ahb_clk);
				goto err_disable_peri_clk;
			}
			ret = clk_enable(drv_data->ahb_clk);
			if (ret)
				goto err_put_ahb_clk;
		}

		/* optional clock (in the USB host case,
		* that's the 12 MHz clock
		*/
		if (hw_cfg->opt_clk_name) {
			drv_data->opt_clk =
			    clk_get(drv_data->dev, hw_cfg->opt_clk_name);
			if (IS_ERR(drv_data->opt_clk)) {
				ret = PTR_ERR(drv_data->opt_clk);
				goto err_disable_ahb_clk;
			}
			ret = clk_enable(drv_data->opt_clk);
			if (ret)
				goto err_put_opt_clk;
		}

		dbg_printk(KERN_INFO "USB HOST: Clocks enabled!\n");
		return 0;

 err_put_opt_clk:
		if (drv_data->opt_clk) {
			clk_put(drv_data->ahb_clk);
			drv_data->ahb_clk = NULL;
		}
 err_disable_ahb_clk:
		if (drv_data->ahb_clk)
			clk_disable(drv_data->ahb_clk);
 err_put_ahb_clk:
		if (drv_data->ahb_clk) {
			clk_put(drv_data->ahb_clk);
			drv_data->ahb_clk = NULL;
		}
 err_disable_peri_clk:
		if (drv_data->peri_clk)
			clk_disable(drv_data->peri_clk);
 err_put_peri_clk:
		if (drv_data->peri_clk) {
			clk_put(drv_data->peri_clk);
			drv_data->peri_clk = NULL;
		}

		return ret;
	} else {
		if (drv_data->peri_clk) {
			clk_disable(drv_data->peri_clk);
			clk_put(drv_data->peri_clk);
			drv_data->peri_clk = NULL;
		}
		if (drv_data->ahb_clk) {
			clk_disable(drv_data->ahb_clk);
			clk_put(drv_data->ahb_clk);
			drv_data->ahb_clk = NULL;
		}
		if (drv_data->opt_clk) {
			clk_disable(drv_data->opt_clk);
			clk_put(drv_data->opt_clk);
			drv_data->opt_clk = NULL;
		}
	}
	dbg_printk(KERN_INFO "USB HOST: Clocks disabled!\n");
	return 0;
}

static int bcm_usbh_host_init(void)
{
	struct usbh_priv *drv_data = &usbh_data;
	int retries;
	uint32_t tmp;

	/* Enable OHCI access even if pll_lock is not asserted. */
	tmp = readl(&drv_data->ctrl_regs->clkrst_ctrl);
	writel(tmp | USBH_PHY_CLKRST_CTRL_SW_OHCI_ACCESS_EN_MASK,
	       &drv_data->ctrl_regs->clkrst_ctrl);
	dbg_printk("Change Clock Reset Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_data->ctrl_regs->clkrst_ctrl));

	/* Enable 48MHz reference clock to PHY */
	tmp = readl(&drv_data->ctrl_regs->suspend_wrap_ctrl);
	writel(tmp | USBH_PHY_SUSPEND_WRAP_CTRL_PHY_CLK_REQ_MASK |
	       USBH_PHY_SUSPEND_WRAP_CTRL_PHY_CLK_REQ_CLR_MASK,
	       &drv_data->ctrl_regs->suspend_wrap_ctrl);
	dbg_printk
	    ("Chge PHY Susp Pwrdwn from 0x%08x to 0x%08x\n",
	     tmp, readl(&drv_data->ctrl_regs->suspend_wrap_ctrl));

	/* Deassert power downs */
	tmp = readl(&drv_data->ctrl_regs->afe_ctrl);
	writel(tmp | USBH_PHY_AFE_CTRL_SW_AFE_LDO_PWRDWNB_1_MASK |
	       USBH_PHY_AFE_CTRL_SW_AFE_LDOBG_PWRDWNB_MASK,
	       &drv_data->ctrl_regs->afe_ctrl);
	dbg_printk("Change PHY AFE Control from 0x%08x to 0x%08x\n", tmp,
		   readl(&drv_data->ctrl_regs->afe_ctrl));

	mdelay(1);

	/* Deassert ISO and IDDQ */
	tmp = readl(&drv_data->ctrl_regs->phy_ctrl);
	writel(tmp &
	       ~(USBH_PHY_PHY_CTRL_SW_PHY_ISO_MASK |
		 USBH_PHY_PHY_CTRL_PHY_IDDQ_MASK),
	       &drv_data->ctrl_regs->phy_ctrl);
	dbg_printk("Change Dual PHY Control from 0x%08x to 0x%08x\n", tmp,
		   readl(&drv_data->ctrl_regs->phy_ctrl));

	mdelay(1);

	/* Pull PHYs out of reset */
	tmp = readl(&drv_data->ctrl_regs->phy_ctrl);
	writel(tmp | USBH_PHY_PHY_CTRL_SW_PHY_RESETB_MASK,
	       &drv_data->ctrl_regs->phy_ctrl);
	dbg_printk("Change Dual PHY Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_data->ctrl_regs->phy_ctrl));

	/* Pull PLL out of reset */
	tmp = readl(&drv_data->ctrl_regs->pll_ctrl);
	writel(tmp | USBH_PHY_PLL_CTRL_PLL_RESETB_MASK,
	       &drv_data->ctrl_regs->pll_ctrl);
	dbg_printk("Change Dual PHY PLL Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_data->ctrl_regs->pll_ctrl));

	/* Wait for PLL lock */
	retries = 100;
	while ((!(readl(&drv_data->ctrl_regs->pll_ctrl) &
		  USBH_PHY_PLL_CTRL_PLL_LOCK_MASK)) && (retries--)) {
		schedule_timeout_interruptible(HZ / 1000);
	}

	if (retries == 0) {
		dbg_printk("ERROR: USBH PLL Lock failed!\n");

		/* Put PLL in reset */
		tmp = readl(&drv_data->ctrl_regs->pll_ctrl);
		writel(tmp & ~USBH_PHY_PLL_CTRL_PLL_RESETB_MASK,
		       &drv_data->ctrl_regs->pll_ctrl);

		/* Put PHYs in reset */
		tmp = readl(&drv_data->ctrl_regs->phy_ctrl);
		writel((tmp & ~USBH_PHY_PHY_CTRL_SW_PHY_RESETB_MASK) |
		       USBH_PHY_PHY_CTRL_PHY_IDDQ_MASK,
		       &drv_data->ctrl_regs->phy_ctrl);

		/* Assert software reset for UTMI and port */
		tmp = readl(&drv_data->ctrl_regs->clkrst_ctrl);
		writel(tmp & ~(USBH_PHY_CLKRST_CTRL_UTMIRESETN_SW_MASK |
			       USBH_PHY_CLKRST_CTRL_RESETN_SW_MASK),
		       &drv_data->ctrl_regs->clkrst_ctrl);

		/* Disable OHCI access */
		tmp = readl(&drv_data->ctrl_regs->clkrst_ctrl);
		writel(tmp & ~USBH_PHY_CLKRST_CTRL_SW_OHCI_ACCESS_EN_MASK,
		       &drv_data->ctrl_regs->clkrst_ctrl);

		return -EFAULT;
	}

	/* Deassert software reset for UTMI and port */
	tmp = readl(&drv_data->ctrl_regs->clkrst_ctrl);
	writel(tmp | USBH_PHY_CLKRST_CTRL_UTMIRESETN_SW_MASK |
	       USBH_PHY_CLKRST_CTRL_RESETN_SW_MASK,
	       &drv_data->ctrl_regs->clkrst_ctrl);
	dbg_printk("Change Clock Reset Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_data->ctrl_regs->clkrst_ctrl));

	/* Deassert soft resets to each port */

	/* Pull port0 out of reset */
	tmp = readl(&drv_data->ctrl_regs->phy_p1ctrl);
	writel((tmp | USBH_PHY_PHY_P1CTRL_SOFT_RESET_N_MASK) &
	       ~USBH_PHY_PHY_P1CTRL_NON_DRIVING_MASK,
	       &drv_data->ctrl_regs->phy_p1ctrl);
	dbg_printk("Change Dual PHY Port 0 Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_data->ctrl_regs->phy_p1ctrl));

	/* Pull port1 out of reset */
	tmp = readl(&drv_data->ctrl_regs->phy_p2ctrl);
	writel((tmp | USBH_PHY_PHY_P2CTRL_SOFT_RESET_N_MASK) &
	       ~USBH_PHY_PHY_P2CTRL_NON_DRIVING_MASK,
	       &drv_data->ctrl_regs->phy_p2ctrl);
	dbg_printk("Change Dual PHY Port 1 Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_data->ctrl_regs->phy_p2ctrl));

	return 0;
}

/*
 * Function to initialize USB host related low level hardware including PHY,
 * clocks, etc.
 *
 * TODO: expand support for more than one host in the future if needed
 */
static int bcm_usbh_init(unsigned int host_index)
{
	int ret;
	struct usbh_priv *drv_data = &usbh_data;
	struct usbh_cfg *hw_cfg;

	dbg_printk("%s: HOST\n", __func__);
	hw_cfg = &drv_data->hw_cfg;

	/* enable clocks */
	ret = usbh_clk_ctrl(drv_data, 1);
	if (ret) {
		dev_err(drv_data->dev,
			"unable to enable one of the HOST clocks\n");
		goto err_exit;
	}

	ret = bcm_usbh_host_init();
	if (ret  < 0)
		goto err_free_gpio;

	return 0;

 err_free_gpio:
	usbh_clk_ctrl(drv_data, 0);

 err_exit:

	return ret;
}

static int __devinit usbh_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *iomem, *ioarea;

	if (pdev->dev.platform_data == NULL) {
		dev_err(&pdev->dev, "platform_data missing\n");
		ret = -EFAULT;
		goto err_exit;
	}

	memset(&usbh_data, 0, sizeof(usbh_data));

	memcpy(&usbh_data.hw_cfg, pdev->dev.platform_data,
	       sizeof(usbh_data.hw_cfg));
	usbh_data.dev = &pdev->dev;

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iomem) {
		dev_err(&pdev->dev, "no mem resource\n");
		ret = -ENODEV;
		goto err_exit;
	}
	dbg_printk("%s: HOST %s %d 0x%08x\n",
		   __func__, pdev->name, pdev->id, iomem->start);

	/* mark the memory region as used */
	ioarea = request_mem_region(iomem->start, resource_size(iomem),
				    pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_exit;
	}

	/* now map the I/O memory */
	usbh_data.ctrl_regs = (struct usbh_ctrl_regs __iomem *)
	    ioremap(iomem->start, sizeof(usbh_data.ctrl_regs));
	if (!usbh_data.ctrl_regs) {
		dev_err(&pdev->dev, "failed to remap registers\n");
		ret = -ENOMEM;
		goto err_free_mem_region;
	}
	dbg_printk("%s: HOST ctrl_regs=0x%08x\n", __func__,
		   (uint32_t) usbh_data.ctrl_regs);

	platform_set_drvdata(pdev, &usbh_data);

	bcm_usbh_init(0);

	printk("Capri USB Host Low-Level Driver: Probe completed sucessfully.\n");
	return 0;

 err_free_mem_region:
	release_mem_region(iomem->start, resource_size(iomem));

 err_exit:
	memset(&usbh_data, 0, sizeof(usbh_data));

	return ret;
}

static int __devexit usbh_remove(struct platform_device *pdev)
{
	struct usbh_priv *drv_data;
	struct resource *iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	drv_data = platform_get_drvdata(pdev);

	/* disable clocks */
	usbh_clk_ctrl(drv_data, 0);

	platform_set_drvdata(pdev, NULL);
	iounmap(drv_data->ctrl_regs);
	release_mem_region(iomem->start, resource_size(iomem));
	memset(&usbh_data, 0, sizeof(usbh_data));

	return 0;
}

static struct platform_driver usbh_driver = {
	.driver = {
		   .name = "usbh",
		   .owner = THIS_MODULE,
		   },
	.remove = __devexit_p(usbh_remove),
};

static char banner[] __initdata =
	KERN_INFO "Capri USB Host Low-Level Driver: 1.0\n";

static int __init usbh_init(void)
{
	printk(banner);

	return platform_driver_probe(&usbh_driver, usbh_probe);
}

static void __exit usbh_exit(void)
{
	platform_driver_unregister(&usbh_driver);
}

module_init(usbh_init);
module_exit(usbh_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Capri USB Host Low-Level Driver");
MODULE_LICENSE("GPL");
