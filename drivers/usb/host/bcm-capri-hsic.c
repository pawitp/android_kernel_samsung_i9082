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
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>

#include <mach/pinmux.h>
#include <mach/usbh_cfg.h>

#include <mach/rdb/brcm_rdb_hsic_phy.h>

#define HSIC_TEST

#ifdef DEBUG
#define dbg_printk(fmt, args...) printk(KERN_INFO "%s: " fmt, __func__, ## args)
#else
#define dbg_printk(fmt, args...)
#endif

struct usbh_hsic_ctrl_regs {
	u32 clkrst_ctrl;
	u32 core_strap_ctrl;
	u32 ss_fladj_val_host_i;
	u32 ss_fladj_val[USBH_HSIC_NUM_PORTS];
	u32 hsic_ldo_ctrl;
	u32 hsic_phy_ctrl;
	u32 hsic_cfg1;
	u32 hsic_cfg2;
	u32 hsic_pll_ctrl;
	u32 test_mux_sel;
	u32 test_mux_out;
	u32 hsic_sts;
};

struct usbh_hsic_priv {
	struct device *dev;
	struct usbh_cfg hw_cfg;
	struct clk *peri_clk;
	struct clk *ahb_clk;
	struct clk *opt_clk;
	struct usbh_hsic_ctrl_regs __iomem *ctrl_regs;
};

static struct usbh_hsic_priv usbh_hsic_data;

static int usbh_hsic_clk_ctrl(struct usbh_hsic_priv *drv_data, int enable)
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

		dbg_printk(KERN_INFO "USB HSIC: Clocks enabled!\n");
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
	dbg_printk(KERN_INFO "USB HSIC: Clocks disabled!\n");
	return 0;
}

