/*****************************************************************************
 * Copyright 2012 Broadcom Corporation.  All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>


#include "vchiq.h"
#include "vchiq_connected.h"

#define DRIVER_NAME  "vc-watchdog"

/*#define ENABLE_LOG_DBG*/

#ifdef ENABLE_LOG_DBG
#define LOG_DBG(fmt, arg...)   printk(KERN_INFO "[D] " fmt "\n", ##arg)
#else
#define LOG_DBG(fmt, arg...)
#endif
#define LOG_INFO(fmt, arg...)  printk(KERN_INFO "[I] " fmt "\n", ##arg)
#define LOG_ERR(fmt, arg...)   printk(KERN_ERR  "[E] " fmt "\n", ##arg)

#define PROC_WRITE_BUF_SIZE 256

#define WDOG_PING_MSG       0x08192A3B
#define WDOG_PING_RESPONSE  0x4C5D6E7F
#define VC_WDOG_VERSION     1
#define VC_WDOG_VERSION_MIN 1


/* How often we will ping VideoCore in ms */
#define WATCHDOG_PING_RATE_MS         500
/* Number of pings without response before we kernel panic */
#define WATCHDOG_NO_RESPONSE_COUNT     10
/* How long we will wait for a response to a ping message */
#define VC_PING_RESPONSE_TIMEOUT_MS  1000

/* Turn this on by default.  It's better to not include the config option
 * on platforms which we might want to attach a debugger */
#define ENABLE_VC_WATCHDOG


static int vc_watchdog_probe(struct platform_device *p_dev);
static int __devexit vc_watchdog_remove(struct platform_device *p_dev);
static int vc_watchdog_proc_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data);

/* Structures exporting functions. */
static struct platform_driver vc_watchdog_driver = {
	.probe      = vc_watchdog_probe,
	.remove     = __devexit_p(vc_watchdog_remove),
	.driver = {
		.name = "vc-watchdog"
	}
};

struct vc_watchdog_state {
	VCHIQ_SERVICE_HANDLE_T  service_handle;
	VCHIQ_INSTANCE_T        initialise_instance;
	struct platform_device *p_dev;
	struct proc_dir_entry  *proc_entry;
	struct completion       wdog_ping_response;
	struct completion       wdog_disable_blocker;
	struct mutex            wdog_ping_mutex;
	atomic_t                wdog_enabled;
	struct task_struct     *wdog_thread;
	int                     failed_pings;
	unsigned int            returned_seqnum;
};
static struct vc_watchdog_state *vc_wdog_state;


static VCHIQ_STATUS_T
vc_watchdog_vchiq_callback(VCHIQ_REASON_T reason,
	VCHIQ_HEADER_T *header,
	VCHIQ_SERVICE_HANDLE_T service_user,
	void *bulk_user)
{
	switch (reason) {
	case VCHIQ_MESSAGE_AVAILABLE:
		{
		unsigned long *msg = (unsigned long *)header->data;

		if (msg && (*msg == WDOG_PING_RESPONSE)) {
			if (header->size > sizeof(unsigned long))
				vc_wdog_state->returned_seqnum =
							*(msg + 1) & 0xFF;
			else
				vc_wdog_state->returned_seqnum = 0x100;
			complete(&vc_wdog_state->wdog_ping_response);
			LOG_DBG("%s received ping response", __func__);
		} else {
			LOG_ERR("%s received unexpected message - ignoring",
				__func__);
		}
		}
		vchiq_release_message(service_user, header);
		break;
	default:
		break;
	}
	return VCHIQ_SUCCESS;
}

