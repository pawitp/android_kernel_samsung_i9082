/*****************************************************************************
* Copyright 2006 - 2011 Broadcom Corporation.  All rights reserved.
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
 * Frameworks:
 *
 *    - SMP:          Fully supported.    Big mutex to keep everything safe.
 *    - GPIO:         Fully supported.    No GPIOs are used.
 *    - MMU:          Fully supported.    Platform model with ioremap used.
 *    - Dynamic /dev: Fully supported.    Uses PWM framework.
 *    - Suspend:      Fully supported.    PMWs disabled and cloks released on suspend.
 *    - Clocks:       Fully supported.    Done.
 *    - Power:        Fully supported.    Clocks disabled when PWMs not in use.
 *
 */

/*
 * KONA PWM driver
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/pwm/pwm.h>
#include <mach/rdb/brcm_rdb_pwm_top.h>
#include <mach/pinmux.h>



#define KONA_PWM_CHANNEL_CNT 6
#define PWM_PRESCALER_MAX    7

DEFINE_MUTEX(pwm_lock);		/* lock for data access */

struct kona_pwmc {
	struct pwm_device *p[KONA_PWM_CHANNEL_CNT];
	struct pwm_device_ops ops;
	void __iomem *iobase;
	struct clk *clk;
	unsigned int clk_cnt;
	struct pin_config *sleep_config;
};

struct pwm_control {
	u32 smooth_type_mask;
	u32 smooth_type_shift;
	u32 pwmout_type_mask;
	u32 pwmout_type_shift;
	u32 pwmout_polarity_mask;
	u32 pwmout_polarity_shift;
	u32 pwmout_enable_mask;
	u32 pwmout_enable_shift;
	u32 offset;
};

struct pwm_reg_def {
	u32 mask;
	u32 shift;
	u32 offset;
};

#define PWM_CONTROL_PROP(chan, stm, sts, ptm, pts, pm, ps, em, es, addr) \
    [chan] = {  \
        .smooth_type_mask                       =       stm, \
        .smooth_type_shift                      =       sts, \
        .pwmout_type_mask                       =       ptm, \
        .pwmout_type_shift                      =       pts, \
        .pwmout_polarity_mask		=       pm,  \
        .pwmout_polarity_shift          =       ps,  \
        .pwmout_enable_mask             =       em,  \
        .pwmout_enable_shift            =       es,  \
        .offset                     =   addr \
    }

#define CHAN_SHIFT(shift,chan) (shift+chan)
#define CHAN_MASK(mask,shift,chan) (mask & ( 1 << CHAN_SHIFT(shift,chan) ) )
#define PWM_CONTROL_SET(chan) \
    PWM_CONTROL_PROP( chan , \
    CHAN_MASK(PWM_TOP_PWM_CONTROL_SMOOTH_TYPE_MASK,PWM_TOP_PWM_CONTROL_SMOOTH_TYPE_SHIFT,chan), \
    CHAN_SHIFT(PWM_TOP_PWM_CONTROL_SMOOTH_TYPE_SHIFT,chan),  \
    CHAN_MASK(PWM_TOP_PWM_CONTROL_PWMOUT_TYPE_MASK,PWM_TOP_PWM_CONTROL_PWMOUT_TYPE_SHIFT,chan), \
    CHAN_SHIFT(PWM_TOP_PWM_CONTROL_PWMOUT_TYPE_SHIFT,chan),  \
    CHAN_MASK(PWM_TOP_PWM_CONTROL_PWMOUT_POLARITY_MASK,PWM_TOP_PWM_CONTROL_PWMOUT_POLARITY_SHIFT,chan), \
    CHAN_SHIFT(PWM_TOP_PWM_CONTROL_PWMOUT_POLARITY_SHIFT,chan),  \
    CHAN_MASK(PWM_TOP_PWM_CONTROL_PWMOUT_ENABLE_MASK,PWM_TOP_PWM_CONTROL_PWMOUT_ENABLE_SHIFT,chan), \
    CHAN_SHIFT(PWM_TOP_PWM_CONTROL_PWMOUT_ENABLE_SHIFT,chan),  \
    PWM_TOP_PWM_CONTROL_OFFSET \
)