#if defined(CONFIG_MACH_CAPRI_FPGA)
static int bcm_usbh_hsic_init(struct usbh_cfg *hw_cfg)
{
	int ret;
	struct usbh_hsic_priv *drv_hsic_data = &usbh_hsic_data;
	int retries;
	uint32_t tmp;

	/* Request GPIOs */
	gpio_free(57);
	gpio_free(58);
	gpio_free(59);
	gpio_free(61);
	gpio_free(62);
	gpio_free(63);
	ret = gpio_request(57, "HSICp0 device reset")
	if (ret < 0) {
		dbg_printk("GPIO57 requests failed\n");
		return ret;
	}
	ret = gpio_request(58, "HSICp0 PHY reset");
	if (ret < 0) {
		dbg_printk("GPIO58 requests failed\n");
		return ret;
	}
	ret = gpio_request(59, "HSICp0 Evatronix patch");
	if (ret < 0) {
		dbg_printk("GPIO59 requests failed\n");
		return ret;
	}
	ret = gpio_request(61, "HSICp1 device reset");
	if (ret < 0) {
		dbg_printk("GPIO61 requests failed\n");
		return ret;
	}
	ret = gpio_request(62, "HSICp1 PHY reset");
	if (ret < 0) {
		dbg_printk("GPIO62 requests failed\n");
		return ret;
	}
	ret = gpio_request(63, "HSICp1 Evatronix patch");
	if (ret < 0) {
		dbg_printk("GPIO63 requests failed\n");
		return ret;
	}

	/* GPIOs 57, 58, 59, 61, 62, and 63 to output mode */

	/* Disable Evatronix 'patch'; clear GPIO59 and GPIO63 */
	gpio_direction_output(59, 0);
	gpio_set_value(59, 0);
	gpio_direction_output(63, 0);
	gpio_set_value(63, 0);

	/* Put device in reset mode; clear (assert low) GPIO57 and GPIO61 */
	gpio_direction_output(57, 1);
	gpio_set_value(57, 0);
	gpio_direction_output(61, 1);
	gpio_set_value(61, 0);

	/* Put Evatronix PHY in reset mode; set (assert high)
	* GPIO58 and GPIO62
	*/
	gpio_direction_output(58, 1);
	gpio_set_value(58, 1);
	gpio_direction_output(62, 1);
	gpio_set_value(62, 1);

	/* Enable 48MHz reference clock to PHY */
	tmp = readl(&drv_hsic_data->ctrl_regs->clkrst_ctrl);
	writel(tmp | HSIC_PHY_CLKRST_CTRL_CLK48_REQ_MASK,
	       &drv_hsic_data->ctrl_regs->clkrst_ctrl);
	dbg_printk("Change PHY Clock & Reset Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->clkrst_ctrl));

	/* Enable LDO */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_ldo_ctrl);
	writel(tmp | HSIC_PHY_HSIC_LDO_CTRL_LDO_EN_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_ldo_ctrl);
	dbg_printk("Change PHY AFE Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_ldo_ctrl));

	mdelay(1);

	/* Deassert power downs */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	writel(tmp | HSIC_PHY_HSIC_PHY_CTRL_UTMI_PWRDNB_MASK |
	       HSIC_PHY_HSIC_PHY_CTRL_PHY_PWRDNB_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	dbg_printk("Change HSIC PHY Control from 0x%08x to 0x%08x\n", tmp,
		   readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl));

	mdelay(1);

	/* Deassert PLL power down */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_pll_ctrl);
	writel(tmp | HSIC_PHY_HSIC_PLL_CTRL_PLL_POWERDOWNB_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_pll_ctrl);
	dbg_printk("Change HSIC PLL Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_pll_ctrl));

	/* Deassert ISO */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	writel(tmp & ~HSIC_PHY_HSIC_PHY_CTRL_PHY_ISO_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	dbg_printk("Change HSIC PHY Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl));

	mdelay(1);

	/* Pull PHY out of reset */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	writel(tmp | HSIC_PHY_HSIC_PHY_CTRL_RESETB_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	dbg_printk("Change HSIC PHY Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl));

	/* Deassert PHY reset - Clear GPIO58 & GPIO62 */
	gpio_set_value(58, 0);
	gpio_set_value(62, 0);

	/* Pull PLL out of reset */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_pll_ctrl);
	writel(tmp & ~HSIC_PHY_HSIC_PLL_CTRL_PLL_RESET_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_pll_ctrl);
	dbg_printk("Change HSIC PLL Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_pll_ctrl));

	/* Deassert device reset - Set GPIO57 & GPIO61 */
	gpio_set_value(57, 1);
	gpio_set_value(61, 1);

	/* Wait for PLL lock */
	retries = 100;
	while ((!(readl(&drv_hsic_data->ctrl_regs->hsic_pll_ctrl) &
		  HSIC_PHY_HSIC_PLL_CTRL_PLL_LOCK_MASK)) && retries--) {
		schedule_timeout_interruptible(HZ / 1000);
	}

	if (retries == 0)
		dbg_printk("ERROR: HSIC PLL Lock failed!\n");

	/* Deassert software reset for UTMI and port */
	tmp = readl(&drv_hsic_data->ctrl_regs->clkrst_ctrl);
	writel(tmp |
	       HSIC_PHY_CLKRST_CTRL_UTMIRESETN_SW_MASK |
	       HSIC_PHY_CLKRST_CTRL_RESETN_SW_MASK,
	       &drv_hsic_data->ctrl_regs->clkrst_ctrl);
	dbg_printk("Change HSIC Clock Reset Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->clkrst_ctrl));

	/* Deassert soft resets to the PHY */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	writel((tmp | HSIC_PHY_HSIC_PHY_CTRL_SOFT_RESETB_MASK) &
	       ~HSIC_PHY_HSIC_PHY_CTRL_NON_DRIVING_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	dbg_printk("Change HSIC PHY Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl));

	/* Enable Evatronix 'patch'.  Set GPIO59 and GPIO63 */
	gpio_set_value(59, 1);
	gpio_set_value(63, 1);

	return 0;
}
#else				/* #if defined(CONFIG_MACH_CAPRI_FPGA) */
static int bcm_usbh_hsic_init(struct usbh_cfg *hw_cfg)
{
	int ret;
	struct usbh_hsic_priv *drv_hsic_data = &usbh_hsic_data;
	int retries;
	uint32_t tmp;
	int i;

#if defined(HSIC_TEST)
	for (i = 0; i < hw_cfg->num_ports; i++) {
		struct usbh_port_cfg *port = &hw_cfg->port[i];

		dbg_printk("%s: HSIC.%d - reset_gpio=%d\n",
			   __func__, i, port->reset_gpio);

		if (port->reset_gpio >= 0) {
			/* Enable second port and setup pinmux. */
			gpio_free(port->reset_gpio);
			if (i == 1) {
				struct pin_config pin_cfg =
				    PIN_CFG(SRI_E, GPIO_032,
				    0, ON, OFF, 0, 0, 8MA);

				/* Configure GPIO04 for port1
				* power & enable MIC2015
				*/
				pinmux_set_pin_config(&pin_cfg);
			}

			ret = gpio_request(port->reset_gpio,
				 "HSIC reset");
			if (ret < 0) {
				dbg_printk("GPIO%d request failed\n",
					   port->reset_gpio);
				goto err_hsic_init_cleanup;
			}
			gpio_direction_output(port->reset_gpio, 0);
		}
	}
#endif				/* #if defined(CONFIG_MACH_CAPRI_FPGA) */

	/* Enable 48MHz reference clock to PHY */
	tmp = readl(&drv_hsic_data->ctrl_regs->clkrst_ctrl);
	writel(tmp | HSIC_PHY_CLKRST_CTRL_CLK48_REQ_MASK,
	       &drv_hsic_data->ctrl_regs->clkrst_ctrl);
	dbg_printk("Change PHY Clock & Reset Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->clkrst_ctrl));

	/* Enable LDO */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_ldo_ctrl);
	writel(tmp | HSIC_PHY_HSIC_LDO_CTRL_LDO_EN_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_ldo_ctrl);
	dbg_printk("Change PHY AFE Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_ldo_ctrl));

	mdelay(1);

	/* Deassert power downs */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	writel(tmp | HSIC_PHY_HSIC_PHY_CTRL_UTMI_PWRDNB_MASK |
	       HSIC_PHY_HSIC_PHY_CTRL_PHY_PWRDNB_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	dbg_printk("Change HSIC PHY Control from 0x%08x to 0x%08x\n", tmp,
		   readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl));

	mdelay(1);

	/* Deassert PLL power down */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_pll_ctrl);
	writel(tmp | HSIC_PHY_HSIC_PLL_CTRL_PLL_POWERDOWNB_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_pll_ctrl);
	dbg_printk("Change HSIC PLL Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_pll_ctrl));

	/* Deassert ISO */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	writel(tmp & ~HSIC_PHY_HSIC_PHY_CTRL_PHY_ISO_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	dbg_printk("Change HSIC PHY Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl));

	mdelay(1);

	/* Pull PHY out of reset */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	writel(tmp | HSIC_PHY_HSIC_PHY_CTRL_RESETB_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	dbg_printk("Change HSIC PHY Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl));

	/* Pull PLL out of reset */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_pll_ctrl);
	writel(tmp & ~HSIC_PHY_HSIC_PLL_CTRL_PLL_RESET_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_pll_ctrl);
	dbg_printk("Change HSIC PLL Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_pll_ctrl));

	/* Wait for PLL lock */
	retries = 100;
	while ((!(readl(&drv_hsic_data->ctrl_regs->hsic_pll_ctrl) &
		  HSIC_PHY_HSIC_PLL_CTRL_PLL_LOCK_MASK)) && retries--) {
		schedule_timeout_interruptible(HZ / 1000);
	}

	if (retries == 0)
		dbg_printk("ERROR: HSIC PLL Lock failed!\n");

	/* Deassert software reset for UTMI and port */
	tmp = readl(&drv_hsic_data->ctrl_regs->clkrst_ctrl);
	writel(tmp |
	       HSIC_PHY_CLKRST_CTRL_UTMIRESETN_SW_MASK |
	       HSIC_PHY_CLKRST_CTRL_RESETN_SW_MASK,
	       &drv_hsic_data->ctrl_regs->clkrst_ctrl);
	dbg_printk("Change HSIC Clock Reset Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->clkrst_ctrl));

	/* Deassert soft resets to the PHY */
	tmp = readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	writel((tmp | HSIC_PHY_HSIC_PHY_CTRL_SOFT_RESETB_MASK) &
	       ~HSIC_PHY_HSIC_PHY_CTRL_NON_DRIVING_MASK,
	       &drv_hsic_data->ctrl_regs->hsic_phy_ctrl);
	dbg_printk("Change HSIC PHY Control from 0x%08x to 0x%08x\n",
		   tmp, readl(&drv_hsic_data->ctrl_regs->hsic_phy_ctrl));

#if defined(HSIC_TEST)
	for (i = 0; i < hw_cfg->num_ports; i++) {
		struct usbh_port_cfg *port = &hw_cfg->port[i];

		dbg_printk("%s: HSIC.%d - Pull out of reset. reset_gpio=%d\n",
			   __func__, i, port->reset_gpio);

		if (port->reset_gpio >= 0)
			gpio_set_value(port->reset_gpio, 1);
	}
	return 0;

 err_hsic_init_cleanup:
	for (; i >= 0; i--) {
		struct usbh_port_cfg *port = &hw_cfg->port[i];

		dbg_printk("%s: HSIC.%d - Free reset_gpio=%d\n",
			   __func__, i, port->reset_gpio);

		if (port->reset_gpio >= 0)
			gpio_free(port->reset_gpio);
	}
#endif				/* #if defined(HSIC_TEST) */
	return 0;
}
#endif				/* #if defined(CONFIG_MACH_CAPRI_FPGA) */

/*
 * Function to initialize USB host related low level hardware including PHY,
 * clocks, etc.
 *
 * TODO: expand support for more than one host in the future if needed
 */
static int bcm_usbhsic_init(unsigned int host_index)
{
	int ret;
	struct usbh_hsic_priv *drv_hsic_data = &usbh_hsic_data;
	struct usbh_cfg *hw_cfg;

	dbg_printk("%s: HSIC\n", __func__);
	hw_cfg = &drv_hsic_data->hw_cfg;

	/* enable clocks */
	ret = usbh_hsic_clk_ctrl(drv_hsic_data, 1);
	if (ret) {
		dev_err(drv_hsic_data->dev,
			"unable to enable one of the HSIC clocks\n");
		goto err_exit;
	}

	ret = bcm_usbh_hsic_init(hw_cfg);
	if (ret < 0)
		goto err_free_gpio;

	return 0;

 err_free_gpio:
	usbh_hsic_clk_ctrl(drv_hsic_data, 0);

 err_exit:

	return ret;
}

static int __devinit usbhsic_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *iomem, *ioarea;

	if (pdev->dev.platform_data == NULL) {
		dev_err(&pdev->dev, "platform_data missing\n");
		ret = -EFAULT;
		goto err_exit;
	}

	memset(&usbh_hsic_data, 0, sizeof(usbh_hsic_data));

	memcpy(&usbh_hsic_data.hw_cfg, pdev->dev.platform_data,
	       sizeof(usbh_hsic_data.hw_cfg));
	usbh_hsic_data.dev = &pdev->dev;

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iomem) {
		dev_err(&pdev->dev, "no mem resource\n");
		ret = -ENODEV;
		goto err_exit;
	}
	dbg_printk("%s: HSIC %s %d 0x%08x\n",
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
	usbh_hsic_data.ctrl_regs = (struct usbh_hsic_ctrl_regs __iomem *)
	    ioremap(iomem->start, sizeof(usbh_hsic_data.ctrl_regs));
	if (!usbh_hsic_data.ctrl_regs) {
		dev_err(&pdev->dev, "failed to remap registers\n");
		ret = -ENOMEM;
		goto err_free_mem_region;
	}

	platform_set_drvdata(pdev, &usbh_hsic_data);

	bcm_usbhsic_init(0);

	printk("Capri USB HSIC Low-Level Driver: Probe completed sucessfully.\n");
	return 0;

 err_free_mem_region:
	release_mem_region(iomem->start, resource_size(iomem));

 err_exit:

	memset(&usbh_hsic_data, 0, sizeof(usbh_hsic_data));

	return ret;
}

static int __devexit usbhsic_remove(struct platform_device *pdev)
{
	struct usbh_hsic_priv *drv_hsic_data;
	struct resource *iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	drv_hsic_data = platform_get_drvdata(pdev);

	/* disable clocks */
	usbh_hsic_clk_ctrl(drv_hsic_data, 0);

	platform_set_drvdata(pdev, NULL);
	iounmap(drv_hsic_data->ctrl_regs);
	release_mem_region(iomem->start, resource_size(iomem));
	memset(&usbh_hsic_data, 0, sizeof(usbh_hsic_data));

	return 0;
}

static struct platform_driver usbhsic_driver = {
	.driver = {
		   .name = "usbhsic",
		   .owner = THIS_MODULE,
		   },
	.remove = __devexit_p(usbhsic_remove),
};

static char banner[] __initdata =
	KERN_INFO "Capri USB HSIC Low-Level Driver: 1.0\n";

static int __init usbhsic_init(void)
{
	printk(banner);

	return platform_driver_probe(&usbhsic_driver, usbhsic_probe);
}

static void __exit usbhsic_exit(void)
{
	platform_driver_unregister(&usbhsic_driver);
}

module_init(usbhsic_init);
module_exit(usbhsic_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Capri USB HSIC Low-Level Driver");
MODULE_LICENSE("GPL");