static int
vc_watchdog_thread_func(void *v)
{
	while (1) {
		long rc;
		static unsigned int seqnum;
		u64 microsecs;
		unsigned int cmp_seq;
		unsigned long msg[2] = { WDOG_PING_MSG, 0 };
		VCHIQ_ELEMENT_T elem = {
			.data = (void *)&msg,
			.size = sizeof(msg)
		};
		int time_remaining =
			msecs_to_jiffies(WATCHDOG_PING_RATE_MS);

		LOG_DBG("%s: waiting on disable blocker...", __func__);
		if (wait_for_completion_interruptible(
				&vc_wdog_state->wdog_disable_blocker) != 0) {
			flush_signals(current);
			continue;
		}

		LOG_DBG("%s: Waiting for VC to be awake...", __func__);
		/* Ensure we only ping videocore when it's awake. Call use
		 * service in a mode which will not initiate a wakeup */
		vchiq_use_service_no_resume(vc_wdog_state->service_handle);

		if (!atomic_read(&vc_wdog_state->wdog_enabled)) {
			vchiq_release_service(vc_wdog_state->service_handle);
			LOG_DBG("%s: VC watchdog disabled", __func__);
			continue;
		}

		if (mutex_lock_interruptible(&vc_wdog_state->wdog_ping_mutex)
				!= 0) {
			vchiq_release_service(vc_wdog_state->service_handle);
			LOG_DBG("%s: Interrupted waiting for ping mutex",
				__func__);
			continue;
		}

		LOG_DBG("%s: Pinging videocore", __func__);
		/* ping vc...
		 * include timestamp to allow vchiq slot data to be correlated
		 * with kernel log.
		 * include sequence number to allow ping and response to be
		 * correlated. */
		microsecs = cpu_clock(0);
		do_div(microsecs, 1000);
		cmp_seq = (seqnum++) & 0xFF;
		msg[1] = (((unsigned long)microsecs) & 0xFFFFFF00) |
							(cmp_seq & 0xFF);
		vchiq_queue_message(vc_wdog_state->service_handle, &elem, 1);

		LOG_DBG("%s: Waiting for ping response", __func__);
		/* ...and wait for the response with a timeout */
		rc = wait_for_completion_interruptible_timeout(
			&vc_wdog_state->wdog_ping_response,
			msecs_to_jiffies(VC_PING_RESPONSE_TIMEOUT_MS));

		if (rc == 0) {
			/* Timed out... BANG! */
			vc_wdog_state->failed_pings++;
			LOG_ERR("%s VideoCore Watchdog timed out!! (%d - "
				"seqnum %d)",
				__func__, vc_wdog_state->failed_pings, cmp_seq);
			if (vc_wdog_state->failed_pings >=
						WATCHDOG_NO_RESPONSE_COUNT)
				BUG();
		} else if (rc < 0)
			LOG_ERR("%s: Interrupted waiting for ping", __func__);
		else {
			if (vc_wdog_state->returned_seqnum != 0x100 &&
					(vc_wdog_state->returned_seqnum !=
						cmp_seq))
				LOG_ERR("%s: Out of sequence ping response "
					"received. Expected %d, got %d",
					__func__,
					cmp_seq,
					vc_wdog_state->returned_seqnum);
			else
				LOG_DBG("%s: Ping response received", __func__);
			vc_wdog_state->failed_pings = 0;
		}

		mutex_unlock(&vc_wdog_state->wdog_ping_mutex);

		vchiq_release_service(vc_wdog_state->service_handle);

		LOG_DBG("%s: waiting before pinging again...", __func__);
		/* delay before running again */
		do {
			set_current_state(TASK_INTERRUPTIBLE);
			time_remaining = schedule_timeout(time_remaining);
			if (time_remaining) {
				LOG_ERR("%s interrupted", __func__);
				flush_signals(current);
			}
		} while (time_remaining > 0);
	}

	return 0;
}

static void vc_watchdog_connected_init(void)
{
	int ret = 0;
	VCHIQ_SERVICE_PARAMS_T vchiq_params = {
		.fourcc      = VCHIQ_MAKE_FOURCC('W', 'D', 'O', 'G'),
		.callback    = vc_watchdog_vchiq_callback,
		.version     = VC_WDOG_VERSION,
		.version_min = VC_WDOG_VERSION_MIN
	};

	LOG_INFO("%s: start", __func__);

	/* Initialize and create a VCHIQ connection */
	ret = vchiq_initialise(&vc_wdog_state->initialise_instance);
	if (ret != 0) {
		LOG_ERR("%s: failed to initialise VCHIQ instance (ret=%d)",
				__func__, ret);
		ret = -EIO;
		goto out;
	}
	ret = vchiq_connect(vc_wdog_state->initialise_instance);
	if (ret != 0) {
		LOG_ERR("%s: failed to connect VCHIQ instance (ret=%d)",
				__func__, ret);
		ret = -EIO;
		goto out;
	}

	ret = vchiq_open_service(vc_wdog_state->initialise_instance,
			&vchiq_params, &vc_wdog_state->service_handle);
	if (ret != 0 || (vc_wdog_state->service_handle == 0)) {
		LOG_ERR("%s: failed to add WDOG service: error %d",
				__func__, ret);
		ret = -EPERM;
		goto out;
	}

	vchiq_release_service(vc_wdog_state->service_handle);

	init_completion(&vc_wdog_state->wdog_ping_response);
	mutex_init(&vc_wdog_state->wdog_ping_mutex);
	init_completion(&vc_wdog_state->wdog_disable_blocker);

#ifdef ENABLE_VC_WATCHDOG
	complete_all(&vc_wdog_state->wdog_disable_blocker);
	atomic_set(&vc_wdog_state->wdog_enabled, 1);
#else
	atomic_set(&vc_wdog_state->wdog_enabled, 0);
#endif

	vc_wdog_state->wdog_thread = kthread_create(
		&vc_watchdog_thread_func, NULL, "vc-watchdog");
	if (vc_wdog_state->wdog_thread == NULL)
		LOG_ERR("FATAL: couldn't create thread vc-watchdog");
	else
		wake_up_process(vc_wdog_state->wdog_thread);

out:
	LOG_INFO("%s: end (ret=%d)", __func__, ret);
}

