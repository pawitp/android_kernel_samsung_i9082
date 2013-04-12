/*****************************************************************************
*  Copyright 2012 Broadcom Corporation.  All rights reserved.
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

/*
 * drivers/cpufreq/cpufreq_profile.c
 *
 * Inspired by cpufreq_stats.c, by Venkatesh Pallipadi and Zou Nan hai
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sysdev.h>
#include <linux/cpu.h>
#include <linux/sysfs.h>
#include <linux/cpufreq.h>
#include <linux/jiffies.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/kernel_stat.h>
#include <linux/percpu.h>
#include <linux/kobject.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <asm/cputime.h>
#include <linux/uaccess.h>

#define MAX_PROC_BUF_SIZE       256
#define MAX_PROC_NAME_SIZE      20
#define PROC_PARENT_DIR_NAME    "cpufreq-profile"
#define PROC_ENTRY_ENABLE       "enable"
#define PROC_ENTRY_PROFILE      "profile"

/* per freqeuncy/state info */
struct cpufreq_state {
	unsigned int freq;
	cputime64_t wall_time;
	cputime64_t idle_time;
};

/* per CPU statistics */
struct cpufreq_stats {
	unsigned int cpu;
	unsigned int max_num_freqs;
	unsigned int prev_index;
	cputime64_t prev_wall_time;
	cputime64_t prev_idle_time;
	struct cpufreq_state *state;
};

struct procfs {
	struct proc_dir_entry *parent;
};

struct cpufreq_profile {
	atomic_t enable;
	struct procfs proc;
};

static spinlock_t cpufreq_profile_lock;
static DEFINE_PER_CPU(struct cpufreq_stats *, cpufreq_stats_table);
static struct cpufreq_profile profile;

static int freq_table_get_index(const struct cpufreq_stats *stat,
					unsigned int freq)
{
	unsigned int index;

	for (index = 0; index < stat->max_num_freqs; index++)
		if (stat->state[index].freq == freq)
			return index;
	return -1;
}

static inline cputime64_t get_cpu_idle_time_jiffy(unsigned int cpu,
							cputime64_t *wall)
{
	cputime64_t idle_time;
	cputime64_t cur_wall_time;
	cputime64_t busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());
	busy_time = cputime64_add(kstat_cpu(cpu).cpustat.user,
			kstat_cpu(cpu).cpustat.system);

	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.irq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.softirq);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.steal);
	busy_time = cputime64_add(busy_time, kstat_cpu(cpu).cpustat.nice);

	idle_time = cputime64_sub(cur_wall_time, busy_time);
	if (wall)
		*wall = (cputime64_t)jiffies_to_usecs(cur_wall_time);

	return (cputime64_t)jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, wall);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);

	return idle_time;
}

static int cpufreq_profile_start(unsigned int cpu)
{
	unsigned int freq, freq_index;
	struct cpufreq_stats *stat;

	freq = cpufreq_get(cpu);
	if (freq == 0)
		return -EFAULT;

	stat = per_cpu(cpufreq_stats_table, cpu);
	if (!stat)
		return -EFAULT;

	spin_lock(&cpufreq_profile_lock);

	for (freq_index = 0; freq_index < stat->max_num_freqs; freq_index++) {
		stat->state[freq_index].wall_time = 0;
		stat->state[freq_index].idle_time = 0;
	}

	stat->prev_index = freq_table_get_index(stat, freq);
	stat->prev_idle_time = get_cpu_idle_time(cpu, &stat->prev_wall_time);

	spin_unlock(&cpufreq_profile_lock);

	return 0;
}

static int cpufreq_profile_stop(unsigned int cpu)
{
	struct cpufreq_stats *stat;
	unsigned int freq;
	cputime64_t cur_wall_time, cur_idle_time, wall_time, idle_time;

	freq = cpufreq_get(cpu);
	if (freq == 0)
		return -EFAULT;

	stat = per_cpu(cpufreq_stats_table, cpu);
	if (!stat)
		return -EFAULT;

	spin_lock(&cpufreq_profile_lock);

	cur_idle_time = get_cpu_idle_time(cpu, &cur_wall_time);

	wall_time = cputime64_sub(cur_wall_time, stat->prev_wall_time);
	idle_time = cputime64_sub(cur_idle_time, stat->prev_idle_time);

	stat->state[stat->prev_index].wall_time += wall_time;
	stat->state[stat->prev_index].idle_time += idle_time;

	spin_unlock(&cpufreq_profile_lock);

	return 0;
}

