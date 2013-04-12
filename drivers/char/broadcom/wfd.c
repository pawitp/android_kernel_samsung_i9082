/*****************************************************************************
*  Copyright 2012 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */
#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/vmalloc.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/broadcom/wfd_ioctl.h>
#include <linux/broadcom/hdmi.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/netfilter_ipv4.h>

/* ---- Private Constants and Types -------------------------------------- */

#define WFD_DEVICE_NAME       "wfd"
#define WFD_BUFFER_MAX_SIZE   256

/**
* Debug traces
*/
#define WFD_ENABLE_KNLLOG        0
#if WFD_ENABLE_KNLLOG
#include <linux/broadcom/knllog.h>
#define WFD_KNLLOG               KNLLOG
#else
#define WFD_KNLLOG(...)
#endif

#define WFD_MAX_TS_PER_LIST 10000
#define WFD_TS_DUMP_FILE "/tmp/wfd_profiling_timestamps.txt"

/* ---- Private Function Prototypes -------------------------------------- */

static int wfd_probe(struct platform_device *pdev);
static int wfd_remove(struct platform_device *pdev);

static int wfd_open(struct inode *inode, struct file *file);
static int wfd_release(struct inode *inode, struct file *file);
static ssize_t wfd_read(struct file *file, char __user *buffer, size_t count,
			loff_t *ppos);
static int wfd_fsync(struct file *file, int datasync);
static ssize_t wfd_write(struct file *file, const char __user *buffer,
			 size_t count, loff_t *ppos);
static long wfd_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static unsigned int wfd_poll(struct file *file,
			     struct poll_table_struct *poll_table);
static unsigned int wfd_ts_lists_get_max_count(void);
static int wfd_queue_ts(unsigned int type, unsigned long timestamp);
static int wfd_prof_add_ts(unsigned int type, unsigned int count);
static void wfd_prof_prepare_dump_file(void);
static unsigned int wfd_prof_netfilter_hook(unsigned int hooknum,
					    struct sk_buff *skb,
					    const struct net_device *in,
					    const struct net_device *out,
					    int (*okfn) (struct sk_buff *));

/* ---- Private Variables ------------------------------------------------- */

static int gDriverMajor;

#ifdef CONFIG_SYSFS
static struct class *wfd_class;
static struct device *wfd_dev;
#endif

/* File Operations (these are the device driver entry points) */
static const struct file_operations gfops = {
	.owner = THIS_MODULE,
	.open = wfd_open,
	.release = wfd_release,
	.read = wfd_read,
	.write = wfd_write,
	.fsync = wfd_fsync,
	.unlocked_ioctl = wfd_ioctl,
	.poll = wfd_poll,
};

/* Net filter hook structure for profiling */
static struct nf_hook_ops wfd_nf_ops = {
	.hook = wfd_prof_netfilter_hook,
	.pf = PF_INET,
	.hooknum = NF_INET_POST_ROUTING,
	.priority = NF_IP_PRI_LAST,
};

#define WFD_DATA_SIZE   188	/* size of MPEG2TS packet */
struct wfd_data_block {
	struct list_head wfd_list;
	size_t size;
	uint8_t data[WFD_DATA_SIZE];
	uint8_t eof;
};

#define AUDIO_SAMPLE_RATE       48000LL
#define AUDIO_SAMPLE_CHANNELS   2	/* stereo samples */
#define AUDIO_SAMPLE_BYTES      2	/* 16-bit samples */
#define AUDIO_SAMPLE_SIZE       (AUDIO_SAMPLE_CHANNELS * AUDIO_SAMPLE_BYTES)
struct wfd_audio_block {
	struct list_head wfd_list;

	char *data;
	uint32_t size;

};

struct wfd_stat_block {
	uint64_t wfd_rd;	/* Read counter */
	uint64_t wfd_rd_bytes;	/* Bytes successfully read */
	uint64_t wfd_rd_fail;	/* Read failure counter */
	uint64_t wfd_wt;	/* Write counter */
	uint64_t wfd_wt_bytes;	/* Bytes successfully written */
	uint64_t wfd_wt_fail;	/* Write failure counter */
	uint64_t wfd_flush;
};

struct wfd_audio_stat_block {
	uint64_t wfd_wt;
	uint64_t wfd_wt_fail;
	uint64_t wfd_rd;
	uint64_t wfd_rd_fail;
};

struct wfd_ts_block {
	struct list_head list_head;
	unsigned long timestamp;
};

struct wfd_ts_list {
	unsigned int num_entries;
	struct wfd_ts_block list;
};

enum wfd_state {
	WFD_STATE_IDLE,
	WFD_STATE_START,
	WFD_STATE_PLAY,
	WFD_STATE_PAUSE
};

static bool wfd_flush_required;
static struct wfd_data_block wfd_data;
static struct wfd_audio_block wfd_audio;
static struct wfd_audio_stat_block wfd_audio_stat;
static struct wfd_stat_block wfd_stat;
static struct mutex wfd_queue_lock;
static struct mutex wfd_audio_lock;
static struct mutex wfd_partial_lock;
static struct mutex wfd_prof_lock;
struct wfd_ioctl_metadata wfd_metadata;
struct wfd_ioctl_neg_config wfd_neg_config;
struct wfd_ioctl_con_state wfd_con_state;
static int wfd_refcount;

static long wfd_prof_enabled;
static long wfd_prof_max_ts = WFD_MAX_TS_PER_LIST;
static enum wfd_state wfd_current_state = WFD_STATE_IDLE;

static DECLARE_WAIT_QUEUE_HEAD(read_wq);
static DECLARE_WAIT_QUEUE_HEAD(audio_wq);

/* audio frame count and start time */
static uint64_t audio_frame_cnt;
static struct timeval audio_start_time;

static struct wfd_data_block *wfd_partial_block;
static struct wfd_ts_list wfd_ts_lists[WFD_PROFILING_TS_LAST];

static struct proc_dir_entry *wfd_proc_dir;
static struct proc_dir_entry *wfd_info_proc_entry;
static struct proc_dir_entry *wfd_prof_proc_dir;
static struct proc_dir_entry *wfd_prof_enable_proc_entry;
static struct proc_dir_entry *wfd_prof_dump_proc_entry;
static struct proc_dir_entry *wfd_prof_counts_proc_entry;
static struct proc_dir_entry *wfd_prof_max_ts_proc_entry;
static struct proc_dir_entry *wfd_prof_add_ts_proc_entry;

/* ---- Public Variables ------------------------------------------------- */

/* ---- Functions -------------------------------------------------------- */

static unsigned int wfd_poll(struct file *file,
			     struct poll_table_struct *poll_table)
{
	unsigned int mask = 0;

	poll_wait(file, &read_wq, poll_table);

	if (!list_empty(&(wfd_data.wfd_list)))
		mask |= POLLIN | POLLRDNORM;	/* readable */

	return mask;
}