static int vc_watchdog_probe(struct platform_device *p_dev)
{
	int ret = 0;
	LOG_INFO("%s: start", __func__);
	vc_wdog_state = kzalloc(sizeof(struct vc_watchdog_state), GFP_KERNEL);
	if (!vc_wdog_state) {
		ret = -ENOMEM;
		goto exit;
	}

	vc_wdog_state->p_dev = p_dev;

	vc_wdog_state->proc_entry = create_proc_entry(DRIVER_NAME, 0444, NULL);
	if (vc_wdog_state->proc_entry == NULL) {
		ret = -EFAULT;
		goto out_efault;
	}
	vc_wdog_state->proc_entry->data = (void *)vc_wdog_state;
	vc_wdog_state->proc_entry->write_proc = vc_watchdog_proc_write;

	vchiq_add_connected_callback(vc_watchdog_connected_init);

	goto exit;

out_efault:
	kfree(vc_wdog_state);
	vc_wdog_state = NULL;
exit:
	LOG_INFO("%s: end, ret=%d", __func__, ret);
	return ret;
}


static int vc_watchdog_proc_write(
	struct file *file,
	const char __user *buffer,
	unsigned long count,
	void *data)
{
	char *command;
	char kbuf[PROC_WRITE_BUF_SIZE + 1];

	(void)file;

	memset(kbuf, 0, PROC_WRITE_BUF_SIZE + 1);
	if (count >= PROC_WRITE_BUF_SIZE)
		count = PROC_WRITE_BUF_SIZE;

	if (copy_from_user(kbuf, buffer, count) != 0)
		return -EFAULT;
	kbuf[count - 1] = 0;

	command = kbuf;

	if (strncmp("enable", command, strlen("enable")) == 0) {
		LOG_INFO("%s: Enabling VC watchdog", __func__);
		atomic_set(&vc_wdog_state->wdog_enabled, 1);
		complete_all(&vc_wdog_state->wdog_disable_blocker);
	} else if (strncmp("disable", command, strlen("disable")) == 0) {
		LOG_INFO("%s: Disabling VC watchdog", __func__);
		atomic_set(&vc_wdog_state->wdog_enabled, 0);
		init_completion(&vc_wdog_state->wdog_disable_blocker);
	} else if (strncmp("ping", command, strlen("ping")) == 0) {
		unsigned long msg = WDOG_PING_MSG;
		VCHIQ_ELEMENT_T elem = {
			.data = (void *)&msg,
			.size = sizeof(msg)
		};
		long rc = 0;
		/* on-demand ping of videocore.  Wake VC up to do this. */
		LOG_INFO("%s: Pinging VC...", __func__);
		vchiq_use_service(vc_wdog_state->service_handle);

		if (mutex_lock_interruptible(&vc_wdog_state->wdog_ping_mutex)
									!= 0) {
			LOG_ERR("%s: Interrupted waiting for watchdog",
				__func__);
		} else {
			/* ping vc... */
			vchiq_queue_message(vc_wdog_state->service_handle,
				&elem, 1);

			/* ...and wait for the response with a timeout */
			rc = wait_for_completion_interruptible_timeout(
				&vc_wdog_state->wdog_ping_response,
				msecs_to_jiffies(VC_PING_RESPONSE_TIMEOUT_MS));

			if (rc == 0)
				LOG_ERR("%s VideoCore ping timed out!!",
					__func__);
			else if (rc < 0)
				LOG_ERR("%s: Interrupted waiting for ping",
					__func__);
			else
				LOG_INFO("%s: Ping response received",
					__func__);
		}
		mutex_unlock(&vc_wdog_state->wdog_ping_mutex);

		vchiq_release_service(vc_wdog_state->service_handle);
	}

	return count;
}



static int __devexit vc_watchdog_remove(struct platform_device *p_dev)
{
	if (!vc_wdog_state)
		goto out;

	remove_proc_entry(vc_wdog_state->proc_entry->name, NULL);
	if (vc_wdog_state->service_handle) {
		vchiq_use_service(vc_wdog_state->service_handle);
		vchiq_close_service(vc_wdog_state->service_handle);
		vchiq_shutdown(vc_wdog_state->initialise_instance);
	}
	kfree(vc_wdog_state);
	vc_wdog_state = NULL;

out:
	return 0;
}

static int __init vc_watchdog_init(void)
{
	int ret;
	LOG_INFO("%s: Videocore watchdog driver\n", DRIVER_NAME);

	ret = platform_driver_register(&vc_watchdog_driver);
	if (ret) {
		LOG_ERR("%s : Unable to register VC watchdog driver (%d)",
				__func__, ret);
	} else {
		LOG_INFO("%s : Registered Videocore watchdog driver",
				__func__);
	}

	return ret;
}

static void __exit vc_watchdog_exit(void)
{
	LOG_INFO("%s: start", __func__);

	platform_driver_unregister(&vc_watchdog_driver);

	LOG_INFO("%s: end", __func__);
}

subsys_initcall(vc_watchdog_init);
module_exit(vc_watchdog_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("VC Watchdog Driver");
MODULE_LICENSE("GPL");