static int cpufreq_profile_update(unsigned int cpu, unsigned int freq_index)
{
	struct cpufreq_stats *stat;
	cputime64_t cur_wall_time, cur_idle_time, wall_time, idle_time;

	cur_idle_time = get_cpu_idle_time(cpu, &cur_wall_time);
	stat = per_cpu(cpufreq_stats_table, cpu);

	wall_time = cputime64_sub(cur_wall_time, stat->prev_wall_time);
	idle_time = cputime64_sub(cur_idle_time, stat->prev_idle_time);

	stat->state[freq_index].wall_time += wall_time;
	stat->state[freq_index].idle_time += idle_time;

	stat->prev_wall_time = cur_wall_time;
	stat->prev_idle_time = cur_idle_time;

	return 0;
}

static int cpufreq_profile_notifier_trans(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = data;
	struct cpufreq_stats *stat;
	int old_index, new_index;

	if (atomic_read(&profile.enable) == 0)
		return 0;

	/* only update after CPU frequency change has finished */
	if (val != CPUFREQ_POSTCHANGE)
		return 0;

	stat = per_cpu(cpufreq_stats_table, freq->cpu);
	if (!stat)
		return 0;

	spin_lock(&cpufreq_profile_lock);

	old_index = stat->prev_index;
	new_index = freq_table_get_index(stat, freq->new);

	if (old_index == -1 || new_index == -1) {
		spin_unlock(&cpufreq_profile_lock);
		return 0;
	}

	/* now update the information */
	cpufreq_profile_update(freq->cpu, old_index);

	if (old_index == new_index) {
		spin_unlock(&cpufreq_profile_lock);
		return 0;
	}

	stat->prev_index = new_index;

	spin_unlock(&cpufreq_profile_lock);
	return 0;
}

