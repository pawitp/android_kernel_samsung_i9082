/*******************************************************************************
* Copyright 2010 Broadcom Corporation.  All rights reserved.
*
*	@file	arch/arm/plat-bcmap/sysfs.c
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

/*
 * SYSFS infrastructure specific Broadcom SoCs
 */
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <plat/kona_reset_reason.h>

#ifdef CONFIG_KONA_TIMER_UNIT_TESTS
#include <mach/kona_timer.h>

static atomic_t timer_exp_count = ATOMIC_INIT(0);

#define TIMER_MEASURE_MAX  1000
struct timer_measure {
	spinlock_t lock;
	int busy;
	int counter;
	unsigned long req_cycle[TIMER_MEASURE_MAX];
	u64 start[TIMER_MEASURE_MAX];
	u64 end[TIMER_MEASURE_MAX];
};
static struct timer_measure *timer_m;
#endif

static struct kobject *bcm_kobj;

static char *str_reset_reason[] = {
	"power_on_reset",
	"soft_reset",
	"charging",
	"ap_only",
	"bootloader",
	"recovery",
#ifdef CONFIG_BCM_RTC_ALARM_BOOT
	"rtc_alarm",
#endif

	"unknown"
};

static void set_emu_reset_reason(unsigned int const emu, int val)
{
	void __iomem *rst_addr = ioremap(emu, 0x4);
	unsigned int soc0;

	if (!rst_addr)
		return;

	soc0 = readl(rst_addr);
	soc0 &= ~(0xf);
	soc0 |= val;
	writel(soc0, rst_addr);

	pr_debug("%s: Reset reason: 0x%x", __func__, readl(rst_addr));

	iounmap(rst_addr);
}

static unsigned int get_emu_reset_reason(unsigned int const emu)
{
	void __iomem *rst_addr = ioremap(emu, 0x4);
	unsigned int rst;

	if (!rst_addr)
		return 0;

	rst = readl(rst_addr) & 0xf;

	pr_debug("%s: reset_reason 0x%x\n", __func__, rst);

	iounmap(rst_addr);

	return rst;
}

unsigned int is_charging_state(void)
{
	unsigned int state;

	state = get_emu_reset_reason(REG_EMU_AREA);

	state = state & 0xf;

	pr_debug("%s\n reset reason = 0x%x", __func__, state);
	return (state == CHARGING_STATE) ? 1 : 0;
}


void do_set_recovery_boot(void)
{
	pr_info("%s\n", __func__);
	set_emu_reset_reason(REG_EMU_AREA, RECOVERY_BOOT);
}
EXPORT_SYMBOL(do_set_recovery_boot);

void do_set_ap_only_boot(void)
{
	pr_debug("%s\n", __func__);
	set_emu_reset_reason(REG_EMU_AREA, AP_ONLY_BOOT);
}
EXPORT_SYMBOL(do_set_ap_only_boot);

void do_set_bootloader_boot(void)
{
	pr_debug("%s\n", __func__);
	set_emu_reset_reason(REG_EMU_AREA, BOOTLOADER_BOOT);
}
EXPORT_SYMBOL(do_set_bootloader_boot);

void do_clear_boot_reason(void)
{
	pr_debug("%s\n", __func__);
	set_emu_reset_reason(REG_EMU_AREA, POR_BOOT);
}
EXPORT_SYMBOL(do_clear_boot_reason);


/**
 * This API checks to see if kernel boot is done for AP_ONLY mode
 * Return Values:
 * 1 = ap-only mode
 * 0 = AP + CP mode
 *
 */
unsigned int is_ap_only_boot(void)
{
	unsigned int rst;

	rst = get_emu_reset_reason(REG_EMU_AREA);
	rst = rst & 0xf;

	pr_debug("%s\n reset_reason = 0x%x", __func__, rst);
	return (rst == AP_ONLY_BOOT) ? 1 : 0;
}
EXPORT_SYMBOL(is_ap_only_boot);