/***************************************************************************/
/**
*  Flush all queued audio packets
*
*  @return
*   none
*/
static void flush_audio()
{
	struct wfd_audio_block *data = NULL;

	mutex_lock(&wfd_audio_lock);
	while (!list_empty(&(wfd_audio.wfd_list))) {
		data =
		    list_first_entry(&(wfd_audio.wfd_list),
				     struct wfd_audio_block, wfd_list);
		if (data) {
			list_del(&(data->wfd_list));

			kfree(data->data);
			data->data = NULL;
			kfree(data);
			data = NULL;
		}
	}
	mutex_unlock(&wfd_audio_lock);
}

/***************************************************************************/
/**
*  Flush all queued ts packets
*
*  @return
*   none
*/
static void flush_ts()
{
	struct wfd_data_block *data = NULL;

	/* free the partial block */
	mutex_lock(&wfd_partial_lock);
	if (wfd_partial_block != NULL) {
		kfree(wfd_partial_block);
		wfd_partial_block = NULL;
	}
	mutex_unlock(&wfd_partial_lock);

	/* free any queued packets */
	mutex_lock(&wfd_queue_lock);
	while (!list_empty(&(wfd_data.wfd_list))) {
		data =
		    list_first_entry(&(wfd_data.wfd_list),
				     struct wfd_data_block, wfd_list);
		if (data) {
			list_del(&(data->wfd_list));
			kfree(data);
			data = NULL;
		}
	}
	mutex_unlock(&wfd_queue_lock);
}

