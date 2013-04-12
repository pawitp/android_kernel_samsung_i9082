/************************************************************************************************/
/*                                                                                              */
/*  Copyright 2012  Broadcom Corporation                                                        */
/*                                                                                              */
/*     Unless you and Broadcom execute a separate written software license agreement governing  */
/*     use of this software, this software is licensed to you under the terms of the GNU        */
/*     General Public License version 2 (the GPL), available at                                 */
/*                                                                                              */
/*          http://www.broadcom.com/licenses/GPLv2.php                                          */
/*                                                                                              */
/*     with the following added to such license:                                                */
/*                                                                                              */
/*     As a special exception, the copyright holders of this software give you permission to    */
/*     link this software with independent modules, and to copy and distribute the resulting    */
/*     executable under terms of your choice, provided that you also meet, for each linked      */
/*     independent module, the terms and conditions of the license of that module.              */
/*     An independent module is a module which is not derived from this software.  The special  */
/*     exception does not apply to any modifications of the software.                           */
/*                                                                                              */
/*     Notwithstanding the above, under no circumstances may you combine this software in any   */
/*     way with any other Broadcom software provided under a license other than the GPL,        */
/*     without Broadcom's express prior written consent.                                        */
/*                                                                                              */
/************************************************************************************************/

#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/broadcom/knllog.h>
#include <linux/hrtimer.h>

#include <mach/kona_timer.h>

/* debugfs - for statistics */
static int kona_timer_module_show(struct seq_file *m, void *data)
{
	struct kona_timer_module *timer_module = m->private;
	struct kona_timer *aon_hub_timer = timer_module->pkt;
	int i;

	seq_printf(m, "timer_module: %s\n"
			"  max repeat read times:%lu\n",
			timer_module->name,
			timer_module->max_repeat_count);

	for (i = 0; i < NUM_OF_CHANNELS; i++) {
		seq_printf(m, "ch:%d, total:%lu, fired:%lu, canceled:%lu,"
				" early_expire:%lu, wrong-int:%lu\n", i,
			aon_hub_timer[i].nr_total,
			aon_hub_timer[i].nr_timedout,
			aon_hub_timer[i].nr_canceled,
			aon_hub_timer[i].nr_early_expire,
			aon_hub_timer[i].nr_wrong_interrupt);

		seq_printf(m, "    CANCELED: total:%lu, expired:%lu,"
				" matched:%lu\n",
			aon_hub_timer[i].nr_canceled,
			aon_hub_timer[i].nr_canceled_expired,
			aon_hub_timer[i].nr_canceled_expired_intr);

		seq_printf(m, "    delta:<5=%lu, <10=%lu, <50=%lu, <100=%lu,"
				" <500=%lu, rest=%lu\n",
			aon_hub_timer[i].nr_5,
			aon_hub_timer[i].nr_10,
			aon_hub_timer[i].nr_50,
			aon_hub_timer[i].nr_100,
			aon_hub_timer[i].nr_500,
			aon_hub_timer[i].nr_500_plus);

		seq_printf(m, "    max delta:%lu - load:%lu, expire:%lu,"
				" expired:%lu, max_delta_early_expire:%lu,"
				" max_delta_early_expire_load:%lu\n",
			aon_hub_timer[i].max_delta,
			aon_hub_timer[i].max_delta_load,
			aon_hub_timer[i].max_delta_expire,
			aon_hub_timer[i].max_delta_expired,
			aon_hub_timer[i].max_delta_early_expire,
			aon_hub_timer[i].max_delta_load_early_expire);
	}

	seq_printf(m, "\n");

	{
		unsigned long flags;
		spin_lock_irqsave(&timer_module->lock, flags);
		for (i = 0; i < NUM_OF_CHANNELS; i++) {
			aon_hub_timer[i].max_delta = 0;
			aon_hub_timer[i].max_delta_load = 0;
			aon_hub_timer[i].max_delta_expire = 0;
			aon_hub_timer[i].max_delta_expired = 0;
		}
		spin_unlock_irqrestore(&timer_module->lock, flags);
	}

	return 0;
}

static int kona_timer_module_open(struct inode *inode, struct file *file)
{
	return single_open(file, kona_timer_module_show, inode->i_private);
}

static const struct file_operations kona_timer_module_fops = {
	.open           = kona_timer_module_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

int __init init_kona_timer_debugfs(void)
{
	struct dentry *root;

	root = debugfs_create_dir("kona_timer", NULL);

	if (!root)
		return -ENXIO;

	struct kona_timer_module *timer_module;
	timer_module = kona_get_timer_module("aon-timer");

	if (!(debugfs_create_file("aon_timer_module", S_IRUSR, root,
					timer_module,
					&kona_timer_module_fops)))
		return -ENOMEM;

	return 0;
}

late_initcall(init_kona_timer_debugfs);