#define PWM_REG_DEF(c, m, s, a) \
    [c] = {     \
        .mask           =       m,  \
        .shift          =       s, \
        .offset         =       a \
    }

static const struct pwm_control pwm_chan_ctrl_info[KONA_PWM_CHANNEL_CNT] = {
	PWM_CONTROL_SET(0),
	PWM_CONTROL_SET(1),
	PWM_CONTROL_SET(2),
	PWM_CONTROL_SET(3),
	PWM_CONTROL_SET(4),
	PWM_CONTROL_SET(5)
};

static const struct pwm_reg_def pwm_chan_pre_scaler_info[KONA_PWM_CHANNEL_CNT] = {
	PWM_REG_DEF(0, PWM_TOP_PRESCALE_CONTROL_PWM0_PRESCALE_MASK,
		    PWM_TOP_PRESCALE_CONTROL_PWM0_PRESCALE_SHIFT,
		    PWM_TOP_PRESCALE_CONTROL_OFFSET),
	PWM_REG_DEF(1, PWM_TOP_PRESCALE_CONTROL_PWM1_PRESCALE_MASK,
		    PWM_TOP_PRESCALE_CONTROL_PWM1_PRESCALE_SHIFT,
		    PWM_TOP_PRESCALE_CONTROL_OFFSET),
	PWM_REG_DEF(2, PWM_TOP_PRESCALE_CONTROL_PWM2_PRESCALE_MASK,
		    PWM_TOP_PRESCALE_CONTROL_PWM2_PRESCALE_SHIFT,
		    PWM_TOP_PRESCALE_CONTROL_OFFSET),
	PWM_REG_DEF(3, PWM_TOP_PRESCALE_CONTROL_PWM3_PRESCALE_MASK,
		    PWM_TOP_PRESCALE_CONTROL_PWM3_PRESCALE_SHIFT,
		    PWM_TOP_PRESCALE_CONTROL_OFFSET),
	PWM_REG_DEF(4, PWM_TOP_PRESCALE_CONTROL_PWM4_PRESCALE_MASK,
		    PWM_TOP_PRESCALE_CONTROL_PWM4_PRESCALE_SHIFT,
		    PWM_TOP_PRESCALE_CONTROL_OFFSET),
	PWM_REG_DEF(5, PWM_TOP_PRESCALE_CONTROL_PWM5_PRESCALE_MASK,
		    PWM_TOP_PRESCALE_CONTROL_PWM5_PRESCALE_SHIFT,
		    PWM_TOP_PRESCALE_CONTROL_OFFSET),
};

static const struct pwm_reg_def pwm_chan_period_cnt_info[KONA_PWM_CHANNEL_CNT] = {
	PWM_REG_DEF(0, PWM_TOP_PWM0_PERIOD_COUNT_PWM0_CNT_MASK,
		    PWM_TOP_PWM0_PERIOD_COUNT_PWM0_CNT_SHIFT,
		    PWM_TOP_PWM0_PERIOD_COUNT_OFFSET),
	PWM_REG_DEF(1, PWM_TOP_PWM1_PERIOD_COUNT_PWM1_CNT_MASK,
		    PWM_TOP_PWM1_PERIOD_COUNT_PWM1_CNT_SHIFT,
		    PWM_TOP_PWM1_PERIOD_COUNT_OFFSET),
	PWM_REG_DEF(2, PWM_TOP_PWM2_PERIOD_COUNT_PWM2_CNT_MASK,
		    PWM_TOP_PWM2_PERIOD_COUNT_PWM2_CNT_SHIFT,
		    PWM_TOP_PWM2_PERIOD_COUNT_OFFSET),
	PWM_REG_DEF(3, PWM_TOP_PWM3_PERIOD_COUNT_PWM3_CNT_MASK,
		    PWM_TOP_PWM3_PERIOD_COUNT_PWM3_CNT_SHIFT,
		    PWM_TOP_PWM3_PERIOD_COUNT_OFFSET),
	PWM_REG_DEF(4, PWM_TOP_PWM4_PERIOD_COUNT_PWM4_CNT_MASK,
		    PWM_TOP_PWM4_PERIOD_COUNT_PWM4_CNT_SHIFT,
		    PWM_TOP_PWM4_PERIOD_COUNT_OFFSET),
	PWM_REG_DEF(5, PWM_TOP_PWM5_PERIOD_COUNT_PWM5_CNT_MASK,
		    PWM_TOP_PWM5_PERIOD_COUNT_PWM5_CNT_SHIFT,
		    PWM_TOP_PWM5_PERIOD_COUNT_OFFSET),
};