/***************************************************************************/
/**
*  Driver ioctl method to support user library API.
*
*  @return
*     >= 0           Number of bytes write
*     -ve            Error code
*/
static long wfd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = -EPERM;
	unsigned int cmdnr;

	cmdnr = _IOC_NR(cmd);

	switch (cmdnr) {
	case WFD_CMD_SET_METADATA:
		{
			struct wfd_ioctl_metadata metadata;
			rc = copy_from_user(&metadata,
					    (void __user *)arg,
					    sizeof(struct wfd_ioctl_metadata));
			if (rc)
				return rc;

			wfd_metadata.action = metadata.action;
			WFD_KNLLOG("%s: set-metadata to 0x%08X\n", __func__,
				   wfd_metadata.action);

			switch (wfd_metadata.action) {
			case WFDSCRAPER_METADATA_START:
				wfd_current_state = WFD_STATE_START;
				/* reset the audio frame count */
				audio_frame_cnt = 0;
				hdmi_set_wifi_hdmi(1);
				break;

			case WFDSCRAPER_METADATA_STOP:
			case WFDSCRAPER_METADATA_PAUSE:
				if (wfd_metadata.action ==
				    WFDSCRAPER_METADATA_STOP) {
					wfd_current_state = WFD_STATE_IDLE;
				hdmi_set_wifi_hdmi(0);
				} else {
					wfd_current_state = WFD_STATE_PAUSE;
				}

				/* flush all audio */
				flush_audio();

				/* flush all ts packets */
				flush_ts();

				/* unblock the reader */
				wfd_flush_required = true;
				wake_up_interruptible(&read_wq);

				break;

			case WFDSCRAPER_METADATA_PLAY:
				wfd_current_state = WFD_STATE_PLAY;
				break;

			default:
				break;
			}
		}
		break;

	case WFD_CMD_GET_METADATA:
		WFD_KNLLOG("%s: get-metadata reports 0x%08X\n",
			   __func__, wfd_metadata.action);

		rc = copy_to_user((void __user *)arg,
				  &wfd_metadata,
				  sizeof(struct wfd_ioctl_metadata));
		if (rc)
			return rc;

		break;

	case WFD_CMD_SET_NEG_CONFIG:
		{
			struct wfd_ioctl_neg_config neg_config;

			rc = copy_from_user(&neg_config,
					    (void __user *)arg,
					    sizeof(struct
						   wfd_ioctl_neg_config));
			if (rc)
				return rc;

			memcpy(&wfd_neg_config,
			       &neg_config, sizeof(wfd_neg_config));
		}
		break;

	case WFD_CMD_GET_NEG_CONFIG:
		rc = copy_to_user((void __user *)arg,
				  &wfd_neg_config,
				  sizeof(struct wfd_ioctl_neg_config));
		if (rc)
			return rc;
		break;

	case WFD_CMD_SET_CON_STATE:
		{
			struct wfd_ioctl_con_state con_state;

			rc = copy_from_user(&con_state,
					    (void __user *)arg,
					    sizeof(struct wfd_ioctl_con_state));

			if (rc)
				return rc;

			memcpy(&wfd_con_state,
			       &con_state, sizeof(wfd_con_state));
		}
		break;

	case WFD_CMD_GET_CON_STATE:
		rc = copy_to_user((void __user *)arg,
				  &wfd_con_state,
				  sizeof(struct wfd_ioctl_con_state));

		if (rc)
			return rc;

		break;

	case WFD_CMD_SET_AUDIO_DATA:
		if (wfd_current_state != WFD_STATE_PLAY)
			return -EINVAL;
		else {
			struct wfd_ioctl_audio_data audio_data;
			struct wfd_audio_block *data = NULL;

			wfd_audio_stat.wfd_wt++;

			rc = copy_from_user(&audio_data,
					    (void __user *)arg,
					    sizeof(struct
						   wfd_ioctl_audio_data));
			if (rc) {
				wfd_audio_stat.wfd_wt_fail++;
				printk(KERN_ERR
				       "WFD: failed audio data ioctl, rc=%d",
				       rc);
				return rc;
			}

			/* verify that the requested size is expected */
			if (audio_data.size != WFD_AUDIO_SIZE) {
				printk(KERN_ERR
				       "WFD: unexpected audio set size. Expected=%d, got=%d",
				       WFD_AUDIO_SIZE, audio_data.size);
				return -EINVAL;
			}

			data =
			    kzalloc(sizeof(struct wfd_data_block), GFP_KERNEL);
			if (data == NULL) {
				wfd_audio_stat.wfd_wt_fail++;
				printk(KERN_ERR
				       "WFD: failed alloc audio block");
				return -ENOMEM;
			}

			/* allocate enough memory per the writer's request */
			data->data =
			    kzalloc(sizeof(unsigned char) * WFD_AUDIO_SIZE,
				    GFP_KERNEL);
			if (data->data == NULL) {
				wfd_audio_stat.wfd_wt_fail++;
				kfree(data);
				printk(KERN_ERR
				       "WFD: failed alloc audio data block");
				return -ENOMEM;
			}

			data->size = WFD_AUDIO_SIZE;

			rc = copy_from_user(data->data,
					    (void __user *)audio_data.data,
					    data->size);
			if (rc) {
				wfd_audio_stat.wfd_wt_fail++;

				kfree(data->data);
				data->data = NULL;
				kfree(data);
				data = NULL;

				printk(KERN_ERR
				       "WFD: failed audio data->data ioctl, rc=%d",
				       rc);
				return rc;
			}

			mutex_lock(&wfd_audio_lock);
			list_add_tail(&(data->wfd_list), &(wfd_audio.wfd_list));
			mutex_unlock(&wfd_audio_lock);

			/* wake up the audio reader */
			wake_up_interruptible(&audio_wq);

			rc = 0;
		}
		break;

	case WFD_CMD_GET_AUDIO_DATA:
		if (wfd_current_state != WFD_STATE_PLAY &&
		    wfd_current_state != WFD_STATE_PAUSE)
			/* we will allow audio reading in PLAY
			 * and PAUSE states
			 */
			rc = -EBUSY;
		else {
			struct wfd_ioctl_audio_data audio_data;
			struct wfd_audio_block *data = NULL;
			int valid = 0;
			int retval;
			uint64_t usecs_since_start;
			int32_t usecs_to_sleep;
			long jiffies_to_sleep;
			struct timeval curr_time;

			/* get the struct from the user */
			rc = copy_from_user(&audio_data,
					    (void __user *)arg,
					    sizeof(struct
						   wfd_ioctl_audio_data));
			if (rc) {
				printk(KERN_ERR
				       "WFD: failed copy audio data ioctl, rc=%d",
				       rc);
				wfd_audio_stat.wfd_rd_fail++;
				return rc;
			}

			/* verify that the read size is expected */
			if (audio_data.size != WFD_AUDIO_SIZE) {
				printk(KERN_ERR
				       "WFD: unexpected audio get size. Expected=%d, got=%d",
				       WFD_AUDIO_SIZE, audio_data.size);
				return -EINVAL;
			}

			/* get the audio start time */
			if (audio_frame_cnt == 0)
				do_gettimeofday(&audio_start_time);

			/* increment the audio frames */
			audio_frame_cnt++;

			/* wait until data is valid or we timed out */
			if (list_empty(&(wfd_audio.wfd_list))) {

				/* calculate the number of usecs passed
				 * since the beginning, given the number
				 * of audio frames
				 */
				usecs_since_start =
				    div64_u64((1000000LL * WFD_AUDIO_SIZE *
					       audio_frame_cnt),
					      (AUDIO_SAMPLE_RATE *
					       AUDIO_SAMPLE_SIZE));

				/* get the current time */
				do_gettimeofday(&curr_time);

				/* determine the number of usecs to sleep */
				usecs_to_sleep =
				    (audio_start_time.tv_sec * 1000000LL +
				     audio_start_time.tv_usec +
				     usecs_since_start) -
				    (curr_time.tv_sec * 1000000LL +
				     curr_time.tv_usec);
				if (usecs_to_sleep < 0) {
					/* cannot sleep for negative values */
					usecs_to_sleep = 0;
				}

				WFD_KNLLOG
				    ("WFD_CMD_GET_AUDIO_DATA:"
				     " sleeping %d usecs", usecs_to_sleep);

				jiffies_to_sleep =
				    usecs_to_jiffies(usecs_to_sleep);

				if (jiffies_to_sleep > 1) {
					retval =
					    wait_event_interruptible_timeout
					    (audio_wq,
					     (!list_empty
					      (&(wfd_audio.wfd_list))),
					     jiffies_to_sleep - 1);
					if (retval < 0) {
						/* system interrupted */
						return -ERESTARTSYS;
					}

					WFD_KNLLOG
					    ("WFD_CMD_GET_AUDIO_DATA:"
					     " %d jiffies, %u usecs",
					     retval, jiffies_to_usecs(retval));
				}
			}

			mutex_lock(&wfd_audio_lock);
			/* check again to see if list is empty */
			if (!list_empty(&(wfd_audio.wfd_list))) {
				data =
				    list_first_entry(&(wfd_audio.wfd_list),
						     struct wfd_audio_block,
						     wfd_list);
				if (data) {
					valid = 1;
					list_del(&(data->wfd_list));
				}
			}
			mutex_unlock(&wfd_audio_lock);

			if (valid) {
				rc = copy_to_user((void __user *)
						  audio_data.data, data->data,
						  WFD_AUDIO_SIZE *
						  sizeof(char));
				if (rc) {
					printk(KERN_ERR
					       "WFD: failed read audio data ioctl, rc=%d",
					       rc);
					wfd_audio_stat.wfd_rd_fail++;
				} else
					wfd_audio_stat.wfd_rd++;

				kfree(data->data);
				data->data = NULL;
				kfree(data);
				data = NULL;
			} else {
				rc = -EINVAL;
				wfd_audio_stat.wfd_rd_fail++;
			}
		}
		break;

	case WFD_CMD_FLUSH_AUDIO_DATA:
		{
			flush_audio();
		}
		rc = 0;
		break;

	case WFD_CMD_SET_PROFILING_TS:

		/* add a new profiling timestamp(s) */
		{
			struct wfd_ioctl_profiling_ts ctrl;
			struct wfd_profiling_ts *ts_ptr;
			struct wfd_profiling_ts ts;

			if (!wfd_prof_enabled)
				return -EPERM;

			rc = copy_from_user(&ctrl, (void __user *)arg,
					    sizeof(struct
						   wfd_ioctl_profiling_ts));
			if (rc)
				return rc;

			ts_ptr = ctrl.first;
			while (ctrl.num_ts--) {
				rc = copy_from_user(&ts,
						    (void __user *)ts_ptr,
						    sizeof(struct
							   wfd_profiling_ts));
				if (rc)
					break;
				if (ts.type < WFD_PROFILING_TS_LAST)
					rc = wfd_queue_ts(ts.type,
							  ts.timestamp);
				else {
					rc = -EINVAL;
					break;
				}
				ts_ptr++;
			}
		}
		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

/***************************************************************************/
/**
*  Driver open routine
*
*  @return
*     0        Success
*     -ENOMEM  Insufficient memory
*/
static int wfd_open(struct inode *inode, struct file *file)
{
	/*
	 * Should ever been called once per run from the WFD core process, the
	 * media server process uses a dup of the descriptor opened by WFD core
	 * to write to the device...
	 */
	if (!wfd_refcount)
		memset(&wfd_stat, 0, sizeof(wfd_stat));

	wfd_refcount++;

	return 0;
}

/***************************************************************************/
/**
*  Driver release routine
*
*  @return
*     0        Success
*     -ve      Error code
*/
static int wfd_release(struct inode *inode, struct file *file)
{
	/*
	 * Should ever been called once per run from the WFD core process, the
	 * media server process uses a dup of the descriptor opened by WFD core
	 * to write to the device...
	 */
	if (wfd_refcount > 0)
		wfd_refcount--;

	return 0;
}

static ssize_t wfd_dequeue_and_copy(char __user *buffer, size_t count)
{
	struct wfd_data_block *data = NULL;
	int read = 0;
	int num_data_blocks = count / WFD_DATA_SIZE;
	uint8_t done = 0;

	/*
	 * Dequeue and copy in multiples of the WFD data
	 * block size (TS packet size).
	 */
	while (num_data_blocks > 0) {

		mutex_lock(&wfd_queue_lock);
		if (!list_empty(&(wfd_data.wfd_list))) {
			data = list_first_entry(&(wfd_data.wfd_list),
						struct wfd_data_block,
						wfd_list);
			if (likely(data)) {
				list_del(&(data->wfd_list));
				mutex_unlock(&wfd_queue_lock);
			} else {
				mutex_unlock(&wfd_queue_lock);
				break;
			}
		} else {
			mutex_unlock(&wfd_queue_lock);

			/* no data is available, wait */
			if (done)
			break;

			wfd_flush_required = false;

			if (wait_event_interruptible(read_wq,
						     (!list_empty
						      (&(wfd_data.wfd_list))
						      || wfd_flush_required))) {
				return -ERESTARTSYS;
			}

			/* if list is empty here, then we simply return */
			if (list_empty(&(wfd_data.wfd_list))) {
				wfd_flush_required = false;
				break;
		}

			continue;
		}

		/* we managed to get a block of data */
		num_data_blocks--;

		if (likely(copy_to_user((void *)buffer,
					&(data->data), data->size) == 0)) {
			read += data->size;
			buffer += data->size;

			if (data->eof)
				/* signal that we are not willing
				 * to block anymore
				 */
				done = 1;

			kfree(data);
		} else {
			kfree(data);
			wfd_stat.wfd_rd_fail++;
			return -EFAULT;
		}
		if (wfd_prof_enabled)
			wfd_prof_add_ts(WFD_PROFILING_TS_DRV_READ, 1);

		data = NULL;
	}

	wfd_stat.wfd_rd_bytes += read;

	return read;
}

/***************************************************************************/
/**
*  Driver read method
*
*  @return
*     >= 0           Number of bytes read
*     -ve            Error code
*/
static ssize_t wfd_read(struct file *file,
			char __user *buffer, size_t count, loff_t *ppos)
{
	int read = 0;

	if (wfd_current_state != WFD_STATE_PLAY) {
		/* tell the reader we have read nothing */
		return 0;
	}

	wfd_stat.wfd_rd++;

	if (!list_empty(&(wfd_data.wfd_list))) {
		read = wfd_dequeue_and_copy(buffer, count);
	} else {
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		while (list_empty(&(wfd_data.wfd_list))) {
			if (wait_event_interruptible(read_wq,
						     (!list_empty
						      (&(wfd_data.wfd_list)))))
				return -ERESTARTSYS;
		}

		read = wfd_dequeue_and_copy(buffer, count);
	}

	return read;
}

static ssize_t wfd_queue_and_copy(const char __user *buffer, size_t count)
{
	struct wfd_data_block *data = NULL;
	int write = 0;
	int num_data_blocks = count / WFD_DATA_SIZE;

	/*
	 * Queue and copy multiples of the WFD data block size
	 * (TS packet size) at a time
	 */
	while (num_data_blocks--) {
		data = kzalloc(sizeof(struct wfd_data_block), GFP_KERNEL);
		if (unlikely(data == NULL)) {
			wfd_stat.wfd_wt_fail++;
			return -ENOMEM;
		} else {
			if (likely(copy_from_user(&(data->data),
						  buffer,
						  sizeof(data->data)) == 0)) {
				data->size = sizeof(data->data);
				write += data->size;
				buffer += data->size;

				mutex_lock(&wfd_queue_lock);
				list_add_tail(&(data->wfd_list),
					      &(wfd_data.wfd_list));
				mutex_unlock(&wfd_queue_lock);
				if (wfd_prof_enabled)
					wfd_prof_add_ts
					    (WFD_PROFILING_TS_DRV_WRITE, 1);

				wake_up_interruptible(&read_wq);
			} else {
				wfd_stat.wfd_wt_fail++;
				kfree(data);
				return -EFAULT;
			}
		}
	}

	wfd_stat.wfd_wt_bytes += write;

	return write;
}

/***************************************************************************/
/**
*  Driver sync method.
*
*  @return
*   0 on success
*/
static int wfd_fsync(struct file *file, int datasync)
{
	struct wfd_data_block *data = NULL;

	if (wfd_current_state != WFD_STATE_PLAY) {
		/* pretend we have flushed the data */
		return 0;
	}

	mutex_lock(&wfd_queue_lock);
	if (!list_empty(&(wfd_data.wfd_list))) {

		/* simply mark the last item on the list with EOF */
		data = list_entry((&(wfd_data.wfd_list))->prev,
				  struct wfd_data_block, wfd_list);
		if (data)
			data->eof = 1;

		mutex_unlock(&wfd_queue_lock);
	} else {
		/* wake up the reader without queuing any data */
		wfd_flush_required = true;

		/* unlock mutex before signalling reader */
		mutex_unlock(&wfd_queue_lock);

		wake_up_interruptible(&read_wq);
	}

	wfd_stat.wfd_flush++;

	return 0;
}

/***************************************************************************/
/**
*  Driver write method.
*
*  @return
*     >= 0           Number of bytes write
*     -ve            Error code
*/
static ssize_t wfd_write(struct file *file,
			 const char __user *buffer, size_t count, loff_t *ppos)
{
	struct wfd_data_block *data;
	int write = 0;
	int ret;

	if (wfd_current_state != WFD_STATE_PLAY) {
		/* pretend we have written the data */
		return count;
	}

	wfd_stat.wfd_wt++;

	mutex_lock(&wfd_partial_lock);

	if (wfd_partial_block == NULL) {

		if (count < WFD_DATA_SIZE) {

			/*
			 * Since less than a WFD data block is being
			 * written, allocate a new partial block
			 */
			data = kzalloc(sizeof(struct wfd_data_block),
				       GFP_KERNEL);
			if (unlikely(data == NULL)) {
				mutex_unlock(&wfd_partial_lock);
				wfd_stat.wfd_wt_fail++;
				return -ENOMEM;
			}

			if (likely(copy_from_user(&(data->data),
						  buffer, count) == 0)) {
				data->size = count;
				write = count;
			} else {
				kfree(data);
				mutex_unlock(&wfd_partial_lock);
				wfd_stat.wfd_wt_fail++;
				return -EFAULT;
			}

			wfd_partial_block = data;
			mutex_unlock(&wfd_partial_lock);

			wfd_stat.wfd_wt_bytes += write;

			return write;
		}

		mutex_unlock(&wfd_partial_lock);

	} else {

		size_t avail_bytes = WFD_DATA_SIZE - wfd_partial_block->size;
		size_t copy_bytes = (count > avail_bytes ? avail_bytes : count);

		data = wfd_partial_block;

		if (likely(copy_from_user(&(data->data[data->size]),
					  buffer, copy_bytes) == 0)) {
			data->size += copy_bytes;
		} else {
			mutex_unlock(&wfd_partial_lock);
			wfd_stat.wfd_wt_fail++;
			return -EFAULT;
		}

		if (data->size == WFD_DATA_SIZE) {
			/*
			 * The partial block is now full, add it to the
			 * read queue
			 */
			mutex_lock(&wfd_queue_lock);
			list_add_tail(&(data->wfd_list), &(wfd_data.wfd_list));
			mutex_unlock(&wfd_queue_lock);

			wfd_partial_block = NULL;
			mutex_unlock(&wfd_partial_lock);
			if (wfd_prof_enabled)
				wfd_prof_add_ts(WFD_PROFILING_TS_DRV_WRITE, 1);

			wake_up_interruptible(&read_wq);
		} else
			mutex_unlock(&wfd_partial_lock);

		count -= copy_bytes;
		buffer += copy_bytes;

		write = copy_bytes;
		wfd_stat.wfd_wt_bytes += copy_bytes;
	}

	ret = wfd_queue_and_copy(buffer, count);
	if (unlikely(0 > ret))
		return ret;

	write += ret;

	return write;
}

/***************************************************************************/
/**
*  Driver info proc entry read method.
*
*/
static int wfd_info_proc_read(char *buf,
			      char **start,
			      off_t offset, int count, int *eof, void *data)
{
	char *p = buf;
	char *current_state;

	(void)start;
	(void)count;
	(void)data;

	if (offset > 0) {
		*eof = 1;
		return 0;
	}

	p += sprintf(p, "[wfd-conn-state]   %s\n\n",
		     wfd_con_state.status ? "CONNECTED" : "DISCONNECTED");

	switch (wfd_current_state) {
	case WFD_STATE_IDLE:
		current_state = "IDLE";
		break;
	case WFD_STATE_START:
		current_state = "START";
		break;
	case WFD_STATE_PLAY:
		current_state = "PLAY";
		break;
	case WFD_STATE_PAUSE:
		current_state = "PAUSE";
		break;
	default:
		break;
	}

	p += sprintf(p, "[wfd-state]   %s\n\n", current_state);

	p += sprintf(p, "[wfd-meta]    \'%c%c%c%c\'\n\n",
		     (wfd_metadata.action >> 24) & 0xFF,
		     (wfd_metadata.action >> 16) & 0xFF,
		     (wfd_metadata.action >> 8) & 0xFF,
		     (wfd_metadata.action) & 0xFF);

	p += sprintf(p, "[wfd-config]  video: %dx%d @ %d fps - \'%c\' scan\n",
		     wfd_neg_config.width,
		     wfd_neg_config.height,
		     wfd_neg_config.fps, wfd_neg_config.scan_mode);

	p += sprintf(p, "[wfd-config]  audio: %d @ %d Hz - %d ch\n\n",
		     (int)wfd_neg_config.audio_fmt,
		     wfd_neg_config.audio_hz, wfd_neg_config.audio_ch);

	p += sprintf(p, "[wfd-vid-rd]  %llu calls, %llu bytes, %llu failure\n",
		     wfd_stat.wfd_rd,
		     wfd_stat.wfd_rd_bytes, wfd_stat.wfd_rd_fail);
	p += sprintf(p,
		     "[wfd-vid-wt]  %llu calls, %llu bytes, %llu failure\n\n",
		     wfd_stat.wfd_wt, wfd_stat.wfd_wt_bytes,
		     wfd_stat.wfd_wt_fail);

	p += sprintf(p, "[wfd-aud-rd]  %llu calls, %llu failure\n",
		     wfd_audio_stat.wfd_rd, wfd_audio_stat.wfd_rd_fail);
	p += sprintf(p,
		     "[wfd-aud-wt]  %llu calls, %llu failure\n\n",
		     wfd_audio_stat.wfd_wt, wfd_audio_stat.wfd_wt_fail);

	p += sprintf(p, "[wfd-flush]  %llu calls\n\n", wfd_stat.wfd_flush);

	*eof = 1;
	return p - buf;
}

/***************************************************************************/
/**
*  Proc read function to display counters for each of the timestamp types.
*
*/
static int wfd_prof_counts_proc_read(char *buf,
				     char **start,
				     off_t offset, int count, int *eof,
				     void *data)
{
	char *p = buf;
	unsigned long hw_composer;
	unsigned long codec;
	unsigned long mpeg2ts_writer;
	unsigned long driver_write;
	unsigned long driver_read;
	unsigned long net_send;

	(void)start;
	(void)count;
	(void)data;

	if (offset > 0) {
		*eof = 1;
		return 0;
	}

	mutex_lock(&wfd_prof_lock);
	hw_composer = wfd_ts_lists[WFD_PROFILING_TS_HW_COMPOSER].num_entries;
	codec = wfd_ts_lists[WFD_PROFILING_TS_CODEC].num_entries;
	mpeg2ts_writer =
	    wfd_ts_lists[WFD_PROFILING_TS_MPEG2TS_WRITER].num_entries;
	driver_write = wfd_ts_lists[WFD_PROFILING_TS_DRV_WRITE].num_entries;
	driver_read = wfd_ts_lists[WFD_PROFILING_TS_DRV_READ].num_entries;
	net_send = wfd_ts_lists[WFD_PROFILING_TS_NETWORK_SEND].num_entries;
	mutex_unlock(&wfd_prof_lock);

	p += sprintf(p, "[HW Composer]       %lu\n", hw_composer);
	p += sprintf(p, "[Codec]             %lu\n", codec);
	p += sprintf(p, "[MPEG2TS Writer]    %lu\n", mpeg2ts_writer);
	p += sprintf(p, "[WFD Driver Write]  %lu\n", driver_write);
	p += sprintf(p, "[WFD Driver Read]   %lu\n", driver_read);
	p += sprintf(p, "[Network Send]      %lu\n", net_send);

	*eof = 1;
	return p - buf;
}

/***************************************************************************/
/**
*  Proc read function to show whether profiling is enabled or not.
*
*/
static int wfd_prof_enable_proc_read(char *buf, char **start,
				     off_t offset, int count, int *eof,
				     void *data)
{
	char *p = buf;

	(void)start;
	(void)count;
	(void)data;

	if (offset > 0) {
		*eof = 1;
		return 0;
	}

	p += sprintf(p, "%ld\n", wfd_prof_enabled);

	*eof = 1;
	return p - buf;
}

/***************************************************************************/
/**
*  Proc write function to control whether profiling is enabled or not.
*  A value of '1' is used to enable profiling, and '0' to disable it.
*
*/
static int wfd_prof_enable_proc_write(struct file *file,
				      const char __user *buf,
				      unsigned long count, void *data)
{
	char temp[3];
	long enable;

	(void)file;
	(void)data;

	if (count >= sizeof(temp))
		return -EINVAL;

	if (copy_from_user(temp, buf, count))
		return -EFAULT;

	temp[count] = '\0';

	if (kstrtol(temp, 10, &enable))
		return -EINVAL;

	if ((enable != 1) && (enable != 0))
		return -EINVAL;

	if (enable != wfd_prof_enabled) {

		if (enable && (wfd_ts_lists_get_max_count() != 0)) {
			printk(KERN_ALERT "WFD: please dump existing"
			       "timestamps before re-enabling profiling\n");
			return -EINVAL;
		}

		wfd_prof_enabled = enable;

		if (wfd_prof_enabled)
			nf_register_hook(&wfd_nf_ops);
		else
			nf_unregister_hook(&wfd_nf_ops);
	}

	return count;
}

/***************************************************************************/
/**
*  Proc read function to show the max number of timestamps stored.
*
*/
static int wfd_prof_max_ts_proc_read(char *buf, char **start,
				     off_t offset, int count, int *eof,
				     void *data)
{
	char *p = buf;

	(void)start;
	(void)count;
	(void)data;

	if (offset > 0) {
		*eof = 1;
		return 0;
	}

	p += sprintf(p, "%ld\n", wfd_prof_max_ts);

	*eof = 1;
	return p - buf;
}

/***************************************************************************/
/**
*  Proc write function to set the max number of timestamps that will be
*  stored.
*
*/
static int wfd_prof_max_ts_proc_write(struct file *file,
				      const char __user *buf,
				      unsigned long count, void *data)
{
	char temp[64];
	long max_ts;

	(void)file;
	(void)data;

	if (count >= sizeof(temp))
		return -EINVAL;

	if (wfd_prof_enabled) {
		printk(KERN_ALERT "WFD: please disable profiling before"
		       "changing this value\n");
		return -EINVAL;
	}

	if (copy_from_user(temp, buf, count))
		return -EFAULT;

	temp[count] = '\0';

	if (kstrtol(temp, 10, &max_ts))
		return -EINVAL;

	wfd_prof_max_ts = max_ts;

	return count;
}

/***************************************************************************/
/**
*  Proc write function to dump all of the collected timestamps to a file.
*  A value of '1' is used to trigger the dump.
*/
static int wfd_prof_dump_proc_write(struct file *file, const char __user *buf,
				    unsigned long count, void *data)
{
	char temp[3];
	long dump;

	(void)file;
	(void)data;

	if (count >= sizeof(temp))
		return -EINVAL;

	if (copy_from_user(temp, buf, count))
		return -EFAULT;

	temp[count] = '\0';

	if (kstrtol(temp, 10, &dump))
		return -EINVAL;

	if (wfd_prof_enabled) {
		printk(KERN_ALERT "WFD: please disable profiling first\n");
		return -EINVAL;
	}

	if (dump == 1) {
		printk(KERN_ALERT
		       "WFD: starting dump of profiling timestamps ...\n");
		wfd_prof_prepare_dump_file();
		printk(KERN_ALERT "WFD: ... dump complete! Wrote to %s\n",
		       WFD_TS_DUMP_FILE);
	} else {
		printk(KERN_ALERT "WFD: invalid dump value\n");
		return -EINVAL;
	}

	return count;
}

/***************************************************************************/
/**
*  Proc write function to add a new timestamp.
*  Written as a string with format: 'type,timestamp' e.g. '2,23043923'
*
*/
static int wfd_prof_add_ts_proc_write(struct file *file,
				      const char __user *buf,
				      unsigned long count, void *data)
{
	char temp[64];
	char *s;
	long type;
	long timestamp;

	(void)file;
	(void)data;

	if (!wfd_prof_enabled)
		return -EPERM;

	if (count >= sizeof(temp))
		return -EINVAL;

	if (copy_from_user(temp, buf, count))
		return -EFAULT;

	temp[count] = '\0';

	s = strstr(temp, ",");

	if (!s)
		goto bad_format;

	*s++ = '\0';		/* replace comma will null char */

	if (kstrtol(temp, 10, &type))
		goto bad_format;

	if (kstrtol(s, 10, &timestamp))
		goto bad_format;

	if (type >= WFD_PROFILING_TS_LAST) {
		printk(KERN_ALERT "WFD: invalid timestamp type %ld\n", type);
		return -EINVAL;
	}

	wfd_queue_ts(type, timestamp);

	return count;

bad_format:
	printk(KERN_ALERT
	       "WFD: bad string format, unable to add new timestamp\n");
	return -EINVAL;
}

/***************************************************************************/
/**
*  Create all procfs entries for WFD driver
*
*/
static int wfd_create_proc_entries(void)
{
	wfd_proc_dir = proc_mkdir(WFD_DEVICE_NAME, NULL);
	if (!wfd_proc_dir)
		goto proc_dir_fail;

	wfd_prof_proc_dir = proc_mkdir("profiling", wfd_proc_dir);
	if (!wfd_prof_proc_dir)
		goto prof_proc_dir_fail;

	/* WFD driver info proc entry */
	wfd_info_proc_entry = create_proc_entry("info", 0660, wfd_proc_dir);
	if (!wfd_info_proc_entry)
		goto info_proc_fail;

	wfd_info_proc_entry->read_proc = wfd_info_proc_read;
	wfd_info_proc_entry->write_proc = NULL;

	/* Profiling enable proc entry */
	wfd_prof_enable_proc_entry =
	    create_proc_entry("enable", 0660, wfd_prof_proc_dir);
	if (!wfd_prof_enable_proc_entry)
		goto prof_enable_proc_fail;

	wfd_prof_enable_proc_entry->read_proc = wfd_prof_enable_proc_read;
	wfd_prof_enable_proc_entry->write_proc = wfd_prof_enable_proc_write;

	/* Profiling timetstamp dump proc entry */
	wfd_prof_dump_proc_entry =
	    create_proc_entry("dump", 0660, wfd_prof_proc_dir);
	if (!wfd_prof_dump_proc_entry)
		goto prof_dump_proc_fail;

	wfd_prof_dump_proc_entry->read_proc = NULL;
	wfd_prof_dump_proc_entry->write_proc = wfd_prof_dump_proc_write;

	/* Profiling counts proc entry */
	wfd_prof_counts_proc_entry =
	    create_proc_entry("counts", 0660, wfd_prof_proc_dir);
	if (!wfd_prof_counts_proc_entry)
		goto prof_counts_proc_fail;

	wfd_prof_counts_proc_entry->read_proc = wfd_prof_counts_proc_read;
	wfd_prof_counts_proc_entry->write_proc = NULL;

	/* Profiling max timestamps proc entry */
	wfd_prof_max_ts_proc_entry =
	    create_proc_entry("max_ts", 0660, wfd_prof_proc_dir);
	if (!wfd_prof_max_ts_proc_entry)
		goto prof_max_ts_proc_fail;

	wfd_prof_max_ts_proc_entry->read_proc = wfd_prof_max_ts_proc_read;
	wfd_prof_max_ts_proc_entry->write_proc = wfd_prof_max_ts_proc_write;

	/* Profiling add timestamp proc entry */
	wfd_prof_add_ts_proc_entry =
	    create_proc_entry("add_ts", 0660, wfd_prof_proc_dir);
	if (!wfd_prof_add_ts_proc_entry)
		goto prof_add_ts_proc_fail;

	wfd_prof_add_ts_proc_entry->read_proc = NULL;
	wfd_prof_add_ts_proc_entry->write_proc = wfd_prof_add_ts_proc_write;

	return 0;

prof_add_ts_proc_fail:
	remove_proc_entry(wfd_prof_max_ts_proc_entry->name, wfd_prof_proc_dir);
prof_max_ts_proc_fail:
	remove_proc_entry(wfd_prof_counts_proc_entry->name, wfd_prof_proc_dir);
prof_counts_proc_fail:
	remove_proc_entry(wfd_prof_dump_proc_entry->name, wfd_prof_proc_dir);
prof_dump_proc_fail:
	remove_proc_entry(wfd_prof_enable_proc_entry->name, wfd_prof_proc_dir);
prof_enable_proc_fail:
	remove_proc_entry(wfd_info_proc_entry->name, wfd_prof_proc_dir);
info_proc_fail:
	remove_proc_entry(wfd_prof_proc_dir->name, wfd_proc_dir);
prof_proc_dir_fail:
	remove_proc_entry(wfd_proc_dir->name, NULL);
proc_dir_fail:
	return -EFAULT;
}

/***************************************************************************/
/**
*  Remove all procfs entries for WFD driver.
*
*/
static void wfd_remove_proc_entries(void)
{
	remove_proc_entry(wfd_prof_max_ts_proc_entry->name, wfd_prof_proc_dir);
	remove_proc_entry(wfd_prof_counts_proc_entry->name, wfd_prof_proc_dir);
	remove_proc_entry(wfd_prof_dump_proc_entry->name, wfd_prof_proc_dir);
	remove_proc_entry(wfd_prof_enable_proc_entry->name, wfd_prof_proc_dir);
	remove_proc_entry(wfd_info_proc_entry->name, wfd_prof_proc_dir);
	remove_proc_entry(wfd_prof_proc_dir->name, wfd_proc_dir);
	remove_proc_entry(wfd_proc_dir->name, NULL);
}

/***************************************************************************/
/**
*  Netfilter hook used for WFD profiling. The final timestamp for each TS
*  packet is added when the RTP packet is sent to the p2p interface.
*/
static unsigned int wfd_prof_netfilter_hook(unsigned int hooknum,
					    struct sk_buff *skb,
					    const struct net_device *in,
					    const struct net_device *out,
					    int (*okfn) (struct sk_buff *))
{
	struct iphdr *ip_header;

	if (unlikely(!skb || !out))
		return NF_ACCEPT;

	ip_header = ip_hdr(skb);

	/* verify transport type is UDP */
	if (ip_header->protocol == IPPROTO_UDP) {

		/* verify this is a P2P interface */
		if (strstr(out->name, "p2p")) {
			struct udphdr *udp_header = udp_hdr(skb);
			u8 *rtp_header = (u8 *)(udp_header + 1);

			/* verify that this is an RTP packet */
			if ((rtp_header[0]) == 0x80 && (rtp_header[1] == 33)) {
				/*
				 * Determine the number of MPEG2 TS packets
				 * since there can be more than one per RTP
				 * packet. Subtract the UDP header (8 bytes)
				 * and the RTP header (12 bytes) to
				 * determine the payload length.
				 */
				int num_ts =
				    ((ntohs(udp_header->len) - 8 -
				      12) / WFD_DATA_SIZE);

				wfd_prof_add_ts(WFD_PROFILING_TS_NETWORK_SEND,
						num_ts);
			}
		}
	}
	return NF_ACCEPT;
}

/***************************************************************************/
/**
*  Platform support constructor
*/
static int wfd_probe(struct platform_device *pdev)
{
	int err = 0;
	int i;

	gDriverMajor = register_chrdev(0, WFD_DEVICE_NAME, &gfops);
	if (gDriverMajor < 0) {
		printk(KERN_ERR
		       "WFD: Failed to register character device major\n");
		err = -EFAULT;
		goto error_cleanup;
	}
#ifdef CONFIG_SYSFS
	wfd_class = class_create(THIS_MODULE, "wfd-class");
	if (IS_ERR(wfd_class)) {
		printk(KERN_ERR "WFD: Class create failed\n");
		err = -EFAULT;
		goto err_unregister_chrdev;
	}

	wfd_dev = device_create(wfd_class,
				NULL,
				MKDEV(gDriverMajor, 0), NULL, WFD_DEVICE_NAME);
	if (IS_ERR(wfd_dev)) {
		printk(KERN_ERR "WFD: Device create failed\n");
		err = -EFAULT;
		goto err_class_destroy;
	}
#endif

	if (wfd_create_proc_entries()) {
		err = -EFAULT;
		printk(KERN_ERR "WFD: Proc-entry create failed\n");
		goto err_device_destroy;
	}

	memset(&wfd_stat, 0, sizeof(wfd_stat));
	memset(&wfd_neg_config, 0, sizeof(wfd_neg_config));
	memset(&wfd_con_state, 0, sizeof(wfd_con_state));

	INIT_LIST_HEAD(&(wfd_data.wfd_list));
	INIT_LIST_HEAD(&(wfd_audio.wfd_list));
	mutex_init(&wfd_queue_lock);
	mutex_init(&wfd_partial_lock);
	mutex_init(&wfd_audio_lock);
	mutex_init(&wfd_prof_lock);

	for (i = 0; i < (sizeof(wfd_ts_lists) / sizeof(wfd_ts_lists[0])); i++)
		INIT_LIST_HEAD(&(wfd_ts_lists[i].list.list_head));

	printk(KERN_INFO "WiFi Display - \'WFD\' driver...\n");
	return 0;

err_device_destroy:
#ifdef CONFIG_SYSFS
	device_destroy(wfd_class, MKDEV(gDriverMajor, 0));
err_class_destroy:
	class_destroy(wfd_class);
err_unregister_chrdev:
	unregister_chrdev(gDriverMajor, WFD_DEVICE_NAME);
#endif
error_cleanup:
	wfd_remove(pdev);
	return err;
}

/***************************************************************************/
/**
*  Platform support destructor
*/
static int wfd_remove(struct platform_device *pdev)
{
	unsigned int i;
	struct wfd_ts_block *ts_block;

	wfd_remove_proc_entries();
#ifdef CONFIG_SYSFS
	device_destroy(wfd_class, MKDEV(gDriverMajor, 0));
	class_destroy(wfd_class);
#endif

	wfd_prof_enabled = 0;

	/* drain any remaining profiling timestamps */
	mutex_lock(&wfd_prof_lock);
	for (i = 0; i < WFD_PROFILING_TS_LAST; i++) {
		while (!list_empty(&(wfd_ts_lists[i].list.list_head))) {
			ts_block =
			    list_first_entry
			    (&(wfd_ts_lists[i].list.list_head),
			     struct wfd_ts_block, list_head);
			if (likely(ts_block)) {
				list_del(&(ts_block->list_head));
				kfree(ts_block);
			}
		}
		wfd_ts_lists[i].num_entries = 0;
	}
	mutex_unlock(&wfd_prof_lock);

	unregister_chrdev(gDriverMajor, "wfd");

	mutex_destroy(&wfd_queue_lock);
	mutex_destroy(&wfd_audio_lock);
	mutex_destroy(&wfd_partial_lock);
	mutex_destroy(&wfd_prof_lock);

	return 0;
}

/* Platform driver */
static struct platform_driver wfd_driver = {
	.driver = {
		   .name = "bcm-wfd",
		   .owner = THIS_MODULE,
		   },
	.probe = wfd_probe,
	.remove = wfd_remove,
};

/***************************************************************************/
/**
*  Allocate and queue up a timestamp of a particular type.
*
*  @return
*     ==0            Success
*     -ve            Error code
*/
static int wfd_queue_ts(unsigned int type, unsigned long timestamp)
{
	struct wfd_ts_block *ts_block;

	ts_block = kzalloc(sizeof(struct wfd_ts_block), GFP_KERNEL);
	if (unlikely(ts_block == NULL))
		return -ENOMEM;

	ts_block->timestamp = timestamp;

	mutex_lock(&wfd_prof_lock);
	if (wfd_ts_lists[type].num_entries < wfd_prof_max_ts) {
		list_add_tail(&(ts_block->list_head),
			      &(wfd_ts_lists[type].list.list_head));
		wfd_ts_lists[type].num_entries++;
	} else
		kfree(ts_block);

	mutex_unlock(&wfd_prof_lock);

	return 0;
}

/***************************************************************************/
/**
*  Dequeue and free a timestamp of a particular type.
*
*  @return
*     ==0            Success
*     -ve            Error code
*/
static int wfd_dequeue_ts(unsigned int type, unsigned long *timestamp)
{
	struct wfd_ts_block *ts_block = NULL;
	int ret = -EINVAL;

	mutex_lock(&wfd_prof_lock);
	if (!list_empty(&(wfd_ts_lists[type].list.list_head))) {

		ts_block =
		    list_first_entry(&(wfd_ts_lists[type].list.list_head),
				     struct wfd_ts_block, list_head);
		if (likely(ts_block)) {
			list_del(&(ts_block->list_head));
			wfd_ts_lists[type].num_entries--;
			*timestamp = ts_block->timestamp;
			kfree(ts_block);
			ret = 0;
		}
	}
	mutex_unlock(&wfd_prof_lock);

	return ret;
}

/***************************************************************************/
/**
*  Add a new timestamp(s) of a particular type.
*
*  @return
*     ==0            Success
*     -ve            Error code
*/
static int wfd_prof_add_ts(unsigned int type, unsigned int count)
{
	struct timeval tv;
	unsigned long ts;
	unsigned int ret = -EINVAL;

	if (!wfd_prof_enabled)
		return -EPERM;

	do_gettimeofday(&tv);
	ts = (tv.tv_sec * 1000000) + tv.tv_usec;

	while (count--) {
		ret = wfd_queue_ts(type, ts);
		if (unlikely(ret))
			break;
	}

	return ret;
}

/***************************************************************************/
/**
*  Get the number of timestamps of a particular type / in a particular list.
*
*  @return
*     >=0            Number of timestamps
*/
static unsigned int wfd_ts_list_get_count(unsigned int type)
{
	unsigned int count;

	mutex_lock(&wfd_prof_lock);
	count = wfd_ts_lists[type].num_entries;
	mutex_unlock(&wfd_prof_lock);

	return count;
}

/***************************************************************************/
/**
*  Get the max number of timestamps of all types.
*
*  @return
*     >=0            Number of timestamps
*/
static unsigned int wfd_ts_lists_get_max_count(void)
{
	int i;
	int count;
	int max = 0;

	for (i = 0; i < WFD_PROFILING_TS_LAST; i++) {
		count = wfd_ts_list_get_count(i);
		if (count > max)
			max = count;
	}

	return max;
}

/***************************************************************************/
/**
*  Create and open the file used to dump timestamps.
*
*/
static struct file *wfd_prof_dump_file_open(const char *filename)
{
	struct file *filep;
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	filep = filp_open(filename, O_TRUNC | O_WRONLY | O_CREAT, 0644);
	set_fs(old_fs);

	return filep;
}

/***************************************************************************/
/**
*  Close the file used to dump timestamps.
*
*/
static void wfd_prof_dump_file_close(struct file *filep)
{
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	filp_close(filep, current->files);
	set_fs(old_fs);
}

/***************************************************************************/
/**
*  Write to the file used to dump timestamps.
*
*/
static void wfd_prof_dump_file_write(struct file *filep, char *str)
{
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	filep->f_op->write(filep, str, strlen(str), &filep->f_pos);
	set_fs(old_fs);
}

/***************************************************************************/
/**
*  Creates a CSV (comma separated values) format file with the timestamps.
*  Missing timestamps are listed as 'x'.
*
*/
static void wfd_prof_prepare_dump_file(void)
{
	struct file *filep;
	unsigned int num_ts;
	unsigned int i;
	unsigned int j;
	unsigned int offset;
	unsigned long ts;
	char buf[256];

	filep = wfd_prof_dump_file_open(WFD_TS_DUMP_FILE);

	if (!filep)
		return;

	/*
	 * Determine the max number of timestamps in all
	 * of the lists
	 */
	num_ts = wfd_ts_lists_get_max_count();

	/*
	 * Look through all of the timestamp types and
	 * store the timestamps for a single TS packet on
	 * one line.
	 */
	for (i = 0; i < num_ts; i++, offset = 0) {
		offset += sprintf(&buf[offset], "%u, ", i);
		for (j = 0; j < WFD_PROFILING_TS_LAST; j++) {
			if (0 > wfd_dequeue_ts(j, &ts))
				ts = 0;
			if (!ts)
				offset += sprintf(&buf[offset], "x");
			else
				offset += sprintf(&buf[offset], "%lu", ts);
			if (j != WFD_PROFILING_TS_LAST - 1)
				offset += sprintf(&buf[offset], ", ");
			else
				offset += sprintf(&buf[offset], "\n");
		}
		wfd_prof_dump_file_write(filep, buf);
	}

	wfd_prof_dump_file_close(filep);
}

static int __init wfd_init(void)
{
	WFD_KNLLOG("%s: called...\n", __func__);

	return platform_driver_register(&wfd_driver);
}

static void __exit wfd_exit(void)
{
	WFD_KNLLOG("%s: called...\n", __func__);

	platform_driver_unregister(&wfd_driver);
}

module_init(wfd_init);
module_exit(wfd_exit);
MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("WiFi Display Datapath.");
MODULE_LICENSE("GPL");