static ssize_t
reset_reason_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int index, rst;

	rst = get_emu_reset_reason(REG_EMU_AREA);

	switch (rst) {
	case 0x1:
		index = 0;
		break;
	case 0x3:
		index = 2;
		break;
	case 0x4:
		index = 3;
		break;
	case 0x6:
		index = 5;
		break;
#ifdef CONFIG_BCM_RTC_ALARM_BOOT
	case 0x7:
		index = 6;
		break;
#endif
	default:
		index = 0;
	}

	pr_debug("%s: reset reason index %d\n", __func__, index);
	sprintf(buf, "%s\n", str_reset_reason[index]);

	return strlen(str_reset_reason[index]) + 1;
}

static ssize_t
reset_reason_store(struct device *dev, struct device_attribute *attr,
		   const char *buf, size_t n)
{
	char reset_reason[32];
	int i;


	if (sscanf(buf, "%s", reset_reason) == 1) {
		pr_debug("%s: Reset reason: %s", __func__, reset_reason);

		for (i = 0; i < ARRAY_SIZE(str_reset_reason); i++) {
			if (strcmp(reset_reason, str_reset_reason[i]) == 0)
				break;
		}

		set_emu_reset_reason(REG_EMU_AREA, (i + 1));

		return n;
	}

	return -EINVAL;
}

static DEVICE_ATTR(reset_reason, 0664, reset_reason_show, reset_reason_store);

#ifdef CONFIG_KONA_TIMER_UNIT_TESTS
static ssize_t
kona_timer_module_cfg(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t n)
{
	char         name[255];
	unsigned int rate;

	if (sscanf(buf, "%s %d", name, &rate) == 2) {

		pr_info("timer name:%s rate %d\n", name, rate);

		/*
		 * Assuming that kona_timer_modules_init has happend already
		 * This is safe because this function is called during
		 * system timer init itself
		 */
		if (kona_timer_module_set_rate(name, rate) < 0) {
			pr_err("kona_timer_module_cfg: Unable to set rate\n");
			return n;
		}

		pr_info("kona_timer_module_cfg: Configured the module with "
		"rate %d\n", rate);

		return n;
	}

	pr_info("\nusage: echo [timer_name(aon-timer/slave-timer)]"
	"[rate 32000 (32KHz), 1000000 (1MHz), 19500000 (19.5MHz)] > /sys/bcm/timer_module_cfg\n");

	return -EINVAL;
}

static struct kona_timer *kt;
static struct timer_ch_cfg cfg;

/* Note that this is called back from ISR context */
static int timer_callback(void *p)
{
	unsigned long flags;
	struct kona_timer *lkt = (struct kona_timer *)p;
	u64 exp_time;

	exp_time = (u64)kona_timer_get_counter(lkt);

	if (timer_m) {
		atomic_inc(&timer_exp_count);

		/* SMP safe handling, timer_m can be modified in irq isr.
		 * Consider to be no interrupt on unit test.
		 * Just count interrupt only.
		 */
		if (timer_m->busy)
			goto ret;

		spin_lock_irqsave(&timer_m->lock, flags);
		timer_m->end[timer_m->counter] = exp_time;

		if (lkt->cfg.mode == MODE_PERIODIC &&
		   timer_m->counter < TIMER_MEASURE_MAX - 1) {
			timer_m->counter++;

			timer_m->start[timer_m->counter] =
				timer_m->end[timer_m->counter-1];
		}
		spin_unlock_irqrestore(&timer_m->lock, flags);

		pr_debug("%s: curr: %lld\n", __func__, exp_time);
	} else {
		pr_info("%s: curr: %lld\n", __func__, exp_time);
	}
ret:
	return 0;
}