static const struct pwm_reg_def pwm_chan_duty_cycle_info[KONA_PWM_CHANNEL_CNT] = {
	PWM_REG_DEF(0, PWM_TOP_PWM0_DUTY_CYCLE_HIGH_PWM0_HIGH_MASK,
		    PWM_TOP_PWM0_DUTY_CYCLE_HIGH_PWM0_HIGH_SHIFT,
		    PWM_TOP_PWM0_DUTY_CYCLE_HIGH_OFFSET),
	PWM_REG_DEF(1, PWM_TOP_PWM1_DUTY_CYCLE_HIGH_PWM1_HIGH_MASK,
		    PWM_TOP_PWM1_DUTY_CYCLE_HIGH_PWM1_HIGH_SHIFT,
		    PWM_TOP_PWM1_DUTY_CYCLE_HIGH_OFFSET),
	PWM_REG_DEF(2, PWM_TOP_PWM2_DUTY_CYCLE_HIGH_PWM2_HIGH_MASK,
		    PWM_TOP_PWM2_DUTY_CYCLE_HIGH_PWM2_HIGH_SHIFT,
		    PWM_TOP_PWM2_DUTY_CYCLE_HIGH_OFFSET),
	PWM_REG_DEF(3, PWM_TOP_PWM3_DUTY_CYCLE_HIGH_PWM3_HIGH_MASK,
		    PWM_TOP_PWM3_DUTY_CYCLE_HIGH_PWM3_HIGH_SHIFT,
		    PWM_TOP_PWM3_DUTY_CYCLE_HIGH_OFFSET),
	PWM_REG_DEF(4, PWM_TOP_PWM4_DUTY_CYCLE_HIGH_PWM4_HIGH_MASK,
		    PWM_TOP_PWM4_DUTY_CYCLE_HIGH_PWM4_HIGH_SHIFT,
		    PWM_TOP_PWM4_DUTY_CYCLE_HIGH_OFFSET),
	PWM_REG_DEF(5, PWM_TOP_PWM5_DUTY_CYCLE_HIGH_PWM5_HIGH_MASK,
		    PWM_TOP_PWM5_DUTY_CYCLE_HIGH_PWM5_HIGH_SHIFT,
		    PWM_TOP_PWM5_DUTY_CYCLE_HIGH_OFFSET),
};

static int kona_get_chan(const struct kona_pwmc *ap, const struct pwm_device *p)
{
	int chan;
	for (chan = 0; chan < KONA_PWM_CHANNEL_CNT; chan++)
		if (p == ap->p[chan])
			return chan;
	BUG();
	return 0;
}

static void kona_pwmc_clear_set_bit(const struct kona_pwmc *ap,
				    unsigned int offset, unsigned int shift,
				    unsigned char en_dis)
{
	unsigned long val = readl(ap->iobase + offset);

	// Clear bit.
	clear_bit(shift, &val);

	if (en_dis == 1)
		set_bit(shift, &val);

	writel(val, (ap->iobase + offset));
}

static void kona_pwmc_set_field(const struct kona_pwmc *ap, unsigned int offset,
				unsigned int mask, unsigned int shift,
				unsigned int wval)
{
	unsigned int val = readl(ap->iobase + offset);
	val = (val & ~mask) | (wval << shift);
	writel(val, (ap->iobase + offset));
}

static void kona_pwmc_get_field(const struct kona_pwmc *ap, unsigned int offset,
				unsigned int mask, unsigned int shift,
				unsigned int *val)
{
	*val = readl(ap->iobase + offset);
	*val = (*val & mask) >> shift;
}

static void kona_pwmc_set_smooth(const struct kona_pwmc *ap, int chan,
				 int enable)
{
	kona_pwmc_clear_set_bit(ap, pwm_chan_ctrl_info[chan].offset,
				pwm_chan_ctrl_info[chan].smooth_type_shift,
				enable);
}