static int cpufreq_profile_create_table(unsigned int cpu)
{
	int ret;
	unsigned int i, freq_count;
	struct cpufreq_stats *stat;
	struct cpufreq_frequency_table *table;

	stat = kzalloc(sizeof(struct cpufreq_stats), GFP_KERNEL);
	if (stat == NULL)
		return -ENOMEM;

	stat->cpu = cpu;
	per_cpu(cpufreq_stats_table, cpu) = stat;

	/* Capri only populates frequency table for CPU0 */
#ifdef CONFIG_ARCH_CAPRI
	table = cpufreq_frequency_get_table(0);
#else
	table = cpufreq_frequency_get_table(cpu);
#endif
	if (!table) {
		ret = -ENOMEM;
		goto out;
	}

	/* get max number of supported frequencies */
	freq_count = 0;
	for (i = 0; table[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (table[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;
		freq_count++;
	}

	stat->max_num_freqs = freq_count;

	stat->state = kcalloc(freq_count, sizeof(struct cpufreq_state),
			GFP_KERNEL);
	if (stat->state == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	/* record all available frequencies */
	freq_count = 0;
	for (i = 0; table[i].frequency != CPUFREQ_TABLE_END; i++) {
		if (table[i].frequency == CPUFREQ_ENTRY_INVALID)
			continue;
		stat->state[freq_count++].freq = table[i].frequency;
	}

	return 0;

out:
	kfree(stat);
	per_cpu(cpufreq_stats_table, cpu) = NULL;
	return ret;
}

static void cpufreq_profile_free_table(unsigned int cpu)
{
	struct cpufreq_stats *stat = per_cpu(cpufreq_stats_table, cpu);

	if (stat == NULL)
		return;

	kfree(stat->state);
	kfree(stat);
	per_cpu(cpufreq_stats_table, cpu) = NULL;
}

/*
 * Should only be called when the profiler has been stopped
 */
static void cpufreq_profile_print(void)
{
	unsigned int i, cpu;

	for_each_online_cpu(cpu) {
		struct cpufreq_stats *stat = per_cpu(cpufreq_stats_table, cpu);

		printk(KERN_INFO "cpu%u:\n", cpu);
		printk(KERN_INFO "freq idle wall\n");
		for (i = 0; i < stat->max_num_freqs; i++) {
			printk(KERN_INFO "%u %llu %llu\n",
					stat->state[i].freq,
					stat->state[i].idle_time,
					stat->state[i].wall_time);
		}
		printk(KERN_INFO "\n");
	}
}

static int proc_enable_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	unsigned int cpu;
	int ret, enable;
	struct cpufreq_profile *profile = (struct cpufreq_profile *)data;
	unsigned char kbuf[MAX_PROC_BUF_SIZE];

	if (count > MAX_PROC_BUF_SIZE)
		count = MAX_PROC_BUF_SIZE;

	ret = copy_from_user(kbuf, buffer, count);
	if (ret) {
		pr_err("copy_from_user failed status=%d\n", ret);
		return -EFAULT;
	}

	if (sscanf(kbuf, "%d", &enable) != 1) {
		pr_err("echo <enable> > %s\n", PROC_ENTRY_ENABLE);
		return count;
	}

	if (enable) {
		if (atomic_read(&profile->enable) == 0) {
			for_each_online_cpu(cpu) {
				cpufreq_profile_start(cpu);
				atomic_set(&profile->enable, 1);
			}
		}
	} else {
		if (atomic_read(&profile->enable)) {
			for_each_online_cpu(cpu) {
				atomic_set(&profile->enable, 0);
				cpufreq_profile_stop(cpu);
			}
			cpufreq_profile_print();
		}
	}

	return count;
}

static int proc_enable_read(char *buffer, char **start, off_t off, int count,
		int *eof, void *data)
{
	unsigned int len = 0;
	struct cpufreq_profile *profile = (struct cpufreq_profile *)data;

	if (off > 0)
		return 0;

	len += sprintf(buffer + len, "cpufreq-profile is currently %s\n",
			atomic_read(&profile->enable) ?
			"enabled" : "disabled");
	return len;
}

static int proc_profile_read(char *buffer, char **start, off_t off, int count,
		int *eof, void *data)
{
	unsigned int len = 0;

	if (off > 0)
		return 0;

	cpufreq_profile_print();

	return len;
}

static int proc_init(struct cpufreq_profile *profile)
{
	int ret;
	struct procfs *proc = &profile->proc;
	struct proc_dir_entry *proc_enable, *proc_profile;

	proc->parent = proc_mkdir(PROC_PARENT_DIR_NAME, NULL);
	if (proc->parent == NULL)
		return -ENOMEM;

	proc_enable = create_proc_entry(PROC_ENTRY_ENABLE, 0644, proc->parent);
	if (proc_enable == NULL) {
		ret = -ENOMEM;
		goto proc_exit;
	}
	proc_enable->read_proc = proc_enable_read;
	proc_enable->write_proc = proc_enable_write;
	proc_enable->data = profile;

	proc_profile = create_proc_entry(PROC_ENTRY_PROFILE, 0644,
				proc->parent);
	if (proc_profile == NULL) {
		ret = -ENOMEM;
		goto proc_exit;
	}
	proc_profile->read_proc = proc_profile_read;
	proc_profile->write_proc = NULL;
	proc_profile->data = profile;

	return 0;

	remove_proc_entry(PROC_ENTRY_ENABLE, proc->parent);
proc_exit:
	remove_proc_entry(PROC_PARENT_DIR_NAME, NULL);
	return ret;
}

static void proc_term(struct cpufreq_profile *profile)
{
	struct procfs *proc = &profile->proc;

	remove_proc_entry(PROC_ENTRY_PROFILE, proc->parent);
	remove_proc_entry(PROC_ENTRY_ENABLE, proc->parent);
	remove_proc_entry(PROC_PARENT_DIR_NAME, NULL);
}

static struct notifier_block notifier_trans_block = {
	.notifier_call = cpufreq_profile_notifier_trans
};

static int __init cpufreq_profile_init(void)
{
	int ret;
	unsigned int cpu;

	spin_lock_init(&cpufreq_profile_lock);
	ret = cpufreq_register_notifier(&notifier_trans_block,
				CPUFREQ_TRANSITION_NOTIFIER);
	if (ret)
		return ret;

	for_each_online_cpu(cpu) {
		ret = cpufreq_profile_create_table(cpu);
		if (ret)
			goto out;
	}

	ret = proc_init(&profile);
	if (ret)
		goto out;

	printk(KERN_INFO "cpufreq-profile initialized\n");

	return 0;

out:
	for_each_online_cpu(cpu) {
		cpufreq_profile_free_table(cpu);
	}
	cpufreq_unregister_notifier(&notifier_trans_block,
			CPUFREQ_TRANSITION_NOTIFIER);

	printk(KERN_INFO "cpufreq-profile initialization failed ret=%d\n", ret);
	return ret;
}
static void __exit cpufreq_profile_exit(void)
{
	unsigned int cpu;

	proc_term(&profile);
	for_each_online_cpu(cpu) {
		cpufreq_profile_free_table(cpu);
	}
	cpufreq_unregister_notifier(&notifier_trans_block,
			CPUFREQ_TRANSITION_NOTIFIER);
}

MODULE_AUTHOR("Broadcom>");
MODULE_DESCRIPTION("Driver to track CPU frequency stats and utilization");
MODULE_LICENSE("GPL");

module_init(cpufreq_profile_init);
module_exit(cpufreq_profile_exit);