static ssize_t
kona_timer_start_test(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t n)
{
	unsigned int ch_num, mode, count;
	char name[255];

	if (sscanf(buf, "%s %d %d %d", name, &ch_num, &mode, &count) == 4) {
		pr_info("channel_num:%d mode(0-periodic 1-oneshot):%d "
			"count:%d\n", ch_num, mode, count);

		if (kt == NULL)
			kt = kona_timer_request(name, ch_num);

		if (kt == NULL) {
			pr_err("kona_timer_request returned error\n");
			goto out;
		}

		cfg.mode = mode;
		cfg.arg  = kt;
		cfg.cb	 = timer_callback;
		cfg.reload = count;

		if (kona_timer_config(kt, &cfg) < 0) {
			pr_err("kona_timer_config returned error\n");
			goto out;
		}

		if (kona_timer_set_match_start(kt, count) < 0) {
			pr_err("kona_timer_set_match_start returned error\n");
			goto out;
		}
		pr_info("Timer test started\n");
out:
		return n;
	}

	pr_info("\nusage: echo [name (aon-timer/slave-timer)] "
		"[channel num (0-3)] [mode(0-periodic"
		"1-oneshot)] [count value] > /sys/bcm/timer_start_test\n");
	return -EINVAL;
}

static ssize_t
kona_timer_stop_test(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t n)
{
	unsigned int ch_num;

	if (sscanf(buf, "%d", &ch_num) == 1) {
		pr_info("channel_num:%d\n", ch_num);

		if (kt == NULL) {
			pr_err("No timer to stop!\n");
			goto out;
		}

		if (kt->ch_num != ch_num) {
			pr_err("channel %d is in use. Your request %d channel "
				"can't be stopped.\n", kt->ch_num, ch_num);
			goto out;
		}

		kona_timer_stop(kt);
		kona_timer_free(kt);
		kt = NULL;

		pr_info("Stopped and freed the timer\n");
out:
		return n;
	}

	pr_info("\nusage:echo [channel num(0-3)] > /sys/bcm/timer_stop_test\n");
	return -EINVAL;
}

static int kona_timer_unit_test_program(struct kona_timer *lkt,
					 enum timer_mode mode,
					 unsigned long count)
{
	struct timer_ch_cfg lcfg;

	lcfg.mode = mode;
	lcfg.arg  = lkt;
	lcfg.cb = timer_callback;

	if (kona_timer_config(lkt, &lcfg) < 0) {
		pr_err("kona_timer_config returned error.\n");
		goto error;
	}

	if (kona_timer_set_match_start(lkt, count) < 0) {
		pr_err("kona_timer_set_match_start returned error.\n");
		goto error;
	}

	return 0;
error:
	return -1;
}

static void kona_clock_delay(struct kona_timer *lkt, unsigned long clock_delay)
{
	u64 count;

	count = (u64)kona_timer_get_counter(lkt);

	/* Add 1 to guarantee minimum delay clock count */
	while ((u64)kona_timer_get_counter(lkt) - count < (u64)clock_delay + 1)
		;
}

static void print_unit_test_result(unsigned long expire_cnt,
				    int check_count,
				    unsigned long clk_rate)
{
	#define SW_OVERHEAD  1
	#define FREE_RUN_TIMER_ERROR 1
	unsigned long run_cycle;
	unsigned long run_time;
	int idx;
	int timer_error;

	for (idx = 0; idx < check_count; idx++) {
		if (timer_m->end[idx] > timer_m->start[idx])
			run_cycle = (unsigned long)(timer_m->end[idx]
					- timer_m->start[idx]);
		else
			run_cycle = 0;

		run_time = (unsigned long)(run_cycle*1000000/clk_rate);

		pr_info("requested cycles: %ld, expire cnt: %ld, start: %lld, "
			"end: %lld, run: %ld cycles( %ld us)\n",
			timer_m->req_cycle[idx], expire_cnt,
			timer_m->start[idx], timer_m->end[idx], run_cycle,
			run_time);

		if (!run_cycle) {
			pr_info("=>Timer couldn't be expired!\n");
			continue;
		}

		timer_error = (int)(run_cycle - SW_OVERHEAD -
				    timer_m->req_cycle[idx]);
		if (timer_error < 0)
			timer_error = 0;

		pr_info("=>%ld (Total timer run cycle) = %ld "
			"(requested cycle) +"
			" %d (SW overhead) + %d (timer error rate)\n",
			run_cycle, timer_m->req_cycle[idx],
			SW_OVERHEAD, timer_error);
	}
}