static void kona_pwmc_stop(const struct kona_pwmc *ap, int chan)
{
	kona_pwmc_clear_set_bit(ap, pwm_chan_ctrl_info[chan].offset,
				pwm_chan_ctrl_info[chan].pwmout_enable_shift,
				0);
}

static void kona_pwmc_start(const struct kona_pwmc *ap, int chan)
{
	udelay(1);
	kona_pwmc_clear_set_bit(ap, pwm_chan_ctrl_info[chan].offset,
				pwm_chan_ctrl_info[chan].pwmout_enable_shift,
				1);
}

static void kona_pwmc_config_polarity(struct kona_pwmc *ap, int chan,
				      struct pwm_config *c)
{
	if (c->polarity)
		kona_pwmc_clear_set_bit(ap, pwm_chan_ctrl_info[chan].offset,
					pwm_chan_ctrl_info
					[chan].pwmout_polarity_shift, 1);
	else
		kona_pwmc_clear_set_bit(ap, pwm_chan_ctrl_info[chan].offset,
					pwm_chan_ctrl_info
					[chan].pwmout_polarity_shift, 0);

}

static void kona_pwmc_config_duty_ticks(struct kona_pwmc *ap, int chan,
					struct pwm_config *c)
{
	unsigned int pre_scaler = 0;
	unsigned int duty_cnt = 0;

	kona_pwmc_get_field(ap, pwm_chan_pre_scaler_info[chan].offset,
			    pwm_chan_pre_scaler_info[chan].mask,
			    pwm_chan_pre_scaler_info[chan].shift, &pre_scaler);

	// Read prescaler value from register.
	duty_cnt = c->duty_ticks / (pre_scaler + 1);

	// program duty cycle.
	kona_pwmc_set_field(ap, pwm_chan_duty_cycle_info[chan].offset,
			    pwm_chan_duty_cycle_info[chan].mask,
			    pwm_chan_duty_cycle_info[chan].shift, duty_cnt);

}

static int kona_pwmc_config_period_ticks(struct kona_pwmc *ap, int chan,
					 struct pwm_config *c)
{
	unsigned int pcnt;
	unsigned char pre_scaler = 0;

	// pcnt = ( 26 * 1000000 * period_ns ) / (pre_scaler * 1000000000 )
	// Calculate period cnt.
	pre_scaler = c->period_ticks / 0xFFFFFF;
	if (pre_scaler > PWM_PRESCALER_MAX)
		pre_scaler = PWM_PRESCALER_MAX;

	pcnt = c->period_ticks / (pre_scaler + 1);

	// program prescaler
	kona_pwmc_set_field(ap, pwm_chan_pre_scaler_info[chan].offset,
			    pwm_chan_pre_scaler_info[chan].mask,
			    pwm_chan_pre_scaler_info[chan].shift, pre_scaler);

	// program period count.
	kona_pwmc_set_field(ap, pwm_chan_period_cnt_info[chan].offset,
			    pwm_chan_period_cnt_info[chan].mask,
			    pwm_chan_period_cnt_info[chan].shift, pcnt);

	return 0;
}

static int kona_pwmc_config(struct pwm_device *p, struct pwm_config *c)
{
	struct kona_pwmc *ap = pwm_get_drvdata(p);
	int chan = kona_get_chan(ap, p);
	int ret = 0;

	if (test_bit(PWM_CONFIG_POLARITY, &c->config_mask))
		p->polarity = c->polarity ? 1 : 0;

	if (test_bit(PWM_CONFIG_PERIOD_TICKS, &c->config_mask))
		p->period_ticks = c->period_ticks;

	if (test_bit(PWM_CONFIG_DUTY_TICKS, &c->config_mask))
		p->duty_ticks = c->duty_ticks;

	if (test_bit(PWM_CONFIG_START, &c->config_mask)) {
		p->flags |= PWM_FLAG_RUNNING;
		p->flags &= ~PWM_FLAG_STOP;

		mutex_lock(&pwm_lock);
		clk_enable(ap->clk);
		ap->clk_cnt++;
		kona_pwmc_set_smooth(ap, chan, 1);
		mutex_unlock(&pwm_lock);
	}

	if (test_bit(PWM_CONFIG_STOP, &c->config_mask)) {
		struct pwm_config d = {
			.config_mask =
			PWM_CONFIG_DUTY_TICKS | PWM_CONFIG_PERIOD_TICKS |
			PWM_CONFIG_POLARITY,
			.duty_ticks = 0,
			.period_ticks = 0,
			.polarity = 1,
		};

		p->flags |= PWM_FLAG_STOP;
		p->flags &= ~PWM_FLAG_RUNNING;

		mutex_lock(&pwm_lock);
		kona_pwmc_set_smooth(ap, chan, 0);
		kona_pwmc_stop(ap, chan);
		kona_pwmc_config_polarity(ap, chan, &d);
		kona_pwmc_config_duty_ticks(ap, chan, &d);
		kona_pwmc_config_period_ticks(ap, chan, &d);
		kona_pwmc_start(ap, chan);
		if (ap->clk_cnt == 0)
			pr_err("%s: clock already off!\n",  __func__);
		else {
			clk_disable(ap->clk);
			ap->clk_cnt--;
		}
		mutex_unlock(&pwm_lock);
	}

	if (p->flags & PWM_FLAG_RUNNING) {
		struct pwm_config d = {
			.config_mask =
			PWM_CONFIG_DUTY_TICKS | PWM_CONFIG_PERIOD_TICKS |
			PWM_CONFIG_POLARITY,
			.duty_ticks = p->duty_ticks,
			.period_ticks = p->period_ticks,
			.polarity = p->polarity,
		};

		mutex_lock(&pwm_lock);
		kona_pwmc_stop(ap, chan);
		kona_pwmc_config_polarity(ap, chan, &d);
		kona_pwmc_config_duty_ticks(ap, chan, &d);
		kona_pwmc_config_period_ticks(ap, chan, &d);
		kona_pwmc_start(ap, chan);
		mutex_unlock(&pwm_lock);
	}

	return ret;
}

static const struct pwm_device_ops kona_pwm_ops = {
	.config = kona_pwmc_config,
	.owner = THIS_MODULE,
};

static int __devinit kona_pwmc_probe(struct platform_device *pdev)
{
	struct kona_pwmc *ap;
	struct resource *r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	int chan = 0, tick_hz;
	int ret = 0;

	ap = devm_kzalloc(&pdev->dev, sizeof(*ap), GFP_KERNEL);
	if (!ap) {
		ret = -ENOMEM;
		goto err_clk_get;
	}

	platform_set_drvdata(pdev, ap);
	ap->clk = clk_get(&pdev->dev, "pwm_clk");
	if (IS_ERR(ap->clk)) {
		ret = -ENODEV;
		devm_kfree(&pdev->dev, ap);
		goto err_clk_get;
	}

	/* clk is off by default by clk driver */
	ap->clk_cnt = 0;
	tick_hz = clk_get_rate(ap->clk);

	ap->iobase = ioremap_nocache(r->start, resource_size(r));
	if (!ap->iobase) {
		ret = -ENODEV;
		goto err_ioremap;
	}

	ap->sleep_config = pdev->dev.platform_data;

	if (!ap->sleep_config) {
		dev_err(&pdev->dev, "failed to find sleep configuration\n");
		goto err_sleep_config;
	}

	for (chan = 0; chan < KONA_PWM_CHANNEL_CNT; chan++) {

		struct pwm_config d = {
			.config_mask =
			PWM_CONFIG_DUTY_TICKS | PWM_CONFIG_PERIOD_TICKS |
			PWM_CONFIG_POLARITY,
			.duty_ticks = 0,
			.period_ticks = 0,
			.polarity = 1,
		};

		ap->p[chan] = pwm_register(&kona_pwm_ops, &pdev->dev, "%s:%d",
					   dev_name(&pdev->dev), chan);
		if (IS_ERR_OR_NULL(ap->p[chan]))
			goto err_pwm_register;
		pwm_set_drvdata(ap->p[chan], ap);

		/* Init all channels to a know state and leave clock disabled. */
		kona_pwmc_set_smooth(ap, chan, 1);
		kona_pwmc_stop(ap, chan);
		kona_pwmc_config_polarity(ap, chan, &d);
		kona_pwmc_config_duty_ticks(ap, chan, &d);
		kona_pwmc_config_period_ticks(ap, chan, &d);
		kona_pwmc_start(ap, chan);

		ap->p[chan]->tick_hz = tick_hz;
	}

	printk(KERN_INFO "PWM: driver initialized properly");

	return 0;

err_pwm_register:
	while (--chan >= 0)
		pwm_unregister(ap->p[chan]);
err_sleep_config:
	iounmap(ap->iobase);
err_ioremap:
	clk_put(ap->clk);
err_clk_get:
	platform_set_drvdata(pdev, NULL);
	printk(KERN_ERR "%s: error, returning %d\n", __func__, ret);
	return ret;
}