static int kona_timer_unit_test_run(const char *clk_name,
				     unsigned long clk_rate)
{
	struct kona_timer *lkt;
	int i;
	unsigned long flags;
	unsigned long req_cycle;

	pr_info("%s started!\n", __func__);

	lkt = kona_timer_request((char *)clk_name, -1);
	if (lkt == NULL) {
		pr_err("kona_timer_request returned error\n");
		return -EINVAL;
	}
	pr_info("%s ch_num: %d acquired!\n", clk_name, lkt->ch_num);

	timer_m = (struct timer_measure *)
			kmalloc(sizeof(struct timer_measure), GFP_KERNEL);

	if (!timer_m) {
		pr_err("%s memory allocation failed!\n", __func__);
		return -EINVAL;
	}

	spin_lock_init(&timer_m->lock);

#ifdef CONFIG_PREEMPT
	/* Ensure that cond_resched() won't try to preempt anybody */
	add_preempt_count(PREEMPT_ACTIVE);
#endif

	/*----------------------------------------------------------------------
	 * Test 1 : 1 clock cycle test
	 *  - set up 1 clock cycle timer request and measure the expiration
	 *    time.
	 */
	msleep(50);
	pr_info("=== Test case 1 ===\n");
	pr_info("1 clock tick one shot for 1 ms\n");

	req_cycle = 1;
	atomic_set(&timer_exp_count, 0);

	spin_lock_irqsave(&timer_m->lock, flags);
	memset(timer_m, 0, sizeof(struct timer_measure));
	timer_m->start[timer_m->counter] = (u64)kona_timer_get_counter(lkt);
	timer_m->req_cycle[timer_m->counter] = req_cycle;
	spin_unlock_irqrestore(&timer_m->lock, flags);

	kona_timer_unit_test_program(lkt, MODE_ONESHOT, req_cycle);

	mdelay(1);
	kona_timer_stop(lkt);

	/* Error check */
	print_unit_test_result(atomic_read(&timer_exp_count), 1, clk_rate);

	if (atomic_read(&timer_exp_count) != 1)
		goto error;

	/*----------------------------------------------------------------------
	 * Test 2 : peridic 1 clock timer test
	 *  - set up periodic 1 clock cycle timer request. SW will handle
	 *    periodic timer on every timer expiration. Wait for certain time
	 *    and check how many timers were expired and mesaure the time.
	 */
	msleep(50);
	pr_info("=== Test case 2 ===\n");
	pr_info("Periodic 1 clock tick for 1 ms\n");

	req_cycle = 1;
	atomic_set(&timer_exp_count, 0);

	spin_lock_irqsave(&timer_m->lock, flags);
	memset(timer_m, 0, sizeof(struct timer_measure));
	timer_m->start[timer_m->counter] = (u64)kona_timer_get_counter(lkt);

	for (i = 0; i < TIMER_MEASURE_MAX; i++)
		timer_m->req_cycle[i] = req_cycle;
	spin_unlock_irqrestore(&timer_m->lock, flags);

	kona_timer_unit_test_program(lkt, MODE_PERIODIC, req_cycle);

	/* mdelay() may be delayed by busy timer request. Should be short */
	mdelay(1);
	kona_timer_stop(lkt);
	pr_info("Total expiration count: %d\n", atomic_read(&timer_exp_count));
	print_unit_test_result(atomic_read(&timer_exp_count),
			       atomic_read(&timer_exp_count), clk_rate);

	/*----------------------------------------------------------------------
	* Test 3 : one-shot timer test with various time values
	*  - test various short clock cycles and check
	*    timer expiration and real run time of timer.
	*/
	msleep(50);
	pr_info("=== Test case 3 ===\n");
	pr_info("0~20 clock tick test for 1s, 50ms delay between each req\n");
	atomic_set(&timer_exp_count, 0);

	spin_lock_irqsave(&timer_m->lock, flags);
	memset(timer_m, 0, sizeof(struct timer_measure));
	spin_unlock_irqrestore(&timer_m->lock, flags);

	for (i = 0; i < 20; i++) {
		spin_lock_irqsave(&timer_m->lock, flags);
		timer_m->busy = 1;
		timer_m->counter = i;
		timer_m->start[i] =
			(u64)kona_timer_get_counter(lkt);
		timer_m->req_cycle[i] = i;
		kona_timer_unit_test_program(lkt, MODE_ONESHOT, i);
		timer_m->busy = 0;
		spin_unlock_irqrestore(&timer_m->lock, flags);

		mdelay(50);
		kona_timer_stop(lkt);
	}

	print_unit_test_result(atomic_read(&timer_exp_count), 20, clk_rate);

	if (atomic_read(&timer_exp_count) != 20)
		goto error;

	/*----------------------------------------------------------------------
	* Test 4 : one-shot timer test with various time values and delays.
	*  - test various short clock cycles with various short delays.
	*    Verify the timer expiration and timer run-time.
	*/
	msleep(50);
	pr_info("=== Test case 4 ===\n");
	pr_info("0~29 cycle timer test, 0~29 + 3 clock cycle wait\n");
	pr_info("   Ex)1 clock : wait for 4 clock cycle time\n");
	pr_info("   Ex)10 clock : wait for 13 clock cycle time\n");
	atomic_set(&timer_exp_count, 0);

	spin_lock_irqsave(&timer_m->lock, flags);
	memset(timer_m, 0, sizeof(struct timer_measure));
	spin_unlock_irqrestore(&timer_m->lock, flags);

	for (i = 0; i < 30; i++) {
		spin_lock_irqsave(&timer_m->lock, flags);
		timer_m->busy = 1;
		timer_m->counter = i;
		timer_m->start[i] =
			(u64)kona_timer_get_counter(lkt);
		timer_m->req_cycle[i] = i;
		kona_timer_unit_test_program(lkt, MODE_ONESHOT, i);
		timer_m->busy = 0;
		spin_unlock_irqrestore(&timer_m->lock, flags);

		kona_clock_delay(lkt, i + 3);
	}
	kona_timer_stop(lkt);
	pr_info("Total expiration count: %d\n", atomic_read(&timer_exp_count));
	print_unit_test_result(atomic_read(&timer_exp_count), 30, clk_rate);

	/*----------------------------------------------------------------------
	 * Test 5 : one-shot timer test with short-long time/delay.
	 *  - test various short/long clock cycles with various short/long
	 *    delays. Verify the timer expiration and timer run-time.
	 */
	msleep(50);
	pr_info("=== Test case 5 ===\n");
	pr_info("short and long timer test\n");
	pr_info("   short delay: 0~19 clock cycle wait\n");
	pr_info("   long  delay: 50 ms wait\n");
	atomic_set(&timer_exp_count, 0);

	spin_lock_irqsave(&timer_m->lock, flags);
	memset(timer_m, 0, sizeof(struct timer_measure));
	spin_unlock_irqrestore(&timer_m->lock, flags);

	for (i = 0; i < 20; i++) {
		/* Short timer request */
		spin_lock_irqsave(&timer_m->lock, flags);
		timer_m->busy = 1;
		timer_m->counter = 2 * i;
		timer_m->start[2 * i] =
			(u64)kona_timer_get_counter(lkt);
		timer_m->req_cycle[2 * i] = i;
		kona_timer_unit_test_program(lkt, MODE_ONESHOT, i);
		timer_m->busy = 0;
		spin_unlock_irqrestore(&timer_m->lock, flags);

		kona_clock_delay(lkt, i + 1);

		/* Long timer request */
		req_cycle = clk_rate / 50; /* 20ms */
		spin_lock_irqsave(&timer_m->lock, flags);
		timer_m->busy = 1;
		timer_m->counter = 2 * i + 1;
		timer_m->start[2 * i + 1] =
			(u64)kona_timer_get_counter(lkt);
		timer_m->req_cycle[2 * i + 1] = req_cycle;
		kona_timer_unit_test_program(lkt, MODE_ONESHOT, req_cycle);
		timer_m->busy = 0;
		spin_unlock_irqrestore(&timer_m->lock, flags);

		mdelay(1000/50 * 2); /* 40 ms */
	}
	kona_timer_stop(lkt);
	pr_info("Total expiration count: %d\n", atomic_read(&timer_exp_count));
	print_unit_test_result(atomic_read(&timer_exp_count), 20 * 2, clk_rate);

	/*----------------------------------------------------------------------
	 * Test 6 : periodic 1s timer test
	 *          wait for 10.5 sec with 1s periodic timer request
	 */
	msleep(50);
	pr_info("=== Test case 6 ===\n");
	pr_info("Periodic 1s timer test for 10.5s\n");

	req_cycle = clk_rate;
	atomic_set(&timer_exp_count, 0);

	spin_lock_irqsave(&timer_m->lock, flags);
	memset(timer_m, 0, sizeof(struct timer_measure));
	timer_m->start[timer_m->counter] = (u64)kona_timer_get_counter(lkt);

	for (i = 0; i < TIMER_MEASURE_MAX; i++)
		timer_m->req_cycle[i] = req_cycle;
	spin_unlock_irqrestore(&timer_m->lock, flags);

	kona_timer_unit_test_program(lkt, MODE_PERIODIC, clk_rate);

	msleep(10500);
	kona_timer_stop(lkt);

	print_unit_test_result(atomic_read(&timer_exp_count),
			       atomic_read(&timer_exp_count), clk_rate);

	if (atomic_read(&timer_exp_count) != 10)
		goto error;
	msleep(50);

	/*
	 * End of kona timer unit test
	 */

	kona_timer_free(lkt);

	pr_info("%s Passed!\n", __func__);

#ifdef CONFIG_PREEMPT
	sub_preempt_count(PREEMPT_ACTIVE);
#endif
	kfree(timer_m);
	timer_m = NULL;

	return 0;

error:
	kona_timer_stop(lkt);
	kona_timer_free(lkt);

	pr_err("%s Failed\n", __func__);

#ifdef CONFIG_PREEMPT
	sub_preempt_count(PREEMPT_ACTIVE);
#endif
	kfree(timer_m);
	timer_m = NULL;

	return -EINVAL;
}

static ssize_t
kona_timer_unit_test(struct device *dev, struct device_attribute *attr,
	  const char *buf, size_t count)
{
	/* aon-timer */
	if (kona_timer_unit_test_run("aon-timer", CLOCK_TICK_RATE))
		goto error;

	/* slave-timer */

	return count;
error:
	return -EINVAL;
}
#endif

#ifdef CONFIG_KONA_TIMER_UNIT_TESTS
static DEVICE_ATTR(timer_module_cfg, 0666, NULL, kona_timer_module_cfg);
static DEVICE_ATTR(timer_start_test, 0666, NULL, kona_timer_start_test);
static DEVICE_ATTR(timer_stop_test, 0666, NULL, kona_timer_stop_test);
static DEVICE_ATTR(timer_unit_test, 0666, NULL, kona_timer_unit_test);
#endif

static struct attribute *bcm_attrs[] = {
#ifdef CONFIG_KONA_TIMER_UNIT_TESTS
	&dev_attr_timer_module_cfg.attr,
	&dev_attr_timer_start_test.attr,
	&dev_attr_timer_stop_test.attr,
	&dev_attr_timer_unit_test.attr,
#endif
	&dev_attr_reset_reason.attr,
	NULL,
};

static struct attribute_group bcm_attr_group = {
	.attrs = bcm_attrs,
};

static int __init bcm_sysfs_init(void)
{
	bcm_kobj = kobject_create_and_add("bcm", NULL);
	if (!bcm_kobj)
		return -ENOMEM;
	return sysfs_create_group(bcm_kobj, &bcm_attr_group);
}

static void __exit bcm_sysfs_exit(void)
{
	sysfs_remove_group(bcm_kobj, &bcm_attr_group);
}

module_init(bcm_sysfs_init);
module_exit(bcm_sysfs_exit);