static int __devexit kona_pwmc_remove(struct platform_device *pdev)
{
	struct kona_pwmc *ap = platform_get_drvdata(pdev);
	int chan;

	for (chan = 0; chan < KONA_PWM_CHANNEL_CNT; chan++)
		pwm_unregister(ap->p[chan]);

	clk_put(ap->clk);
	iounmap(ap->iobase);

	return 0;
}

#ifdef CONFIG_PM
static int kona_pwmc_suspend(struct platform_device *pdev, pm_message_t state)
{
	int chan;
	struct kona_pwmc *ap = platform_get_drvdata(pdev);
	struct pin_config config;

	if (ap->clk_cnt) {
		pr_err("%s: unbalanced clock found! %d\n",
			__func__, ap->clk_cnt);
		/* Make sure clock driver's ref count be 0 */
		chan = ap->clk_cnt;
		do {
			clk_disable(ap->clk);
		} while (--chan);
	}

	/* Save current config, change to sleep config */
	for (chan = 0; chan < KONA_PWM_CHANNEL_CNT; chan++) {
		config.name =  ap->sleep_config[chan].name;
		if (config.name < PN_MAX) {
			pinmux_get_pin_config(&config);
			pr_debug("%s: val=%u\n", __func__, config.reg.val);
			pinmux_set_pin_config(&ap->sleep_config[chan]);
			ap->sleep_config[chan] = config;
		}
	}
	return 0;
}

static int kona_pwmc_resume(struct platform_device *pdev)
{
	int chan;
	struct kona_pwmc *ap = platform_get_drvdata(pdev);
	struct pin_config config;

	/* Resore the saved configuration */
	for (chan = 0; chan < KONA_PWM_CHANNEL_CNT; chan++) {
		config.name =  ap->sleep_config[chan].name;
		if (config.name < PN_MAX) {
			pinmux_get_pin_config(&config);
			pr_debug("%s: val=%u\n", __func__, config.reg.val);
			pinmux_set_pin_config(&ap->sleep_config[chan]);
			ap->sleep_config[chan] = config;
		}
	}
	if (ap->clk_cnt) {
		pr_err("%s: restore unbalanced clock!\n", __func__);
		/* Make sure clock is restored for bad user(s) */
		chan = ap->clk_cnt;
		do {
			clk_enable(ap->clk);
		} while (--chan);
	}
	return 0;
}
#else
#define kona_pwmc_suspend    NULL
#define kona_pwmc_resume     NULL
#endif


static struct platform_driver kona_pwmc_driver = {
	.driver = {
		   .name = "kona_pwmc",
		   .owner = THIS_MODULE,
		   },
	.remove = __devexit_p(kona_pwmc_remove),
	.suspend = kona_pwmc_suspend,
	.resume = kona_pwmc_resume,
};

static const __devinitconst char gBanner[] =
    KERN_INFO "Broadcom Pulse Width Modulator Driver: 1.00\n";
static int __init kona_pwmc_init(void)
{
	printk(gBanner);
	return platform_driver_probe(&kona_pwmc_driver, kona_pwmc_probe);
}

static void __exit kona_pwmc_exit(void)
{
	platform_driver_unregister(&kona_pwmc_driver);
}

module_init(kona_pwmc_init);
module_exit(kona_pwmc_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("Driver for KONA PWMC");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
